#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <dwrite.h>
#include <wrl/client.h>

#include <tiny/core/Size.h>

#include <tiny/graphics/TextLayout.h>

namespace tiny {
	struct TextLayout::Impl {
		Microsoft::WRL::ComPtr<IDWriteTextLayout> nativeLayout;

		std::u32string text;
		std::wstring wideText;

		std::vector<std::uint32_t> utf32ToUtf16Offsets;
		std::vector<std::size_t> caretStops;

		Size measureSize;

		float baseline = 0.0f;

		std::uint32_t toUtf16(std::size_t index) const {
			if (utf32ToUtf16Offsets.empty())
				return 0;

			std::size_t clamped = std::min(index, text.size());
			return utf32ToUtf16Offsets[clamped];
		}

		std::size_t toUtf32(std::uint32_t offset) const {
			if (utf32ToUtf16Offsets.empty())
				return 0;

			std::vector<std::uint32_t>::const_iterator iterator = std::lower_bound(utf32ToUtf16Offsets.begin(), utf32ToUtf16Offsets.end(), offset);
			if (iterator == utf32ToUtf16Offsets.end())
				return text.size();

			return static_cast<std::size_t>(iterator - utf32ToUtf16Offsets.begin());
		}
	};
}