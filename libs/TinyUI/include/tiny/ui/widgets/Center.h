#pragma once

#include <memory>

#include <tiny/ui/Widget.h>

namespace tiny {
	class Center : public Widget {
	public:
		explicit Center(std::unique_ptr<Widget> child, Key key = Key());

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;
	};
}