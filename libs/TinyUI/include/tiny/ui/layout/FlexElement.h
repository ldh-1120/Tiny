#pragma once

#include <memory>
#include <vector>

#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Alignment.h>
#include <tiny/ui/layout/Axis.h>

namespace tiny {
	class FlexElement : public MultiChildElement {
	protected:
		FlexElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children, Axis axis, float spacing, CrossAxisAlignment crossAxisAlignment);

		void setSpacing(float spacing);
		void setCrossAxisAlignment(CrossAxisAlignment alignment);

		float spacing() const;
		CrossAxisAlignment crossAxisAlignment() const;

		Size measureOverride(LayoutContext& context, const Constraints& constraints) override;

	private:
		Axis axisValue = Axis::Horizontal;

		float spacingValue = 0.0f;
		CrossAxisAlignment crossAxisAlignmentValue = CrossAxisAlignment::Start;
	};
}