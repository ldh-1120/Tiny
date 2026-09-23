#pragma once

#include <chrono>

namespace tiny {
	struct FrameEvent {
		std::chrono::steady_clock::time_point now;
		std::chrono::duration<float> delta;
	};
}