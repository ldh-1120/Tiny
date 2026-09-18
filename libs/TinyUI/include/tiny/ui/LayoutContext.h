#pragma once

namespace tiny {
	class GraphicsContext;

	class LayoutContext {
	public:
		explicit LayoutContext(GraphicsContext& graphicsContext) : graphicsContextValue(graphicsContext) { }

		GraphicsContext& graphicsContext() {
			return graphicsContextValue;
		}

		const GraphicsContext& graphicsContext() const {
			return graphicsContextValue;
		}

	private:
		GraphicsContext& graphicsContextValue;
	};
}