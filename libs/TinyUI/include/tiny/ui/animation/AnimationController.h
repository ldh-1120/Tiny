#pragma once

#include <tiny/ui/animation/Easing.h>

namespace tiny {
	class AnimationController {
	public:
		explicit AnimationController(float initialValue = 0.0f);

		float value() const;

		bool isRunning() const;

		void setValue(float value);

		void animateTo(float targetValue, float durationSeconds, Easing easing = Easing::Linear);

		bool advance(float deltaSeconds);

	private:
		float currentValue = 0.0f;

		float startValue = 0.0f;
		float targetValue = 0.0f;

		float elapsedSeconds = 0.0f;
		float durationSeconds = 0.0f;

		Easing easingValue = Easing::Linear;

		bool running = false;
	};
}