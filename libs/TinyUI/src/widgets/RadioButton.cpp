#include <tiny/ui/widgets/RadioButton.h>

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
#include <tiny/ui/visual/FocusRing.h>

namespace tiny {
	namespace {
        class RadioButtonElement final : public Element {
        public:
            explicit RadioButtonElement(const RadioButton& widget)
                : Element(widget), selectedValue(widget.selected()), labelValue(widget.label()), selectedCallback(widget.onSelected()), radioStyle(widget.style()), arrowNavigationValue(widget.arrowNavigation()) {
                setEnabled(widget.enabled());
            }

        protected:
            void updateOverride(const Widget& widget) override {
                const RadioButton& radio = static_cast<const RadioButton&>(widget);

                selectedValue = radio.selected();
                labelValue = radio.label();
                selectedCallback = radio.onSelected();
                radioStyle = radio.style();

                arrowNavigationValue = radio.arrowNavigation();

                setEnabled(radio.enabled());

                if (!isEnabled()) {
                    hovered = false;
                    pointerPressedState = false;
                    keyboardPressedState = false;
                }

                markNeedsLayout();
            }

            Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
                float diameter = std::max(radioStyle.diameter, 0.0f);

                Size textSize { };
                if (!labelValue.empty()) {
                    GraphicsContext& graphics = context.graphicsContext();

                    float maxTextWidth = std::numeric_limits<float>::infinity();
                    if (constraints.hasBoundedWidth())
                        maxTextWidth = std::max(constraints.maxWidth() - radioStyle.padding.horizontal() - diameter - radioStyle.spacing, 0.0f);

                    textLayout = graphics.createTextLayout(labelValue, radioStyle.textStyle, maxTextWidth);
                    if (textLayout)
                        textSize = textLayout->size();

                    FontMetrics metrics = graphics.getFontMetrics(radioStyle.textStyle);
                    textSize.height = std::max(textSize.height, metrics.lineHeight);
                } else
                    textLayout.reset();

                float width = diameter;
                if (!labelValue.empty())
                    width += radioStyle.spacing + textSize.width;

                float height = std::max(diameter, textSize.height);
                return constraints.constrain(Size(width + radioStyle.padding.horizontal(), height + radioStyle.padding.vertical()));
            }

            void paintOverride(Canvas& canvas) override {
                const Rect& area = bounds();

                float availableWidth = std::max(area.width - radioStyle.padding.horizontal(), 0.0f);
                float availableHeight = std::max(area.height - radioStyle.padding.vertical(), 0.0f);

                float diameter = std::min(std::max(radioStyle.diameter, 0.0f), std::min(availableWidth, availableHeight));
                Rect circle(area.x + radioStyle.padding.left, area.y + radioStyle.padding.top + std::max((availableHeight - diameter) * 0.5f, 0.0f), diameter, diameter);

                Color background = hovered && isEnabled() ? radioStyle.hoveredBackground : radioStyle.background;
                canvas.fillEllipse(circle, background);

                Color ringColor = selectedValue ? radioStyle.selectedColor : radioStyle.borderColor;
                if (!isEnabled())
                    ringColor = radioStyle.disabledColor;

                float stroke = std::clamp(radioStyle.borderWidth, 0.0f, diameter);
                Rect ring(circle.x + stroke * 0.5f, circle.y + stroke * 0.5f, std::max(circle.width - stroke, 0.0f), std::max(circle.height - stroke, 0.0f));

                canvas.drawEllipse(ring, ringColor, stroke);

                if (selectedValue) {
                    float dotSize = std::min(std::max(radioStyle.dotDiameter, 0.0f), diameter);
                    Rect dot(circle.x + (diameter - dotSize) * 0.5f, circle.y + (diameter - dotSize) * 0.5f, dotSize, dotSize);

                    canvas.fillEllipse(dot, ringColor);
                }

                if (textLayout) {
                    Size textSize = textLayout->size();
                    Point origin(circle.right() + radioStyle.spacing, area.y + radioStyle.padding.top + std::max((availableHeight - textSize.height) * 0.5f, 0.0f));

                    canvas.drawTextLayout(*textLayout, origin, isEnabled() ? radioStyle.textColor : radioStyle.disabledTextColor);
                }

                paintFocusRing(canvas, area, radioStyle.focusColor, radioStyle.focusWidth, isFocusVisible() ? 1.0f : 0.0f);
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
                    select();
            }

            void pointerCancelOverride() override {
                pointerPressedState = false;
                markNeedsPaint();
            }

            bool keyDownOverride(const KeyEvent& event) override {
                if (arrowNavigationValue && !event.modifiers.control && !event.modifiers.alt && !event.modifiers.shift) {
                    switch (event.key) {
                        case KeyCode::Right:
                        case KeyCode::Down:
                            return navigateGroup(true);

                        case KeyCode::Left:
                        case KeyCode::Up:
                            return navigateGroup(false);

                        default:
                            break;
                    }
                }

                if (event.key == KeyCode::Space) {
                    if (!keyboardPressedState) {
                        keyboardPressedState = true;
                        markNeedsPaint();
                    }

                    return true;
                }

                if (event.key == KeyCode::Enter) {
                    if (!event.repeated)
                        select();

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

                select();
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
            void select() {
                if (selectedValue || !isEnabled())
                    return;

                RadioButton::SelectedCallback callback = selectedCallback;
                if (callback)
                    callback();
            }

            bool navigateGroup(bool forward) {
                Element* group = parent();
                if (!group)
                    return false;

                std::vector<Element*> focusableElements;
                group->collectFocusableElements(focusableElements);

                std::vector<RadioButtonElement*> radios;
                for (Element* element : focusableElements) {
                    RadioButtonElement* radio = dynamic_cast<RadioButtonElement*>(element);
                    if (!radio)
                        continue;

                    if (!radio->arrowNavigationValue)
                        continue;

                    if (!radio->isEnabled())
                        continue;

                    radios.push_back(radio);
                }

                if (radios.empty())
                    return false;

                std::size_t currentIndex = radios.size();
                for (std::size_t index = 0; index < radios.size(); ++index)
                    if (radios[index] == this) {
                        currentIndex = index;
                        break;
                    }

                if (currentIndex == radios.size())
                    return false;

                std::size_t nextIndex;
                if (forward)
                    nextIndex = (currentIndex + 1) % radios.size();
                else
                    nextIndex = (currentIndex + radios.size() - 1) % radios.size();

                RadioButtonElement* next = radios[nextIndex];
                if (!next->requestFocus(FocusReason::Keyboard))
                    return true;

                next->select();

                return true;
            }

        private:
            bool selectedValue = false;
            bool hovered = false;
            bool pointerPressedState = false;
            bool keyboardPressedState = false;

            bool arrowNavigationValue = false;

            std::u32string labelValue;
            RadioButton::SelectedCallback selectedCallback;
            RadioButtonStyle radioStyle;

            std::unique_ptr<TextLayout> textLayout;
        };
	}

    RadioButton::RadioButton(bool selected, std::u32string label, SelectedCallback onSelected, RadioButtonStyle style, bool enabled, Key key, bool arrowNavigation)
        : Widget(std::move(key)), selectedValue(selected), enabledValue(enabled), labelValue(std::move(label)), selectedCallback(std::move(onSelected)), radioStyle(std::move(style)), arrowNavigationValue(arrowNavigation) {}

    bool RadioButton::selected() const {
        return selectedValue;
    }

    bool RadioButton::enabled() const {
        return enabledValue;
    }

    bool RadioButton::arrowNavigation() const {
        return arrowNavigationValue;
    }

    const std::u32string& RadioButton::label() const {
        return labelValue;
    }

    const RadioButton::SelectedCallback& RadioButton::onSelected() const {
        return selectedCallback;
    }

    const RadioButtonStyle& RadioButton::style() const {
        return radioStyle;
    }

    std::unique_ptr<Element> RadioButton::createElement() const {
        return std::make_unique<RadioButtonElement>(*this);
    }
}