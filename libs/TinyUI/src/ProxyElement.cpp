#include <tiny/ui/ProxyElement.h>

#include <utility>

#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	ProxyElement::ProxyElement(const Widget& widget, std::unique_ptr<Element> child) : SingleChildElement(widget, std::move(child)) {}
	ProxyElement::~ProxyElement() = default;

	Size ProxyElement::measureOverride(LayoutContext& context, const Constraints& constraints) {
		if (!hasChild())
			return constraints.smallest();

		return child()->measure(context, constraints);
	}

	void ProxyElement::arrangeOverride(const Rect& bounds) {
		if (!hasChild())
			return;

		child()->arrange(bounds);
	}
}