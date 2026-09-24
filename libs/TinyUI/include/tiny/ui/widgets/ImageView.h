#pragma once

#include <memory>

#include <tiny/core/Size.h>

#include <tiny/graphics/Image.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

namespace tiny {
	enum class ImageFit {
		Contain,
		Cover,
		Stretch
	};

	enum class ImageAlignment {
		TopLeft,
		TopCenter,
		TopRight,

		CenterLeft,
		Center,
		CenterRight,

		BottomLeft,
		BottomCenter,
		BottomRight
	};

	class ImageView : public Widget {
	public:
		ImageView(std::shared_ptr<Image> image, const Size& size, ImageFit fit = ImageFit::Contain, ImageInterpolation interpolation = ImageInterpolation::Linear, Key key = Key(), ImageAlignment alignment = ImageAlignment::Center);
		ImageView(std::shared_ptr<Image> image, ImageFit fit = ImageFit::Contain, ImageInterpolation interpolation = ImageInterpolation::Linear, Key key = Key(), ImageAlignment alignment = ImageAlignment::Center);

		const std::shared_ptr<Image>& image() const;

		const Size& requestedSize() const;

		ImageFit fit() const;
		ImageInterpolation interpolation() const;
		ImageAlignment alignment() const;

		bool usesIntrinsicSize() const;

		std::unique_ptr<Element> createElement() const override;

	private:
		std::shared_ptr<Image> imageValue;

		Size requestedSizeValue;

		ImageFit fitValue = ImageFit::Contain;
		ImageInterpolation interpolationValue = ImageInterpolation::Linear;
		ImageAlignment alignmentValue = ImageAlignment::Center;

		bool useIntrinsicSizeValue = false;
	};
}