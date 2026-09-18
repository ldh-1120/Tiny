#include <tiny/core/text/Unicode.h>

#include <cstddef>

namespace tiny {
	constexpr char32_t REPLACEMENT_CHARACTER = U'\uFFFD';

	bool isHighSurrogate(char16_t value) {
		return value >= 0xD800 && value <= 0xDBFF;
	}

	bool isLowSurrogate(char16_t value) {
		return value >= 0xDC00 && value <= 0xDFFF;
	}

	bool isUnicodeScalar(char32_t value) {
		if (value > 0x10FFFF)
			return false;

		if (value >= 0xD800 && value <= 0xDFFF)
			return false;

		return true;
	}

	char32_t combineSurrogates(char32_t high, char32_t low) {
		return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
	}

	std::wstring utf32ToWide(std::u32string_view text) {
		std::wstring result;
		for (char32_t codePoint : text) {
			if (!isUnicodeScalar(codePoint))
				codePoint = REPLACEMENT_CHARACTER;

			if constexpr (sizeof(wchar_t) == 2) {
				if (codePoint <= 0xFFFF) {
					result.push_back(static_cast<wchar_t>(codePoint));

					continue;
				}

				char32_t value = codePoint - 0x10000;

				wchar_t high = static_cast<wchar_t>(0xD800 + (value >> 10));
				wchar_t low = static_cast<wchar_t>(0xDC00 + (value & 0x3FF));

				result.push_back(high);
				result.push_back(low);

				continue;
			}

			result.push_back(static_cast<wchar_t>(codePoint));
		}

		return result;
	}

	std::u32string wideToUtf32(std::wstring_view text) {
		std::u32string result;
		if constexpr (sizeof(wchar_t) == 2) {
			std::size_t index = 0;
			while (index < text.size()) {
				char32_t current = static_cast<char32_t>(text[index]);
				if (isHighSurrogate(current)) {
					if (index + 1 < text.size()) {
						char32_t next = static_cast<char32_t>(text[index + 1]);
						if (isLowSurrogate(next)) {
							result.push_back(combineSurrogates(current, next));

							index += 2;
							continue;
						}
					}

					result.push_back(REPLACEMENT_CHARACTER);

					++index;
					continue;
				}

				if (isLowSurrogate(current)) {
					result.push_back(REPLACEMENT_CHARACTER);

					++index;
					continue;
				}

				result.push_back(current);

				++index;
			}

			return result;
		}

		for (wchar_t value : text) {
			char32_t codePoint = static_cast<char32_t>(value);
			if (!isUnicodeScalar(codePoint))
				codePoint = REPLACEMENT_CHARACTER;

			result.push_back(codePoint);
		}

		return result;
	}
}