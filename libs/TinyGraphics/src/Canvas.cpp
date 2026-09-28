#include "TextLayoutInternal.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <cstddef>

#include <d2d1_1.h>
#include <d2d1helper.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/TextLayout.h>
#include <tiny/graphics/Image.h>
#include <tiny/graphics/RenderSurface.h>

#pragma comment(lib, "d2d1.lib")

namespace {
	D2D1_COLOR_F toD2DColor(const tiny::Color& color) {
		return D2D1::ColorF(color.r, color.g, color.b, color.a);
	}

	D2D1_RECT_F toD2DRect(const tiny::Rect& rect) {
		return D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
	}

	class ClipSuspension {
	public:
		ClipSuspension(ID2D1RenderTarget* target, const std::vector<tiny::Rect>& clips) : target(target), clips(clips) {
			if (!target)
				return;

			for (std::size_t index = 0; index < clips.size(); ++index)
				target->PopAxisAlignedClip();
		}

		~ClipSuspension() {
			if (!target)
				return;

			for (const tiny::Rect& clip : clips)
				target->PushAxisAlignedClip(toD2DRect(clip), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		}

	private:
		ID2D1RenderTarget* target = nullptr;

		const std::vector<tiny::Rect>& clips;
	};
}

namespace tiny {
	Canvas::Canvas(void* renderTarget, void* textFactory, void* solidBrush) : renderTarget(renderTarget), textFactory(textFactory), solidBrush(solidBrush) { }

	Canvas::~Canvas() {
		while (!opacityLayers.empty())
			popOpacity();
	}

	void Canvas::clear(const Color& color) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		target->Clear(toD2DColor(color));
	}

	void Canvas::fillRect(const Rect& rect, const Color& color) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		ID2D1SolidColorBrush* brush = static_cast<ID2D1SolidColorBrush*>(solidBrush);
		brush->SetColor(toD2DColor(color));
		target->FillRectangle(toD2DRect(rect), brush);
	}

	void Canvas::drawRect(const Rect& rect, const Color& color, float strokeWidth) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		ID2D1SolidColorBrush* brush = static_cast<ID2D1SolidColorBrush*>(solidBrush);
		brush->SetColor(toD2DColor(color));
		target->DrawRectangle(toD2DRect(rect), brush, strokeWidth);
	}

	void Canvas::drawTextLayout(const TextLayout& layout, const Point& origin, const Color& color) {
		if (!layout.impl)
			return;

		if (!layout.impl->nativeLayout)
			return;

		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);

		ID2D1SolidColorBrush* brush = static_cast<ID2D1SolidColorBrush*>(solidBrush);
		brush->SetColor(toD2DColor(color));

		D2D1_POINT_2F nativeOrigin = { origin.x, origin.y };
		target->DrawTextLayout(nativeOrigin, layout.impl->nativeLayout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
	}

	void Canvas::pushClip(const Rect& rect) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return;

		target->PushAxisAlignedClip(toD2DRect(rect), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		clipRects.push_back(rect);
	}

	void Canvas::popClip() {
		if (clipRects.empty())
			return;

		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return;

		target->PopAxisAlignedClip();
		clipRects.pop_back();
	}

	bool Canvas::pushOpacity(float opacity) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return false;

		float safeOpacity = std::clamp(opacity, 0.0f, 1.0f);

		ID2D1Layer* layer = nullptr;
		HRESULT result = target->CreateLayer(nullptr, &layer);
		if (FAILED(result) || !layer)
			return false;

		D2D1_LAYER_PARAMETERS parameters = { };
		parameters.contentBounds = D2D1::InfiniteRect();
		parameters.geometricMask = nullptr;
		parameters.maskAntialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
		parameters.maskTransform = D2D1::Matrix3x2F::Identity();
		parameters.opacity = safeOpacity;
		parameters.opacityBrush = nullptr;
		parameters.layerOptions = D2D1_LAYER_OPTIONS_NONE;

		target->PushLayer(parameters, layer);
		opacityLayers.push_back(layer);

		return true;
	}

	void Canvas::popOpacity() {
		if (opacityLayers.empty())
			return;

		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		ID2D1Layer* layer = static_cast<ID2D1Layer*>(opacityLayers.back());

		opacityLayers.pop_back();
		if (target)
			target->PopLayer();

		if (layer)
			layer->Release();
	}

	void Canvas::drawImage(const Image& image, const Rect& destination, ImageInterpolation interpolation) {
		drawImage(image, destination, Rect(0.0f, 0.0f, static_cast<float>(image.width()), static_cast<float>(image.height())), interpolation);
	}

	void Canvas::drawImage(const Image& image, const Rect& destination, const Rect& source, ImageInterpolation interpolation) {
		if (destination.width <= 0.0f || destination.height <= 0.0f || source.width <= 0.0f || source.height <= 0.0f)
			return;

		float imageWidth = static_cast<float>(image.width());
		float imageHeight = static_cast<float>(image.height());

		if (source.x < 0.0f || source.y < 0.0f || source.x + source.width > imageWidth || source.y + source.height > imageHeight)
			return;

		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return;

		ID2D1Bitmap* bitmap = static_cast<ID2D1Bitmap*>(image.nativeBitmap(renderTarget));
		if (!bitmap)
			return;

		D2D1_BITMAP_INTERPOLATION_MODE nativeInterpolation = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
		if (interpolation == ImageInterpolation::Nearest)
			nativeInterpolation = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

		D2D1_RECT_F destinationRect = D2D1::RectF(destination.x, destination.y, destination.x + destination.width, destination.y + destination.height);
		D2D1_RECT_F sourceRect = D2D1::RectF(source.x, source.y, source.x + source.width, source.y + source.height);

		target->DrawBitmap(bitmap, destinationRect, 1.0f, nativeInterpolation, &sourceRect);
	}

	std::unique_ptr<RenderSurface> Canvas::createRenderSurface(const Size& size) {
		if (size.isEmpty())
			return nullptr;

		ID2D1DeviceContext* sourceContext = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!sourceContext)
			return nullptr;

		Microsoft::WRL::ComPtr<ID2D1Device> device;
		sourceContext->GetDevice(device.ReleaseAndGetAddressOf());

		if (!device)
			return nullptr;

		Microsoft::WRL::ComPtr<ID2D1DeviceContext> surfaceContext;
		HRESULT result = device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, surfaceContext.ReleaseAndGetAddressOf());
		if (FAILED(result))
			return nullptr;

		float dpiX = 96.0f;
		float dpiY = 96.0f;

		sourceContext->GetDpi(&dpiX, &dpiY);

		float scaleX = dpiX / 96.0f;
		float scaleY = dpiY / 96.0f;

		UINT pixelWidth = static_cast<UINT>(std::max(std::ceil(size.width * scaleX), 1.0f));
		UINT pixelHeight = static_cast<UINT>(std::max(std::ceil(size.height * scaleY), 1.0f));

		D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
		result = surfaceContext->CreateBitmap(D2D1::SizeU(pixelWidth, pixelHeight), nullptr, 0, &properties, bitmap.ReleaseAndGetAddressOf());
		if (FAILED(result))
			return nullptr;

		surfaceContext->SetDpi(dpiX, dpiY);

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
		result = surfaceContext->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), brush.ReleaseAndGetAddressOf());
		if (FAILED(result))
			return nullptr;

		return std::unique_ptr<RenderSurface>(new RenderSurface(size, scaleX, surfaceContext.Get(), bitmap.Get(), brush.Get()));
	}

	bool Canvas::pushRenderSurface(RenderSurface& surface, const Point& origin) {
		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(surface.contextHandle());
		ID2D1Bitmap1* bitmap = static_cast<ID2D1Bitmap1*>(surface.bitmapHandle());
		ID2D1SolidColorBrush* brush = static_cast<ID2D1SolidColorBrush*>(surface.brushHandle());

		if (!context || !bitmap || !brush)
			return false;

		ID2D1DeviceContext* currentContext = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!currentContext)
			return false;

		Microsoft::WRL::ComPtr<ID2D1Device> currentDevice;
		Microsoft::WRL::ComPtr<ID2D1Device> surfaceDevice;

		currentContext->GetDevice(currentDevice.ReleaseAndGetAddressOf());
		context->GetDevice(surfaceDevice.ReleaseAndGetAddressOf());

		if (!currentDevice || !surfaceDevice || currentDevice.Get() != surfaceDevice.Get())
			return false;

		RenderSurfaceState state;
		state.renderTarget = renderTarget;
		state.solidBrush = solidBrush;
		state.origin = renderTargetOrigin;

		state.clipRects = std::move(clipRects);
		state.opacityLayers = std::move(opacityLayers);

		clipRects.clear();
		opacityLayers.clear();

		renderSurfaceStates.push_back(std::move(state));

		renderTarget = context;
		solidBrush = brush;
		renderTargetOrigin = origin;

		context->SetTarget(bitmap);
		context->BeginDraw();

		context->SetTransform(D2D1::Matrix3x2F::Translation(-origin.x, -origin.y));
		context->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		return true;
	}

	void Canvas::popRenderSurface() {
		if (renderSurfaceStates.empty())
			return;

		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (context) {
			context->SetTransform(D2D1::Matrix3x2F::Identity());
			context->EndDraw();

			context->SetTarget(nullptr);
		}

		RenderSurfaceState state = renderSurfaceStates.back();
		renderSurfaceStates.pop_back();

		renderTarget = state.renderTarget;
		solidBrush = state.solidBrush;
		renderTargetOrigin = state.origin;

		clipRects = std::move(state.clipRects);
		opacityLayers = std::move(state.opacityLayers);
	}

	void Canvas::drawRenderSurface(const RenderSurface& surface, const Rect& destination, float opacity) {
		if (destination.isEmpty())
			return;

		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return;

		ID2D1Bitmap1* bitmap = static_cast<ID2D1Bitmap1*>(surface.bitmapHandle());
		if (!bitmap)
			return;

		float safeOpacity = std::clamp(opacity, 0.0f, 1.0f);
		context->DrawBitmap(bitmap, toD2DRect(destination), safeOpacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	}

	void Canvas::drawBlurredRenderSurface(const RenderSurface& surface, const Point& origin, float standardDeviation) {
		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return;

		float safeStandardDeviation = std::clamp(standardDeviation, 0.0f, 250.0f);
		if (safeStandardDeviation <= 0.0f) {
			drawRenderSurface(surface, Rect(origin, surface.size()));
			return;
		}

		ID2D1Effect* effect = static_cast<ID2D1Effect*>(surface.gaussianBlurEffectHandle(safeStandardDeviation));
		if (!effect)
			return;

		context->DrawImage(effect, D2D1::Point2F(origin.x, origin.y));
	}

	void Canvas::drawShadowRenderSurface(const RenderSurface& surface, const Point& origin, const Point& offset, float standardDeviation, const Color& color) {
		if (color.a <= 0.0f)
			return;

		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return;

		float safeStandardDeviation = std::clamp(standardDeviation, 0.0f, 250.0f);
		ID2D1Effect* effect = static_cast<ID2D1Effect*>(surface.shadowEffectHandle(safeStandardDeviation, color));
		if (!effect)
			return;

		context->DrawImage(effect, D2D1::Point2F(origin.x + offset.x, origin.y + offset.y));
	}

	bool Canvas::captureRenderSurface(RenderSurface& surface, const Rect& sourceBounds) {
		if (sourceBounds.isEmpty())
			return false;

		ID2D1DeviceContext* sourceContext = static_cast<ID2D1DeviceContext*>(renderTarget);
		ID2D1DeviceContext* destinationContext = static_cast<ID2D1DeviceContext*>(surface.contextHandle());
		ID2D1Bitmap1* destinationBitmap = static_cast<ID2D1Bitmap1*>(surface.bitmapHandle());

		if (!sourceContext || !destinationContext || !destinationBitmap)
			return false;

		if (sourceContext == destinationContext)
			return false;

		Microsoft::WRL::ComPtr<ID2D1Device> sourceDevice;
		Microsoft::WRL::ComPtr<ID2D1Device> destinationDevice;

		sourceContext->GetDevice(sourceDevice.ReleaseAndGetAddressOf());
		destinationContext->GetDevice(destinationDevice.ReleaseAndGetAddressOf());

		if (!sourceDevice || !destinationDevice || sourceDevice.Get() != destinationDevice.Get())
			return false;

		if (!opacityLayers.empty())
			return false;

		ClipSuspension clipSuspension(sourceContext, clipRects);

		HRESULT result = sourceContext->Flush();
		if (FAILED(result))
			return false;

		destinationContext->SetTarget(destinationBitmap);
		destinationContext->BeginDraw();

		destinationContext->SetTransform(D2D1::Matrix3x2F::Identity());
		destinationContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		result = destinationContext->EndDraw();
		destinationContext->SetTarget(nullptr);
		if (FAILED(result))
			return false;

		D2D1_SIZE_F sourceSize = sourceContext->GetSize();

		float targetLeft = renderTargetOrigin.x;
		float targetTop = renderTargetOrigin.y;

		float targetRight = targetLeft + sourceSize.width;
		float targetBottom = targetTop + sourceSize.height;

		float copyLeft = std::max(sourceBounds.left(), targetLeft);
		float copyTop = std::max(sourceBounds.top(), targetTop);

		float copyRight = std::min(sourceBounds.right(), targetRight);
		float copyBottom = std::min(sourceBounds.bottom(), targetBottom);

		for (const Rect& clip : clipRects) {
			copyLeft = std::max(copyLeft, clip.left());
			copyTop = std::max(copyTop, clip.top());

			copyRight = std::min(copyRight, clip.right());
			copyBottom = std::min(copyBottom, clip.bottom());
		}

		if (copyRight <= copyLeft || copyBottom <= copyTop)
			return true;

		float sourceDpiX = 96.0f;
		float sourceDpiY = 96.0f;

		sourceContext->GetDpi(&sourceDpiX, &sourceDpiY);

		float destinationDpiX = 96.0f;
		float destinationDpiY = 96.0f;

		destinationBitmap->GetDpi(&destinationDpiX, &destinationDpiY);

		float sourceScaleX = sourceDpiX / 96.0f;
		float sourceScaleY = sourceDpiY / 96.0f;

		float destinationScaleX = destinationDpiX / 96.0f;
		float destinationScaleY = destinationDpiY / 96.0f;

		D2D1_SIZE_U sourcePixelSize = sourceContext->GetPixelSize();
		D2D1_SIZE_U destinationPixelSize = destinationBitmap->GetPixelSize();

		UINT32 sourceLeft = static_cast<UINT32>(std::max(std::floor((copyLeft - targetLeft) * sourceScaleX), 0.0f));
		UINT32 sourceTop = static_cast<UINT32>(std::max(std::floor((copyTop - targetTop) * sourceScaleY), 0.0f));

		UINT32 sourceRight = static_cast<UINT32>(std::max(std::ceil((copyRight - targetLeft) * sourceScaleX), 0.0f));
		UINT32 sourceBottom = static_cast<UINT32>(std::max(std::ceil((copyBottom - targetTop) * sourceScaleY), 0.0f));

		sourceLeft = std::min(sourceLeft, sourcePixelSize.width);
		sourceTop = std::min(sourceTop, sourcePixelSize.height);

		sourceRight = std::min(sourceRight, sourcePixelSize.width);
		sourceBottom = std::min(sourceBottom, sourcePixelSize.height);

		if (sourceRight <= sourceLeft || sourceBottom <= sourceTop)
			return true;

		UINT32 destinationX = static_cast<UINT32>(std::max(std::floor((copyLeft - sourceBounds.x) * destinationScaleX), 0.0f));
		UINT32 destinationY = static_cast<UINT32>(std::max(std::floor((copyTop - sourceBounds.y) * destinationScaleY), 0.0f));

		if (destinationX >= destinationPixelSize.width || destinationY >= destinationPixelSize.height)
			return true;

		UINT32 copyWidth = sourceRight - sourceLeft;
		UINT32 copyHeight = sourceBottom - sourceTop;

		copyWidth = std::min(copyWidth, destinationPixelSize.width - destinationX);
		copyHeight = std::min(copyHeight, destinationPixelSize.height - destinationY);

		if (copyWidth == 0 || copyHeight == 0)
			return true;

		D2D1_POINT_2U destinationPoint = { destinationX, destinationY };
		D2D1_RECT_U sourceRect = { sourceLeft, sourceTop, sourceLeft + copyWidth, sourceTop + copyHeight };

		result = destinationBitmap->CopyFromRenderTarget(&destinationPoint, sourceContext, &sourceRect);
		return SUCCEEDED(result);
	}

	float Canvas::dpiScale() const {
		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return 1.0f;

		float dpiX = 96.0f;
		float dpiY = 96.0f;

		context->GetDpi(&dpiX, &dpiY);

		return dpiX / 96.0f;
	}
}