#pragma once

#include <memory>

#include <tiny/core/Size.h>

#include <tiny/graphics/Image.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	class ImageViewer : public Widget {
	public:
		ImageViewer(std::shared_ptr<Image> image, const Size& size, ImageInterpolation interpolation = ImageInterpolation::Linear, Key key = Key());

		const std::shared_ptr<Image>& image() const;
		const Size& requestedSize() const;

		ImageInterpolation interpolation() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::shared_ptr<Image> imageValue;
		Size requestedSizeValue;

		ImageInterpolation interpolationValue = ImageInterpolation::Linear;
	};
}