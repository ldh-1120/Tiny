#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <tiny/core/Color.h>
#include <tiny/core/Thickness.h>

#include <tiny/graphics/TextStyle.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	struct ButtonStyle {
        Color background = Color::fromRgb(69, 71, 90);
        Color hoveredBackground = Color::fromRgb(88, 91, 112);
        Color pressedBackground = Color::fromRgb(108, 112, 134);
        Color disabledBackground = Color::fromRgb(49, 50, 68);

        Color textColor = Color::fromRgb(205, 214, 244);
        Color disabledTextColor = Color::fromRgb(108, 112, 134);

        Color focusBorderColor = Color::fromRgb(137, 180, 250);
        float focusBorderWidth = 2.0f;

        TextStyle textStyle;

        Thickness padding = Thickness(14.0f, 8.0f);
	};

    class Button : public Widget {
    public:
        Button(std::u32string text, std::function<void()> onClick, ButtonStyle style = ButtonStyle(), Key key = Key(), bool enabled = true);

        const std::u32string& text() const;

        const std::function<void()>& onClick() const;

        const ButtonStyle& style() const;

        std::unique_ptr<Element> createElement() const override;

        bool enabled() const;

    private:
        std::u32string textValue;

        std::function<void()> clickCallback;

        ButtonStyle buttonStyle;

        bool enabledValue = false;
    };
}