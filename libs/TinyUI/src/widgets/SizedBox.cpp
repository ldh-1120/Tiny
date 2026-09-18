#include <tiny/ui/widgets/SizedBox.h>

#include <memory>
#include <utility>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/SingleChildElement.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class SizedBoxElement final : public SingleChildElement {
		public:
			explicit SizedBoxElement(const SizedBox& widget) : SingleChildElement(widget, createChild(widget)), requestedSize(widget.requestedSize()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const SizedBox& sizedBox = static_cast<const SizedBox&>(widget);
				requestedSize = sizedBox.requestedSize();
				updateChild(sizedBox.child());
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				Size actualSize = constraints.constrain(requestedSize);
				if (hasChild())
					child()->measure(context, Constraints::tight(actualSize));

				return actualSize;
			}

			void arrangeOverride(const Rect& bounds) override {
				if (!hasChild())
					return;

				child()->arrange(Rect(bounds.x, bounds.y, bounds.width, bounds.height));
			}

		private:
			static std::unique_ptr<Element> createChild(const SizedBox& widget) {
				const Widget* child = widget.child();
				if (!child)
					return nullptr;

				return child->createElement();
			}

		private:
			Size requestedSize;
		};
	}

	SizedBox::SizedBox(const Size& size, std::unique_ptr<Widget> child, Key key) : Widget(key), boxSize(size), childWidget(std::move(child)) { }

	const Size& SizedBox::requestedSize() const {
		return boxSize;
	}

	const Widget* SizedBox::child() const {
		return childWidget.get();
	}

	std::unique_ptr<Element> SizedBox::createElement() const {
		return std::make_unique<SizedBoxElement>(*this);
	}
}