#include "TextLayoutInternal.h"

#include <algorithm>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/TextLayout.h>
#include <tiny/graphics/Image.h>

namespace {
	D2D1_COLOR_F toD2DColor(const tiny::Color& color) {
		return D2D1::ColorF(color.r, color.g, color.b, color.a);
	}

	D2D1_RECT_F toD2DRect(const tiny::Rect& rect) {
		return D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
	}
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
		D2D1_RECT_F nativeRect = { rect.x, rect.y, rect.x + rect.width, rect.y + rect.height };

		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		target->PushAxisAlignedClip(nativeRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}

	void Canvas::popClip() {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		target->PopAxisAlignedClip();
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
}