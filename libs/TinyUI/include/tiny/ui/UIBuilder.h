#pragma once

#include <functional>
#include <memory>

namespace tiny {
	class Widget;

	class UIBuilder {
	public:
		using BuildFunction = std::function<std::unique_ptr<Widget>()>;

		UIBuilder() = default;

		explicit UIBuilder(BuildFunction buildFunction);

		std::unique_ptr<Widget> build() const;

		bool valid() const;

	private:
		BuildFunction buildFunction;
	};
}