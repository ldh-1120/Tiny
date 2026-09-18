#include <tiny/graphics/TextLayout.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <dwrite.h>

#include "TextLayoutInternal.h"

namespace tiny {
	TextLayout::TextLayout(std::unique_ptr<Impl> impl) : impl(std::move(impl)) { }
	TextLayout::~TextLayout() = default;

	const Size& TextLayout::size() const {
		return impl->measureSize;
	}

	float TextLayout::baseline() const {
		return impl->baseline;
	}

	std::size_t TextLayout::textLength() const {
		return impl->text.size();
	}

	TextHitTestResult TextLayout::hitTestPoint(const Point& point) const {
		TextHitTestResult result;
		if (!impl->nativeLayout)
			return result;

		BOOL trailing = FALSE;
		BOOL inside = FALSE;

		DWRITE_HIT_TEST_METRICS metrics = { };

		HRESULT hr = impl->nativeLayout->HitTestPoint(point.x, point.y, &trailing, &inside, &metrics);
		if (FAILED(hr))
			return result;

		std::uint32_t utf16Position = metrics.textPosition;
		if (trailing != FALSE)
			utf16Position += metrics.length;

		result.textPosition = impl->toUtf32(utf16Position);
		result.inside = inside != FALSE;
		result.trailing = trailing != FALSE;

		return result;
	}

	TextPositionMetrics TextLayout::hitTestTextPosition(std::size_t textPosition) const {
		TextPositionMetrics result;
		if (!impl->nativeLayout)
			return result;

		std::size_t clamped = std::min(textPosition, impl->text.size());
		
		std::uint32_t utf16Position = impl->toUtf16(clamped);

		FLOAT x = 0.0f;
		FLOAT y = 0.0f;

		DWRITE_HIT_TEST_METRICS metrics = { };

		HRESULT hr = impl->nativeLayout->HitTestTextPosition(utf16Position, FALSE, &x, &y, &metrics);
		if (SUCCEEDED(hr)) {
			result.position = Point(x, y);
			result.height = metrics.height;

			return result;
		}

		if (clamped == impl->text.size() && !impl->text.empty()) {
			std::uint32_t lastPosition = impl->toUtf16(impl->text.size() - 1);
			
			hr = impl->nativeLayout->HitTestTextPosition(lastPosition, TRUE, &x, &y, &metrics);
			if (SUCCEEDED(hr)) {
				result.position = Point(x, y);
				result.height = metrics.height;
			}
		}

		return result;
	}

	std::vector<Rect> TextLayout::hitTestRange(std::size_t start, std::size_t length) const {
		std::vector<Rect> rectangles;
		if (!impl->nativeLayout)
			return rectangles;

		if (length == 0)
			return rectangles;

		std::size_t clampedStart = std::min(start, impl->text.size());
		std::size_t clampedEnd = std::min(clampedStart + length, impl->text.size());
		if (clampedEnd <= clampedStart)
			return rectangles;

		std::uint32_t utf16Start = impl->toUtf16(clampedStart);
		std::uint32_t utf16End = impl->toUtf16(clampedEnd);

		std::uint32_t utf16Length = utf16End - utf16Start;

		UINT32 actualCount = 0;
		impl->nativeLayout->HitTestTextRange(utf16Start, utf16Length, 0.0f, 0.0f, nullptr, 0, &actualCount);

		if (actualCount == 0)
			return rectangles;

		std::vector<DWRITE_HIT_TEST_METRICS> metrics(actualCount);

		HRESULT hr = impl->nativeLayout->HitTestTextRange(utf16Start, utf16Length, 0.0f, 0.0f, metrics.data(), actualCount, &actualCount);
		if (FAILED(hr))
			return rectangles;

		rectangles.reserve(actualCount);
		for (UINT32 index = 0; index < actualCount; ++index) {
			const DWRITE_HIT_TEST_METRICS& metric = metrics[index];
			

			rectangles.push_back(Rect(metric.left, metric.top, metric.width, metric.height));
		}

		return rectangles;
	}

	std::size_t TextLayout::previousCaretPosition(std::size_t position) const {
		if (impl->caretStops.empty())
			return 0;

		std::size_t clamped = std::min(position, impl->text.size());

		std::vector<std::size_t>::const_iterator iterator = std::lower_bound(impl->caretStops.begin(), impl->caretStops.end(), clamped);
		if (iterator == impl->caretStops.begin())
			return 0;

		--iterator;

		return *iterator;
	}

	std::size_t TextLayout::nextCaretPosition(std::size_t position) const {
		if (impl->caretStops.empty())
			return 0;

		std::size_t clamped = std::min(position, impl->text.size());

		std::vector<std::size_t>::const_iterator iterator = std::upper_bound(impl->caretStops.begin(), impl->caretStops.end(), clamped);
		if (iterator == impl->caretStops.end())
			return impl->text.size();

		return *iterator;
	}
}