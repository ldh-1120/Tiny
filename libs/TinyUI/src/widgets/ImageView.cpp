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
			explicit ImageViewElement(const ImageView& widget) : Element(widget), imageValue(widget.image()), requestedSizeValue(widget.requestedSize()), fitValue(widget.fit()), interpolationValue(widget.interpolation()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ImageView& imageView = static_cast<const ImageView&>(widget);

				imageValue = imageView.image();
				requestedSizeValue = imageView.requestedSize();
				fitValue = imageView.fit();
				interpolationValue = imageView.interpolation();

				markNeedsPaint();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				(void)context;

				return constraints.constrain(requestedSizeValue);
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

				Rect destination = area;
				if (fitValue != ImageFit::Stretch) {
					float scaleX = area.width / imageWidth;
					float scaleY = area.height / imageHeight;

					float scale = 1.0f;
					if (fitValue == ImageFit::Contain)
						scale = std::min(scaleX, scaleY);
					else
						scale = std::max(scaleX, scaleY);

					float drawWidth = imageWidth * scale;
					float drawHeight = imageHeight * scale;

					float drawX = area.x + (area.width - drawWidth) * 0.5f;
					float drawY = area.y + (area.height - drawHeight) * 0.5f;

					destination = Rect(drawX, drawY, drawWidth, drawHeight);
				}

				canvas.pushClip(area);

				canvas.drawImage(*imageValue, destination, interpolationValue);

				canvas.popClip();
			}

			bool acceptsPointerEvents() const override {
				return false;
			}

		private:
			std::shared_ptr<Image> imageValue;

			Size requestedSizeValue;

			ImageFit fitValue = ImageFit::Contain;

			ImageInterpolation interpolationValue = ImageInterpolation::Linear;
		};
	}

	ImageView::ImageView(std::shared_ptr<Image> image, const Size& size, ImageFit fit, ImageInterpolation interpolation, Key key) 
		: Widget(std::move(key)), imageValue(std::move(image)), requestedSizeValue(std::max(size.width, 0.0f), std::max(size.height, 0.0f)), fitValue(fit), interpolationValue(interpolation) { }

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

	std::unique_ptr<Element> ImageView::createElement() const {
		return std::make_unique<ImageViewElement>(*this);
	}
}