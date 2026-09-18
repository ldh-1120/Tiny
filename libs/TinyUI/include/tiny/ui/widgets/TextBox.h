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
	struct TextBoxStyle {
        Color background = Color::fromRgb(49, 50, 68);

        Color borderColor = Color::fromRgb(88, 91, 112);
        Color focusedBorderColor = Color::fromRgb(137, 180, 250);

        Color textColor = Color::fromRgb(205, 214, 244);
        Color caretColor = Color::fromRgb(205, 214, 244);
        Color selectionColor = Color::fromRgb(69, 90, 130);

        float borderWidth = 1.0f;
        float focusedBorderWidth = 2.0f;

        float caretWidth = 1.0f;
        float width = 240.0f;

        Thickness padding = Thickness(10.0f, 8.0f);

        TextStyle textStyle;
	};

    class TextBox : public Widget {
    public:
        using ChangedCallback = std::function<void(const std::u32string&)>;

        TextBox(std::u32string text, ChangedCallback onChanged, TextBoxStyle style = TextBoxStyle(), Key key = Key());

        const std::u32string& text() const;

        const ChangedCallback& onChanged() const;

        const TextBoxStyle& style() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        std::u32string textValue;

        ChangedCallback changedCallback;

        TextBoxStyle textBoxStyle;
    };
}