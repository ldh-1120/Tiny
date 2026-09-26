#pragma once

#include <memory>
#include <utility>

#include <tiny/ui/Key.h>

namespace tiny {
	class Element;
	class Widget;

	using WidgetPtr = std::unique_ptr<Widget>;

	class Widget {
	public:
		virtual ~Widget() = default;

		Widget(const Widget&) = delete;
		Widget& operator=(const Widget&) = delete;

		Widget(Widget&&) = delete;
		Widget& operator=(Widget&&) = delete;

		const Key& key() const {
			return widgetKey;
		}

		virtual std::unique_ptr<Element> createElement() const = 0;

	protected:
		explicit Widget(Key key = Key()) : widgetKey(std::move(key)) { }

	private:
		Key widgetKey;
	};
}