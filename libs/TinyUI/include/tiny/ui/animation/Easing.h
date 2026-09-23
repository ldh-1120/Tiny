#pragma once

#include <algorithm>

namespace tiny {
	enum class Easing {
		Linear,
		EaseOutCubic,
		EaseInOutCubic
	};

	inline float applyEasing(Easing easing, float progress) {
		float t = std::clamp(progress, 0.0f, 1.0f);

		switch (easing) {
			case Easing::Linear:
				return t;

			case Easing::EaseOutCubic: {
				float remaining = 1.0f - t;

				return 1.0f - remaining * remaining * remaining;
			}

			case Easing::EaseInOutCubic:
				if (t < 0.5f)
					return 4.0f * t * t * t;

				return 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * 0.5f;
		}

		return t;
	}
}