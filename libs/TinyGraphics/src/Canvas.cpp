#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/TextLayout.h>

#include "TextLayoutInternal.h"

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wrl/client.h>

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
}