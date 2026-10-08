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
#include <tiny/ui/ProxyElement.h>
#include <tiny/ui/layout/Constraints.h>

#include "EffectSurfaceCache.h"
#include "OffscreenLayer.h"
#include "BackdropSnapshot.h"

namespace tiny {
	namespace {
		class BlurElement final : public ProxyElement {
		public:
			explicit BlurElement(const Blur& widget) : ProxyElement(widget, createChild(widget)), radiusValue(widget.radius()) {}

		protected:
			void updateOverride(const Widget& widget) override {
				const Blur& blur = static_cast<const Blur&>(widget);

				radiusValue = blur.radius();

				updateChild(blur.child());
				markNeedsLayout();
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

				Rect backdropBounds = intersectRect(child()->backdropReadBounds(), canvas.currentPaintBounds());
				if (!backdropBounds.isEmpty() && !backdropSnapshot.capture(canvas, backdropBounds)) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				detail::OffscreenLayer layer(canvas, surfaceCache);

				bool began = layer.begin(effectBounds.size(), effectBounds.position());
				if (!began) {
					SingleChildElement::paintOverride(canvas);
					return;
				}
				
				{
					detail::BackdropScope backdropScope(canvas, backdropSnapshot);
					SingleChildElement::paintOverride(canvas);
				}

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
			detail::BackdropSnapshot backdropSnapshot;
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