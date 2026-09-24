#include <tiny/core/text/Unicode.h>

#include <tiny/platform/Window.h>

#include <stdexcept>
#include <utility>
#include <string>
#include <optional>
#include <chrono>

#include <Windows.h>
#include <windowsx.h>
#include <imm.h>

#include "Win32/PlatformWin32.h"

namespace {
	constexpr wchar_t WINDOW_CLASS_NAME[] = L"Tiny.Window";
	
	tiny::PointerModifiers getPointerModifiers(WPARAM wParam) {
		tiny::PointerModifiers modifiers;
		modifiers.shift = (wParam & MK_SHIFT) != 0;
		modifiers.control = (wParam & MK_CONTROL) != 0;
		modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

		return modifiers;
	}

	tiny::PointerEvent createPointerEvent(HWND hwnd, LPARAM lParam, WPARAM wParam, tiny::PointerButton button) {
		int physicalX = GET_X_LPARAM(lParam);
		int physicalY = GET_Y_LPARAM(lParam);

		UINT dpi = GetDpiForWindow(hwnd);

		float scale = static_cast<float>(dpi) / 96.0f;
		if (scale <= 0.0f)
			scale = 1.0f;

		tiny::PointerEvent event;
		event.position = tiny::Point(static_cast<float>(physicalX) / scale, static_cast<float>(physicalY) / scale);
		event.button = button;
		event.modifiers = getPointerModifiers(wParam);

		return event;
	}

	tiny::KeyCode translateKey(WPARAM virtualKey) {
		if (virtualKey >= 'A' && virtualKey <= 'Z') {
			int offset = static_cast<int>(virtualKey - 'A');
			return static_cast<tiny::KeyCode>(static_cast<int>(tiny::KeyCode::A) + offset);
		}

		if (virtualKey >= '0' && virtualKey <= '9') {
			int offset = static_cast<int>(virtualKey - '0');
			return static_cast<tiny::KeyCode>(static_cast<int>(tiny::KeyCode::Digit0) + offset);
		}

		if (virtualKey >= VK_F1 && virtualKey <= VK_F12) {
			int offset = static_cast<int>(virtualKey - VK_F1);
			return static_cast<tiny::KeyCode>(static_cast<int>(tiny::KeyCode::F1) + offset);
		}

		switch (virtualKey) {
			case VK_TAB:
				return tiny::KeyCode::Tab;

			case VK_RETURN:
				return tiny::KeyCode::Enter;

			case VK_ESCAPE:
				return tiny::KeyCode::Escape;

			case VK_SPACE:
				return tiny::KeyCode::Space;

			case VK_BACK:
				return tiny::KeyCode::Backspace;

			case VK_DELETE:
				return tiny::KeyCode::DeleteKey;

			case VK_INSERT:
				return tiny::KeyCode::Insert;

			case VK_HOME:
				return tiny::KeyCode::Home;

			case VK_END:
				return tiny::KeyCode::End;

			case VK_PRIOR:
				return tiny::KeyCode::PageUp;

			case VK_NEXT:
				return tiny::KeyCode::PageDown;

			case VK_LEFT:
				return tiny::KeyCode::Left;

			case VK_RIGHT:
				return tiny::KeyCode::Right;

			case VK_UP:
				return tiny::KeyCode::Up;

			case VK_DOWN:
				return tiny::KeyCode::Down;

			case VK_SHIFT:
				return tiny::KeyCode::Shift;

			case VK_CONTROL:
				return tiny::KeyCode::Control;

			case VK_MENU:
				return tiny::KeyCode::Alt;

			case VK_CAPITAL:
				return tiny::KeyCode::CapsLock;

			case VK_NUMLOCK:
				return tiny::KeyCode::NumLock;

			case VK_SCROLL:
				return tiny::KeyCode::ScrollLock;

			default:
				return tiny::KeyCode::Unknown;
		}
	}

	tiny::KeyboardModifiers getKeyboardModifiers() {
		tiny::KeyboardModifiers modifiers;
		modifiers.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		modifiers.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		modifiers.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

		return modifiers;
	}

	tiny::KeyEvent createKeyEvent(WPARAM wParam, LPARAM lParam) {
		tiny::KeyEvent event;
		event.key = translateKey(wParam);
		event.modifiers = getKeyboardModifiers();
		event.repeated = (static_cast<unsigned long long>(lParam) & (1ULL << 30)) != 0;
		event.repeatCount = static_cast<unsigned int>(lParam & 0xFFFF);

		return event;
	}

	bool isHighSurrogate(char16_t value) {
		return value >= 0xD800 && value <= 0xDBFF;
	}

	bool isLowSurrogate(char16_t value) {
		return value >= 0xDC00 && value <= 0xDFFF;
	}

	char32_t combineSurrogates(char16_t high, char16_t low) {
		char32_t highValue = static_cast<char32_t>(high - 0xD800);
		char32_t lowValue = static_cast<char32_t>(low - 0xDC00);

		return 0x10000 + (highValue << 10) + lowValue;
	}

	bool isUnicodeScalar(char32_t value) {
		if (value > 0x10FFFF)
			return false;

		if (value >= 0xD800 && value <= 0xDFFF)
			return false;

		return true;
	}

	bool isTextCodePoint(char32_t value) {
		if (!isUnicodeScalar(value))
			return false;

		if (value < 0x20)
			return false;

		if (value == 0x7F)
			return false;

		return true;
	}

	std::wstring readCompositionString(HIMC context, DWORD index) {
		LONG byteLength = ImmGetCompositionStringW(context, index, nullptr, 0);
		if (byteLength <= 0)
			return std::wstring();

		std::size_t characterCount = static_cast<std::size_t>(byteLength) / sizeof(wchar_t);

		std::wstring result(characterCount, L'\0');

		LONG copiedBytes = ImmGetCompositionStringW(context, index, result.data(), byteLength);
		if (copiedBytes <= 0)
			return std::wstring();

		result.resize(static_cast<std::size_t>(copiedBytes) / sizeof(wchar_t));

		return result;
	}

	std::optional<std::size_t> readCompositionCaret(HIMC context) {
		LONG position = ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0);
		if (position < 0)
			return std::nullopt;

		return static_cast<std::size_t>(position);
	}
}

namespace tiny {
	constexpr UINT_PTR FrameTimerId = 1;
	constexpr UINT FrameTimerIntervalMilliseconds = 16;

	class Window::Impl {
	public:
		explicit Impl(Window& owner, const WindowCreateInfo& createInfo);
		~Impl();

		void show();
		void hide();
		void close();

		void minimize();
		void toggleMaximize();

		bool isMaximized() const;

		WindowCaptionButton captionButtonAt(const Point& position) const;

		void requestRepaint();

		void capturePointer();
		void releasePointerCapture();

		bool hasPointerCapture() const;

		void setTitle(const std::wstring& title);

		Size clientSize() const;
		float dpiScale() const;

		NativeWindowHandle nativeHandle() const;

		bool isVisible() const;
		bool isClosed() const;

		bool frameUpdatesEnabledValue = false;

		std::chrono::steady_clock::time_point lastFrameTime;

	private:
		static LRESULT CALLBACK windowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		LRESULT handleMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

		static void ensureWindowClassRegistered();

		void beginMouseLeaveTracking();

		std::u32string decodeTextInput(char16_t codeUnit);

		LRESULT hitTestCustomFrame(HWND windowHandle, LPARAM lParam) const;

	private:
		Window& owner;

		HWND handle = nullptr;

		bool closed = false;
		bool countedAsOpen = false;
		bool mouseLeaveTracking = false;

		char16_t pendingHighSurrogate = 0;

		bool customTitleBar = false;
		bool resizable = true;

		float titleBarHeight = 40.0f;
	};

	void Window::Impl::ensureWindowClassRegistered() {
		static bool registered = false;
		if (registered)
			return;

		HINSTANCE instance = GetModuleHandleW(nullptr);

		WNDCLASSEXW windowClass { };
		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = &Window::Impl::windowProcedure;
		windowClass.hInstance = instance;
		windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		windowClass.lpszClassName = WINDOW_CLASS_NAME;

		ATOM atom = RegisterClassExW(&windowClass);
		if (atom == 0)
			throw std::runtime_error("Failed to register Tiny window class.");

		registered = true;
	}

	Window::Impl::Impl(Window& owner, const WindowCreateInfo& createInfo) : owner(owner) {
		customTitleBar = createInfo.customTitleBar;
		resizable = createInfo.resizable;
		titleBarHeight = createInfo.titleBarHeight;

		ensureWindowClassRegistered();

		DWORD style = WS_OVERLAPPEDWINDOW;
		if (!createInfo.resizable) {
			style &= ~WS_THICKFRAME;
			style &= ~WS_MAXIMIZEBOX;
		}

		RECT windowRect { 0, 0, createInfo.width, createInfo.height };
		AdjustWindowRectEx(&windowRect, style, FALSE, 0);

		int width = windowRect.right - windowRect.left;
		int height = windowRect.bottom - windowRect.top;
		handle = CreateWindowExW(
			0,
			WINDOW_CLASS_NAME,
			createInfo.title.c_str(),
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			width,
			height,
			nullptr,
			nullptr,
			GetModuleHandleW(nullptr),
			this
		);

		if (!handle)
			throw std::runtime_error("Failed to create Tiny window.");

		detail::notifyWindowCreated();
		countedAsOpen = true;
	}

	LRESULT CALLBACK Window::Impl::windowProcedure(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) {
		Impl* impl = nullptr;
		if (message == WM_NCCREATE) {
			CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			impl = static_cast<Impl*>(createStruct->lpCreateParams);
			SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));

			impl->handle = windowHandle;
		} else
			impl = reinterpret_cast<Impl*>(GetWindowLongPtrW(windowHandle, GWLP_USERDATA));

		if (impl)
			return impl->handleMessage(windowHandle, message, wParam, lParam);
		else
			return DefWindowProcW(windowHandle, message, wParam, lParam);
	}

	LRESULT Window::Impl::handleMessage(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
			case WM_NCCALCSIZE: {
				if (!customTitleBar)
					break;

				if (wParam == TRUE) {
					NCCALCSIZE_PARAMS* parameters = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
					if (IsZoomed(windowHandle)) {
						HMONITOR monitor = MonitorFromWindow(windowHandle, MONITOR_DEFAULTTONEAREST);

						MONITORINFO monitorInfo { };
						monitorInfo.cbSize = sizeof(MONITORINFO);

						if (GetMonitorInfoW(monitor, &monitorInfo))
							parameters->rgrc[0] = monitorInfo.rcWork;
					}
				}

				return 0;
			}

			case WM_NCHITTEST: {
				if (!customTitleBar)
					break;

				return hitTestCustomFrame(windowHandle, lParam);
			}

			case WM_CLOSE: {
				WindowClosingEvent event;
				owner.closing.emit(event);

				if (!event.cancel)
					DestroyWindow(windowHandle);

				return 0;
			}

			case WM_MOUSEMOVE: {
				beginMouseLeaveTracking();

				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::None);
				owner.pointerMoved.emit(event);

				return 0;
			}

			case WM_LBUTTONDOWN: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Left);
				owner.pointerPressed.emit(event);

				return 0;
			}

			case WM_LBUTTONUP: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Left);
				owner.pointerReleased.emit(event);

				return 0;
			}

			case WM_RBUTTONDOWN: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Right);
				owner.pointerPressed.emit(event);

				return 0;
			}

			case WM_RBUTTONUP: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Right);
				owner.pointerReleased.emit(event);

				return 0;
			}

			case WM_MBUTTONDOWN: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Middle);
				owner.pointerPressed.emit(event);

				return 0;
			}

			case WM_MBUTTONUP: {
				PointerEvent event = createPointerEvent(handle, lParam, wParam, PointerButton::Middle);
				owner.pointerReleased.emit(event);

				return 0;
			}

			case WM_XBUTTONDOWN: {
				WORD nativeButton = HIWORD(wParam);

				PointerButton button = nativeButton == XBUTTON1 ? PointerButton::X1 : PointerButton::X2;
				PointerEvent event = createPointerEvent(handle, lParam, wParam, button);
				owner.pointerPressed.emit(event);

				return TRUE;
			}

			case WM_XBUTTONUP: {
				WORD nativeButton = HIWORD(wParam);

				PointerButton button = nativeButton == XBUTTON1 ? PointerButton::X1 : PointerButton::X2;
				PointerEvent event = createPointerEvent(handle, lParam, wParam, button);
				owner.pointerReleased.emit(event);

				return TRUE;
			}

			case WM_MOUSELEAVE:
				mouseLeaveTracking = false;

				owner.pointerExited.emit();

				return 0;

			case WM_KEYDOWN:
			case WM_SYSKEYDOWN: {
				KeyEvent event = createKeyEvent(wParam, lParam);
				owner.keyPressed.emit(event);

				if (event.handled)
					return 0;

				break;
			}

			case WM_KEYUP:
			case WM_SYSKEYUP: {
				KeyEvent event = createKeyEvent(wParam, lParam);
				owner.keyReleased.emit(event);

				if (event.handled)
					return 0;

				break;
			}

			case WM_CHAR: {
				char16_t codeUnit = static_cast<char16_t>(wParam);

				std::u32string decodedText = decodeTextInput(codeUnit);
				if (decodedText.empty())
					return 0;

				std::u32string text;
				for (char32_t codePoint : decodedText) {
					if (!isTextCodePoint(codePoint))
						continue;

					text.push_back(codePoint);
				}

				if (text.empty())
					return 0;

				TextInputEvent event;
				event.text = std::move(text);

				owner.textInput.emit(event);
				if (event.handled)
					return 0;

				break;
			}

			case WM_UNICHAR: {
				if (wParam == UNICODE_NOCHAR)
					return TRUE;

				char32_t codePoint = static_cast<char32_t>(wParam);
				if (!isTextCodePoint(codePoint))
					return 0;

				TextInputEvent event;
				event.text.push_back(codePoint);

				owner.textInput.emit(event);
				if (event.handled)
					return 0;

				break;
			}

			case WM_IME_STARTCOMPOSITION: {
				TextCompositionEvent event;
				event.type = TextCompositionEventType::Started;

				owner.textComposition.emit(event);

				break;
			}

			case WM_IME_COMPOSITION: {
				bool hasResult = (lParam & GCS_RESULTSTR) != 0;
				bool hasCompositionUpdate = (lParam & (GCS_COMPSTR | GCS_CURSORPOS)) != 0;
				if (!hasResult && !hasCompositionUpdate) {
					TextCompositionEvent event;
					event.type = TextCompositionEventType::Ended;

					owner.textComposition.emit(event);

					return 0;
				}

				HIMC context = ImmGetContext(handle);
				if (!context)
					break;

				std::wstring resultText;
				std::wstring compositionText;

				std::optional<std::size_t> compositionCaret;
				if (hasResult)
					resultText = readCompositionString(context, GCS_RESULTSTR);

				if (hasCompositionUpdate) {
					compositionText = readCompositionString(context, GCS_COMPSTR);
					compositionCaret = readCompositionCaret(context);
				}

				ImmReleaseContext(handle, context);
				if (hasResult && !resultText.empty()) {
					TextInputEvent inputEvent;
					inputEvent.text = wideToUtf32(resultText);

					owner.textInput.emit(inputEvent);
				}

				if (hasCompositionUpdate) {
					TextCompositionEvent compositionEvent;
					compositionEvent.type = TextCompositionEventType::Updated;
					compositionEvent.text = wideToUtf32(compositionText);
					compositionEvent.caretIndex = compositionCaret;

					owner.textComposition.emit(compositionEvent);
				}

				return 0;
			}

			case WM_IME_ENDCOMPOSITION: {
				TextCompositionEvent event;
				event.type = TextCompositionEventType::Ended;

				owner.textComposition.emit(event);

				break;
			}

			case WM_IME_SETCONTEXT: {
				if (wParam != FALSE)
					lParam &= ~static_cast<LPARAM>(ISC_SHOWUICOMPOSITIONWINDOW);

				return DefWindowProcW(handle, message, wParam, lParam);
			}

			case WM_TIMER: {
				if (wParam != FrameTimerId || !frameUpdatesEnabledValue)
					break;

				std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

				FrameEvent event;
				event.now = now;
				event.delta = now - lastFrameTime;

				lastFrameTime = now;

				owner.frame.emit(event);

				return 0;
			}

			case WM_KILLFOCUS:
				pendingHighSurrogate = 0;
				break;

			case WM_CAPTURECHANGED:
				owner.pointerCaptureLost.emit();
				return 0;

			case WM_PAINT: {
				PAINTSTRUCT paint { };
				
				BeginPaint(windowHandle, &paint);

				owner.paintRequested.emit();

				EndPaint(windowHandle, &paint);

				return 0;
			}

			case WM_SIZE: {
				RECT clientRect { };
				GetClientRect(windowHandle, &clientRect);

				Size size(static_cast<float>(clientRect.right - clientRect.left), static_cast<float>(clientRect.bottom - clientRect.top));
				owner.resized.emit(size);

				return 0;
			}

			case WM_DPICHANGED: {
				UINT dpi = HIWORD(wParam);

				float scale = static_cast<float>(dpi) / 96.0f;
				owner.dpiChanged.emit(scale);

				const RECT* suggestedRect = reinterpret_cast<const RECT*>(lParam);
				SetWindowPos(windowHandle, nullptr, suggestedRect->left, suggestedRect->top, suggestedRect->right - suggestedRect->left, suggestedRect->bottom - suggestedRect->top, SWP_NOZORDER | SWP_NOACTIVATE);

				return 0;
			}

			case WM_DESTROY:
				closed = true;
				owner.closed.emit();

				if (countedAsOpen) {
					detail::notifyWindowDestroyed();
					countedAsOpen = false;
				}

				return 0;

			case WM_NCDESTROY:
				SetWindowLongPtrW(windowHandle, GWLP_USERDATA, 0);
				handle = nullptr;
				break;
		}

		return DefWindowProcW(windowHandle, message, wParam, lParam);
	}

	void Window::Impl::beginMouseLeaveTracking() {
		if (!handle)
			return;

		if (mouseLeaveTracking)
			return;

		TRACKMOUSEEVENT tracking { };
		tracking.cbSize = sizeof(TRACKMOUSEEVENT);
		tracking.dwFlags = TME_LEAVE;
		tracking.hwndTrack = handle;

		if (TrackMouseEvent(&tracking) != FALSE)
			mouseLeaveTracking = true;
	}

	std::u32string Window::Impl::decodeTextInput(char16_t codeUnit) {
		std::u32string result;
		if (isHighSurrogate(codeUnit)) {
			if (pendingHighSurrogate != 0)
				result.push_back(U'\uFFFD');

			pendingHighSurrogate = codeUnit;

			return result;
		}

		if (isLowSurrogate(codeUnit)) {
			if (pendingHighSurrogate == 0) {
				result.push_back(U'\uFFFD');

				return result;
			}

			char32_t codePoint = combineSurrogates(pendingHighSurrogate, codeUnit);

			pendingHighSurrogate = 0;

			result.push_back(codePoint);

			return result;
		}

		if (pendingHighSurrogate != 0) {
			result.push_back(U'\uFFFD');

			pendingHighSurrogate = 0;
		}

		result.push_back(static_cast<char32_t>(codeUnit));

		return result;
	}

	LRESULT Window::Impl::hitTestCustomFrame(HWND windowHandle, LPARAM lParam) const {
		POINT screenPoint { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		POINT clientPoint = screenPoint;
		if (!ScreenToClient(windowHandle, &clientPoint))
			return HTCLIENT;

		RECT windowRect { };
		if (!GetWindowRect(windowHandle, &windowRect))
			return HTCLIENT;

		UINT dpi = GetDpiForWindow(windowHandle);

		float scale = static_cast<float>(dpi) / 96.0f;
		if (scale <= 0.0f)
			scale = 1.0f;

		int captionHeight = static_cast<int>(titleBarHeight * scale);
		if (resizable && !IsZoomed(windowHandle)) {
			int horizontalBorder = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
			int verticalBorder = GetSystemMetricsForDpi(SM_CYSIZEFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

			bool left = screenPoint.x < windowRect.left + horizontalBorder;
			bool right = screenPoint.x >= windowRect.right - horizontalBorder;
			bool top = screenPoint.y < windowRect.top + verticalBorder;
			bool bottom = screenPoint.y >= windowRect.bottom - verticalBorder;

			if (top && left)
				return HTTOPLEFT;

			if (top && right)
				return HTTOPRIGHT;

			if (bottom && left)
				return HTBOTTOMLEFT;

			if (bottom && right)
				return HTBOTTOMRIGHT;

			if (left)
				return HTLEFT;

			if (right)
				return HTRIGHT;

			if (top)
				return HTTOP;

			if (bottom)
				return HTBOTTOM;
		}

		RECT clientRect { };
		if (!GetClientRect(windowHandle, &clientRect))
			return HTCLIENT;

		bool insideCaption = clientPoint.x >= clientRect.left && clientPoint.x < clientRect.right && clientPoint.y >= clientRect.top && clientPoint.y < captionHeight;
		if (insideCaption) {
			float scale = dpiScale();
			if (scale <= 0.0f)
				scale = 1.0f;

			Point logicalPoint(static_cast<float>(clientPoint.x) / scale, static_cast<float>(clientPoint.y) / scale);

			WindowCaptionButton button = captionButtonAt(logicalPoint);
			if (button != WindowCaptionButton::None)
				return HTCLIENT;

			return HTCAPTION;
		}

		return HTCLIENT;
	}

	Window::Window(const WindowCreateInfo& createInfo) : impl(std::make_unique<Impl>(*this, createInfo)) { }

	Window::~Window() = default;

	void Window::show() {
		impl->show();
	}

	void Window::hide() {
		impl->hide();
	}

	void Window::close() {
		impl->close();
	}


	void Window::minimize() {
		impl->minimize();
	}

	void Window::toggleMaximize() {
		impl->toggleMaximize();
	}

	bool Window::isMaximized() const {
		return impl->isMaximized();
	}

	WindowCaptionButton Window::captionButtonAt(const Point& position) const {
		return impl->captionButtonAt(position);
	}

	void Window::requestRepaint() {
		impl->requestRepaint();
	}

	void Window::capturePointer() {
		impl->capturePointer();
	}

	void Window::releasePointerCapture() {
		impl->releasePointerCapture();
	}

	bool Window::hasPointerCapture() const {
		return impl->hasPointerCapture();
	}

	void Window::setTitle(const std::wstring& title) {
		impl->setTitle(title);
	}

	Size Window::clientSize() const {
		return impl->clientSize();
	}

	float Window::dpiScale() const {
		return impl->dpiScale();
	}

	NativeWindowHandle Window::nativeHandle() const {
		return impl->nativeHandle();
	}

	bool Window::isVisible() const {
		return impl->isVisible();
	}

	bool Window::isClosed() const {
		return impl->isClosed();
	}

	void Window::setFrameUpdatesEnabled(bool enabled) {
		if (impl->frameUpdatesEnabledValue == enabled)
			return;

		if (enabled) {
			impl->lastFrameTime = std::chrono::steady_clock::now();
			UINT_PTR result = SetTimer(static_cast<HWND>(nativeHandle()), FrameTimerId, FrameTimerIntervalMilliseconds, nullptr);
			if (result == 0)
				return;

			impl->frameUpdatesEnabledValue = true;

			return;
		}

		KillTimer(static_cast<HWND>(nativeHandle()), FrameTimerId);
		
		impl->frameUpdatesEnabledValue = false;
	}

	bool Window::frameUpdatesEnabled() const {
		return impl->frameUpdatesEnabledValue;
	}

	void Window::Impl::show() {
		if (!handle)
			return;

		ShowWindow(handle, SW_SHOW);
		UpdateWindow(handle);
	}

	void Window::Impl::hide() {
		if (!handle)
			return;

		ShowWindow(handle, SW_HIDE);
	}

	void Window::Impl::close() {
		if (!handle || closed)
			return;

		SendMessageW(handle, WM_CLOSE, 0, 0);
	}

	void Window::Impl::minimize() {
		if (!handle || closed)
			return;

		ShowWindow(handle, SW_MINIMIZE);
	}

	void Window::Impl::toggleMaximize() {
		if (!handle || closed)
			return;

		if (IsZoomed(handle))
			ShowWindow(handle, SW_RESTORE);
		else
			ShowWindow(handle, SW_MAXIMIZE);
	}

	bool Window::Impl::isMaximized() const {
		if (!handle || closed)
			return false;

		return IsZoomed(handle) != FALSE;
	}

	WindowCaptionButton Window::Impl::captionButtonAt(const Point& position) const {
		if (!customTitleBar || !handle || closed)
			return WindowCaptionButton::None;

		float scale = dpiScale();
		if (scale <= 0.0f)
			scale = 1.0f;

		float clientWidth = clientSize().width / scale;
		float totalButtonWidth = WindowCaptionButtonWidth * 3.0f;

		if (clientWidth < totalButtonWidth)
			return WindowCaptionButton::None;

		if (position.y < 0.0f || position.y >= titleBarHeight)
			return WindowCaptionButton::None;

		float buttonsStart = clientWidth - totalButtonWidth;
		if (position.x < buttonsStart || position.x >= clientWidth)
			return WindowCaptionButton::None;

		float localX = position.x - buttonsStart;
		if (localX < WindowCaptionButtonWidth)
			return WindowCaptionButton::Minimize;

		if (localX < WindowCaptionButtonWidth * 2.0f)
			return WindowCaptionButton::Maximize;

		return WindowCaptionButton::Close;
	}

	void Window::Impl::requestRepaint() {
		if (!handle)
			return;

		InvalidateRect(handle, nullptr, FALSE);
	}

	void Window::Impl::capturePointer() {
		if (!handle)
			return;

		SetCapture(handle);
	}

	void Window::Impl::releasePointerCapture() {
		if (!handle)
			return;

		if (GetCapture() != handle)
			return;

		ReleaseCapture();
	}

	bool Window::Impl::hasPointerCapture() const {
		if (!handle)
			return false;

		return GetCapture() == handle;
	}

	void Window::Impl::setTitle(const std::wstring& title) {
		if (!handle)
			return;

		SetWindowTextW(handle, title.c_str());
	}

	Size Window::Impl::clientSize() const {
		if (!handle)
			return Size();

		RECT rect { };
		GetClientRect(handle, &rect);

		return Size(static_cast<float>(rect.right - rect.left), static_cast<float>(rect.bottom - rect.top));
	}

	float Window::Impl::dpiScale() const {
		if (!handle)
			return 1.0f;

		UINT dpi = GetDpiForWindow(handle);

		return static_cast<float>(dpi) / 96.0f;
	}

	NativeWindowHandle Window::Impl::nativeHandle() const {
		return reinterpret_cast<NativeWindowHandle>(handle);
	}

	bool Window::Impl::isVisible() const {
		if (!handle)
			return false;

		return IsWindowVisible(handle) != FALSE;
	}

	bool Window::Impl::isClosed() const {
		return closed;
	}

	Window::Impl::~Impl() {
		if (handle && IsWindow(handle))
			DestroyWindow(handle);
	}
}