#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class AspectRatio : public Widget {
	public:
		AspectRatio(float aspectRatio, std::unique_ptr<Widget> child, Key key = Key());

		float aspectRatio() const;
		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		float aspectRatioValue = 1.0f;

		std::unique_ptr<Widget> childWidget;
	};
}