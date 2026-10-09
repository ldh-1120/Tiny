#include <tiny/ui/layout/FlexElement.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>

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

		Constraints makeNonFlexConstraints(const Constraints& constraints, Axis axis, CrossAxisAlignment crossAlignment) {
			constexpr float infinity = std::numeric_limits<float>::infinity();
			if (axis == Axis::Horizontal) {
				float minimumHeight = 0.0f;
				if (crossAlignment == CrossAxisAlignment::Stretch && constraints.hasBoundedHeight())
					minimumHeight = constraints.maxHeight();

				return Constraints(0.0f, infinity, minimumHeight, constraints.maxHeight());
			}

			float minimumWidth = 0.0f;
			if (crossAlignment == CrossAxisAlignment::Stretch && constraints.hasBoundedWidth())
				minimumWidth = constraints.maxWidth();

			return Constraints(minimumWidth, constraints.maxWidth(), 0.0f, infinity);
		}

		Constraints makeFlexConstraints(const Constraints& constraints, Axis axis, float allocatedMainExtent, FlexFit fit, CrossAxisAlignment crossAlignment) {
			float minimumMainExtent = fit == FlexFit::Tight ? allocatedMainExtent : 0.0f;
			if (axis == Axis::Horizontal) {
				float minimumHeight = 0.0f;
				if (crossAlignment == CrossAxisAlignment::Stretch && constraints.hasBoundedHeight())
					minimumHeight = constraints.maxHeight();

				return Constraints(minimumMainExtent, allocatedMainExtent, minimumHeight, constraints.maxHeight());
			}

			float minimumWidth = 0.0f;
			if (crossAlignment == CrossAxisAlignment::Stretch && constraints.hasBoundedWidth())
				minimumWidth = constraints.maxWidth();

			return Constraints(minimumWidth, constraints.maxWidth(), minimumMainExtent, allocatedMainExtent);
		}

		float mainPosition(const Rect& rect, Axis axis) {
			return axis == Axis::Horizontal ? rect.x : rect.y;
		}

		float crossPosition(const Rect& rect, Axis axis) {
			return axis == Axis::Horizontal ? rect.y : rect.x;
		}

		float mainExtent(const Rect& rect, Axis axis) {
			return axis == Axis::Horizontal ? rect.width : rect.height;
		}

		float crossExtent(const Rect& rect, Axis axis) {
			return axis == Axis::Horizontal ? rect.height : rect.width;
		}

		Rect makeRect(float mainPositionValue, float crossPositionValue, float mainExtentValue, float crossExtentValue, Axis axis) {
			return axis == Axis::Horizontal ? Rect(mainPositionValue, crossPositionValue, mainExtentValue, crossExtentValue) : Rect(crossPositionValue, mainPositionValue, crossExtentValue, mainExtentValue);
		}
	}

	FlexElement::FlexElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children, Axis axis, float spacing, MainAxisAlignment mainAxisAlignment, CrossAxisAlignment crossAxisAlignment)
		: MultiChildElement(widget, std::move(children)), axisValue(axis), spacingValue(spacing), mainAxisAlignmentValue(mainAxisAlignment), crossAxisAlignmentValue(crossAxisAlignment) {}

	void FlexElement::setSpacing(float spacing) {
		spacingValue = std::max(spacing, 0.0f);
	}

	void FlexElement::setMainAxisAlignment(MainAxisAlignment alignment) {
		mainAxisAlignmentValue = alignment;
	}

	void FlexElement::setCrossAxisAlignment(CrossAxisAlignment alignment) {
		crossAxisAlignmentValue = alignment;
	}

	float FlexElement::spacing() const {
		return spacingValue;
	}

	MainAxisAlignment FlexElement::mainAxisAlignment() const {
		return mainAxisAlignmentValue;
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
		Constraints normalChildConstraints = makeNonFlexConstraints(constraints, axisValue, crossAxisAlignmentValue);

		for (const std::unique_ptr<Element>& child : children()) {
			if (!child)
				continue;

			++visibleChildCount;

			const FlexParentData* flexData = child->parentData<FlexParentData>();
			if (flexData && flexData->flex > 0.0f && !boundedMainAxis && flexData->fit == FlexFit::Tight)
				throw std::runtime_error("Tight flex requires bounded main-axis constraints.");

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
				Constraints flexConstraints = makeFlexConstraints(constraints, axisValue, allocatedMainExtent, flexData->fit, crossAxisAlignmentValue);

				Size childSize = child->measure(context, flexConstraints);
				totalMainExtent += mainExtent(childSize, axisValue);

				maximumCrossExtentValue = std::max(maximumCrossExtentValue, crossExtent(childSize, axisValue));
			}
		}

		totalMainExtent += totalSpacing;
		return makeSize(totalMainExtent, maximumCrossExtentValue, axisValue);
	}

	void FlexElement::arrangeOverride(const Rect& bounds) {
		std::size_t visibleChildCount = 0;

		float occupiedMainExtent = 0.0f;
		for (const std::unique_ptr<Element>& child : children()) {
			if (!child)
				continue;

			++visibleChildCount;
			occupiedMainExtent += mainExtent(child->desiredSize(), axisValue);
		}

		if (visibleChildCount > 1)
			occupiedMainExtent += spacingValue * static_cast<float>(visibleChildCount - 1);

		float availableMainExtent = mainExtent(bounds, axisValue);
		float freeMainExtent = std::max(availableMainExtent - occupiedMainExtent, 0.0f);

		float leadingSpace = 0.0f;
		float additionalSpacing = 0.0f;

		switch (mainAxisAlignmentValue) {
			case MainAxisAlignment::Start:
				break;

			case MainAxisAlignment::Center:
				leadingSpace = freeMainExtent * 0.5f;
				break;

			case MainAxisAlignment::End:
				leadingSpace = freeMainExtent;
				break;

			case MainAxisAlignment::SpaceBetween:
				if (visibleChildCount > 1)
					additionalSpacing = freeMainExtent / static_cast<float>(visibleChildCount - 1);
				break;
		}

		float currentMainPosition = mainPosition(bounds, axisValue) + leadingSpace;
		float availableCrossExtent = crossExtent(bounds, axisValue);

		for (const std::unique_ptr<Element>& child : children()) {
			if (!child)
				continue;

			Size childSize = child->desiredSize();
			float childMainExtent = mainExtent(childSize, axisValue);
			float childCrossExtent = std::min(crossExtent(childSize, axisValue), availableCrossExtent);
			float childCrossPosition = crossPosition(bounds, axisValue);

			switch (crossAxisAlignmentValue) {
				case CrossAxisAlignment::Start:
					break;

				case CrossAxisAlignment::Center:
					childCrossPosition += (availableCrossExtent - childCrossExtent) * 0.5f;
					break;

				case CrossAxisAlignment::End:
					childCrossPosition += availableCrossExtent - childCrossExtent;
					break;

				case CrossAxisAlignment::Stretch:
					childCrossExtent = availableCrossExtent;
					break;
			}

			child->arrange(makeRect(currentMainPosition, childCrossPosition, childMainExtent, childCrossExtent, axisValue));
			currentMainPosition += childMainExtent + spacingValue + additionalSpacing;
		}
	}
}