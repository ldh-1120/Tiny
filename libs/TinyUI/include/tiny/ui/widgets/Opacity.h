#pragma once

#include <memory>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
    class Opacity : public Widget {
    public:
        Opacity(float opacity, std::unique_ptr<Widget> child, Key key = Key());

        float opacity() const;

        const Widget* child() const;

        std::unique_ptr<Element> createElement() const override;

    private:
        float opacityValue = 1.0f;

        std::unique_ptr<Widget> childWidget;
    };
}