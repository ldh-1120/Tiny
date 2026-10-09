#pragma once

#include <memory>
#include <vector>

#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Alignment.h>
#include <tiny/ui/layout/Axis.h>

namespace tiny {
	class FlexElement : public MultiChildElement {
	protected:
		FlexElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children, Axis axis, float spacing, MainAxisAlignment mainAxisAlignment, CrossAxisAlignment crossAxisAlignment);

		void setSpacing(float spacing);
		void setCrossAxisAlignment(CrossAxisAlignment alignment);
		void setMainAxisAlignment(MainAxisAlignment alignment);

		float spacing() const;

		MainAxisAlignment mainAxisAlignment() const;
		CrossAxisAlignment crossAxisAlignment() const;

		Size measureOverride(LayoutContext& context, const Constraints& constraints) override;
		void arrangeOverride(const Rect& bounds) override;

	private:
		Axis axisValue = Axis::Horizontal;

		float spacingValue = 0.0f;

		MainAxisAlignment mainAxisAlignmentValue = MainAxisAlignment::Start;
		CrossAxisAlignment crossAxisAlignmentValue = CrossAxisAlignment::Start;
	};
}