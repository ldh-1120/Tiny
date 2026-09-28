#include <tiny/ui/widgets/DropShadow.h>

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
#include "OffscreenLayer.h"

namespace tiny {
	namespace {
		class DropShadowElement final : public SingleChildElement {
		public:
			explicit DropShadowElement(const DropShadow& widget) : SingleChildElement(widget, createChild(widget)), blurRadiusValue(widget.blurRadius()), offsetValue(widget.offset()), colorValue(widget.color()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const DropShadow& dropShadow = static_cast<const DropShadow&>(widget);

				blurRadiusValue = dropShadow.blurRadius();
				offsetValue = dropShadow.offset();
				colorValue = dropShadow.color();

				updateChild(dropShadow.child());
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

				if (colorValue.a <= 0.0f) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				Rect childBounds = child()->visualBounds();
				float padding = std::ceil(blurRadiusValue * 3.0f);

				float leftExtra = padding + std::max(-offsetValue.x, 0.0f);
				float rightExtra = padding + std::max(offsetValue.x, 0.0f);

				float topExtra = padding + std::max(-offsetValue.y, 0.0f);
				float bottomExtra = padding + std::max(offsetValue.y, 0.0f);

				Rect effectBounds(childBounds.x - leftExtra, childBounds.y - topExtra, childBounds.width + leftExtra + rightExtra, childBounds.height + topExtra + bottomExtra);
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

				canvas.drawShadowRenderSurface(*surface, effectBounds.position(), offsetValue, blurRadiusValue, colorValue);
				canvas.drawRenderSurface(*surface, effectBounds);
			}

			Rect visualBoundsOverride() const override {
				if (!hasChild())
					return Rect();

				Rect childBounds = child()->visualBounds();
				if (colorValue.a <= 0.0f)
					return childBounds;

				float padding = std::ceil(blurRadiusValue * 3.0f);
				Rect shadowBounds(childBounds.x + offsetValue.x - padding, childBounds.y + offsetValue.y - padding, childBounds.width + padding * 2.0f, childBounds.height + padding * 2.0f);

				return unionRect(childBounds, shadowBounds);
			}

		private:
			static std::unique_ptr<Element> createChild(const DropShadow& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

		private:
			float blurRadiusValue = 0.0f;

			Point offsetValue;
			Color colorValue;

			detail::EffectSurfaceCache surfaceCache;
		};
	}

	DropShadow::DropShadow(float blurRadius, Point offset, Color color, std::unique_ptr<Widget> child, Key key)
		: Widget(std::move(key)), blurRadiusValue(std::clamp(blurRadius, 0.0f, 250.0f)), offsetValue(offset), colorValue(color), childWidget(std::move(child)) {}

	float DropShadow::blurRadius() const {
		return blurRadiusValue;
	}

	const Point& DropShadow::offset() const {
		return offsetValue;
	}

	const Color& DropShadow::color() const {
		return colorValue;
	}

	const Widget* DropShadow::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> DropShadow::createElement() const {
		return std::make_unique<DropShadowElement>(*this);
	}
}