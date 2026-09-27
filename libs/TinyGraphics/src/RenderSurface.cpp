#include <tiny/graphics/RenderSurface.h>

#include <memory>
#include <cmath>

#include <d2d1_1.h>
#include <d2d1effects.h>
#include <wrl/client.h>

#pragma comment(lib, "dxguid.lib")

namespace tiny {
	class RenderSurface::Impl {
	public:
		Size sizeValue;

		float dpiScaleValue = 1.0f;

		Microsoft::WRL::ComPtr<ID2D1DeviceContext> context;
		Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;

		Microsoft::WRL::ComPtr<ID2D1Effect> gaussianBlurEffect;
		float gaussianBlurStandardDeviation = -1.0f;
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

	void* RenderSurface::gaussianBlurEffectHandle(float standardDeviation) const {
		if (!impl->context)
			return nullptr;

		if (!impl->bitmap)
			return nullptr;

		if (!impl->gaussianBlurEffect) {
			HRESULT result = impl->context->CreateEffect(CLSID_D2D1GaussianBlur, impl->gaussianBlurEffect.ReleaseAndGetAddressOf());
			if (FAILED(result))
				return nullptr;

			impl->gaussianBlurEffect->SetInput(0, impl->bitmap.Get());
			result = impl->gaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED);
			if (FAILED(result)) {
				impl->gaussianBlurEffect.Reset();
				return nullptr;
			}

			result = impl->gaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
			if (FAILED(result)) {
				impl->gaussianBlurEffect.Reset();
				return nullptr;
			}

			impl->gaussianBlurStandardDeviation = -1.0f;
		}

		constexpr float Epsilon = 0.001f;
		if (std::abs(impl->gaussianBlurStandardDeviation - standardDeviation) > Epsilon) {
			HRESULT result = impl->gaussianBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, standardDeviation);
			if (FAILED(result))
				return nullptr;

			impl->gaussianBlurStandardDeviation = standardDeviation;
		}

		return impl->gaussianBlurEffect.Get();
	}
}