#pragma once

#include <functional>
#include <memory>
#include <string>

#include <tiny/core/Color.h>
#include <tiny/core/Thickness.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
    struct RadioButtonStyle {
        float diameter = 16.0f;
        float dotDiameter = 7.0f;
        float spacing = 9.0f;
        float borderWidth = 1.5f;

        Thickness padding = Thickness(4.0f, 3.0f);

        Color background = Color::fromRgb(30, 32, 42);
        Color hoveredBackground = Color::fromRgb(43, 46, 58);
        Color borderColor = Color::fromRgb(90, 95, 110);
        Color selectedColor = Color::fromRgb(103, 143, 190);
        Color disabledColor = Color::fromRgb(94, 97, 109);

        Color textColor = Color::fromRgb(205, 214, 244);
        Color disabledTextColor = Color::fromRgb(108, 112, 134);

        Color focusColor = Color::fromRgb(137, 180, 250);
        float focusWidth = 1.0f;

        TextStyle textStyle;
    };

    class RadioButton final : public Widget {
    public:
        using SelectedCallback = std::function<void()>;

        RadioButton(bool selected, std::u32string label, SelectedCallback onSelected, RadioButtonStyle style = RadioButtonStyle(), bool enabled = true, Key key = Key());

        bool selected() const;
        bool enabled() const;

        const std::u32string& label() const;
        const SelectedCallback& onSelected() const;
        const RadioButtonStyle& style() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        bool selectedValue = false;
        bool enabledValue = true;

        std::u32string labelValue;
        SelectedCallback selectedCallback;
        RadioButtonStyle radioStyle;
    };
}