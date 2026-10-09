#include <tiny/ui/layout/FlexElement.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>
#include <tiny/ui/widgets/Flexible.h>

namespace tiny {
	namespace {
		float mainExtent(const Size& size, Axis axis) {
			return axis == Axis::Horizontal ? size.width : size.height;
		}

		float crossExtent(const Size& size, Axis axis) {
			return axis == Axis::Horizontal ? size.height : size.width;
		}

		Size makeSize(float main, float cross, Axis axis) {
			return axis == Axis::Horizontal ? Size(main, cross) : Size(cross, main);
		}

		bool hasBoundedMainAxis(const Constraints& constraints, Axis axis) {
			return axis == Axis::Horizontal ? constraints.hasBoundedWidth() : constraints.hasBoundedHeight();
		}

		float maximumMainExtent(const Constraints& constraints, Axis axis) {
			return axis == Axis::Horizontal ? constraints.maxWidth() : constraints.maxHeight();
		}

		Constraints makeNonFlexConstraints(const Constraints& constraints, Axis axis) {
			constexpr float infinity = std::numeric_limits<float>::infinity();
			return axis == Axis::Horizontal ? Constraints(0.0f, infinity, 0.0f, constraints.maxHeight()) : Constraints(0.0f, constraints.maxWidth(), 0.0f, infinity);
		}

		Constraints makeFlexConstraints(const Constraints& constraints, Axis axis, float allocatedMainExtent, FlexFit fit) {
			float minimumMainExtent = fit == FlexFit::Tight ? allocatedMainExtent : 0.0f;
			return axis == Axis::Horizontal ? Constraints(minimumMainExtent, allocatedMainExtent, 0.0f, constraints.maxHeight()) : Constraints(0.0f, constraints.maxWidth(), minimumMainExtent, allocatedMainExtent);
		}
	}

	FlexElement::FlexElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children, Axis axis, float spacing, CrossAxisAlignment crossAxisAlignment)
		: MultiChildElement(widget, std::move(children)), axisValue(axis), spacingValue(spacing), crossAxisAlignmentValue(crossAxisAlignment) {}

	void FlexElement::setSpacing(float spacing) {
		spacingValue = std::max(spacing, 0.0f);
	}

	void FlexElement::setCrossAxisAlignment(CrossAxisAlignment alignment) {
		crossAxisAlignmentValue = alignment;
	}

	float FlexElement::spacing() const {
		return spacingValue;
	}

	CrossAxisAlignment FlexElement::crossAxisAlignment() const {
		return crossAxisAlignmentValue;
	}

	Size FlexElement::measureOverride(LayoutContext& context, const Constraints& constraints) {
		float totalMainExtent = 0.0f;
		float maximumCrossExtentValue = 0.0f;

		float totalFlex = 0.0f;

		std::size_t visibleChildCount = 0;

		bool boundedMainAxis = hasBoundedMainAxis(constraints, axisValue);
		Constraints normalChildConstraints = makeNonFlexConstraints(constraints, axisValue);

		for (const std::unique_ptr<Element>& child : children()) {
			if (!child)
				continue;

			++visibleChildCount;

			const FlexParentData* flexData = child->parentData<FlexParentData>();

			float flex = 0.0f;
			if (flexData && boundedMainAxis)
				flex = flexData->flex;

			if (flex > 0.0f) {
				totalFlex += flex;
				continue;
			}

			Size childSize = child->measure(context, normalChildConstraints);
			totalMainExtent += mainExtent(childSize, axisValue);

			maximumCrossExtentValue = std::max(maximumCrossExtentValue, crossExtent(childSize, axisValue));
		}

		float totalSpacing = 0.0f;
		if (visibleChildCount > 1)
			totalSpacing = spacingValue * static_cast<float>(visibleChildCount - 1);

		if (boundedMainAxis && totalFlex > 0.0f) {
			float remainingMainExtent = std::max(maximumMainExtent(constraints, axisValue) - totalMainExtent - totalSpacing, 0.0f);
			for (const std::unique_ptr<Element>& child : children()) {
				if (!child)
					continue;

				const FlexParentData* flexData = child->parentData<FlexParentData>();
				if (!flexData || flexData->flex <= 0.0f)
					continue;

				float allocatedMainExtent = remainingMainExtent * flexData->flex / totalFlex;
				Constraints flexConstraints = makeFlexConstraints(constraints, axisValue, allocatedMainExtent, flexData->fit);

				Size childSize = child->measure(context, flexConstraints);
				totalMainExtent += mainExtent(childSize, axisValue);

				maximumCrossExtentValue = std::max(maximumCrossExtentValue, crossExtent(childSize, axisValue));
			}
		}

		totalMainExtent += totalSpacing;
		return makeSize(totalMainExtent, maximumCrossExtentValue, axisValue);
	}
}