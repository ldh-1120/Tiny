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
			explicit ImageViewerElement(const ImageViewer& widget) : Element(widget), imageValue(widget.image()), requestedSizeValue(widget.requestedSize()), interpolationValue(widget.interpolation()), zoomChangedCallback(widget.onZoomChanged()) { }

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
				zoomChangedCallback = viewer.onZoomChanged();

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
				if (event.delta == 0.0f)
					return false;

				float zoomFactor = std::pow(ZoomStep, event.delta);
				setZoomAt(zoomValue * zoomFactor, event.position);

				return true;
			}

			void pointerCancelOverride() override {
				dragging = false;
			}

			bool focusable() const override {
				return imageValue != nullptr;
			}

			bool keyDownOverride(const KeyEvent& event) override {
				if (!imageValue)
					return false;

				if (!event.modifiers.control || event.modifiers.alt)
					return false;

				if (event.key == KeyCode::Equal) {
					setZoomCentered(zoomValue * ZoomStep);
					return true;
				}

				if (event.key == KeyCode::Minus) {
					setZoomCentered(zoomValue / ZoomStep);
					return true;
				}
				
				if (event.key == KeyCode::Digit0 && !event.repeated) {
					setFitZoom();
					return true;
				}

				if (event.key == KeyCode::Digit1 && !event.repeated) {
					setZoomCentered(1.0f);
					return true;
				}

				return true;
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

			float calculateFitZoom() const {
				if (!imageValue)
					return 1.0f;

				const Rect& area = bounds();

				float imageWidth = static_cast<float>(imageValue->width());
				float imageHeight = static_cast<float>(imageValue->height());

				if (area.width <= 0.0f || area.height <= 0.0f || imageWidth <= 0.0f || imageHeight <= 0.0f)
					return 1.0f;

				float scaleX = area.width / imageWidth;
				float scaleY = area.height / imageHeight;

				return std::min(1.0f, std::max(scaleX, scaleY));
			}

			void setFitZoom() {
				float fitZoom = calculateFitZoom();
				zoomValue = fitZoom;

				panX = 0.0f;
				panY = 0.0f;

				dragging = false;

				clampPan();
				notifyZoomChanged();
			}

			void notifyZoomChanged() {
				markNeedsPaint();

				if (zoomChangedCallback)
					zoomChangedCallback(zoomValue);
			}

			void setZoomCentered(float zoom) {
				if (!imageValue)
					return;

				float nextZoom = std::clamp(zoom, MinimumZoom, MaximumZoom);
				if (nextZoom == zoomValue)
					return;

				zoomValue = nextZoom;

				panX = 0.0f;
				panY = 0.0f;

				clampPan();
				notifyZoomChanged();
			}

			void setZoomAt(float zoom, const Point& position) {
				if (!imageValue)
					return;

				const Rect& area = bounds();
				if (area.width <= 0.0f || area.height <= 0.0f)
					return;

				float originalWidth = static_cast<float>(imageValue->width());
				float originalHeight = static_cast<float>(imageValue->height());

				if (originalWidth <= 0.0f || originalHeight <= 0.0f)
					return;

				float oldZoom = zoomValue;
				float nextZoom = std::clamp(zoom, MinimumZoom, MaximumZoom);

				if (nextZoom == oldZoom)
					return;

				float oldWidth = originalWidth * oldZoom;
				float oldHeight = originalHeight * oldZoom;

				float oldLeft = area.x + (area.width - oldWidth) * 0.5f + panX;
				float oldTop = area.y + (area.height - oldHeight) * 0.5f + panY;

				float imageX = (position.x - oldLeft) / oldZoom;
				float imageY = (position.y - oldTop) / oldZoom;

				zoomValue = nextZoom;

				float newWidth = originalWidth * zoomValue;
				float newHeight = originalHeight * zoomValue;

				float newCenterLeft = area.x + (area.width - newWidth) * 0.5f;
				float newCenterTop = area.y + (area.height - newHeight) * 0.5f;

				panX = position.x - imageX * zoomValue - newCenterLeft;
				panY = position.y - imageY * zoomValue - newCenterTop;

				clampPan();
				notifyZoomChanged();
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

			ImageViewer::ZoomChangedCallback zoomChangedCallback;

			static constexpr float MinimumZoom = 0.05f;
			static constexpr float MaximumZoom = 64.0f;
			static constexpr float ZoomStep = 1.15f;
		};
	}
	
	ImageViewer::ImageViewer(std::shared_ptr<Image> image, const Size& size, ImageInterpolation interpolation, Key key, ZoomChangedCallback onZoomChanged)
		: Widget(std::move(key)), imageValue(std::move(image)), requestedSizeValue(std::max(size.width, 0.0f), std::max(size.height, 0.0f)), interpolationValue(interpolation), zoomChangedCallback(std::move(onZoomChanged)) { }

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

	const ImageViewer::ZoomChangedCallback& ImageViewer::onZoomChanged() const {
		return zoomChangedCallback;
	}
}