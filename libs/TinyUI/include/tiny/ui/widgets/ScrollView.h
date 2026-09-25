#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class ScrollView : public Widget {
	public:
		ScrollView(std::unique_ptr<Widget> child, float wheelStep = 48.0f, Key key = Key());

		const Widget* child() const;

		float wheelStep() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		float wheelStepValue = 48.0f;
	};
}