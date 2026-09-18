#pragma once

#include <memory>
#include <vector>

#include <tiny/ui/Element.h>

namespace tiny {
	class Widget;

	class MultiChildElement : public Element {
	public:
		MultiChildElement(const Widget& widget, std::vector<std::unique_ptr<Element>> children);
		~MultiChildElement() override;

	protected:
		std::vector<std::unique_ptr<Element>>& children();
		const std::vector<std::unique_ptr<Element>>& children() const;

		void updateChildren(const std::vector<const Widget*>& widgets);

		void paintOverride(Canvas& canvas) override;

		Element* hitTestChildren(const Point& position) override;

		void collectFocusableChildren(std::vector<Element*>& result) override;

	private:
		void mountOverride() override;
		void unmountOverride() override;

	private:
		std::vector<std::unique_ptr<Element>> childElements;
	};
}