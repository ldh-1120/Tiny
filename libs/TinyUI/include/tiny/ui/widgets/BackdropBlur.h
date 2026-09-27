#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class BackdropBlur : public Widget {
	public:
		BackdropBlur(float radius, std::unique_ptr<Widget> child, Key key = Key());

		float radius() const;

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		float radiusValue = 0.0f;

		std::unique_ptr<Widget> childWidget;
	};
}