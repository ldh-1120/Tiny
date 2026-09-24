#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include <tiny/core/Point.h>

namespace tiny {
	class Canvas;

	class PngIcon {
	public:
		static constexpr std::uint32_t Width = 16;
		static constexpr std::uint32_t Height = 16;

		PngIcon();
		~PngIcon();

		PngIcon(const PngIcon&) = delete;
		PngIcon& operator=(const PngIcon&) = delete;

		static std::shared_ptr<PngIcon> load(const std::wstring& path);

		void paint(Canvas& canvas, const Point& position, float displaySize) const;

	private:
		struct BitmapCache;

		void* nativeBitmap(void* renderTarget) const;

		std::array<std::uint8_t, Width* Height * 4> pixels { };

		mutable std::unique_ptr<BitmapCache> bitmapCache;

		friend class Canvas;
	};
}