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
        float boxSize = 18.0f;
        float spacing = 8.0f;
        float borderWidth = 1.0f;

        Thickness padding = Thickness(4.0f, 3.0f);

        Color background = Color::fromRgb(36, 38, 52);
        Color hoveredBackground = Color::fromRgb(49, 52, 70);
        Color checkedBackground = Color::fromRgb(95, 145, 205);
        Color checkedHoveredBackground = Color::fromRgb(112, 160, 219);
        Color disabledBackground = Color::fromRgb(42, 43, 51);

        Color borderColor = Color::fromRgb(91, 96, 116);
        Color checkColor = Color::fromRgb(245, 247, 252);
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