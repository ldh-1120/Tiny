#pragma once

#include <memory>
#include <string_view>

#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/FontMetrics.h>

namespace tiny {
	class WindowRenderer;
	class TextLayout;
	class TextStyle;

	class GraphicsContext {
	public:
		GraphicsContext();
		~GraphicsContext();

		GraphicsContext(const GraphicsContext&) = delete;
		GraphicsContext& operator=(const GraphicsContext&) = delete;

		GraphicsContext(GraphicsContext&& other) = delete;
		GraphicsContext& operator=(GraphicsContext&& other) = delete;

		std::unique_ptr<TextLayout> createTextLayout(std::u32string_view text, const TextStyle& style, float maximumWidth) const;

		FontMetrics getFontMetrics(const TextStyle& style) const;

	private:
		class Impl;

		std::unique_ptr<Impl> impl;

		void* d2dFactoryHandle() const;
		void* dwriteFactoryHandle() const;

		friend class WindowRenderer;
	};
}