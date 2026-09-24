#include <tiny/graphics/PngIcon.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dxgiformat.h>

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>
#include <tiny/graphics/Canvas.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace tiny {
	struct PngIcon::BitmapCache {
		Microsoft::WRL::ComPtr<ID2D1RenderTarget> target;
		Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
	};

	PngIcon::PngIcon() = default;
	PngIcon::~PngIcon() = default;

	std::shared_ptr<PngIcon> PngIcon::load(const std::wstring& path) {
		HRESULT initializedResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(initializedResult) && initializedResult != RPC_E_CHANGED_MODE)
			return nullptr;

		struct ComGuard {
			bool shouldUninitialize = false;

			~ComGuard() {
				if (shouldUninitialize)
					CoUninitialize();
			}
		};

		ComGuard comGuard;
		comGuard.shouldUninitialize = SUCCEEDED(initializedResult);

		Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
		HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
		if (FAILED(result))
			return nullptr;

		Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
		result = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
		result = decoder->GetFrame(0, frame.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
		result = factory->CreateBitmapScaler(scaler.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		result = scaler->Initialize(frame.Get(), Width, Height, WICBitmapInterpolationModeFant);
		if (FAILED(result))
			return nullptr;

		Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
		result = factory->CreateFormatConverter(converter.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		result = converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
		if (FAILED(result))
			return nullptr;

		std::shared_ptr<PngIcon> icon = std::make_shared<PngIcon>();

		constexpr UINT stride = Width * 4;
		constexpr UINT bufferSize = Width * Height * 4;
		 
		result = converter->CopyPixels(nullptr, stride, bufferSize, icon->pixels.data());
		if (FAILED(result))
			return nullptr;

		return icon;
	}

	void PngIcon::paint(Canvas& canvas, const Point& position, float displaySize) const {
		if (displaySize <= 0.0f)
			return;

		canvas.drawImage(*this, Rect(position.x, position.y, displaySize, displaySize));
	}

	void* PngIcon::nativeBitmap(void* renderTarget) const {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return nullptr;

		if (!bitmapCache)
			bitmapCache = std::make_unique<BitmapCache>();

		if (bitmapCache->target.Get() != target) {
			bitmapCache->bitmap.Reset();
			bitmapCache->target = target;
		}

		if (bitmapCache->bitmap)
			return bitmapCache->bitmap.Get();

		D2D1_BITMAP_PROPERTIES properties = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
		HRESULT result = target->CreateBitmap(D2D1::SizeU(Width, Height), pixels.data(), Width * 4, properties, bitmapCache->bitmap.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		return bitmapCache->bitmap.Get();
	}
}