#include <tiny/ui/animation/AnimationController.h>

#include <algorithm>
#include <cmath>

namespace tiny {
	AnimationController::AnimationController(float initialValue) : currentValue(initialValue), startValue(initialValue), targetValue(initialValue) { }

	float AnimationController::value() const {
		return currentValue;
	}

	bool AnimationController::isRunning() const {
		return running;
	}

	void AnimationController::setValue(float value) {
		currentValue = value;
		startValue = value;
		targetValue = value;

		elapsedSeconds = 0.0f;
		durationSeconds = 0.0f;

		running = false;
	}

	void AnimationController::animateTo(float target, float duration, Easing easing) {
		if (running && targetValue == target)
			return;

		startValue = currentValue;
		targetValue = target;

		elapsedSeconds = 0.0f;
		durationSeconds = duration;

		easingValue = easing;

		if (!std::isfinite(duration) || duration <= 0.0f || startValue == targetValue) {
			setValue(target);
			return;
		}

		running = true;
	}

	bool AnimationController::advance(float deltaSeconds) {
		if (!running)
			return false;

		if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f)
			return false;

		float previousValue = currentValue;
		elapsedSeconds = std::min(elapsedSeconds + deltaSeconds, durationSeconds);

		float progress = elapsedSeconds / durationSeconds;
		float easedProgress = applyEasing(easingValue, progress);

		currentValue = startValue + (targetValue - startValue) * easedProgress;
		if (elapsedSeconds >= durationSeconds) {
			currentValue = targetValue;
			running = false;
		}

		return previousValue != currentValue;
	}
}