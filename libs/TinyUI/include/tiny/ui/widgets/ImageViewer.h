#pragma once

#include <memory>
#include <functional>

#include <tiny/core/Size.h>

#include <tiny/graphics/Image.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class ImageViewer : public Widget {
	public:
		using ZoomChangedCallback = std::function<void(float)>;

		ImageViewer(std::shared_ptr<Image> image, const Size& size, ImageInterpolation interpolation = ImageInterpolation::Linear, Key key = Key(), ZoomChangedCallback onZoomChanged = nullptr);

		const std::shared_ptr<Image>& image() const;
		const Size& requestedSize() const;

		ImageInterpolation interpolation() const;

		std::unique_ptr<Element> createElement() const override;

		const ZoomChangedCallback& onZoomChanged() const;

	private:
		std::shared_ptr<Image> imageValue;
		Size requestedSizeValue;

		ImageInterpolation interpolationValue = ImageInterpolation::Linear;

		ZoomChangedCallback zoomChangedCallback = nullptr;
	};
}