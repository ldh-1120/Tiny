#pragma once

#include <memory>
#include <optional>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
    struct PositionedSpec {
        std::optional<float> left;
        std::optional<float> top;
        std::optional<float> right;
        std::optional<float> bottom;

        std::optional<float> width;
        std::optional<float> height;
    };

    class Positioned : public Widget {
    public:
        Positioned(PositionedSpec spec, std::unique_ptr<Widget> child, Key key = Key());

        const PositionedSpec& spec() const;
        const Widget* child() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        PositionedSpec positionSpec;

        std::unique_ptr<Widget> childWidget;
    };

    namespace detail {
        const PositionedSpec* positionedSpec(const Element& element);
    }
}