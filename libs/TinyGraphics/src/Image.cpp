#include <tiny/graphics/Image.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dxgiformat.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace tiny {
    struct Image::BitmapCache {
        Microsoft::WRL::ComPtr<ID2D1RenderTarget> target;
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
    };

    Image::Image(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> pixels) : widthValue(width), heightValue(height), pixelData(std::move(pixels)) { }
    Image::~Image() = default;

	void* Image::nativeBitmap(void* renderTarget) const {
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
		HRESULT result = target->CreateBitmap(D2D1::SizeU(widthValue, heightValue), pixelData.data(), widthValue * 4, properties, bitmapCache->bitmap.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		return bitmapCache->bitmap.Get();
	}

    std::shared_ptr<Image> Image::load(const std::wstring& path) {
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

		ComGuard guard;
		guard.shouldUninitialize = SUCCEEDED(initializedResult);

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

		UINT width = 0;
		UINT height = 0;

		result = frame->GetSize(&width, &height);
		if (FAILED(result) || width == 0 || height == 0)
			return nullptr;

		constexpr std::uint64_t MaximumPixels = 16ULL * 1024ULL * 1024ULL;

		if (width > 8192 || height > 8192)
			return nullptr;

		std::uint64_t pixelCount = static_cast<std::uint64_t>(width) * height;
		if (pixelCount > MaximumPixels)
			return nullptr;

		std::uint64_t stride64 = static_cast<uint64_t>(width) * 4ULL;
		std::uint64_t bufferSize64 = pixelCount * 4ULL;

		if (stride64 > std::numeric_limits<UINT>::max() || bufferSize64 > std::numeric_limits<UINT>::max())
			return nullptr;

		Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
		result = factory->CreateFormatConverter(converter.GetAddressOf());
		if (FAILED(result))
			return nullptr;

		result = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
		if (FAILED(result))
			return nullptr;

		std::vector<std::uint8_t> pixels(static_cast<std::size_t>(bufferSize64));
		result = converter->CopyPixels(nullptr, static_cast<UINT>(stride64), static_cast<UINT>(bufferSize64), pixels.data());
		if (FAILED(result))
			return nullptr;

		return std::shared_ptr<Image>(new Image(width, height, std::move(pixels)));
    }

    std::uint32_t Image::width() const {
        return widthValue;
    }

    std::uint32_t Image::height() const {
        return heightValue;
    }
}