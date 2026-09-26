#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class ClipRect : public Widget {
	public:
		explicit ClipRect(std::unique_ptr<Widget> child, Key key = Key());

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;
	};
}