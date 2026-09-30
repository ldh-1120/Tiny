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

namespace tiny {
	class CanvasClipState {
	public:
		enum class Kind {
			AxisAligned,
			Geometry
		};

		Kind kind = Kind::AxisAligned;

		Rect localBounds;
		Rect worldBounds;

		D2D1_MATRIX_3X2_F nativeTransform = D2D1::Matrix3x2F::Identity();

		Microsoft::WRL::ComPtr<ID2D1RectangleGeometry> geometry;
	};
}

namespace {
	D2D1_COLOR_F toD2DColor(const tiny::Color& color) {
		return D2D1::ColorF(color.r, color.g, color.b, color.a);
	}

	D2D1_RECT_F toD2DRect(const tiny::Rect& rect) {
		return D2D1::RectF(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
	}

	D2D1_MATRIX_3X2_F toD2DMatrix(const tiny::AffineTransform& transform) {
		D2D1_MATRIX_3X2_F result = {
			transform.m11,
			transform.m12,

			transform.m21,
			transform.m22,

			transform.dx,
			transform.dy
		};

		return result;
	}

	bool isAxisAlignedTransform(const tiny::AffineTransform& transform) {
		constexpr float Epsilon = 0.000001f;
		return std::abs(transform.m12) <= Epsilon && std::abs(transform.m21) <= Epsilon;
	}

	void pushClipState(ID2D1DeviceContext* context, const tiny::CanvasClipState& state) {
		if (!context)
			return;

		context->SetTransform(state.nativeTransform);
		if (state.kind == tiny::CanvasClipState::Kind::AxisAligned) {
			context->PushAxisAlignedClip(toD2DRect(state.localBounds), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			return;
		}

		if (!state.geometry)
			return;

		D2D1_LAYER_PARAMETERS1 parameters = { };
		parameters.contentBounds = D2D1::InfiniteRect();
		parameters.geometricMask = state.geometry.Get();
		parameters.maskAntialiasMode = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
		parameters.maskTransform = D2D1::Matrix3x2F::Identity();
		parameters.opacity = 1.0f;
		parameters.opacityBrush = nullptr;
		parameters.layerOptions = D2D1_LAYER_OPTIONS1_NONE;

		context->PushLayer(parameters, nullptr);
	}

	void popClipState(ID2D1DeviceContext* context, const tiny::CanvasClipState& state) {
		if (!context)
			return;

		if (state.kind == tiny::CanvasClipState::Kind::AxisAligned) {
			context->PopAxisAlignedClip();
			return;
		}

		context->PopLayer();
	}

	class ClipSuspension {
	public:
		ClipSuspension(ID2D1DeviceContext* target, const std::vector<std::shared_ptr<tiny::CanvasClipState>>& clips) : target(target), clips(clips) {
			if (!target)
				return;

			target->GetTransform(&savedTransform);

			for (std::size_t index = clips.size(); index > 0; --index) {
				const std::shared_ptr<tiny::CanvasClipState>& clip = clips[index - 1];
				if (!clip)
					continue;

				popClipState(target, *clip);
			}
		}

		~ClipSuspension() {
			if (!target)
				return;

			for (const std::shared_ptr<tiny::CanvasClipState>& clip : clips) {
				if (!clip)
					continue;

				pushClipState(target, *clip);
			}

			target->SetTransform(savedTransform);
		}

	private:
		ID2D1DeviceContext* target = nullptr;

		const std::vector<std::shared_ptr<tiny::CanvasClipState>>& clips;

		D2D1_MATRIX_3X2_F savedTransform = D2D1::Matrix3x2F::Identity();
	};
}

namespace tiny {
	Canvas::Canvas(void* renderTarget, void* textFactory, void* solidBrush) : renderTarget(renderTarget), textFactory(textFactory), solidBrush(solidBrush) { }

	void Canvas::applyPaintTransform() {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return;

		AffineTransform surfaceTransform = AffineTransform::translation(-renderTargetOrigin.x, -renderTargetOrigin.y);
		AffineTransform finalTransform = paintTransform * surfaceTransform;

		target->SetTransform(toD2DMatrix(finalTransform));
	}

	bool Canvas::captureBackdropSurfaceUntransformed(RenderSurface& surface, const Rect& sourceBounds) {
		if (sourceBounds.isEmpty())
			return false;

		if (backdropSources.empty())
			return captureRenderSurface(surface, sourceBounds);

		const BackdropSourceState& source = backdropSources.back();
		if (!source.surface)
			return false;

		ID2D1DeviceContext* destinationContext = static_cast<ID2D1DeviceContext*>(surface.contextHandle());
		ID2D1Bitmap1* destinationBitmap = static_cast<ID2D1Bitmap1*>(surface.bitmapHandle());
		ID2D1Bitmap1* sourceBitmap = static_cast<ID2D1Bitmap1*>(source.surface->bitmapHandle());

		if (!destinationContext || !destinationBitmap || !sourceBitmap)
			return false;

		if (destinationBitmap == sourceBitmap)
			return false;

		destinationContext->SetTarget(destinationBitmap);
		destinationContext->BeginDraw();

		destinationContext->SetTransform(D2D1::Matrix3x2F::Identity());
		destinationContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		HRESULT result = destinationContext->EndDraw();
		destinationContext->SetTarget(nullptr);

		if (FAILED(result))
			return false;

		Rect copyBounds = intersectRect(sourceBounds, source.bounds);
		if (copyBounds.isEmpty())
			return true;

		float sourceScale = source.surface->dpiScale();
		float destinationScale = surface.dpiScale();

		D2D1_SIZE_U sourcePixelSize = sourceBitmap->GetPixelSize();
		D2D1_SIZE_U destinationPixelSize = destinationBitmap->GetPixelSize();

		UINT32 sourceLeft = static_cast<UINT32>(std::max(std::floor((copyBounds.left() - source.bounds.left()) * sourceScale), 0.0f));
		UINT32 sourceTop = static_cast<UINT32>(std::max(std::floor((copyBounds.top() - source.bounds.top()) * sourceScale), 0.0f));

		UINT32 sourceRight = static_cast<UINT32>(std::max(std::ceil((copyBounds.right() - source.bounds.left()) * sourceScale), 0.0f));
		UINT32 sourceBottom = static_cast<UINT32>(std::max(std::ceil((copyBounds.bottom() - source.bounds.top()) * sourceScale), 0.0f));

		sourceLeft = std::min(sourceLeft, sourcePixelSize.width);
		sourceTop = std::min(sourceTop, sourcePixelSize.height);

		sourceRight = std::min(sourceRight, sourcePixelSize.width);
		sourceBottom = std::min(sourceBottom, sourcePixelSize.height);

		if (sourceRight <= sourceLeft || sourceBottom <= sourceTop)
			return true;

		UINT32 destinationX = static_cast<UINT32>(std::max(std::floor((copyBounds.left() - sourceBounds.left()) * destinationScale), 0.0f));
		UINT32 destinationY = static_cast<UINT32>(std::max(std::floor((copyBounds.top() - sourceBounds.top()) * destinationScale), 0.0f));

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

		result = destinationBitmap->CopyFromBitmap(&destinationPoint, sourceBitmap, &sourceRect);
		return SUCCEEDED(result);
	}

	bool Canvas::captureBackdropSurfaceTransformed(RenderSurface& surface, const Rect& sourceBounds) {
		AffineTransform inverse;
		if (!paintTransform.tryInverse(inverse))
			return false;

		Rect transformedBounds = paintTransform.transformBounds(sourceBounds);
		if (transformedBounds.isEmpty())
			return false;

		std::unique_ptr<RenderSurface> stagingSurface = createRenderSurface(transformedBounds.size());
		if (!stagingSurface)
			return false;

		if (!captureBackdropSurfaceUntransformed(*stagingSurface, transformedBounds))
			return false;

		ID2D1DeviceContext* destinationContext = static_cast<ID2D1DeviceContext*>(surface.contextHandle());
		ID2D1Bitmap1* destinationBitmap = static_cast<ID2D1Bitmap1*>(surface.bitmapHandle());
		ID2D1Bitmap1* stagingBitmap = static_cast<ID2D1Bitmap1*>(stagingSurface->bitmapHandle());

		if (!destinationContext || !destinationBitmap || !stagingBitmap)
			return false;

		AffineTransform sourceToDestination = AffineTransform::translation(transformedBounds.x, transformedBounds.y) * inverse * AffineTransform::translation(-sourceBounds.x, -sourceBounds.y);
		
		destinationContext->SetTarget(destinationBitmap);
		destinationContext->BeginDraw();

		destinationContext->SetTransform(D2D1::Matrix3x2F::Identity());
		destinationContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

		destinationContext->SetTransform(toD2DMatrix(sourceToDestination));

		D2D1_RECT_F destinationRect = D2D1::RectF(0.0f, 0.0f, transformedBounds.width, transformedBounds.height);
		destinationContext->DrawBitmap(stagingBitmap, destinationRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

		destinationContext->SetTransform(D2D1::Matrix3x2F::Identity());

		HRESULT result = destinationContext->EndDraw();
		destinationContext->SetTarget(nullptr);

		return SUCCEEDED(result);
	}

	Canvas::~Canvas() = default;

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
		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return;

		std::shared_ptr<CanvasClipState> state = std::make_shared<CanvasClipState>();
		state->localBounds = rect;
		state->worldBounds = paintTransform.transformBounds(rect);

		context->GetTransform(&state->nativeTransform);
		if (isAxisAlignedTransform(paintTransform))
			state->kind = CanvasClipState::Kind::AxisAligned;
		else {
			Microsoft::WRL::ComPtr<ID2D1Factory> factory;
			context->GetFactory(factory.ReleaseAndGetAddressOf());

			if (factory) {
				HRESULT result = factory->CreateRectangleGeometry(toD2DRect(rect), state->geometry.ReleaseAndGetAddressOf());
				if (SUCCEEDED(result) && state->geometry)
					state->kind = CanvasClipState::Kind::Geometry;
			}
		}

		pushClipState(context, *state);
		clipStates.push_back(std::move(state));
	}

	void Canvas::popClip() {
		if (clipStates.empty())
			return;

		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return;

		std::shared_ptr<CanvasClipState> state = clipStates.back();
		clipStates.pop_back();

		if (!state)
			return;

		popClipState(context, *state);
	}

	bool Canvas::pushTransform(const AffineTransform& transform) {
		ID2D1RenderTarget* target = static_cast<ID2D1RenderTarget*>(renderTarget);
		if (!target)
			return false;

		transformStates.push_back(paintTransform);

		paintTransform = transform * paintTransform;
		applyPaintTransform();

		return true;
	}

	void Canvas::popTransform() {
		if (transformStates.empty())
			return;

		paintTransform = transformStates.back();
		transformStates.pop_back();

		applyPaintTransform();
	}

	void Canvas::pushBackdropSurface(const RenderSurface& surface, const Rect& bounds) {
		if (bounds.isEmpty())
			return;

		BackdropSourceState state;
		state.surface = &surface;
		state.bounds = bounds;

		backdropSources.push_back(state);
	}

	void Canvas::popBackdropSurface() {
		if (backdropSources.empty())
			return;

		backdropSources.pop_back();
	}

	bool Canvas::captureBackdropSurface(RenderSurface& surface, const Rect& sourceBounds) {
		if (sourceBounds.isEmpty())
			return false;

		if (paintTransform.isIdentity())
			return captureBackdropSurfaceUntransformed(surface, sourceBounds);

		return captureBackdropSurfaceTransformed(surface, sourceBounds);
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

	bool Canvas::pushRenderSurface(RenderSurface& surface, const Point& origin, bool clear) {
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
		state.transform = paintTransform;

		state.clips = std::move(clipStates);

		clipStates.clear();

		renderSurfaceStates.push_back(std::move(state));

		renderTarget = context;
		solidBrush = brush;
		renderTargetOrigin = origin;

		paintTransform = AffineTransform::identity();

		context->SetTarget(bitmap);
		context->BeginDraw();

		applyPaintTransform();

		if (clear)
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
		paintTransform = state.transform;

		clipStates = std::move(state.clips);

		applyPaintTransform();
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

		ClipSuspension clipSuspension(sourceContext, clipStates);

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

		for (const std::shared_ptr<CanvasClipState>& clip : clipStates) {
			if (!clip)
				continue;

			const Rect& bounds = clip->worldBounds;

			copyLeft = std::max(copyLeft, bounds.left());
			copyTop = std::max(copyTop, bounds.top());

			copyRight = std::min(copyRight, bounds.right());
			copyBottom = std::min(copyBottom, bounds.bottom());
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

	Rect Canvas::currentPaintBounds() const {
		ID2D1DeviceContext* context = static_cast<ID2D1DeviceContext*>(renderTarget);
		if (!context)
			return Rect();

		D2D1_SIZE_F size = context->GetSize();
		Rect result(renderTargetOrigin.x, renderTargetOrigin.y, size.width, size.height);

		for (const std::shared_ptr<CanvasClipState>& clip : clipStates) {
			if (!clip)
				continue;

			result = intersectRect(result, clip->worldBounds);
			if (result.isEmpty())
				return result;
		}

		return result;
	}
}