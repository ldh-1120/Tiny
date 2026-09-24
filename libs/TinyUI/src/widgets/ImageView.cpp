#include <tiny/ui/widgets/ImageView.h>

#include <algorithm>
#include <memory>
#include <utility>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class ImageViewElement final : public Element {
		public:
			explicit ImageViewElement(const ImageView& widget)
				: Element(widget), imageValue(widget.image()), requestedSizeValue(widget.requestedSize()), fitValue(widget.fit()), interpolationValue(widget.interpolation()), useIntrinsicSizeValue(widget.usesIntrinsicSize()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ImageView& imageView = static_cast<const ImageView&>(widget);

				imageValue = imageView.image();
				requestedSizeValue = imageView.requestedSize();
				fitValue = imageView.fit();
				interpolationValue = imageView.interpolation();
				useIntrinsicSizeValue = imageView.usesIntrinsicSize();

				markNeedsPaint();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				(void)context;

				if (!useIntrinsicSizeValue)
					return constraints.constrain(requestedSizeValue);

				if (!imageValue)
					return constraints.constrain(Size(0.0f, 0.0f));

				float imageWidth = static_cast<float>(imageValue->width());
				float imageHeight = static_cast<float>(imageValue->height());

				if (imageWidth <= 0.0f || imageHeight <= 0.0f)
					return constraints.constrain(Size(0.0f, 0.0f));

				float scale = 1.0f;
				if (constraints.hasBoundedWidth())
					scale = std::min(scale, constraints.maxWidth() / imageWidth);

				if (constraints.hasBoundedHeight())
					scale = std::min(scale, constraints.maxHeight() / imageHeight);

				scale = std::max(scale, 0.0f);

				Size desiredSize(imageWidth * scale, imageHeight * scale);

				return constraints.constrain(desiredSize);
			}

			void paintOverride(Canvas& canvas) override {
				if (!imageValue)
					return;

				const Rect& area = bounds();
				if (area.width <= 0.0f || area.height <= 0.0f)
					return;

				float imageWidth = static_cast<float>(imageValue->width());
				float imageHeight = static_cast<float>(imageValue->height());

				if (imageWidth <= 0.0f || imageHeight <= 0.0f)
					return;

				if (fitValue == ImageFit::Stretch) {
					canvas.drawImage(*imageValue, area, interpolationValue);
					
					return;
				}

				if (fitValue == ImageFit::Cover) {
					Rect source = calculateCoverSource(imageWidth, imageHeight, area);
					canvas.drawImage(*imageValue, area, source, interpolationValue);

					return;
				}

				float scaleX = area.width / imageWidth;
				float scaleY = area.height / imageHeight;
				float scale = std::min(scaleX, scaleY);

				float drawWidth = imageWidth * scale;
				float drawHeight = imageHeight * scale;

				float drawX = area.x + (area.width - drawWidth) * 0.5f;
				float drawY = area.y + (area.height - drawHeight) * 0.5f;

				Rect destination(drawX, drawY, drawWidth, drawHeight);
				canvas.drawImage(*imageValue, destination, interpolationValue);
			}

			bool acceptsPointerEvents() const override {
				return false;
			}

		private:
			Rect calculateCoverSource(float imageWidth, float imageHeight, const Rect& area) const {
				float imageAspect = imageWidth / imageHeight;
				float areaAspect = area.width / area.height;

				float sourceWidth = imageWidth;
				float sourceHeight = imageHeight;

				if (imageAspect > areaAspect)
					sourceWidth = imageHeight * areaAspect;
				else
					sourceHeight = imageWidth / areaAspect;

				float sourceX = (imageWidth - sourceWidth) * 0.5f;
				float sourceY = (imageHeight - sourceHeight) * 0.5f;

				return Rect(sourceX, sourceY, sourceWidth, sourceHeight);
			}

		private:
			std::shared_ptr<Image> imageValue;

			Size requestedSizeValue;

			ImageFit fitValue = ImageFit::Contain;

			ImageInterpolation interpolationValue = ImageInterpolation::Linear;

			bool useIntrinsicSizeValue = false;
		};
	}

	ImageView::ImageView(std::shared_ptr<Image> image, const Size& size, ImageFit fit, ImageInterpolation interpolation, Key key) 
		: Widget(std::move(key)), imageValue(std::move(image)), requestedSizeValue(std::max(size.width, 0.0f), std::max(size.height, 0.0f)), fitValue(fit), interpolationValue(interpolation) { }

	ImageView::ImageView(std::shared_ptr<Image> image, ImageFit fit, ImageInterpolation interpolation, Key key)
		: Widget(std::move(key)), imageValue(std::move(image)), requestedSizeValue(), fitValue(fit), interpolationValue(interpolation), useIntrinsicSizeValue(true) { }

	const std::shared_ptr<Image>& ImageView::image() const {
		return imageValue;
	}

	const Size& ImageView::requestedSize() const {
		return requestedSizeValue;
	}

	ImageFit ImageView::fit() const {
		return fitValue;
	}

	ImageInterpolation ImageView::interpolation() const {
		return interpolationValue;
	}

	bool ImageView::usesIntrinsicSize() const {
		return useIntrinsicSizeValue;
	}

	std::unique_ptr<Element> ImageView::createElement() const {
		return std::make_unique<ImageViewElement>(*this);
	}
}