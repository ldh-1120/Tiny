#pragma once

#include <typeindex>
#include <vector>

#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>
#include <tiny/core/FrameEvent.h>

#include <tiny/core/input/Pointer.h>
#include <tiny/core/input/Keyboard.h>
#include <tiny/core/input/TextInput.h>
#include <tiny/core/input/TextComposition.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	class Canvas;
	class Widget;
	class LayoutContext;
	class UIRoot;
	class FocusManager;
	class Clipboard;
	class TextInputContext;

	class Element {
	public:
		virtual ~Element() = default;

		Element(const Element&) = delete;
		Element& operator=(const Element&) = delete;

		Element(Element&&) = delete;
		Element& operator=(Element&&) = delete;

		bool canUpdate(const Widget& widget) const;

		void update(const Widget& widget);

		void mount(UIRoot& owner, Element* parent);
		void unmount();

		Element* parent();
		const Element* parent() const;

		bool isMounted() const;

		bool canReceiveFocus() const;
		bool hasFocus() const;

		bool requestFocus();

		bool keyDown(const KeyEvent& event);
		bool keyUp(const KeyEvent& event);

		bool textInput(const TextInputEvent& event);
		bool textComposition(const TextCompositionEvent& event);

		void collectFocusableElements(std::vector<Element*>& result);

		Size measure(LayoutContext& context, const Constraints& constraints);
		void arrange(const Rect& bounds);
		void paint(Canvas& canvas);

		Element* hitTest(const Point& position);

		void pointerEnter(const PointerEvent& event);
		void pointerLeave();

		void pointerMove(const PointerEvent& event);
		bool pointerDown(const PointerEvent& event);
		void pointerUp(const PointerEvent& event);

		void pointerCancel();

		const Size& desiredSize() const;
		const Rect& bounds() const;

	protected:
		explicit Element(const Widget& widget);

		void markNeedsPaint();
		void markNeedsLayout();

		UIRoot* ownerRoot();
		const UIRoot* ownerRoot() const;

		virtual void updateOverride(const Widget& widget) = 0;

		virtual void mountOverride();
		virtual void unmountOverride();

		virtual bool focusable() const;

		virtual void focusGainedOverride();
		virtual void focusLostOverride();

		virtual bool keyDownOverride(const KeyEvent& event);
		virtual bool keyUpOverride(const KeyEvent& event);

		virtual bool textInputOverride(const TextInputEvent& event);
		virtual bool textCompositionOverride(const TextCompositionEvent& event);

		virtual void collectFocusableChildren(std::vector<Element*>& result);

		virtual Size measureOverride(LayoutContext& context, const Constraints& constraints) = 0;
		virtual void arrangeOverride(const Rect& bounds);
		virtual void paintOverride(Canvas& canvas) = 0;

		virtual bool acceptsPointerEvents() const;

		virtual Element* hitTestChildren(const Point& position);

		virtual void pointerEnterOverride(const PointerEvent& event);
		virtual void pointerLeaveOverride();

		virtual void pointerMoveOverride(const PointerEvent& event);
		virtual bool pointerDownOverride(const PointerEvent& event);
		virtual void pointerUpOverride(const PointerEvent& event);

		virtual void pointerCancelOverride();

		Clipboard* clipboard();
		TextInputContext* textInputContext();

		void setFrameUpdatesEnabled(bool enabled);

		virtual void frameOverride(const FrameEvent& event);

		bool isFocusVisible() const;

		virtual void focusVisibilityChangedOverride(bool visible);

	private:
		void setFocused(bool value);

		void dispatchFrame(const FrameEvent& event);

		void dispatchFocusVisibilityChanged(bool visible);

		friend class UIRoot;

	private:
		std::type_index widgetType;
		Key widgetKey;

		UIRoot* rootOwner = nullptr;
		Element* parentElement = nullptr;

		bool mounted = false;
		bool focused = false;

		Size measuredSize;
		Rect arrangedBounds;

		bool frameUpdatesEnabledValue = false;

		friend class FocusManager;
	};
}