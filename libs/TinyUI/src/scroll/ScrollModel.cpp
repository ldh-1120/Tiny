#include <tiny/ui/scroll/ScrollModel.h>

#include <algorithm>

namespace tiny {
    float ScrollModel::offset() const {
        return offsetValue;
    }

    float ScrollModel::viewportExtent() const {
        return viewportExtentValue;
    }

    float ScrollModel::contentExtent() const {
        return contentExtentValue;
    }

    float ScrollModel::maximumOffset() const {
        return std::max(contentExtentValue - viewportExtentValue,  0.0f);
    }

    bool ScrollModel::canScroll() const {
        return maximumOffset() > 0.0f;
    }

    float ScrollModel::viewportFraction() const {
        if (contentExtentValue <= 0.0f)
            return 1.0f;

        return std::clamp(viewportExtentValue / contentExtentValue, 0.0f, 1.0f);
    }

    float ScrollModel::offsetFraction() const {
        float maximum = maximumOffset();
        if (maximum <= 0.0f)
            return 0.0f;

        return std::clamp(offsetValue / maximum, 0.0f, 1.0f);
    }

    bool ScrollModel::setExtents(float viewportExtent, float contentExtent) {
        float previousOffset = offsetValue;

        viewportExtentValue = std::max(viewportExtent, 0.0f);
        contentExtentValue = std::max(contentExtent, 0.0f);

        offsetValue = std::clamp(offsetValue, 0.0f, maximumOffset());

        return offsetValue != previousOffset;
    }

    bool ScrollModel::scrollTo(float offset) {
        float nextOffset = std::clamp(offset, 0.0f, maximumOffset());
        if (nextOffset == offsetValue)
            return false;

        offsetValue = nextOffset;

        return true;
    }

    bool ScrollModel::scrollBy(float delta) {
        return scrollTo(offsetValue + delta);
    }
}