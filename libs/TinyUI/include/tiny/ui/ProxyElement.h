#pragma once

#include <memory>

#include <tiny/ui/SingleChildElement.h>

namespace tiny {
	class Widget;

	class ProxyElement : public SingleChildElement {
	public:
		ProxyElement(const Widget& widget, std::unique_ptr<Element> child);
		~ProxyElement() override;

	protected:
		Size measureOverride(LayoutContext& context, const Constraints& constraints) override;
		void arrangeOverride(const Rect& bounds) override;
	};
}