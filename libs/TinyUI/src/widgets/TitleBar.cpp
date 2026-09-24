#include <tiny/ui/widgets/TitleBar.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/TextLayout.h>
#include <tiny/graphics/TextStyle.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/MultiChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		enum class CaptionButton {
			None,
			Minimize,
			Maximize,
			Close
		};

		class TitleBarElement final : public MultiChildElement {
		public:
			explicit TitleBarElement(const TitleBar& widget)
				: MultiChildElement(widget, createChildren(widget)), titleValue(widget.title()), actionCallback(widget.onAction()), maximizedCallback(widget.isMaximized()), heightValue(widget.height()), buttonWidthValue(widget.buttonWidth()), styleValue(widget.style()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const TitleBar& titleBar = static_cast<const TitleBar&>(widget);

				titleValue = titleBar.title();

				actionCallback = titleBar.onAction();
				maximizedCallback = titleBar.isMaximized();

				heightValue = titleBar.height();
				buttonWidthValue = titleBar.buttonWidth();

				styleValue = titleBar.style();

				updateChildren(std::vector<const Widget*> { titleBar.toolbar(), titleBar.child() });
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float availableWidth = constraints.maxWidth();
				float contentHeight = constraints.hasBoundedHeight() ? std::max(constraints.maxHeight() - heightValue, 0.0f) : constraints.maxHeight();

				Size contentSize;
				if (children()[1]) {
					Constraints childConstraints(0.0f, availableWidth, 0.0f, contentHeight);
					contentSize = children()[1]->measure(context, childConstraints);
				}

				float width = constraints.hasBoundedWidth() ? constraints.maxWidth() : contentSize.width;
				float toolbarAvailableWidth = std::max(width - 160.0f - buttonWidthValue * 3.0f, 0.0f);

				if (children()[0]) {
					Constraints toolbarConstraints(0.0f, toolbarAvailableWidth, 0.0f, heightValue);
					children()[0]->measure(context, toolbarConstraints);
				}

				float height = constraints.hasBoundedHeight() ? constraints.maxHeight() : contentSize.height + heightValue;

				TextStyle titleStyle;
				titleStyle.fontFamily = L"Segoe UI";
				titleStyle.fontSize = 14.0f;

				float titleWidth = std::max(std::min(132.0f, width - buttonWidthValue * 3.0f - 28.0f), 0.0f);
				titleLayout = context.graphicsContext().createTextLayout(titleValue, titleStyle, titleWidth);

				TextStyle iconStyle;
				iconStyle.fontFamily = L"Segoe UI";
				iconStyle.fontSize = 18.0f;

				closeIconLayout = context.graphicsContext().createTextLayout(U"\u00D7", iconStyle, buttonWidthValue);

				return constraints.constrain(Size(width, height));
			}

			void arrangeOverride(const Rect& finalBounds) override {
				float toolbarStartX = finalBounds.x + 160.0f;

				float toolbarAvailableWidth = std::max(finalBounds.width - 160.0f - buttonWidthValue * 3.0f, 0.0f);
				if (children()[0]) {
					const Size& desired = children()[0]->desiredSize();
					
					float toolbarWidth = std::min(desired.width, toolbarAvailableWidth);
					float toolbarHeight = std::min(desired.height, heightValue);

					float toolbarY = finalBounds.y + (heightValue - toolbarHeight) * 0.5f;
					children()[0]->arrange(Rect(toolbarStartX, toolbarY, toolbarWidth, toolbarHeight));
				}

				if (children()[1]) {
					float contentHeight = std::max(finalBounds.height - heightValue, 0.0f);
					children()[1]->arrange(Rect(finalBounds.x, finalBounds.y + heightValue, finalBounds.width, contentHeight));
				}
			}

			bool hitTestSelf(const Point& position) const override {
				return buttonAt(position) != CaptionButton::None;
			}

			bool acceptsPointerEvents() const override {
				return true;
			}

			void pointerEnterOverride(const PointerEvent& event) override {
				updateHoveredButton(event.position);
			}

			void pointerMoveOverride(const PointerEvent& event) override {
				updateHoveredButton(event.position);
			}

			void pointerLeaveOverride() override {
				if (hoveredButton == CaptionButton::None)
					return;

				hoveredButton = CaptionButton::None;

				markNeedsPaint();
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				CaptionButton button = buttonAt(event.position);
				if (button == CaptionButton::None)
					return false;

				pressedButton = button;
				hoveredButton = button;

				markNeedsPaint();

				return true;
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				if (pressedButton == CaptionButton::None)
					return;

				CaptionButton pressed = pressedButton;
				CaptionButton released = buttonAt(event.position);

				pressedButton = CaptionButton::None;
				hoveredButton = released;

				markNeedsPaint();

				if (pressed != released)
					return;

				TitleBarAction action;
				switch (pressed) {
					case CaptionButton::Minimize:
						action = TitleBarAction::Minimize;
						break;

					case CaptionButton::Maximize:
						action = TitleBarAction::Maximize;
						break;

					case CaptionButton::Close:
						action = TitleBarAction::Close;
						break;

					default:
						return;
				}

				TitleBar::ActionCallback callback = actionCallback;
				if (callback)
					callback(action);
			}

			void pointerCancelOverride() override {
				if (pressedButton == CaptionButton::None)
					return;

				pressedButton = CaptionButton::None;

				markNeedsPaint();
			}

			void paintOverride(Canvas& canvas) override {
				const Rect& area = bounds();

				Rect titleArea(area.x, area.y, area.width, heightValue);
				canvas.fillRect(titleArea, styleValue.background);
				
				if (titleLayout) {
					float textY = area.y + (heightValue - titleLayout->size().height) * 0.5f;
					canvas.drawTextLayout(*titleLayout, Point(area.x + 14.0f, textY), styleValue.textColor);
				}

				float totalWidth = buttonWidthValue * 3.0f;
				if (area.width < totalWidth)
					return;

				bool maximized = false;
				if (maximizedCallback)
					maximized = maximizedCallback();

				MultiChildElement::paintOverride(canvas);

				for (int index = 0; index < 3; ++index) {
					CaptionButton button = CaptionButton::None;
					switch (index) {
						case 0:
							button = CaptionButton::Minimize;
							break;

						case 1:
							button = CaptionButton::Maximize;
							break;

						case 2:
							button = CaptionButton::Close;
							break;
					}

					float buttonX = area.x + area.width - totalWidth + static_cast<float>(index) * buttonWidthValue;
					Rect buttonBounds(buttonX, area.y, buttonWidthValue, heightValue);

					bool hovered = hoveredButton == button;
					bool pressed = pressedButton == button && hovered;

					Color background = styleValue.background;
					if (button == CaptionButton::Close) {
						if (pressed)
							background = styleValue.closePressedBackground;
						else if (hovered)
							background = styleValue.closeHoveredBackground;
					} else {
						if (pressed)
							background = styleValue.pressedBackground;
						else if (hovered)
							background = styleValue.hoveredBackground;
					}

					canvas.fillRect(buttonBounds, background);

					float centerX = buttonX + buttonWidthValue * 0.5f;
					float centerY = area.y + heightValue * 0.5f;

					if (button == CaptionButton::Minimize)
						canvas.fillRect(Rect(centerX - 5.0f, centerY, 10.0f, 1.0f), styleValue.textColor);
					else if (button == CaptionButton::Maximize) {
						if (maximized) {
							canvas.drawRect(Rect(centerX - 3.0f, centerY - 5.0f, 9.0f, 9.0f), styleValue.textColor, 1.0f);
							canvas.drawRect(Rect(centerX - 6.0f, centerY - 2.0f, 9.0f, 9.0f), styleValue.textColor, 1.0f);
						} else
							canvas.drawRect(Rect(centerX - 5.0f, centerY - 5.0f, 10.0f, 10.0f), styleValue.textColor, 1.0f);
					} else if (button == CaptionButton::Close && closeIconLayout) {
						float iconX = centerX - closeIconLayout->size().width * 0.5f;
						float iconY = centerY - closeIconLayout->size().height * 0.5f;

						canvas.drawTextLayout(*closeIconLayout, Point(iconX, iconY), styleValue.textColor);
					}
				}

				canvas.fillRect(Rect(area.x, area.y + heightValue - 1.0f, area.width, 1.0f), styleValue.borderColor);
			}

		private:
			static std::unique_ptr<Element> createElementFor(const Widget* widget) {
				if (!widget)
					return nullptr;

				return widget->createElement();
			}

			static std::vector<std::unique_ptr<Element>> createChildren(const TitleBar& widget) {
				std::vector<std::unique_ptr<Element>> result;

				result.push_back(createElementFor(widget.toolbar()));
				result.push_back(createElementFor(widget.child()));

				return result;
			}

			CaptionButton buttonAt(const Point& position) const {
				const Rect& area = bounds();

				float totalWidth = buttonWidthValue * 3.0f;
				if (area.width < totalWidth)
					return CaptionButton::None;

				if (position.y < area.y || position.y >= area.y + heightValue)
					return CaptionButton::None;

				float startX = area.x + area.width - totalWidth;
				if (position.x < startX || position.x >= area.x + area.width)
					return CaptionButton::None;

				float localX = position.x - startX;
				if (localX < buttonWidthValue)
					return CaptionButton::Minimize;

				if (localX < buttonWidthValue * 2.0f)
					return CaptionButton::Maximize;

				return CaptionButton::Close;
			}

			void updateHoveredButton(const Point& position) {
				CaptionButton button = buttonAt(position);
				if (hoveredButton == button)
					return;

				hoveredButton = button;

				markNeedsPaint();
			}

		private:
			std::u32string titleValue;

			TitleBar::ActionCallback actionCallback;
			TitleBar::MaximizedCallback maximizedCallback;

			float heightValue = 40.0f;
			float buttonWidthValue = 46.0f;

			TitleBarStyle styleValue;

			CaptionButton hoveredButton = CaptionButton::None;
			CaptionButton pressedButton = CaptionButton::None;

			std::unique_ptr<TextLayout> titleLayout;
			std::unique_ptr<TextLayout> closeIconLayout;
		};
	}

	TitleBar::TitleBar(std::u32string title, std::unique_ptr<Widget> child, ActionCallback onAction, MaximizedCallback isMaximized, float height, float buttonWidth, TitleBarStyle style, Key key, std::unique_ptr<Widget> toolbar)
		: Widget(std::move(key)), titleValue(std::move(title)), childWidget(std::move(child)), toolbarWidget(std::move(toolbar)), actionCallback(std::move(onAction)), maximizedCallback(std::move(isMaximized)), heightValue(std::max(height, 0.0f)), buttonWidthValue(std::max(buttonWidth, 1.0f)), styleValue(std::move(style)) {}

	const std::u32string& TitleBar::title() const {
		return titleValue;
	}

	const Widget* TitleBar::child() const {
		return childWidget.get();
	}

	const Widget* TitleBar::toolbar() const {
		return toolbarWidget.get();
	}

	const tiny::TitleBar::ActionCallback& TitleBar::onAction() const {
		return actionCallback;
	}

	const TitleBar::MaximizedCallback& TitleBar::isMaximized() const {
		return maximizedCallback;
	}

	float TitleBar::height() const {
		return heightValue;
	}

	float TitleBar::buttonWidth() const {
		return buttonWidthValue;
	}

	const TitleBarStyle& TitleBar::style() const {
		return styleValue;
	}

	std::unique_ptr<Element> TitleBar::createElement() const {
		return std::make_unique<TitleBarElement>(*this);
	}
}