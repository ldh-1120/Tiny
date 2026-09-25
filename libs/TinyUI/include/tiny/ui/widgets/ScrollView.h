#pragma once

#include <memory>

#include <tiny/core/Color.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	struct ScrollViewStyle {
		Color trackColor = Color::fromRgba(69, 71, 90, 80);
		Color thumbColor = Color::fromRgba(166, 173, 200, 180);
		Color pressedThumbColor = Color::fromRgba(205, 214, 244, 230);

		float scrollbarWidth = 4.0f;
		float scrollbarHitWidth = 12.0f;
		float scrollbarMargin = 4.0f;

		float minimumThumbExtent = 24.0f;

		bool showScrollbar = true;
	};

	class ScrollView : public Widget {
	public:
		ScrollView(std::unique_ptr<Widget> child, float wheelStep = 48.0f, Key key = Key(), ScrollViewStyle style = ScrollViewStyle());

		const Widget* child() const;

		float wheelStep() const;

		const ScrollViewStyle& style() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		float wheelStepValue = 48.0f;

		ScrollViewStyle scrollViewStyle;
	};
}