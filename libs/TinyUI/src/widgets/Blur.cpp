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

				float padding = std::ceil(radiusValue * 3.0f);

				Rect effectBounds(bounds().x - padding, bounds().y - padding, bounds().width + padding * 2.0f, bounds().height + padding * 2.0f);
				if (effectBounds.isEmpty())
					return;

				RenderSurface* surface = ensureSurface(canvas, effectBounds.size());
				if (!surface) {
					SingleChildElement::paintOverride(canvas);
					return;
				}

				bool pushed = canvas.pushRenderSurface(*surface, effectBounds.position());
				if (!pushed) {
					renderSurface.reset();
					surface = ensureSurface(canvas, effectBounds.size());
					if (surface)
						pushed = canvas.pushRenderSurface(*surface, effectBounds.position());
				}

				if (!pushed) {
					SingleChildElement::paintOverride(canvas);
					return;
				}
				
				SingleChildElement::paintOverride(canvas);

				canvas.popRenderSurface();
				canvas.drawBlurredRenderSurface(*surface, effectBounds.position(), radiusValue);
			}

		private:
			static std::unique_ptr<Element> createChild(const Blur& widget) {
				const Widget* childWidget = widget.child();
				if (!childWidget)
					return nullptr;

				return childWidget->createElement();
			}

			bool surfaceMatches(const RenderSurface& surface, const Size& size, float dpiScale) const {
				constexpr float Epsilon = 0.001f;

				const Size& surfaceSize = surface.size();
				if (std::abs(surfaceSize.width - size.width) > Epsilon)
					return false;

				if (std::abs(surfaceSize.height - size.height) > Epsilon)
					return false;

				if (std::abs(surface.dpiScale() - dpiScale) > Epsilon)
					return false;

				return true;
			}

			RenderSurface* ensureSurface(Canvas& canvas, const Size& size) {
				float currentDpiScale = canvas.dpiScale();
				if (renderSurface && surfaceMatches(*renderSurface, size, currentDpiScale))
					return renderSurface.get();

				renderSurface = canvas.createRenderSurface(size);
				return renderSurface.get();
			}

		private:
			float radiusValue = 0.0f;

			std::unique_ptr<RenderSurface> renderSurface;
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