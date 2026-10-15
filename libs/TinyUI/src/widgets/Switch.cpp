#include <tiny/ui/widgets/Switch.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/FontMetrics.h>
#include <tiny/graphics/TextLayout.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>
#include <tiny/ui/animation/AnimationController.h>
#include <tiny/ui/visual/FocusRing.h>

namespace tiny {
	namespace {
		class SwitchElement final : public Element {
        public:
            explicit SwitchElement(const Switch& widget)
                : Element(widget), checkedValue(widget.checked()), labelValue(widget.label()), changedCallback(widget.onChanged()), switchStyle(widget.style()), thumbAnimation(widget.checked() ? 1.0f : 0.0f) {
                setEnabled(widget.enabled());
            }

        protected:
            void updateOverride(const Widget& widget) override {
                const Switch& control = static_cast<const Switch&>(widget);

                labelValue = control.label();
                changedCallback = control.onChanged();
                switchStyle = control.style();

                setEnabled(control.enabled());
                if (!isEnabled()) {
                    hovered = false;
                    pointerPressedState = false;
                    keyboardPressedState = false;
                }

                if (checkedValue != control.checked()) {
                    checkedValue = control.checked();
                    thumbAnimation.animateTo(checkedValue ? 1.0f : 0.0f, 0.14f, Easing::EaseOutCubic);

                    setFrameUpdatesEnabled(thumbAnimation.isRunning());
                }

                markNeedsLayout();
            }

            Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
                float width = std::max(switchStyle.trackWidth, 0.0f);
                float height = std::max(switchStyle.trackHeight, 0.0f);

                Size textSize { };
                if (!labelValue.empty()) {
                    GraphicsContext& graphics = context.graphicsContext();

                    float maxTextWidth = std::numeric_limits<float>::infinity();
                    if (constraints.hasBoundedWidth())
                        maxTextWidth = std::max(constraints.maxWidth() - switchStyle.padding.horizontal() - width - switchStyle.spacing, 0.0f);

                    textLayout = graphics.createTextLayout(labelValue, switchStyle.textStyle, maxTextWidth);
                    if (textLayout)
                        textSize = textLayout->size();

                    FontMetrics metrics = graphics.getFontMetrics(switchStyle.textStyle);
                    textSize.height = std::max(textSize.height, metrics.lineHeight);

                    width += switchStyle.spacing + textSize.width;
                    height = std::max(height, textSize.height);
                } else
                    textLayout.reset();

                return constraints.constrain(Size(width + switchStyle.padding.horizontal(), height + switchStyle.padding.vertical()));
            }

            void paintOverride(Canvas& canvas) override {
                const Rect& area = bounds();

                float availableWidth = std::max(area.width - switchStyle.padding.horizontal(), 0.0f);
                float availableHeight = std::max(area.height - switchStyle.padding.vertical(), 0.0f);

                float trackWidth = std::min(std::max(switchStyle.trackWidth, 0.0f), availableWidth);
                float trackHeight = std::min(std::max(switchStyle.trackHeight, 0.0f), availableHeight);

                Rect track(area.x + switchStyle.padding.left, area.y + switchStyle.padding.top + std::max((availableHeight - trackHeight) * 0.5f, 0.0f), trackWidth, trackHeight);
                float progress = std::clamp(thumbAnimation.value(), 0.0f, 1.0f);

                bool highlighted = hovered || pointerPressedState;

                Color offColor = highlighted ? switchStyle.offHoverColor : switchStyle.offColor;
                Color onColor = highlighted ? switchStyle.onHoverColor : switchStyle.onColor;
                Color trackColor = isEnabled() ? lerpColor(offColor, onColor, progress) : switchStyle.disabledColor;

                canvas.fillRect(track, trackColor);

                Color borderColor = isEnabled() ? lerpColor(switchStyle.borderColor, onColor, progress) : switchStyle.borderColor;
                canvas.drawRect(track, borderColor, switchStyle.borderWidth);

                float inset = std::max(switchStyle.thumbInset, 0.0f);
                float thumbWidth = std::min(std::max(switchStyle.thumbWidth, 0.0f), std::max(trackWidth - inset * 2.0f, 0.0f));
                float thumbHeight = std::max(trackHeight - inset * 2.0f, 0.0f);

                float travel = std::max(trackWidth - inset * 2.0f - thumbWidth, 0.0f);

                Rect thumb(track.x + inset + travel * progress, track.y + (trackHeight - thumbHeight) * 0.5f, thumbWidth, thumbHeight);
                canvas.fillRect(thumb, isEnabled() ? switchStyle.thumbColor : switchStyle.disabledThumbColor);

                if (textLayout) {
                    Size textSize = textLayout->size();
                    Point origin(track.right() + switchStyle.spacing, area.y + switchStyle.padding.top + std::max((availableHeight - textSize.height) * 0.5f, 0.0f));

                    canvas.drawTextLayout(*textLayout, origin, isEnabled() ? switchStyle.textColor : switchStyle.disabledTextColor);
                }

                paintFocusRing(canvas, area, switchStyle.focusColor, switchStyle.focusWidth, isFocusVisible() ? 1.0f : 0.0f);
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

                bool activate = bounds().contains(event.position);

                pointerPressedState = false;
                markNeedsPaint();

                if (activate)
                    toggle();
            }

            void pointerCancelOverride() override {
                pointerPressedState = false;
                markNeedsPaint();
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
                    if (!event.repeated)
                        toggle();

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

                toggle();
                return true;
            }

            void focusLostOverride() override {
                keyboardPressedState = false;
                markNeedsPaint();
            }

            void focusVisibilityChangedOverride(bool) override {
                markNeedsPaint();
            }

            void frameOverride(const FrameEvent& event) override {
                float deltaSeconds = event.delta.count();
                if (thumbAnimation.advance(deltaSeconds))
                    markNeedsPaint();

                setFrameUpdatesEnabled(thumbAnimation.isRunning());
            }

		private:
            void toggle() {
                if (!isEnabled())
                    return;

                Switch::ChangedCallback callback = changedCallback;
                if (callback)
                    callback(!checkedValue);
            }

        private:
            bool checkedValue = false;

            bool hovered = false;
            bool pointerPressedState = false;
            bool keyboardPressedState = false;

            std::u32string labelValue;
            Switch::ChangedCallback changedCallback;
            SwitchStyle switchStyle;

            AnimationController thumbAnimation;
            std::unique_ptr<TextLayout> textLayout;
		};
	}

    Switch::Switch(bool checked, std::u32string label, ChangedCallback onChanged, SwitchStyle style, bool enabled, Key key)
        : Widget(std::move(key)), checkedValue(checked), enabledValue(enabled), labelValue(std::move(label)), changedCallback(std::move(onChanged)), switchStyle(std::move(style)) {}

    bool Switch::checked() const {
        return checkedValue;
    }

    bool Switch::enabled() const {
        return enabledValue;
    }

    const std::u32string& Switch::label() const {
        return labelValue;
    }

    const Switch::ChangedCallback& Switch::onChanged() const {
        return changedCallback;
    }

    const SwitchStyle& Switch::style() const {
        return switchStyle;
    }

    std::unique_ptr<Element> Switch::createElement() const {
        return std::make_unique<SwitchElement>(*this);
    }
}