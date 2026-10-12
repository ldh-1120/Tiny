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
    struct CheckBoxStyle {
        float boxSize = 16.0f;
        float spacing = 9.0f;
        float borderWidth = 1.0f;

        Thickness padding = Thickness(4.0f, 3.0f);

        Color background = Color::fromRgb(30, 32, 42);
        Color hoveredBackground = Color::fromRgb(43, 46, 58);
        Color checkedBackground = Color::fromRgb(103, 143, 190);
        Color checkedHoveredBackground = Color::fromRgb(118, 157, 203);
        Color disabledBackground = Color::fromRgb(42, 43, 51);

        Color borderColor = Color::fromRgb(90, 95, 110);
        Color checkColor = Color::fromRgb(250, 251, 253);
        Color disabledCheckColor = Color::fromRgb(116, 120, 132);

        Color textColor = Color::fromRgb(205, 214, 244);
        Color disabledTextColor = Color::fromRgb(108, 112, 134);

        Color focusColor = Color::fromRgb(137, 180, 250);
        float focusWidth = 1.0f;

        TextStyle textStyle;
    };

    class CheckBox final : public Widget {
    public:
        using ChangedCallback = std::function<void(bool)>;

        CheckBox(bool checked, std::u32string label, ChangedCallback onChanged, CheckBoxStyle style = CheckBoxStyle(), bool enabled = true, Key key = Key());

        bool checked() const;
        bool enabled() const;

        const std::u32string& label() const;
        const ChangedCallback& onChanged() const;
        const CheckBoxStyle& style() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        bool checkedValue = false;
        bool enabledValue = true;

        std::u32string labelValue;
        ChangedCallback changedCallback;
        CheckBoxStyle checkBoxStyle;
    };
}