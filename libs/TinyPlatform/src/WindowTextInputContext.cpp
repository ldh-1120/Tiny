#include <tiny/platform/WindowTextInputContext.h>

#include <algorithm>

#include <Windows.h>
#include <imm.h>

#include <tiny/platform/Window.h>

namespace tiny {
	WindowTextInputContext::WindowTextInputContext(Window& window) : window(window) { }

	void WindowTextInputContext::setCaretRect(const Rect& rect) {
		HWND handle = static_cast<HWND>(window.nativeHandle());
		if (!handle)
			return;

		float scale = window.dpiScale();

		int x = static_cast<int>(rect.x * scale);
		int y = static_cast<int>((rect.y + rect.height) * scale);

		HIMC context = ImmGetContext(handle);
		if (!context)
			return;

		COMPOSITIONFORM compositionForm = { };
		compositionForm.dwStyle = CFS_POINT;
		compositionForm.ptCurrentPos.x = x;
		compositionForm.ptCurrentPos.y = y;

		ImmSetCompositionWindow(context, &compositionForm);

		CANDIDATEFORM candidateForm = { };
		candidateForm.dwIndex = 0;
		candidateForm.dwStyle = CFS_CANDIDATEPOS;
		candidateForm.ptCurrentPos.x = x;
		candidateForm.ptCurrentPos.y = y;

		ImmSetCandidateWindow(context, &candidateForm);

		ImmReleaseContext(handle, context);
	}
}