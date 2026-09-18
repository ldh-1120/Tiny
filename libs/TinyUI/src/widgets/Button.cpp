#include <tiny/ui/widgets/Button.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/FontMetrics.h>
#include <tiny/graphics/TextLayout.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class ButtonElement final : public Element {
		public:
			explicit ButtonElement(const Button& widget) : Element(widget), textValue(widget.text()), clickCallback(widget.onClick()), buttonStyle(widget.style()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const Button& buttonWidget = static_cast<const Button&>(widget);

				textValue = buttonWidget.text();
				clickCallback = buttonWidget.onClick();
				buttonStyle = buttonWidget.style();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				GraphicsContext& graphicsContext = context.graphicsContext();

				FontMetrics fontMetrics = graphicsContext.getFontMetrics(buttonStyle.textStyle);
				lineHeight = fontMetrics.lineHeight;
				if (lineHeight <= 0.0f)
					lineHeight = buttonStyle.textStyle.fontSize;

				float maximumContentWidth = constraints.maxWidth();
				if (std::isfinite(maximumContentWidth))
					maximumContentWidth = std::max(maximumContentWidth - buttonStyle.padding.horizontal(), 0.0f);

				textLayout = graphicsContext.createTextLayout(textValue, buttonStyle.textStyle, maximumContentWidth);

				Size textSize;
				if (textLayout)
					textSize = textLayout->size();

				float contentHeight = std::max(lineHeight, textSize.height);

				Size desiredSize(textSize.width + buttonStyle.padding.horizontal(), contentHeight + buttonStyle.padding.vertical());

				return constraints.constrain(desiredSize);
			}

			void paintOverride(Canvas& canvas) override {
				Color background = buttonStyle.background;

				bool visuallyPressed = keyboardPressedState || (pointerPressedState && hovered);
				if (visuallyPressed)
					background = buttonStyle.pressedBackground;
				else if (hovered)
					background = buttonStyle.hoveredBackground;

				canvas.fillRect(bounds(), background);

				Rect contentBounds = getContentBounds();
				if (textLayout) {
					Point textOrigin = getTextOrigin(contentBounds);
					canvas.drawTextLayout(*textLayout, textOrigin, buttonStyle.textColor);
				}

				if (hasFocus())
					canvas.drawRect(bounds(), buttonStyle.focusBorderColor, buttonStyle.focusBorderWidth);
			}

			bool acceptsPointerEvents() const override {
				return true;
			}

			void pointerEnterOverride(const PointerEvent& event) override {
				(void)event;

				if (hovered)
					return;

				hovered = true;

				markNeedsPaint();
			}

			void pointerLeaveOverride() override {
				if (!hovered)
					return;

				hovered = false;

				markNeedsPaint();
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				requestFocus();

				if (pointerPressedState)
					return true;

				pointerPressedState = true;

				markNeedsPaint();

				return true;
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				if (!pointerPressedState)
					return;

				bool shouldClick = bounds().contains(event.position);
				
				pointerPressedState = false;

				markNeedsPaint();
				if (!shouldClick)
					return;

				std::function<void()> callback = clickCallback;
				if (callback)
					callback();
			}

			void pointerCancelOverride() override {
				if (!pointerPressedState)
					return;

				pointerPressedState = false;

				markNeedsPaint();
			}

			bool focusable() const override {
				return true;
			}

			bool keyDownOverride(const KeyEvent& event) override {
				if (event.key == KeyCode::Space) {
					if (!keyboardPressedState) {
						keyboardPressedState = true;

						markNeedsPaint();
					}

					return true;
				}

				if (event.key == KeyCode::Enter) {
					if (!event.repeated) {
						std::function<void()> callback = clickCallback;
						if (callback)
							callback();
					}

					return true;
				}

				return false;
			}

			bool keyUpOverride(const KeyEvent& event) override {
				if (event.key == KeyCode::Enter)
					return true;

				if (event.key != KeyCode::Space)
					return false;

				if (!keyboardPressedState)
					return true;

				keyboardPressedState = false;

				markNeedsPaint();

				std::function<void()> callback = clickCallback;
				if (callback)
					callback();

				return true;
			}

 			void focusLostOverride() override {
				if (!keyboardPressedState)
					return;

				keyboardPressedState = false;

				markNeedsPaint();
			}

			Rect getContentBounds() const {
				const Rect& buttonBounds = bounds();

				return Rect(buttonBounds.x + buttonStyle.padding.left, buttonBounds.y + buttonStyle.padding.top, std::max(buttonBounds.width - buttonStyle.padding.horizontal(), 0.0f), std::max(buttonBounds.height - buttonStyle.padding.vertical(), 0.0f));
			}

			Point getTextOrigin(const Rect& contentBounds) {
				if (!textLayout)
					return Point(contentBounds.x, contentBounds.y);

				const Size& textSize = textLayout->size();

				float x = contentBounds.x + std::max((contentBounds.width - textSize.width) * 0.5f, 0.0f);
				float y = contentBounds.y + std::max((contentBounds.height - textSize.height) * 0.5f, 0.0f);

				return Point(x, y);
			}

		private:
			std::u32string textValue;

			std::function<void()> clickCallback;

			ButtonStyle buttonStyle;

			std::unique_ptr<TextLayout> textLayout;

			float lineHeight = 0.0f;

			bool hovered = false;
			bool pointerPressedState = false;
			bool keyboardPressedState = false;
		};
	}

	Button::Button(std::u32string text, std::function<void()> onClick, ButtonStyle style, Key key) : Widget(std::move(key)), textValue(std::move(text)), clickCallback(std::move(onClick)), buttonStyle(std::move(style)) { }

	const std::u32string& Button::text() const {
		return textValue;
	}

	const std::function<void()>& Button::onClick() const {
		return clickCallback;
	}

	const ButtonStyle& Button::style() const {
		return buttonStyle;
	}

	std::unique_ptr<Element> Button::createElement() const {
		return std::make_unique<ButtonElement>(*this);
	}
}