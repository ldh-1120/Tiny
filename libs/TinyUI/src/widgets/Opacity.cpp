#include <tiny/ui/widgets/Opacity.h>

#include <algorithm>
#include <memory>
#include <utility>

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
		class OpacityElement final : public ProxyElement {
		public:
			explicit OpacityElement(const Opacity& widget) : ProxyElement(widget, createChild(widget)), opacityValue(widget.opacity()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Opacity& opacity = static_cast<const Opacity&>(widget);

				opacityValue = opacity.opacity();

				updateChild(opacity.child());
				markNeedsLayout();
			}

			void paintOverride(Canvas& canvas) override {
				if (!hasChild())
					return;

				if (opacityValue <= 0.0f)
					return;

				if (opacityValue >= 1.0f) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				Rect availableBounds = canvas.currentPaintBounds();

				Rect layerBounds = intersectRect(child()->visualBounds(), availableBounds);
				if (layerBounds.isEmpty())
					return;

				Rect backdropBounds = intersectRect(child()->backdropReadBounds(), availableBounds);
				if (!backdropBounds.isEmpty() && !backdropSnapshot.capture(canvas, backdropBounds)) {
					SingleChildElement::paintOverride(canvas);
					return;
				}
				
				detail::OffscreenLayer layer(canvas, surfaceCache);

				bool began = layer.begin(layerBounds.size(), layerBounds.position());
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

				canvas.drawRenderSurface(*surface, layerBounds, opacityValue);
			}

		private:
			static std::unique_ptr<Element> createChild(const Opacity& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float opacityValue = 1.0f;

			detail::EffectSurfaceCache surfaceCache;
			detail::BackdropSnapshot backdropSnapshot;
		};
	}

	Opacity::Opacity(float opacity, std::unique_ptr<Widget> child, Key key) : Widget(std::move(key)), opacityValue(std::clamp(opacity, 0.0f, 1.0f)), childWidget(std::move(child)) { }

	float Opacity::opacity() const {
		return opacityValue;
	}

	const Widget* Opacity::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> Opacity::createElement() const {
		return std::make_unique<OpacityElement>(*this);
	}

}