#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	class ConstrainedBox : public Widget {
	public:
		ConstrainedBox(const Constraints& constraints, std::unique_ptr<Widget> child, Key key = Key());

		const Constraints& constraints() const;

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		Constraints boxConstraints;

		std::unique_ptr<Widget> childWidget;
	};
}