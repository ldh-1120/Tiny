#include <tiny/graphics/RenderSurface.h>

#include <memory>

#include <d2d1_1.h>
#include <wrl/client.h>

namespace tiny {
	class RenderSurface::Impl {
	public:
		Size sizeValue;

		float dpiScaleValue = 1.0f;

		Microsoft::WRL::ComPtr<ID2D1DeviceContext> context;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
	};

	RenderSurface::RenderSurface(const Size& size, float dpiScale, void* context, void* bitmap, void* brush) : impl(std::make_unique<Impl>()) {
		impl->sizeValue = size;
		impl->dpiScaleValue = dpiScale;

		impl->context = static_cast<ID2D1DeviceContext*>(context);
		impl->bitmap = static_cast<ID2D1Bitmap1*>(bitmap);
		impl->brush = static_cast<ID2D1SolidColorBrush*>(brush);
	}

	RenderSurface::~RenderSurface() = default;

	const Size& RenderSurface::size() const {
		return impl->sizeValue;
	}

	float RenderSurface::dpiScale() const {
		return impl->dpiScaleValue;
	}

	void* RenderSurface::contextHandle() const {
		return impl->context.Get();
	}

	void* RenderSurface::bitmapHandle() const {
		return impl->bitmap.Get();
	}

	void* RenderSurface::brushHandle() const {
		return impl->brush.Get();
	}
}