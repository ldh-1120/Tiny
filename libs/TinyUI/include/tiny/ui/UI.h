#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <tiny/core/Color.h>
#include <tiny/core/Size.h>
#include <tiny/core/Thickness.h>

#include <tiny/graphics/TextStyle.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

#include <tiny/ui/layout/Alignment.h>
#include <tiny/ui/layout/Constraints.h>

#include <tiny/ui/widgets/Align.h>
#include <tiny/ui/widgets/Button.h>
#include <tiny/ui/widgets/Column.h>
#include <tiny/ui/widgets/ConstrainedBox.h>
#include <tiny/ui/widgets/Flexible.h>
#include <tiny/ui/widgets/Padding.h>
#include <tiny/ui/widgets/Row.h>
#include <tiny/ui/widgets/ScrollView.h>
#include <tiny/ui/widgets/SizedBox.h>
#include <tiny/ui/widgets/Stack.h>
#include <tiny/ui/widgets/Text.h>
#include <tiny/ui/widgets/TextBox.h>
#include <tiny/ui/widgets/Positioned.h>
#include <tiny/ui/widgets/ClipRect.h>

namespace tiny::ui {
	template <typename WidgetType, typename... Arguments>
	WidgetPtr make(Arguments&&... arguments) {
		return std::make_unique<WidgetType>(std::forward<Arguments>(arguments)...);
	}

	template <typename... Widgets>
	std::vector<WidgetPtr> children(Widgets&&... widgets) {
		static_assert((std::is_same_v<std::remove_cvref_t<Widgets>, WidgetPtr> && ...), "children() only accepts WidgetPtr values");

		std::vector<WidgetPtr> result;
		result.reserve(sizeof...(Widgets));

		(result.push_back(std::forward<Widgets>(widgets)), ...);

		return result;
	}

	inline WidgetPtr row(std::vector<WidgetPtr> children, float spacing = 0.0f, CrossAxisAlignment alignment = CrossAxisAlignment::Start, Key key = Key()) {
		return make<Row>(std::move(children), spacing, alignment, std::move(key));
	}

	inline WidgetPtr column(std::vector<WidgetPtr> children, float spacing = 0.0f, CrossAxisAlignment alignment = CrossAxisAlignment::Start, Key key = Key()) {
		return make<Column>(std::move(children), spacing, alignment, std::move(key));
	}

	inline WidgetPtr stack(std::vector<WidgetPtr> children, Key key = Key()) {
		return make<Stack>(std::move(children), std::move(key));
	}

	inline WidgetPtr padding(const Thickness& thickness, WidgetPtr child, Key key = Key()) {
		return make<Padding>(thickness, std::move(child), std::move(key));
	}

	inline WidgetPtr padding(float amount, WidgetPtr child, Key key = Key()) {
		return padding(Thickness(amount), std::move(child), std::move(key));
	}

	inline WidgetPtr flexible(WidgetPtr child, float flex = 1.0f, FlexFit fit = FlexFit::Loose, Key key = Key()) {
		return make<Flexible>(std::move(child), flex, fit, std::move(key));
	}

	inline WidgetPtr expanded(WidgetPtr child, float flex = 1.0f, Key key = Key()) {
		return make<Expanded>(std::move(child), flex, std::move(key));
	}

	inline WidgetPtr spacer(float flex = 1.0f, Key key = Key()) {
		return make<Spacer>(flex, std::move(key));
	}

	inline WidgetPtr constrained(const Constraints& constraints, WidgetPtr child, Key key = Key()) {
		return make<ConstrainedBox>(constraints, std::move(child), std::move(key));
	}

	inline WidgetPtr fixedWidth(float width, WidgetPtr child, Key key = Key()) {
		return constrained(Constraints::fixedWidth(width), std::move(child), std::move(key));
	}

	inline WidgetPtr fixedHeight(float height, WidgetPtr child, Key key = Key()) {
		return constrained(Constraints::fixedHeight(height), std::move(child), std::move(key));
	}

	inline WidgetPtr sizedBox(const Size& size, WidgetPtr child, Key key = Key()) {
		return make<SizedBox>(size, std::move(child), std::move(key));
	}

	inline WidgetPtr scroll(WidgetPtr child, float wheelStep = 48.0f, ScrollViewStyle style = ScrollViewStyle(), Key key = Key()) {
		return make<ScrollView>(std::move(child), wheelStep, std::move(key), std::move(style));
	}

	inline WidgetPtr align(Alignment alignment, WidgetPtr child, Key key = Key()) {
		return make<Align>(std::move(child), alignment, std::move(key));
	}

	inline WidgetPtr text(std::u32string value, Color color, TextStyle style = TextStyle(), Key key = Key()) {
		return make<Text>(std::move(value), color, std::move(style), std::move(key));
	}

	inline WidgetPtr button(std::u32string text, std::function<void()> onClick, ButtonStyle style = ButtonStyle(), bool enabled = true, Key key = Key()) {
		return make<Button>(std::move(text), std::move(onClick), std::move(style), std::move(key), enabled);
	}

	inline WidgetPtr textBox(std::u32string value, TextBox::ChangedCallback onChanged, TextBoxStyle style = TextBoxStyle(), bool enabled = true, Key key = Key()) {
		return make<TextBox>(std::move(value), std::move(onChanged), std::move(style), std::move(key), enabled);
	}

	inline WidgetPtr positioned(PositionedSpec spec, WidgetPtr child, Key key = Key()) {
		return make<Positioned>(std::move(spec), std::move(child), std::move(key));
	}

	inline WidgetPtr clipRect(WidgetPtr child, Key key = Key()) {
		return make<ClipRect>(std::move(child), std::move(key));
	}
}