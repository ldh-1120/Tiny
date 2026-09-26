#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class IgnorePointer : public Widget {
	public:
		IgnorePointer(std::unique_ptr<Widget> child, bool ignoring = true, Key key = Key());

		const Widget* child() const;

		bool ignoring() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		bool ignoringValue = true;
	};
}