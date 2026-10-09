#include <tiny/graphics/WindowRenderer.h>

#include <algorithm>
#include <stdexcept>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>

#include <wrl/client.h>
#include <Windows.h>

#include <tiny/core/Size.h>
#include <tiny/core/Subscription.h>

#include <tiny/graphics/GraphicsContext.h>
#include <tiny/platform/Window.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d2d1.lib")

namespace tiny {
	class WindowRenderer::Impl {
	public:
		Impl(void* d3dDeviceHandle, void* d2dDeviceHandle, void* dwriteFactoryHandle, Window& window)
			: d3dDevice(static_cast<ID3D11Device*>(d3dDeviceHandle)), d2dDevice(static_cast<ID2D1Device*>(d2dDeviceHandle)), dwriteFactory(static_cast<IDWriteFactory*>(dwriteFactoryHandle)), window(window) {
			createDeviceContext();
			createSwapChain();

			Size clientSize = window.clientSize();
			bufferWidth = static_cast<UINT>(std::max(clientSize.width, 1.0f));
			bufferHeight = static_cast<UINT>(std::max(clientSize.height, 1.0f));

			createTargetBitmap();
			
			resizeStartedSubscription = window.resizeStarted.subscribe([this]() { interactiveSize = true; });
			resizeEndedSubscription = window.resizeEnded.subscribe([this]() { interactiveSize = false; });
			resizedSubscription = window.resized.subscribe([this](const Size& size) { resize(size); });
			dpiScaleSubscription = window.dpiChanged.subscribe([this](float scale) { updateDpi(scale); });
		}
		
		void createDeviceContext() {
			HRESULT result = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContext.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to create D2D device context.");
		}

		void createSwapChain() {
			Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
			HRESULT result = d3dDevice.As(&dxgiDevice);
			if (FAILED(result))
				throw std::runtime_error("Failed to get DXGI Device.");

			Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
			result = dxgiDevice->GetAdapter(adapter.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to get DXGI adapter.");

			Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
			result = adapter->GetParent(IID_PPV_ARGS(factory.ReleaseAndGetAddressOf()));
			if (FAILED(result))
				throw std::runtime_error("Failed to get DXGI factory.");

			DXGI_SWAP_CHAIN_DESC1 description = { };
			description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			description.SampleDesc.Count = 1;
			description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			description.BufferCount = 2;
			description.Scaling = DXGI_SCALING_NONE;
			description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

			HWND handle = static_cast<HWND>(window.nativeHandle());
			result = factory->CreateSwapChainForHwnd(d3dDevice.Get(), handle, &description, nullptr, nullptr, swapChain.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to create DXGI swap chain.");

			factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER);
		}

		void createTargetBitmap() {
			if (!swapChain || !d2dContext)
				return;

			Microsoft::WRL::ComPtr<IDXGISurface> surface;
			HRESULT result = swapChain->GetBuffer(0, IID_PPV_ARGS(surface.ReleaseAndGetAddressOf()));
			if (FAILED(result))
				throw std::runtime_error("Failed to get swap chain surface.");

			float dpi = window.dpiScale() * 96.0f;
			D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

			result = d2dContext->CreateBitmapFromDxgiSurface(surface.Get(), &properties, targetBitmap.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to create D2D target bitmap.");

			d2dContext->SetTarget(targetBitmap.Get());
			d2dContext->SetDpi(dpi, dpi);

			result = d2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), solidBrush.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to create solid brush.");
		}

		void beginFrame() {
			if (!d2dContext)
				return;

			if (!targetBitmap)
				createTargetBitmap();
			
			d2dContext->BeginDraw();
			d2dContext->SetTransform(D2D1::Matrix3x2F::Identity());
		}

		void endFrame() {
			if (!d2dContext)
				return;

			HRESULT result = d2dContext->EndDraw();
			if (result == D2DERR_RECREATE_TARGET) {
				discardTargetBitmap();

				window.requestRepaint();
				return;
			}

			if (FAILED(result))
				return;

			if (!swapChain)
				return;

			UINT syncInterval = interactiveSize ? 0 : 1;	
			result = swapChain->Present(syncInterval, 0);
			if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
				discardTargetBitmap();
				window.requestRepaint();
			}
		}

		void discardTargetBitmap() {
			if (d2dContext)
				d2dContext->SetTarget(nullptr);

			solidBrush.Reset();
			targetBitmap.Reset();
		}

		void resize(const Size& size) {
			if (!swapChain)
				return;

			UINT width = static_cast<UINT>(std::max(size.width, 1.0f));
			UINT height = static_cast<UINT>(std::max(size.height, 1.0f));

			if (width == bufferWidth && height == bufferHeight)
				return;

			discardTargetBitmap();

			HRESULT result = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
			if (FAILED(result))
				return;

			bufferWidth = width;
			bufferHeight = height;

			createTargetBitmap();

			window.requestRepaint();
		}

		void updateDpi(float scale) {
			if (!d2dContext)
				return;

			float dpi = scale * 96.0f;
			d2dContext->SetDpi(dpi, dpi);

			discardTargetBitmap();
			createTargetBitmap();

			window.requestRepaint();
		}

		void requestRepaint() {
			window.requestRepaint();
		}

		void* renderTargetHandle() const {
			return d2dContext.Get();
		}

		void* textFactoryHandle() const {
			return dwriteFactory;
		}

		void* solidBrushHandle() const {
			return solidBrush.Get();
		}

	private:
		IDWriteFactory* dwriteFactory = nullptr;

		Window& window;

		Microsoft::WRL::ComPtr<ID3D11Device> d3dDevice;
		Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
		Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dContext;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> targetBitmap;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;

		Subscription resizedSubscription;
		Subscription dpiScaleSubscription;

		UINT bufferWidth = 0;
		UINT bufferHeight = 0;

		bool interactiveSize = false;

		Subscription resizeStartedSubscription;
		Subscription resizeEndedSubscription;
	};

	WindowRenderer::WindowRenderer(GraphicsContext& graphicsContext, Window& window) : impl(std::make_unique<Impl>(graphicsContext.d3dDeviceHandle(), graphicsContext.d2dDeviceHandle(), graphicsContext.dwriteFactoryHandle(), window)) { }
	WindowRenderer::~WindowRenderer() = default;

	void WindowRenderer::render(const DrawCallback& callback) {
		if (!callback)
			return;

		impl->beginFrame();

		Canvas canvas(impl->renderTargetHandle(), impl->textFactoryHandle(), impl->solidBrushHandle());
		callback(canvas);

		impl->endFrame();
	}

	void WindowRenderer::requestRepaint() {
		impl->requestRepaint();
	}
}