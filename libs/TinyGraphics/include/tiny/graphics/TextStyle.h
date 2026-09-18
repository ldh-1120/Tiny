#pragma once

#include <string>

namespace tiny {
	struct TextStyle {
		std::wstring fontFamily = L"Sagoe UI";

		float fontSize = 14.0f;

		bool bold = false;
		bool italic = false;
	};
}