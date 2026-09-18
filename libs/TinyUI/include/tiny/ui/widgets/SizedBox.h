#pragma once

#include <memory>
#include <utility>

#include <tiny/core/Size.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class SizedBox : public Widget {
	public:
		SizedBox(const Size& size, std::unique_ptr<Widget> child, Key key = Key());

		const Size& requestedSize() const;
		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		Size boxSize;
		std::unique_ptr<Widget> childWidget;
	};
}