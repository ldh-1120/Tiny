#include <tiny/ui/MultiChildElement.h>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Widget.h>
#include <tiny/ui/UIRoot.h>

namespace tiny {
	MultiChildElement::MultiChildElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children) : Element(widget), childElements(std::move(children)) { }
	MultiChildElement::~MultiChildElement() = default;

	std::vector<std::unique_ptr<Element>>& MultiChildElement::children() {
		return childElements;
	}

	const std::vector<std::unique_ptr<Element>>& MultiChildElement::children() const {
		return childElements;
	}

	void MultiChildElement::updateChildren(const std::vector<const Widget*>& widgets) {
		std::vector<std::unique_ptr<Element>> oldChildren = std::move(childElements);

		childElements.clear();
		childElements.reserve(widgets.size());

		std::vector<bool> used(oldChildren.size(), false);
		for (std::size_t newIndex = 0; newIndex < widgets.size(); ++newIndex) {
			const Widget* widget = widgets[newIndex];
			if (!widget)
				continue;

			std::unique_ptr<Element> matchedElement;
			if (!widget->key().hasValue() && newIndex < oldChildren.size() && oldChildren[newIndex] && oldChildren[newIndex]->canUpdate(*widget)) {
				matchedElement = std::move(oldChildren[newIndex]);
				used[newIndex] = true;
			} else {
				for (std::size_t oldIndex = 0; oldIndex < oldChildren.size(); ++oldIndex) {
					if (used[oldIndex])
						continue;

					if (!oldChildren[oldIndex])
						continue;

					if (!oldChildren[oldIndex]->canUpdate(*widget))
						continue;

					matchedElement = std::move(oldChildren[oldIndex]);
					used[oldIndex] = true;

					break;
				}
			}

			if (matchedElement) {
				matchedElement->update(*widget);
				childElements.push_back(std::move(matchedElement));

				continue;
			}

			std::unique_ptr<Element> newElement = widget->createElement();
			if (newElement && isMounted()) {
				UIRoot* root = ownerRoot();
				if (root)
					newElement->mount(*root, this);
			}

			childElements.push_back(std::move(newElement));
		}

		for (std::size_t index = 0; index < oldChildren.size(); ++index) {
			if (used[index])
				continue;

			if (!oldChildren[index])
				continue;

			oldChildren[index]->unmount();
		}
	}

	void  MultiChildElement::paintOverride(Canvas& canvas) {
		for (const std::unique_ptr<Element>& child : childElements) {
			if (!child)
				continue;
			child->paint(canvas);
		}
	}

	Element* MultiChildElement::hitTestChildren(const Point& position) {
		for (std::size_t index = childElements.size(); index > 0; --index) {
			Element* child = childElements[index - 1].get();
			if (!child)
				continue;

			Element* result = child->hitTest(position);
			if (result)
				return result;
		}

		return nullptr;
	}

	void MultiChildElement::collectFocusableChildren(std::vector<Element*>& result) {
		for (const std::unique_ptr<Element>& child : childElements) {
			if (!child)
				continue;

			child->collectFocusableElements(result);
		}
	}

	void MultiChildElement::mountOverride() {
		UIRoot* root = ownerRoot();
		if (!root)
			return;

		for (const std::unique_ptr<Element>& child : childElements) {
			if (!child)
				continue;

			child->mount(*root, this);
		}
	}

	void MultiChildElement::unmountOverride() {
		for (const std::unique_ptr<Element>& child : childElements) {
			if (!child)
				continue;
			child->unmount();
		}
	}
}