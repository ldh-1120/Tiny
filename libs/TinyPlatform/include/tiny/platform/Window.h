#pragma once

#include <memory>
#include <string>

#include <tiny/core/Event.h>
#include <tiny/core/Size.h>

#include <tiny/core/input/Pointer.h>
#include <tiny/core/input/Keyboard.h>
#include <tiny/core/input/TextInput.h>
#include <tiny/core/input/TextComposition.h>

#include <tiny/platform/NativeWindowHandle.h>
#include <tiny/platform/WindowEvents.h>

namespace tiny {
	struct WindowCreateInfo {
		std::wstring title = L"Tiny";
		int width = 1280;
		int height = 720;
		bool resizable = true;
	};

	class Window {
	public:
		explicit Window(const WindowCreateInfo& createInfo);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		Window(Window&& other) = delete;
		Window& operator=(Window&& other) = delete;

		void show();
		void hide();
		void close();

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

		Event<> paintRequested;
		Event<const Size&> resized;
		Event<float> dpiChanged;

		Event<const PointerEvent&> pointerMoved;
		Event<const PointerEvent&> pointerPressed;
		Event<const PointerEvent&> pointerReleased;

		Event<> pointerExited;
		Event<> pointerCaptureLost;

		Event<KeyEvent&> keyPressed;
		Event<KeyEvent&> keyReleased;

		Event<TextInputEvent&> textInput;

		Event<TextCompositionEvent&> textComposition;

		Event<WindowClosingEvent&> closing;
		Event<> closed;

	private:
		class Impl;

		std::unique_ptr<Impl> impl;
	};
}