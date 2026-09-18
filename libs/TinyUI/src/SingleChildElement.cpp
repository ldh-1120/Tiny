#include <tiny/ui/SingleChildElement.h>

#include <utility>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Widget.h>
#include <tiny/ui/UIRoot.h>

namespace tiny {
	SingleChildElement::SingleChildElement(const Widget& widget, std::unique_ptr<Element> child) : Element(widget), childElement(std::move(child)) { }
	SingleChildElement::~SingleChildElement() = default;

	Element* SingleChildElement::child() {
		return childElement.get();
	}
	
	const Element* SingleChildElement::child() const {
		return childElement.get();
	}

	bool SingleChildElement::hasChild() const {
		return childElement != nullptr;
	}

	void SingleChildElement::updateChild(const Widget* widget) {
		if (!widget) {
			if (childElement)
				childElement->unmount();

			childElement.reset();
			return;
		}

		if (childElement && childElement->canUpdate(*widget)) {
			childElement->update(*widget);
			return;
		}

		if (childElement)
			childElement->unmount();

		childElement = widget->createElement();
		if (childElement && isMounted()) {
			UIRoot* root = ownerRoot();
			if (root)
				childElement->mount(*root, this);
		}
	}

	void SingleChildElement::paintOverride(Canvas& canvas) {
		if (!childElement)
			return;

		childElement->paint(canvas);
	}

	Element* SingleChildElement::hitTestChildren(const Point& position) {
		if (!childElement)
			return nullptr;

		return childElement->hitTest(position);
	}

	void SingleChildElement::collectFocusableChildren(std::vector<Element*>& result) {
		if (!childElement)
			return;

		childElement->collectFocusableElements(result);
	}

	void SingleChildElement::mountOverride() {
		if (!childElement)
			return;

		UIRoot* root = ownerRoot();
		if (!root)
			return;

		childElement->mount(*root, this);
	}

	void SingleChildElement::unmountOverride() {
		if (childElement)
			childElement->unmount();
	}
}