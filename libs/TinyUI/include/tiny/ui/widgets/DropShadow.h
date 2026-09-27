#pragma once

#include <memory>

#include <tiny/core/Color.h>
#include <tiny/core/Point.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class DropShadow : public Widget {
	public:
		DropShadow(float blurRadius, Point offset, Color color, std::unique_ptr<Widget> child, Key key = Key());

		float blurRadius() const;

		const Point& offset() const;
		const Color& color() const;

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		float blurRadiusValue = 0.0f;

		Point offsetValue;
		Color colorValue;

		std::unique_ptr<Widget> childWidget;
	};
}