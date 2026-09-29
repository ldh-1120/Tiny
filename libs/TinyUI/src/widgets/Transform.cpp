#include <tiny/ui/widgets/Transform.h>

#include <memory>
#include <utility>

#include <tiny/core/AffineTransform.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class TransformElement final : public SingleChildElement {
		public:
			explicit TransformElement(const Transform& widget) : SingleChildElement(widget, createChild(widget)), specValue(widget.spec()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Transform& transform = static_cast<const Transform&>(widget);

				specValue = transform.spec();

				updateChild(transform.child());
				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				if (!hasChild())
					return constraints.smallest();

				return child()->measure(context, constraints);
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(bounds);
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				Point pivot(bounds().x + bounds().width * specValue.pivot.x, bounds().y + bounds().height * specValue.pivot.y);
				AffineTransform transform = AffineTransform::translation(-pivot.x, -pivot.y)
					* AffineTransform::scale(specValue.scaleX, specValue.scaleY)
					* AffineTransform::rotation(specValue.rotationDegrees)
					* AffineTransform::translation(pivot.x, pivot.y)
					* AffineTransform::translation(specValue.translation.x, specValue.translation.y);

				bool pushed = canvas.pushTransform(transform);
				SingleChildElement::paintOverride(canvas);

				if (pushed)
					canvas.popTransform();
			}

		private:
			static std::unique_ptr<Element> createChild(const Transform& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
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