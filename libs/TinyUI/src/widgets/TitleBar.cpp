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
				float brandWidth = effectiveBrandWidth(width);

				float toolbarAvailableWidth = std::max(width - brandWidth - buttonWidthValue * 3.0f, 0.0f);

				if (children()[0]) {
					Constraints toolbarConstraints(0.0f, toolbarAvailableWidth, 0.0f, heightValue);
					children()[0]->measure(context, toolbarConstraints);
				}

				float height = constraints.hasBoundedHeight() ? constraints.maxHeight() : contentSize.height + heightValue;

				TextStyle titleStyle;
				titleStyle.fontFamily = L"Segoe UI";
				titleStyle.fontSize = 14.0f;

				float titleWidth = std::max(brandWidth - styleValue.titleLeftPadding, 0.0f);
				titleLayout = createTrimmedTitleLayout(context.graphicsContext(), titleStyle, titleWidth);

				TextStyle iconStyle;
				iconStyle.fontFamily = L"Segoe UI";
				iconStyle.fontSize = 18.0f;

				closeIconLayout = context.graphicsContext().createTextLayout(U"\u00D7", iconStyle, buttonWidthValue);

				return constraints.constrain(Size(width, height));
			}

			void arrangeOverride(const Rect& finalBounds) override {
				float brandWidth = effectiveBrandWidth(finalBounds.width);

				float toolbarAvailableWidth = std::max(finalBounds.width - brandWidth - buttonWidthValue * 3.0f, 0.0f);
				if (children()[0]) {
					const Size& desired = children()[0]->desiredSize();
					
					float toolbarWidth = std::min(desired.width, toolbarAvailableWidth);
					float toolbarHeight = std::min(desired.height, heightValue);

					float toolbarY = finalBounds.y + (heightValue - toolbarHeight) * 0.5f;
					children()[0]->arrange(Rect(finalBounds.x + brandWidth, toolbarY, toolbarWidth, toolbarHeight));
				}

				if (children()[1]) {
					float contentHeight = std::max(finalBounds.height - heightValue, 0.0f);
					children()[1]->arrange(Rect(finalBounds.x, finalBounds.y + heightValue, finalBounds.width, contentHeight));
				}
			}

			void paintOverride(Canvas& canvas) override {
				const Rect& area = bounds();

				Rect titleArea(area.x, area.y, area.width, heightValue);
				canvas.fillRect(titleArea, styleValue.background);

				float brandWidth = effectiveBrandWidth(area.width);

				float iconSize = std::min(styleValue.iconSize, heightValue);
				float iconX = area.x + styleValue.iconLeftPadding;
				float iconY = area.y + (heightValue - iconSize) * 0.5f;

				if (iconSize > 0.0f && iconX + iconSize <= area.x + brandWidth)
					canvas.fillRect(Rect(iconX, iconY, iconSize, iconSize), styleValue.iconColor);

				if (titleLayout && brandWidth > 0.0f) {
					float textY = area.y + (heightValue - titleLayout->size().height) * 0.5f;

					Rect brandClip(area.x, area.y, brandWidth, heightValue);
					canvas.pushClip(brandClip);

					canvas.drawTextLayout(*titleLayout, Point(area.x + styleValue.titleLeftPadding, textY), styleValue.textColor);

					canvas.popClip();
				}

				float totalWidth = buttonWidthValue * 3.0f;
				if (area.width < totalWidth)
					return;

				bool maximized = false;
				if (maximizedCallback)
					maximized = maximizedCallback();

				if (children()[1])
					children()[1]->paint(canvas);

				float toolbarAvailableWidth = std::max(area.width - brandWidth - buttonWidthValue * 3.0f, 0.0f);
				if (children()[0] && toolbarAvailableWidth > 0.0f) {
					Rect toolbarClip(area.x + brandWidth, area.y, toolbarAvailableWidth, heightValue);
					canvas.pushClip(toolbarClip);

					children()[0]->paint(canvas);

					canvas.popClip();
				}

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

			Element* hitTestChildren(const Point& position) override {
				const Rect& area = bounds();

				float brandWidth = effectiveBrandWidth(area.width);
				float toolbarAvailableWidth = std::max(area.width - brandWidth - buttonWidthValue * 3.0f, 0.0f);

				Rect toolbarArea(area.x + brandWidth, area.y, toolbarAvailableWidth, heightValue);
				Rect contentArea(area.x, area.y + heightValue, area.width, std::max(area.height - heightValue, 0.0f));
				
				if (children()[0] && toolbarArea.contains(position))
					return children()[0]->hitTest(position);

				if (children()[1] && contentArea.contains(position))
					return children()[1]->hitTest(position);

				return nullptr;
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

			float effectiveBrandWidth(float availableWidth) const {
				float captionButtonsWidth = buttonWidthValue * 3.0f;
				float remainingWidth = std::max(availableWidth - captionButtonsWidth, 0.0f);

				return std::min(std::max(styleValue.brandWidth, 0.0f), remainingWidth);
			}

			std::unique_ptr<TextLayout> createTrimmedTitleLayout(GraphicsContext& graphicsContext, const TextStyle& textStyle, float availableWidth) const {
				if (availableWidth <= 0.0f)
					return nullptr;

				constexpr float measurementWidth = 100000.0f;

				std::unique_ptr<TextLayout> fullLayout = graphicsContext.createTextLayout(titleValue, textStyle, measurementWidth);
				if (!fullLayout)
					return nullptr;

				if (fullLayout->size().width <= availableWidth)
					return graphicsContext.createTextLayout(titleValue, textStyle, availableWidth);

				const std::u32string ellipsis = U"\u2026";

				std::unique_ptr<TextLayout> ellipsisLayout = graphicsContext.createTextLayout(ellipsis, textStyle, measurementWidth);
				if (!ellipsisLayout || ellipsisLayout->size().width > availableWidth)
					return nullptr;

				std::size_t left = 0;
				std::size_t right = titleValue.size();

				while (left < right) {
					std::size_t middle = left + (right - left + 1) / 2;

					std::u32string candidate = titleValue.substr(0, middle) + ellipsis;
					std::unique_ptr<TextLayout> candidateLayout = graphicsContext.createTextLayout(candidate, textStyle, measurementWidth);
					if (candidateLayout && candidateLayout->size().width <= availableWidth)
						left = middle;
					else
						right = middle - 1;
				}

				std::u32string result = titleValue.substr(0, left) + ellipsis;

				return graphicsContext.createTextLayout(result, textStyle, availableWidth);
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