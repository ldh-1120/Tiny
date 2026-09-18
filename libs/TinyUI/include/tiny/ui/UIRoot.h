#pragma once

#include <memory>

#include <tiny/core/Event.h>
#include <tiny/core/Size.h>

#include <tiny/core/input/Pointer.h>
#include <tiny/core/input/Keyboard.h>
#include <tiny/core/input/TextInput.h>
#include <tiny/core/input/TextComposition.h>

#include <tiny/ui/FocusManager.h>
#include <tiny/ui/UIBuilder.h>

namespace tiny {
	class Canvas;
	class Element;
	class Widget;
	class GraphicsContext;
	class Clipboard;
	class TextInputContext;

	class UIRoot {
	public:
		UIRoot();
		~UIRoot();

		UIRoot(const UIRoot&) = delete;
		UIRoot& operator=(const UIRoot&) = delete;

		UIRoot(UIRoot&&) = delete;
		UIRoot& operator=(UIRoot&&) = delete;

		void setWidget(std::unique_ptr<Widget> widget);
		void setBuilder(UIBuilder builder);

		void requestRebuild();

		void layout(GraphicsContext& graphicsContext, const Size& size);
		void paint(Canvas& canvas);

		void pointerMoved(const PointerEvent& event);
		bool pointerPressed(const PointerEvent& event);
		void pointerReleased(const PointerEvent& event);

		void pointerExited();
		void pointerCaptureLost();

		bool empty() const;

		Event<> repaintRequested;

		bool keyPressed(const KeyEvent& event);
		bool keyReleased(const KeyEvent& event);

		bool textInput(const TextInputEvent& event);
		bool textComposition(const TextCompositionEvent& event);

		void setClipboard(Clipboard& clipboard);
		void setTextInputContext(TextInputContext& context);

	private:
		Element* hitTest(const Point& position);

		void updateHoveredElement(Element* element, const PointerEvent& event);

		void clearPointerState();

		void invalidatePaint();
		void invalidateLayout();

		bool requestFocus(Element* element);
		bool moveFocus(bool forward);

		void elementWillUnmount(Element* element);

		void reconcileRootWidget(std::unique_ptr<Widget> widget);

		void rebuildIfNeeded();

		Clipboard* clipboardService();
		TextInputContext* textInputContextService();

	private:
		std::unique_ptr<Widget> rootWidget;
		std::unique_ptr<Element> rootElement;

		UIBuilder uiBuilder;

		bool rebuildDirty = false;

		FocusManager focusManager;

		Clipboard* clipboardValue = nullptr;
		TextInputContext* textInputContextValue = nullptr;

		Element* hoveredElement = nullptr;
		Element* capturedElement = nullptr;

		bool layoutDirty = true;
		bool hasLayoutSize = false;

		Size lastLayoutSize;

		friend class Element;
	};
}