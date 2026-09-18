#pragma once

#include <tiny/core/Rect.h>

namespace tiny {
	class TextInputContext {
	public:
		virtual ~TextInputContext() = default;

		TextInputContext(const TextInputContext&) = delete;
		TextInputContext& operator=(const TextInputContext&) = delete;

		virtual void setCaretRect(const Rect& rect) = 0;

	protected:
		TextInputContext() = default;
	};
}