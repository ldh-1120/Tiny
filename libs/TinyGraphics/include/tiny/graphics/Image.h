#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tiny {
	class Canvas;

	enum class ImageInterpolation {
		Nearest,
		Linear
	};

	class Image {
	public:
		~Image();

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		static std::shared_ptr<Image> load(const std::wstring& path);

		std::uint32_t width() const;
		std::uint32_t height() const;

	private:
		struct BitmapCache;

		Image(std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t> pixels);

		void* nativeBitmap(void* renderTarget) const;

		std::uint32_t widthValue = 0;
		std::uint32_t heightValue = 0;

		std::vector<std::uint8_t> pixelData;

		mutable std::unique_ptr<BitmapCache> bitmapCache;

		friend class Canvas;
	};
}