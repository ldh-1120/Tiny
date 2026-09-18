#include <tiny/platform/Platform.h>

#include <stdexcept>
#pragma comment(lib, "ole32.lib")

#include <Windows.h>

namespace {
	int openWindowCount = 0;

	bool platformInitialized = false;
	bool comInitialized = false;
}

namespace tiny {
	void initializePlatform() {
		if (platformInitialized)
			return;

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		if (FAILED(result))
			throw std::runtime_error("Failed to initialize COM");

		comInitialized = true;
		platformInitialized = true;
	}

	void shutdownPlatform() {
		if (!platformInitialized)
			return;

		if (comInitialized) {
			CoUninitialize();
			comInitialized = false;
		}

		platformInitialized = false;
	}

	int runMessageLoop() {
		MSG message;
		while (GetMessageW(&message, nullptr, 0, 0) > 0) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}

		return static_cast<int>(message.wParam);
	}
}

namespace tiny::detail {
	void notifyWindowCreated() {
		++openWindowCount;
	}

	void notifyWindowDestroyed() {
		--openWindowCount;
		if (openWindowCount == 0)
			PostQuitMessage(0);
	}
}