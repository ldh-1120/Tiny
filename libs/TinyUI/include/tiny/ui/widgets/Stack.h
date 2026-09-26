#pragma once

#include <memory>
#include <vector>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class Stack : public Widget {
	public:
		explicit Stack(std::vector<std::unique_ptr<Widget>> children, Key key = Key());

		const std::vector<std::unique_ptr<Widget>>& children() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::vector<std::unique_ptr<Widget>> childWidgets;
	};
}