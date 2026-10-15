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
    struct SwitchStyle {
        float trackWidth = 36.0f;
        float trackHeight = 18.0f;
        float thumbWidth = 12.0f;
        float thumbInset = 3.0f;
        float spacing = 10.0f;
        float borderWidth = 1.0f;

        Thickness padding = Thickness(4.0f, 3.0f);

        Color offColor = Color::fromRgb(42, 46, 59);
        Color offHoverColor = Color::fromRgb(54, 59, 74);
        Color onColor = Color::fromRgb(103, 143, 190);
        Color onHoverColor = Color::fromRgb(118, 157, 203);

        Color borderColor = Color::fromRgb(96, 103, 120);
        Color thumbColor = Color::fromRgb(240, 242, 245);

        Color disabledColor = Color::fromRgb(42, 44, 51);
        Color disabledThumbColor = Color::fromRgb(110, 114, 125);

        Color textColor = Color::fromRgb(205, 214, 244);
        Color disabledTextColor = Color::fromRgb(108, 112, 134);

        Color focusColor = Color::fromRgb(137, 180, 250);
        float focusWidth = 1.0f;

        TextStyle textStyle;
    };

    class Switch final : public Widget {
    public:
        using ChangedCallback = std::function<void(bool)>;

        Switch(bool checked, std::u32string label, ChangedCallback onChanged, SwitchStyle style = SwitchStyle(), bool enabled = true, Key key = Key());

        bool checked() const;
        bool enabled() const;

        const std::u32string& label() const;
        const ChangedCallback& onChanged() const;
        const SwitchStyle& style() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        bool checkedValue = false;
        bool enabledValue = true;

        std::u32string labelValue;
        ChangedCallback changedCallback;
        SwitchStyle switchStyle;
    };
}