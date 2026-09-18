#pragma once

#include <string>
#include <string_view>

namespace tiny {
	std::wstring utf32ToWide(std::u32string_view text);
	std::u32string wideToUtf32(std::wstring_view text);
}