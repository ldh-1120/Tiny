#pragma once

namespace tiny {
	class ScrollModel {
	public:
		ScrollModel() = default;

		float offset() const;

		float viewportExtent() const;
		float contentExtent() const;

		float maximumOffset() const;

		bool canScroll() const;

		float viewportFraction() const;
		float offsetFraction() const;

		bool setExtents(float viewportExtent, float contentExtent);

		bool scrollTo(float offset);
		bool scrollBy(float delta);

	private:
		float offsetValue = 0.0f;

		float viewportExtentValue = 0.0f;
		float contentExtentValue = 0.0f;
	};
}