#pragma once

#include <memory>
#include <utility>

#include <tiny/core/Color.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class Box : public Widget {
	public:
		Box(const Size& size, const Color& color, Key key = Key());

		const Size& requestedSize() const;
		const Color& color() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		Size boxSize;
		Color boxColor;
	};
}