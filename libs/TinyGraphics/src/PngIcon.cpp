#include <tiny/graphics/PngIcon.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>
#include <tiny/graphics/Canvas.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace tiny {
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

		result = converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
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

		float pixelWidth = displaySize / static_cast<float>(Width);
		float pixelHeight = displaySize / static_cast<float>(Height);

		constexpr int backgroundRed = 24;
		constexpr int backgroundGreen = 24;
		constexpr int backgroundBlue = 37;

		for (std::uint32_t y = 0; y < Height; ++y) {
			for (std::uint32_t x = 0; x < Width; ++x) {
				std::uint32_t index = (y + Width + x) * 4;

				int red = pixels[index + 0];
				int green = pixels[index + 1];
				int blue = pixels[index + 2];

				int alpha = pixels[index + 3];
				if (alpha == 0)
					continue;

				red = (red * alpha + backgroundRed * (255 - alpha) + 127) / 255;
				green = (green * alpha + backgroundGreen * (255 - alpha) + 127) / 255;
				blue = (blue * alpha + backgroundBlue * (255 - alpha) + 127) / 255;

				canvas.fillRect(Rect(position.x + static_cast<float>(x) * pixelWidth, position.y + static_cast<float>(y) * pixelHeight, pixelWidth, pixelHeight), Color::fromRgb(static_cast<std::uint8_t>(red), static_cast<std::uint8_t>(green), static_cast<std::uint8_t>(blue)));
			}
		}
	}
}