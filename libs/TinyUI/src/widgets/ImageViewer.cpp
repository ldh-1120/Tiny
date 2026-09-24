#include <tiny/ui/widgets/ImageViewer.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <cmath>

#include <tiny/core/Color.h>
#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>
#include <tiny/core/input/Pointer.h>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	namespace {
		class ImageViewerElement final : public Element {
		public:
			explicit ImageViewerElement(const ImageViewer& widget) : Element(widget), imageValue(widget.image()), requestedSizeValue(widget.requestedSize()), interpolationValue(widget.interpolation()) { }

		protected:
			void updateOverride(const Widget& widget) override {
				const ImageViewer& viewer = static_cast<const ImageViewer&>(widget);

				if (imageValue != viewer.image()) {
					imageValue = viewer.image();

					zoomValue = 1.0f;

					panX = 0.0f;
					panY = 0.0f;

					dragging = false;
				}

				Size nextSize = viewer.requestedSize();

				bool sizeChanged = nextSize.width != requestedSizeValue.width || nextSize.height != requestedSizeValue.height;
				
				requestedSizeValue = nextSize;
				interpolationValue = viewer.interpolation();

				if (sizeChanged)
					markNeedsLayout();
				else
					markNeedsPaint();
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				(void)context;

				return constraints.constrain(requestedSizeValue);
			}

			void arrangeOverride(const Rect& area) override {
				(void)area;

				clampPan();
			}

			void paintOverride(Canvas& canvas) override {
				const Rect& area = bounds();

				if (area.width <= 0.0f || area.height <= 0.0f)
					return;

				canvas.fillRect(area, Color::fromRgb(24, 24, 37));
				if (!imageValue)
					return;

				float imageWidth = static_cast<float>(imageValue->width()) * zoomValue;
				float imageHeight = static_cast<float>(imageValue->height()) * zoomValue;
				if (imageWidth <= 0.0f || imageHeight <= 0.0f)
					return;

				float imageX = area.x + (area.width - imageWidth) * 0.5f + panX;
				float imageY = area.y + (area.height - imageHeight) * 0.5f + panY;

				canvas.pushClip(area);

				canvas.drawImage(*imageValue, Rect(imageX, imageY, imageWidth, imageHeight), interpolationValue);

				canvas.popClip();
			}

			bool acceptsPointerEvents() const override {
				return imageValue != nullptr;
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				if (!canPan())
					return false;

				dragging = true;
				lastPointerPosition = event.position;

				return true;
			}

			void pointerMoveOverride(const PointerEvent& event) override {
				if (!dragging)
					return;

				float deltaX = event.position.x - lastPointerPosition.x;
				float deltaY = event.position.y - lastPointerPosition.y;

				lastPointerPosition = event.position;

				panX += deltaX;
				panY += deltaY;

				clampPan();
				markNeedsPaint();
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				dragging = false;
			}

			bool pointerWheelOverride(const PointerWheelEvent& event) override {
				if (!imageValue)
					return false;

				if (event.delta == 0.0f)
					return false;

				const Rect& area = bounds();
				if (area.width <= 0.0f || area.height <= 0.0f)
					return false;

				float originalWidth = static_cast<float>(imageValue->width());
				float originalHeight = static_cast<float>(imageValue->height());

				if (originalWidth <= 0.0f || originalHeight <= 0.0f)
					return false;

				float oldZoom = zoomValue;
				float zoomFactor = std::pow(ZoomStep, event.delta);

				float nextZoom = std::clamp(oldZoom * zoomFactor, MinimumZoom, MaximumZoom);
				if (nextZoom == oldZoom)
					return true;

				float oldWidth = originalWidth * oldZoom;
				float oldHeight = originalHeight * oldZoom;

				float oldLeft = area.x + (area.width - oldWidth) * 0.5f + panX;
				float oldTop = area.y + (area.height - oldHeight) * 0.5f + panY;

				float imageX = (event.position.x - oldLeft) / oldZoom;
				float imageY = (event.position.y - oldTop) / oldZoom;

				zoomValue = nextZoom;

				float newWidth = originalWidth * zoomValue;
				float newHeight = originalHeight * zoomValue;

				float newCenterLeft = area.x + (area.width - newWidth) * 0.5f;
				float newCenterTop = area.y + (area.height - newHeight) * 0.5f;

				panX = event.position.x - imageX * zoomValue - newCenterLeft;
				panY = event.position.y - imageY * zoomValue - newCenterTop;

				clampPan();
				markNeedsPaint();

				return true;
			}

			void pointerCancelOverride() override {
				dragging = false;
			}

		private:
			bool canPan() const {
				if (!imageValue)
					return false;

				const Rect& area = bounds();

				float imageWidth = static_cast<float>(imageValue->width()) * zoomValue;
				float imageHeight = static_cast<float>(imageValue->height()) * zoomValue;

				return imageWidth > area.width || imageHeight > area.height;
			}

			void clampPan() {
				if (!imageValue) {
					panX = 0.0f;
					panY = 0.0f;
					return;
				}

				const Rect& area = bounds();

				float imageWidth = static_cast<float>(imageValue->width()) * zoomValue;
				float imageHeight = static_cast<float>(imageValue->height()) * zoomValue;

				if (area.width <= 0.0f || area.height <= 0.0f) {
					panX = 0.0f;
					panY = 0.0f;
					return;
				}

				if (imageWidth <= area.width)
					panX = 0.0f;
				else {
					float centerX = (area.width - imageWidth) * 0.5f;

					float left = std::clamp(centerX + panX, area.width - imageWidth, 0.0f);
					panX = left - centerX;
				}

				if (imageHeight <= area.height)
					panY = 0.0f;
				else {
					float centerY = (area.height - imageHeight) * 0.5f;

					float top = std::clamp(centerY + panY, area.height - imageHeight, 0.0f);
					panY = top - centerY;
				}
			}

		private:
			std::shared_ptr<Image> imageValue;
			Size requestedSizeValue;

			ImageInterpolation interpolationValue = ImageInterpolation::Linear;

			bool dragging = false;

			Point lastPointerPosition;

			float panX = 0.0f;
			float panY = 0.0f;

			float zoomValue = 1.0f;

			static constexpr float MinimumZoom = 0.1f;
			static constexpr float MaximumZoom = 16.0f;
			static constexpr float ZoomStep = 1.15f;
		};
	}
	
	ImageViewer::ImageViewer(std::shared_ptr<Image> image, const Size& size, ImageInterpolation interpolation, Key key) 
		: Widget(std::move(key)), imageValue(std::move(image)), requestedSizeValue(std::max(size.width, 0.0f), std::max(size.height, 0.0f)), interpolationValue(interpolation) { }

	const std::shared_ptr<Image>& ImageViewer::image() const {
		return imageValue;
	}

	const Size& ImageViewer::requestedSize() const {
		return requestedSizeValue;
	}

	ImageInterpolation ImageViewer::interpolation() const {
		return interpolationValue;
	}

	std::unique_ptr<Element> ImageViewer::createElement() const {
		return std::make_unique<ImageViewerElement>(*this);
	}
}