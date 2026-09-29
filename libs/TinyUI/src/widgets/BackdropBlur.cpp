#include <tiny/ui/widgets/BackdropBlur.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/RenderSurface.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

#include "EffectSurfaceCache.h"

namespace tiny {
	namespace {
		class BackdropBlurElement final : public SingleChildElement {
		public:
			explicit BackdropBlurElement(const BackdropBlur& widget) : SingleChildElement(widget, createChild(widget)), radiusValue(widget.radius()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const BackdropBlur& blur = static_cast<const BackdropBlur&>(widget);

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

				Rect captureBounds = ownBackdropReadBounds();
				RenderSurface* surface = surfaceCache.ensure(canvas, captureBounds.size());

				bool captured = false;
				if (surface)
					captured = canvas.captureBackdropSurface(*surface, captureBounds);

				if (!captured) {
					surface = surfaceCache.recreate(canvas, captureBounds.size());
					if (surface)
						captured = canvas.captureBackdropSurface(*surface, captureBounds);
				}

				if (captured && surface) {
					canvas.pushClip(bounds());

					canvas.drawBlurredRenderSurface(*surface, captureBounds.position(), radiusValue);

					canvas.popClip();
				}

				SingleChildElement::paintOverride(canvas);
			}

			Rect backdropReadBoundsOverride() const override {
				Rect result;
				if (hasChild())
					result = child()->backdropReadBounds();
				
				return unionRect(result, ownBackdropReadBounds());
			}

		private:
			static std::unique_ptr<Element> createChild(const BackdropBlur& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

			Rect ownBackdropReadBounds() const {
				if (radiusValue <= 0.0f)
					return Rect();

				float padding = std::ceil(radiusValue * 3.0f);

				return Rect(bounds().x - padding, bounds().y - padding, bounds().width + padding * 2.0f, bounds().height + padding * 2.0f);
			}

		private:
			float radiusValue = 0.0f;

			detail::EffectSurfaceCache surfaceCache;
		};
	}

	BackdropBlur::BackdropBlur(float radius, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), radiusValue(std::clamp(radius, 0.0f, 250.0f)), childWidget(std::move(child)) {}

	float BackdropBlur::radius() const {
		return radiusValue;
	}

	const Widget* BackdropBlur::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> BackdropBlur::createElement() const {
		return std::make_unique<BackdropBlurElement>(*this);
	}
}