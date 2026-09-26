#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class AbsorbPointer : public Widget {
	public:
		AbsorbPointer(std::unique_ptr<Widget> child, bool absorbing = true, Key key = Key());

		const Widget* child() const;

		bool absorbing() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		bool absorbingValue = true;
	};
}