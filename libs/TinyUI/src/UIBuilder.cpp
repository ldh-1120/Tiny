#include <tiny/ui/UIBuilder.h>

#include <utility>

#include <tiny/ui/Widget.h>

namespace tiny {
	UIBuilder::UIBuilder(BuildFunction buildFunction) : buildFunction(std::move(buildFunction)) { }

	std::unique_ptr<Widget> UIBuilder::build() const {
		if (!buildFunction)
			return nullptr;

		return buildFunction();
	}

	bool UIBuilder::valid() const {
		return static_cast<bool>(buildFunction);
	}
}