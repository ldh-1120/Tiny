#include <tiny/ui/widgets/Box.h>

#include <memory>
#include <utility>

#include <tiny/core/Rect.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class BoxElement final : public Element {
		public:
			BoxElement(const Box& widget) : Element(widget), requestedSize(widget.requestedSize()), color(widget.color()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Box& box = static_cast<const Box&>(widget);
				requestedSize = box.requestedSize();
				color = box.color();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				return constraints.constrain(requestedSize);
			}

			void paintOverride(Canvas& canvas) override {
				canvas.fillRect(bounds(), color);
			}

		private:
			Size requestedSize;
			Color color;
		};
	}

	Box::Box(const Size& size, const Color& color, Key key) : Widget(key), boxSize(size), boxColor(color) { }

	const Size& Box::requestedSize() const {
		return boxSize;
	}

	const Color& Box::color() const {
		return boxColor;
	}

	std::unique_ptr<Element> Box::createElement() const {
		return std::make_unique<BoxElement>(*this);
	}
}