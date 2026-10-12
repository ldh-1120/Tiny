#include <tiny/ui/widgets/CheckBox.h>

#include <algorithm>
#include <cmath>
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
#include <tiny/ui/visual/FocusRing.h>

namespace tiny {
	namespace {
		class CheckBoxElement final : public Element {
		public:
			explicit CheckBoxElement(const CheckBox& widget)
				: Element(widget), checkedValue(widget.checked()), labelValue(widget.label()), changedCallback(widget.onChanged()), checkBoxStyle(widget.style()) {
				setEnabled(widget.enabled());
			};
			
		protected:
			void updateOverride(const Widget& widget) override {
				const CheckBox& checkBox = static_cast<const CheckBox&>(widget);

				checkedValue = checkBox.checked();
				labelValue = checkBox.label();
				changedCallback = checkBox.onChanged();
				checkBoxStyle = checkBox.style();

				setEnabled(checkBox.enabled());

				if (!isEnabled()) {
					hovered = false;
					pointerPressedState = false;
					keyboardPressedState = false;
				}

				markNeedsLayout();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				float size = std::max(checkBoxStyle.boxSize, 0.0f);
				float spacing = std::max(checkBoxStyle.spacing, 0.0f);

				Size textSize { };
				if (!labelValue.empty()) {
					GraphicsContext& graphics = context.graphicsContext();

					float maximumTextWidth = std::numeric_limits<float>::infinity();
					if (constraints.hasBoundedWidth())
						maximumTextWidth = std::max(constraints.maxWidth() - checkBoxStyle.padding.horizontal() - size - spacing, 0.0f);

					textLayout = graphics.createTextLayout(labelValue, checkBoxStyle.textStyle, maximumTextWidth);
					if (textLayout)
						textSize = textLayout->size();

					FontMetrics metrics = graphics.getFontMetrics(checkBoxStyle.textStyle);
					textSize.height = std::max(textSize.height, metrics.lineHeight);
				} else
					textLayout.reset();

				float width = size;
				if (!labelValue.empty())
					width += spacing + textSize.width;

				float height = std::max(size, textSize.height);
				return constraints.constrain(Size(width + checkBoxStyle.padding.horizontal(), height + checkBoxStyle.padding.vertical()));
			}

			void paintOverride(Canvas& canvas) override {
				const Rect& area = bounds();

				float availableWidth = std::max(area.width - checkBoxStyle.padding.horizontal(), 0.0f);
				float availableHeight = std::max(area.height - checkBoxStyle.padding.vertical(), 0.0f);

				float size = std::min(std::max(checkBoxStyle.boxSize, 0.0f), std::min(availableWidth, availableHeight));
				Rect box(area.x + checkBoxStyle.padding.left, area.y + checkBoxStyle.padding.top + std::max((availableHeight - size) * 0.5f, 0.0f), size, size);

				Color background = checkedValue ? checkBoxStyle.checkedBackground : checkBoxStyle.background;
				if (isEnabled() && hovered)
					background = checkedValue ? checkBoxStyle.checkedHoveredBackground : checkBoxStyle.hoveredBackground;

				if (!isEnabled())
					background = checkBoxStyle.disabledBackground;

				canvas.fillRect(box, background);

				Color border = checkedValue && isEnabled() ? background : checkBoxStyle.borderColor;
				canvas.drawRect(box, border, checkBoxStyle.borderWidth);

				if (checkedValue)
					paintCheckMark(canvas, box);

				if (textLayout) {
					Size textSize = textLayout->size();
					Point origin(box.right() + checkBoxStyle.spacing, area.y + checkBoxStyle.padding.top + std::max((availableHeight - textSize.height) * 0.5f, 0.0f));

					canvas.drawTextLayout(*textLayout, origin, isEnabled() ? checkBoxStyle.textColor : checkBoxStyle.disabledTextColor);
				}

				paintFocusRing(canvas, area, checkBoxStyle.focusColor, checkBoxStyle.focusWidth, isFocusVisible() ? 1.0f : 0.0f);
			}

			bool acceptsPointerEvents() const override {
				return true;
			}

			bool focusable() const override {
				return true;
			}

			PointerCursor pointerCursorOverride() const override {
				return PointerCursor::Hand;
			}

			void pointerEnterOverride(const PointerEvent&) override {
				hovered = true;
				markNeedsPaint();
			}

			void pointerLeaveOverride() override {
				hovered = false;
				markNeedsPaint();
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				requestFocus();

				pointerPressedState = true;
				markNeedsPaint();

				return true;
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				if (!pointerPressedState)
					return;

				bool shouldToggle = bounds().contains(event.position);

				pointerPressedState = false;
				markNeedsPaint();

				if (shouldToggle)
					emitChanged();
			}

			void pointerCancelOverride() override {
				pointerPressedState = false;
				markNeedsPaint();
			}

			bool keyDownOverride(const KeyEvent& event) override {
				if (event.key != KeyCode::Space)
					return false;

				if (!keyboardPressedState) {
					keyboardPressedState = true;
					markNeedsPaint();
				}

				return true;
			}

			bool keyUpOverride(const KeyEvent& event) override {
				if (event.key != KeyCode::Space)
					return false;

				if (!keyboardPressedState)
					return true;

				keyboardPressedState = false;
				markNeedsPaint();

				emitChanged();
				return true;
			}

			void focusLostOverride() override {
				keyboardPressedState = false;
				markNeedsPaint();
			}

			void focusVisibilityChangedOverride(bool) override {
				markNeedsPaint();
			}

		private:
			void emitChanged() {
				CheckBox::ChangedCallback callback = changedCallback;
				if (callback)
					callback(!checkedValue);
			}

			void paintCheckMark(Canvas& canvas, const Rect& box) const {
				if (box.width <= 0.0f || box.height <= 0.0f)
					return;

				Color color = isEnabled() ? checkBoxStyle.checkColor : checkBoxStyle.disabledCheckColor;
				float strokeWidth = std::clamp(box.width * 0.12f, 1.5f, 2.25f);

				Point start(box.x + box.width * 0.21f, box.y + box.height * 0.52f);
				Point middle(box.x + box.width * 0.41f, box.y + box.height * 0.72f);
				Point end(box.x + box.width * 0.80f, box.y + box.height * 0.29f);

				canvas.drawLine(start, middle, color, strokeWidth);
				canvas.drawLine(middle, end, color, strokeWidth);
			}

		private:
			bool checkedValue = false;
			bool hovered = false;

			bool pointerPressedState = false;
			bool keyboardPressedState = false;

			std::u32string labelValue;
			CheckBox::ChangedCallback changedCallback;
			CheckBoxStyle checkBoxStyle;

			std::unique_ptr<TextLayout> textLayout;
		};
	}

	CheckBox::CheckBox(bool checked, std::u32string label, ChangedCallback onChanged, CheckBoxStyle style, bool enabled, Key key)
		: Widget(std::move(key)), checkedValue(checked), enabledValue(enabled), labelValue(std::move(label)), changedCallback(std::move(onChanged)), checkBoxStyle(std::move(style)) {}

	bool CheckBox::checked() const {
		return checkedValue;
	}

	bool CheckBox::enabled() const {
		return enabledValue;
	}

	const std::u32string& CheckBox::label() const {
		return labelValue;
	}

	const CheckBox::ChangedCallback& CheckBox::onChanged() const {
		return changedCallback;
	}

	const CheckBoxStyle& CheckBox::style() const {
		return checkBoxStyle;
	}

	std::unique_ptr<Element> CheckBox::createElement() const {
		return std::make_unique<CheckBoxElement>(*this);
	}
}