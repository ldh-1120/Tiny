#pragma once

#include <memory>

#include <tiny/core/Thickness.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class Padding : public Widget {
	public:
		Padding(const Thickness& padding, std::unique_ptr<Widget> child, Key key = Key());

		const Thickness& padding() const;
		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		Thickness paddingValue;
		std::unique_ptr<Widget> childWidget;
	};
}