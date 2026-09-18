#pragma once

#include <memory>
#include <string>

#include <tiny/core/Color.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class Text : public Widget {
	public:
		Text(std::u32string text, Color color, TextStyle style = TextStyle(), Key key = Key());

		const std::u32string& text() const;
		const Color& color() const;
		const TextStyle& style() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::u32string textValue;

		TextStyle textStyle;
		Color textColor;
	};
}	