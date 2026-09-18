#pragma once

#include <tiny/core/services/TextInputContext.h>

namespace tiny {
	class Window;

	class WindowTextInputContext final : public TextInputContext {
	public:
		explicit WindowTextInputContext(Window& window);

		void setCaretRect(const Rect& rect) override;

	private:
		Window& window;
	};
}