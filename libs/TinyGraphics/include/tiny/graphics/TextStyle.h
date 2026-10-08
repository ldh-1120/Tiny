#pragma once

#include <string>

namespace tiny {
	struct TextStyle {
		std::wstring fontFamily = L"Segoe UI";

		float fontSize = 14.0f;

		bool bold = false;
		bool italic = false;

		bool operator==(const TextStyle&) const = default;
	};
}