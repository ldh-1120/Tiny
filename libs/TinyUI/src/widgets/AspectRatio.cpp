#include <tiny/ui/widgets/AspectRatio.h>

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		Size resolveAspectRatio(const Constraints& constraints, float aspectRatio) {
			bool boundedWidth = constraints.hasBoundedWidth();
			bool boundedHeight = constraints.hasBoundedHeight();

			if (!boundedWidth && !boundedHeight)
				throw std::runtime_error("AspectRatio requires at least one bounded axis.");

			float width = 0.0f;
			float height = 0.0f;

			if (boundedWidth) {
				width = constraints.maxWidth();
				height = width / aspectRatio;

				if (boundedHeight && height > constraints.maxHeight()) {
					height = constraints.maxHeight();
					width = height * aspectRatio;
				}
			} else {
				height = constraints.maxHeight();
				width = height * aspectRatio;
			}

			if (width < constraints.minWidth()) {
				width = constraints.minWidth();
				height = width / aspectRatio;
			}

			if (height < constraints.minHeight()) {
				height = constraints.minHeight();
				width = height * aspectRatio;
			}

			return constraints.constrain(Size(width, height));
		}

		class AspectRatioElement final : public SingleChildElement {
		public:
			explicit AspectRatioElement(const AspectRatio& widget) : SingleChildElement(widget, createChild(widget)), aspectRatioValue(widget.aspectRatio()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const AspectRatio& aspectRatio = static_cast<const AspectRatio&>(widget);

				aspectRatioValue = aspectRatio.aspectRatio();

				updateChild(aspectRatio.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Size size = resolveAspectRatio(constraints, aspectRatioValue);
				if (hasChild())
					child()->measure(context, Constraints::tight(size));

				return size;
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const AspectRatio& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float aspectRatioValue = 1.0f;
		};
	}

	AspectRatio::AspectRatio(float aspectRatio, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), aspectRatioValue(aspectRatio), childWidget(std::move(child)) {
		if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0f)
			throw std::invalid_argument("AspectRatio must be a finite positive value.");
	}

	float AspectRatio::aspectRatio() const {
		return aspectRatioValue;
	}

	const Widget* AspectRatio::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> AspectRatio::createElement() const {
		return std::make_unique<AspectRatioElement>(*this);
	}
}