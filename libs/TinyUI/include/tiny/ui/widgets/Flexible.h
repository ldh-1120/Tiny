#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	enum class FlexFit {
		Loose,
		Tight
	};

	class Flexible : public Widget {
	public:
		Flexible(std::unique_ptr<Widget> child, float flex = 1.0f, FlexFit fit = FlexFit::Loose, Key key = Key());

		const Widget* child() const;

		float flex() const;
		FlexFit fit() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::unique_ptr<Widget> childWidget;

		float flexValue = 1.0f;
		FlexFit fitValue = FlexFit::Loose;
	};

	class Expanded final : public Flexible {
	public:
		Expanded(std::unique_ptr<Widget> child, float flex = 1.0f, Key key = Key());
	};

	class Spacer final : public Flexible {
	public:
		explicit Spacer(float flex = 1.0f, Key key = Key());
	};

	namespace detail {
		float flexFactor(const Element& element);
		FlexFit flexFit(const Element& element);
	}
}