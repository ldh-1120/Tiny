#pragma once

#include <memory>

#include <tiny/core/Point.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	struct TransformSpec {
		Point translation;

		float scaleX = 1.0f;
		float scaleY = 1.0f;

		float rotationDegrees = 0.0f;

		Point pivot = Point(0.5f, 0.5f);
	};

	class Transform : public Widget {
	public:
		Transform(TransformSpec spec, std::unique_ptr<Widget> child, Key key = Key());

		const TransformSpec& spec() const;

		const Widget* child() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		TransformSpec transformSpec;

		std::unique_ptr<Widget> childWidget;
	};
}