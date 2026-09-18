#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/layout/Alignment.h>

namespace tiny {
	class Column : public Widget {
	public:
		explicit Column(std::vector<std::unique_ptr<Widget>> children, float spacing = 0.0f, CrossAxisAlignment crossAxisAlignment = CrossAxisAlignment::Start, Key key = Key());

		const std::vector<std::unique_ptr<Widget>>& children() const;

		float spacing() const;

		CrossAxisAlignment crossAxisAlignment() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::vector<std::unique_ptr<Widget>> childWidgets;

		float childSpacing = 0.0f;

		CrossAxisAlignment childAlignment = CrossAxisAlignment::Start;
	};
}