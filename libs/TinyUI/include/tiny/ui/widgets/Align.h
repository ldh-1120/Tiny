#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/layout/Alignment.h>

namespace tiny {
	class Align : public Widget {
	public:
		Align(std::unique_ptr<Widget> child, Alignment alignment = Alignment::Center, Key key = Key());

		const Widget* child() const;

		Alignment alignment() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		Alignment alignmentValue = Alignment::Center;
	};
}