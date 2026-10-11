#pragma once

#include <memory>
#include <string>

#include <tiny/ui/UIBuilder.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/widgets/Button.h>

namespace tiny {
	class PopupController;

	class PopupButton final : public Widget {
	public:
		PopupButton(PopupController& controller, std::u32string label, UIBuilder content, ButtonStyle style = ButtonStyle(), float gap = 4.0f, bool enabled = true, Key key = Key());

		const Widget* anchor() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		WidgetPtr anchorWidget;
	};
}