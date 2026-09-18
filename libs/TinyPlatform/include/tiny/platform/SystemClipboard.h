#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <tiny/core/services/Clipboard.h>

namespace tiny {
	class SystemClipboard final : public Clipboard {
	public:
		SystemClipboard() = default;

		bool writeText(std::u32string_view text) override;

		std::optional<std::u32string> readText() override;
	};
}