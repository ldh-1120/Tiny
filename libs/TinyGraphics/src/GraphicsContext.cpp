#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/TextLayout.h>

#include <tiny/core/text/Unicode.h>

#include "TextLayoutInternal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <vector>

#include <Windows.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace {
	DWRITE_FONT_WEIGHT getFontWeight(const tiny::TextStyle& style) {
		if (style.bold)
			return DWRITE_FONT_WEIGHT_BOLD;

		return DWRITE_FONT_WEIGHT_NORMAL;
	}

	DWRITE_FONT_STYLE getFontStyle(const tiny::TextStyle& style) {
		if (style.italic)
			return DWRITE_FONT_STYLE_ITALIC;

		return DWRITE_FONT_STYLE_NORMAL;
	}

	std::uint32_t utf16Length(char32_t codePoint) {
		if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
			return 1;

		if (codePoint <= 0xFFFF)
			return 1;

		return 2;
	}
}

namespace tiny {
	class GraphicsContext::Impl {
	public:
		Microsoft::WRL::ComPtr<ID2D1Factory1> d2dFactory;
		Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory;
		Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
	};

	GraphicsContext::GraphicsContext() : impl(std::make_unique<Impl>()) {
		HRESULT result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, impl->d2dFactory.ReleaseAndGetAddressOf());
		if (FAILED(result))
			throw std::runtime_error("Failed to create D2D factory");

		result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(impl->dwriteFactory.ReleaseAndGetAddressOf()));
		if (FAILED(result))
			throw std::runtime_error("Failed to create DWrite factory");

		result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(impl->wicFactory.ReleaseAndGetAddressOf()));
		if (FAILED(result))
			throw std::runtime_error("Failed to create WIC factory");
	}

	GraphicsContext::~GraphicsContext() = default;

	std::unique_ptr<TextLayout> GraphicsContext::createTextLayout(std::u32string_view text, const TextStyle& style, float maximumWidth) const {
		float safeMaximumWidth = maximumWidth;
		if (!std::isfinite(safeMaximumWidth) || safeMaximumWidth <= 0.0f)
			safeMaximumWidth = std::numeric_limits<float>::max();

		Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;

		HRESULT hr = impl->dwriteFactory->CreateTextFormat(style.fontFamily.c_str(), nullptr, style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, style.fontSize, L"ko-KR", &textFormat);
		if (FAILED(hr))
			return nullptr;

		textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

		std::unique_ptr<TextLayout::Impl> layoutImpl = std::make_unique<TextLayout::Impl>();
		layoutImpl->text.assign(text.begin(), text.end());
		layoutImpl->wideText = utf32ToWide(layoutImpl->text);
		layoutImpl->utf32ToUtf16Offsets.reserve(layoutImpl->text.size() + 1);

		std::uint32_t utf16Offset = 0;
		layoutImpl->utf32ToUtf16Offsets.push_back(utf16Offset);

		for (char32_t codePoint : layoutImpl->text) {
			utf16Offset += utf16Length(codePoint);
			layoutImpl->utf32ToUtf16Offsets.push_back(utf16Offset);
		}

		hr = impl->dwriteFactory->CreateTextLayout(layoutImpl->wideText.data(), static_cast<UINT32>(layoutImpl->wideText.size()), textFormat.Get(), safeMaximumWidth, std::numeric_limits<float>::max(), &layoutImpl->nativeLayout);
		if (FAILED(hr))
			return nullptr;

		DWRITE_TEXT_METRICS metrics = { };

		hr = layoutImpl->nativeLayout->GetMetrics(&metrics);
		if (FAILED(hr))
			return nullptr;

		layoutImpl->measureSize = Size(metrics.widthIncludingTrailingWhitespace, metrics.height);

		UINT lineCount = 0;

		HRESULT lineResult = layoutImpl->nativeLayout->GetLineMetrics(nullptr, 0, &lineCount);
		if (lineCount > 0) {
			std::vector<DWRITE_LINE_METRICS> lineMetrics(lineCount);

			lineResult = layoutImpl->nativeLayout->GetLineMetrics(lineMetrics.data(), lineCount, &lineCount);
			if (SUCCEEDED(lineResult) && !lineMetrics.empty())
				layoutImpl->baseline = lineMetrics[0].baseline;
		}

		layoutImpl->caretStops.clear();
		layoutImpl->caretStops.push_back(0);

		UINT32 clusterCount = 0;

		layoutImpl->nativeLayout->GetClusterMetrics(nullptr, 0, &clusterCount);
		if (clusterCount > 0) {
			std::vector<DWRITE_CLUSTER_METRICS> clusterMetrics(clusterCount);

			UINT32 actualClusterCount = 0;

			HRESULT clusterResult = layoutImpl->nativeLayout->GetClusterMetrics(clusterMetrics.data(), clusterCount, &actualClusterCount);
			if (SUCCEEDED(clusterResult)) {
				std::uint32_t utf16Position = 0;
				for (UINT32 index = 0; index < actualClusterCount; ++index) {
					utf16Position += clusterMetrics[index].length;

					std::size_t utf32Position = layoutImpl->toUtf32(utf16Position);
					if (layoutImpl->caretStops.back() != utf32Position)
						layoutImpl->caretStops.push_back(utf32Position);
				}
			}
		}

		if (layoutImpl->caretStops.back() != layoutImpl->text.size())
			layoutImpl->caretStops.push_back(layoutImpl->text.size());

		return std::unique_ptr<TextLayout>(new TextLayout(std::move(layoutImpl)));
	}

	FontMetrics GraphicsContext::getFontMetrics(const TextStyle& style) const {
		FontMetrics result;

		Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;

		HRESULT hr = impl->dwriteFactory->CreateTextFormat(style.fontFamily.c_str(), nullptr, style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, style.fontSize, L"ko-KR", &textFormat);
		if (FAILED(hr))
			return result;

		Microsoft::WRL::ComPtr<IDWriteFontCollection> fontCollection;

		hr = textFormat->GetFontCollection(&fontCollection);
		if (FAILED(hr))
			return result;

		UINT32 familyNameLength = textFormat->GetFontFamilyNameLength();

		std::wstring familyName(static_cast<std::size_t>(familyNameLength) + 1, L'\0');

		hr = textFormat->GetFontFamilyName(familyName.data(), familyNameLength + 1);
		if (FAILED(hr))
			return result;

		familyName.resize(familyNameLength);

		UINT32 familyIndex = 0;
		BOOL familyExists = FALSE;

		hr = fontCollection->FindFamilyName(familyName.c_str(), &familyIndex, &familyExists);
		if (FAILED(hr) || familyExists == FALSE)
			return result;

		Microsoft::WRL::ComPtr<IDWriteFontFamily> fontFamily;

		hr = fontCollection->GetFontFamily(familyIndex, &fontFamily);
		if (FAILED(hr))
			return result;

		Microsoft::WRL::ComPtr<IDWriteFont> font;

		hr = fontFamily->GetFirstMatchingFont(style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, &font);
		if (FAILED(hr))
			return result;

		Microsoft::WRL::ComPtr<IDWriteFontFace> fontFace;

		hr = font->CreateFontFace(&fontFace);
		if (FAILED(hr))
			return result;

		DWRITE_FONT_METRICS nativeMetrics = { };
		fontFace->GetMetrics(&nativeMetrics);

		if (nativeMetrics.designUnitsPerEm == 0)
			return result;

		float scale = style.fontSize / static_cast<float>(nativeMetrics.designUnitsPerEm);
		result.ascent = static_cast<float>(nativeMetrics.ascent) * scale;
		result.descent = static_cast<float>(nativeMetrics.descent) * scale;
		result.lineGap = static_cast<float>(nativeMetrics.lineGap) * scale;
		result.lineHeight = result.ascent + result.descent + result.lineGap;

		return result;
	}

	void* GraphicsContext::d2dFactoryHandle() const {
		return impl->d2dFactory.Get();
	}

	void* GraphicsContext::dwriteFactoryHandle() const {
		return impl->dwriteFactory.Get();
	}
}