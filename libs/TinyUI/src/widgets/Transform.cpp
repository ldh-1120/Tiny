#include <tiny/ui/widgets/Transform.h>

#include <memory>
#include <utility>

#include <tiny/core/AffineTransform.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class TransformElement final : public ProxyElement {
		public:
			explicit TransformElement(const Transform& widget) : ProxyElement(widget, createChild(widget)), specValue(widget.spec()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Transform& transform = static_cast<const Transform&>(widget);

				specValue = transform.spec();

				updateChild(transform.child());
				markNeedsLayout();
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				AffineTransform transform = effectiveTransform();

				bool pushed = canvas.pushTransform(transform);
				SingleChildElement::paintOverride(canvas);

				if (pushed)
					canvas.popTransform();
			}

			bool mapHitTestPositionOverride(const Point& position, Point& result) const override {
				AffineTransform inverse;
				if (!effectiveTransform().tryInverse(inverse))
					return false;

				result = inverse.transformPoint(position);

				return true;
			}

			Rect visualBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				Rect childBounds = child()->visualBounds();
				if (childBounds.isEmpty())
					return Rect();

				return effectiveTransform().transformBounds(childBounds);
			}

			Rect backdropReadBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				Rect childBounds = child()->backdropReadBounds();
				if (childBounds.isEmpty())
					return Rect();

				return effectiveTransform().transformBounds(childBounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const Transform& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

			AffineTransform effectiveTransform() const {
				Point pivot(bounds().x + bounds().width * specValue.pivot.x, bounds().y + bounds().height * specValue.pivot.y);
				return AffineTransform::translation(-pivot.x, -pivot.y)
					* AffineTransform::scale(specValue.scaleX, specValue.scaleY)
					* AffineTransform::rotation(specValue.rotationDegrees)
					* AffineTransform::translation(pivot.x, pivot.y)
					* AffineTransform::translation(specValue.translation.x, specValue.translation.y);
			}

		private:
			TransformSpec specValue;
		};
	}

	Transform::Transform(TransformSpec spec, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), transformSpec(spec), childWidget(std::move(child)) {}

	const TransformSpec& Transform::spec() const {
		return transformSpec;
	}

	const Widget* Transform::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Transform::createElement() const {
		return std::make_unique<TransformElement>(*this);
	}
}