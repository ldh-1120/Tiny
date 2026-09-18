#include <tiny/ui/widgets/Text.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/TextLayout.h>
#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class TextElement final : public Element {
		public:
			explicit TextElement(const Text& widget) : Element(widget), textValue(widget.text()), textColor(widget.color()), textStyle(widget.style()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Text& textWidget = static_cast<const Text&>(widget);
				textValue = textWidget.text();
				textStyle = textWidget.style();
				textColor = textWidget.color();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				GraphicsContext& graphicsContext = context.graphicsContext();

				float maximumWidth = constraints.maxWidth();
				textLayout = graphicsContext.createTextLayout(textValue, textStyle, maximumWidth);

				if (!textLayout)
					return constraints.constrain(Size());

				return constraints.constrain(textLayout->size());
			}

			void paintOverride(Canvas& canvas) override {
				if (!textLayout)
					return;

				const Rect& elementBounds = bounds();

				canvas.drawTextLayout(*textLayout, Point(elementBounds.x, elementBounds.y), textColor);
			}

		private:
			std::u32string textValue;
			Color textColor;
			TextStyle textStyle;

			std::unique_ptr<TextLayout> textLayout;
		};
	}

	Text::Text(std::u32string text, Color color, TextStyle style, Key key) : Widget(std::move(key)), textValue(std::move(text)), textColor(color), textStyle(std::move(style)) { }

	const std::u32string& Text::text() const {
		return textValue;
	}

	const TextStyle& Text::style() const {
		return textStyle;
	}

	const Color& Text::color() const {
		return textColor;
	}

	std::unique_ptr<Element> Text::createElement() const {
		return std::make_unique<TextElement>(*this);
	}
}