#include <tiny/ui/widgets/Blur.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

#include "EffectSurfaceCache.h"
#include "OffscreenLayer.h"

namespace tiny {
	namespace {
		class BlurElement final : public SingleChildElement {
		public:
			explicit BlurElement(const Blur& widget) : SingleChildElement(widget, createChild(widget)), radiusValue(widget.radius()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Blur& blur = static_cast<const Blur&>(widget);

				radiusValue = blur.radius();

				updateChild(blur.child());
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

				if (radiusValue <= 0.0f) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				Rect effectBounds = visualBoundsOverride();
				if (effectBounds.isEmpty())
					return;

				detail::OffscreenLayer layer(canvas, surfaceCache);

				bool began = layer.begin(effectBounds.size(), effectBounds.position());
				if (!began) {
					SingleChildElement::paintOverride(canvas);
					return;
				}
				
				SingleChildElement::paintOverride(canvas);

				layer.end();

				RenderSurface* surface = layer.surface();
				if (!surface)
					return;

				canvas.drawBlurredRenderSurface(*surface, effectBounds.position(), radiusValue);
			}

			Rect visualBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				Rect childBounds = child()->visualBounds();

				float padding = std::ceil(radiusValue * 3.0f);
				return Rect(childBounds.x - padding, childBounds.y - padding, childBounds.width + padding * 2.0f, childBounds.height + padding * 2.0f);
			}

		private:
			static std::unique_ptr<Element> createChild(const Blur& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float radiusValue = 0.0f;

			detail::EffectSurfaceCache surfaceCache;
		};
	}

	Blur::Blur(float radius, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), radiusValue(std::clamp(radius, 0.0f, 250.0f)), childWidget(std::move(child)) { }

	float Blur::radius() const {
		return radiusValue;
	}

	const Widget* Blur::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Blur::createElement() const {
		return std::make_unique<BlurElement>(*this);
	}
}