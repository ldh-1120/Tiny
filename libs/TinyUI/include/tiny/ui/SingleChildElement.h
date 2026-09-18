#pragma once

#include <memory>
#include <vector>

#include <tiny/ui/Element.h>

namespace tiny {
	class Widget;

	class SingleChildElement : public Element {
	public:
		SingleChildElement(const Widget& widget, std::unique_ptr<Element> child);
		~SingleChildElement() override;

	protected:
		Element* child();
		const Element* child() const;

		bool hasChild() const;

		void updateChild(const Widget* widget);

		void paintOverride(Canvas& canvas) override;

		Element* hitTestChildren(const Point& position) override;	

		void collectFocusableChildren(std::vector<Element*>& result) override;

	private:
		void mountOverride() override;
		void unmountOverride() override;

	private:
		std::unique_ptr<Element> childElement;
	};
}