#pragma once

#include <functional>
#include <memory>

#include <tiny/graphics/Canvas.h>

namespace tiny {
	class GraphicsContext;
	class Window;

	class WindowRenderer {
	public:
		using DrawCallback = std::function<void(Canvas&)>;

		WindowRenderer(GraphicsContext& graphicsContext, Window& window);
		~WindowRenderer();

		WindowRenderer(const WindowRenderer&) = delete;
		WindowRenderer& operator=(const WindowRenderer&) = delete;

		WindowRenderer(WindowRenderer&&) = delete;
		WindowRenderer& operator=(WindowRenderer&&) = delete;

		void render(const DrawCallback& callback);

		void requestRepaint();

	private:
		class Impl;

		std::unique_ptr<Impl> impl;
	};
}