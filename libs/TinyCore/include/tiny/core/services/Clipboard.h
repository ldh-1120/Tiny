#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace tiny {
	class Clipboard {
	public:
		virtual ~Clipboard() = default;

		Clipboard(const Clipboard&) = delete;
		Clipboard& operator=(const Clipboard&) = delete;

		virtual bool writeText(std::u32string_view text) = 0;

		virtual std::optional<std::u32string> readText() = 0;

	protected:
		Clipboard() = default;
	};
}