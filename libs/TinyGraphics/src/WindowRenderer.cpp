#include <tiny/graphics/WindowRenderer.h>

#include <algorithm>
#include <stdexcept>

#include <Windows.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <tiny/core/Size.h>
#include <tiny/core/Subscription.h>

#include <tiny/graphics/GraphicsContext.h>
#include <tiny/platform/Window.h>

namespace tiny {
	class WindowRenderer::Impl {
	public:
		Impl(void* d2dFactoryHandle, void* dwriteFactoryHandle, Window& window) : d2dFactory(static_cast<ID2D1Factory1*>(d2dFactoryHandle)), dwriteFactory(static_cast<IDWriteFactory*>(dwriteFactoryHandle)), window(window) {
			createRenderTarget();

			resizedSubscription = window.resized.subscribe([this](const Size& size) {
				resize(size);
			});
			dpiScaleSubscription = window.dpiChanged.subscribe([this](float scale) {
				updateDpi(scale);
			});
		}

		void beginFrame() {
			if (!renderTarget)
				createRenderTarget();
			
			renderTarget->BeginDraw();
			renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		}

		void endFrame() {
			if (!renderTarget)
				return;

			HRESULT result = renderTarget->EndDraw();
			if (result == D2DERR_RECREATE_TARGET) {
				discardRenderTarget();
				window.requestRepaint();
			}
		}

		void resize(const Size& size) {
			if (!renderTarget)
				return;

			UINT width = static_cast<UINT>(std::max(size.width, 1.0f));
			UINT height = static_cast<UINT>(std::max(size.height, 1.0f));

			HRESULT result = renderTarget->Resize(D2D1::SizeU(width, height));
			if (result == D2DERR_RECREATE_TARGET)
				discardRenderTarget();

			window.requestRepaint();
		}

		void updateDpi(float scale) {
			if (!renderTarget)
				return;

			float dpi = scale * 96.0f;
			renderTarget->SetDpi(dpi, dpi);

			window.requestRepaint();
		}

		void requestRepaint() {
			window.requestRepaint();
		}

		void* renderTargetHandle() const {
			return renderTarget.Get();
		}

		void* textFactoryHandle() const {
			return dwriteFactory;
		}

		void* solidBrushHandle() const {
			return solidBrush.Get();
		}

	private:
		void createRenderTarget() {
			if (renderTarget)
				return;

			HWND hwnd = static_cast<HWND>(window.nativeHandle());
			if (!hwnd)
				throw std::runtime_error("Window has no native handle.");

			Size clientSize = window.clientSize();
			UINT width = static_cast<UINT>(std::max(clientSize.width, 1.0f));
			UINT height = static_cast<UINT>(std::max(clientSize.height, 1.0f));

			float dpi = window.dpiScale() * 96.0f;
			D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();
			renderTargetProperties.dpiX = dpi;
			renderTargetProperties.dpiY = dpi;

			D2D1_HWND_RENDER_TARGET_PROPERTIES windowProperties = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(width, height));

			HRESULT result = d2dFactory->CreateHwndRenderTarget(renderTargetProperties, windowProperties, renderTarget.ReleaseAndGetAddressOf());
			if (FAILED(result))
				throw std::runtime_error("Failed to create D2D HWND render target.");

			result = renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), solidBrush.ReleaseAndGetAddressOf());
			if (FAILED(result)) {
				discardRenderTarget();
				
				throw std::runtime_error("Failed to create D2D solid brush.");
			}
		}

		void discardRenderTarget() {
			solidBrush.Reset();
			renderTarget.Reset();
		}

	private:
		ID2D1Factory1* d2dFactory = nullptr;
		IDWriteFactory* dwriteFactory = nullptr;

		Window& window;

		Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> renderTarget;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> solidBrush;

		Subscription resizedSubscription;
		Subscription dpiScaleSubscription;
	};

	WindowRenderer::WindowRenderer(GraphicsContext& graphicsContext, Window& window) : impl(std::make_unique<Impl>(graphicsContext.d2dFactoryHandle(), graphicsContext.dwriteFactoryHandle(), window)) { }
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