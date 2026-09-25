#include <tiny/ui/Element.h>

#include <typeinfo>

#include <tiny/graphics/Canvas.h>

#include <tiny/ui/Widget.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/UIRoot.h>

namespace tiny {
	Element::Element(const Widget& widget) : widgetType(typeid(widget)), widgetKey(widget.key()) { }

	bool Element::canUpdate(const Widget& widget) const {
		return widgetType == std::type_index(typeid(widget)) && widgetKey == widget.key();
	}

	void Element::update(const Widget& widget) {
		if (!canUpdate(widget))
			return;

		updateOverride(widget);
	}

	void Element::mount(UIRoot& owner, Element* parent) {
		if (mounted)
			return;

		rootOwner = &owner;
		parentElement = parent;
		mounted = true;

		if (frameUpdatesEnabledValue)
			rootOwner->registerFrameElement(this);

		mountOverride();
	}

	void Element::unmount() {
		if (!mounted)
			return;

		UIRoot* owner = rootOwner;
		if (owner)
			owner->elementWillUnmount(this);

		if (frameUpdatesEnabledValue && rootOwner)
			rootOwner->unregisterFrameElement(this);

		unmountOverride();

		focused = false;

		parentElement = nullptr;
		rootOwner = nullptr;
		mounted = false;
	}

	Element* Element::parent() {
		return parentElement;
	}

	const Element* Element::parent() const {
		return parentElement;
	}

	bool Element::isMounted() const {
		return mounted;
	}

	bool Element::canReceiveFocus() const {
		return mounted && enabledValue && focusable();
	}

	bool Element::hasFocus() const {
		return focused;
	}

	bool Element::isEnabled() const {
		return enabledValue;
	}

	bool Element::requestFocus() {
		if (!rootOwner)
			return false;

		return rootOwner->requestFocus(this, FocusReason::Programmatic);
	}

	bool Element::keyDown(const KeyEvent& event) {
		if (!mounted || !enabledValue || !focused)
			return false;

		return keyDownOverride(event);
	}

	bool Element::keyUp(const KeyEvent& event) {
		if (!mounted)
			return false;

		if (!focused)
			return false;

		return keyUpOverride(event);
	}

	bool Element::textInput(const TextInputEvent& event) {
		if (!mounted)
			return false;

		if (!focused)
			return false;

		return textInputOverride(event);
	}

	bool Element::textComposition(const TextCompositionEvent& event) {
		if (!mounted)
			return false;

		if (!focused)
			return false;

		return textCompositionOverride(event);
	}

	void Element::collectFocusableElements(std::vector<Element*>& result) {
		if (!mounted)
			return;

		if (canReceiveFocus())
			result.push_back(this);

		collectFocusableChildren(result);
	}

	Size Element::measure(LayoutContext& context, const Constraints& constraints) {
		measuredSize = constraints.constrain(measureOverride(context, constraints));
		return measuredSize;
	}

	void Element::arrange(const Rect& bounds) {
		arrangedBounds = bounds;
		arrangeOverride(bounds);
	}

	void Element::paint(Canvas& canvas) {
		paintOverride(canvas);
	}

	Element* Element::hitTest(const Point& position) {
		if (!mounted || !enabledValue)
			return nullptr;

		if (!arrangedBounds.contains(position))
			return nullptr;

		Element* childResult = hitTestChildren(position);
		if (childResult)
			return childResult;

		if (!hitTestSelf(position))
			return nullptr;

		return this;
	}

	void Element::pointerEnter(const PointerEvent& event) {
		if (!mounted || !enabledValue)
			return;

		pointerEnterOverride(event);
	}

	void Element::pointerLeave() {
		if (!mounted)
			return;

		pointerLeaveOverride();
	}

	void Element::pointerMove(const PointerEvent& event) {
		if (!mounted || !enabledValue)
			return;

		pointerMoveOverride(event);
	}

	bool Element::pointerDown(const PointerEvent& event) {
		if (!mounted || !enabledValue)
			return false;

		return pointerDownOverride(event);
	}

	void Element::pointerUp(const PointerEvent& event) {
		if (!mounted || !enabledValue)
			return;

		pointerUpOverride(event);
	}

	bool Element::pointerWheel(const PointerWheelEvent& event) {
		if (!mounted || !enabledValue)
			return false;

		return pointerWheelOverride(event);
	}

	void Element::pointerCancel() {
		if (!mounted)
			return;

		pointerCancelOverride();
	}

	PointerCursor Element::pointerCursor() const {
		if (!mounted || !enabledValue)
			return PointerCursor::Arrow;

		return pointerCursorOverride();
	}

	const Size& Element::desiredSize() const {
		return measuredSize;
	}

	const Rect& Element::bounds() const {
		return arrangedBounds;
	}

	void Element::markNeedsPaint() {
		if (!rootOwner)
			return;

		rootOwner->invalidatePaint();
	}

	void Element::markNeedsLayout() {
		if (!rootOwner)
			return;

		rootOwner->invalidateLayout();
	}

	UIRoot* Element::ownerRoot() {
		return rootOwner;
	}

	const UIRoot* Element::ownerRoot() const {
		return rootOwner;
	}

	void Element::mountOverride() { }

	void Element::unmountOverride() { }

	bool Element::focusable() const {
		return false;
	}

	void Element::focusGainedOverride() { }

	void Element::focusLostOverride() { }

	bool Element::keyDownOverride(const KeyEvent& event) {
		(void)event;

		return false;
	}

	bool Element::keyUpOverride(const KeyEvent& event) {
		(void)event;

		return false;
	}

	bool Element::textInputOverride(const TextInputEvent& event) {
		(void)event;

		return false;
	}

	bool Element::textCompositionOverride(const TextCompositionEvent& event) {
		(void)event;

		return false;
	}

	void Element::collectFocusableChildren(std::vector<Element*>& result) {
		(void)result;
	}

	void Element::arrangeOverride(const Rect& bounds) {
		(void)bounds;
	}

	bool Element::acceptsPointerEvents() const {
		return false;
	}

	bool Element::hitTestSelf(const Point& position) const {
		(void)position;

		return acceptsPointerEvents();
	}

	Element* Element::hitTestChildren(const Point& position) {
		(void)position;

		return nullptr;
	}

	void Element::pointerEnterOverride(const PointerEvent& event) {
		(void)event;
	}

	void Element::pointerLeaveOverride() { }

	void Element::pointerMoveOverride(const PointerEvent& event) {
		(void)event;
	}

	bool Element::pointerDownOverride(const PointerEvent& event) {
		(void)event;

		return false;
	}

	void Element::pointerUpOverride(const PointerEvent& event) {
		(void)event;
	}

	bool Element::pointerWheelOverride(const PointerWheelEvent& event) {
		(void)event;

		return false;
	}

	void Element::pointerCancelOverride() { }

	PointerCursor Element::pointerCursorOverride() const {
		return PointerCursor::Arrow;
	}

	Clipboard* Element::clipboard() {
		if (!rootOwner)
			return nullptr;

		return rootOwner->clipboardService();
	}

	TextInputContext* Element::textInputContext() {
		if (!rootOwner)
			return nullptr;

		return rootOwner->textInputContextService();
	}

	void Element::setFrameUpdatesEnabled(bool enabled) {
		if (frameUpdatesEnabledValue == enabled)
			return;

		frameUpdatesEnabledValue = enabled;

		if (!mounted)
			return;

		if (!rootOwner)
			return;

		if (enabled)
			rootOwner->registerFrameElement(this);
		else
			rootOwner->unregisterFrameElement(this);
	}

	void Element::frameOverride(const FrameEvent&) { }

	bool Element::isFocusVisible() const {
		return hasFocus() && rootOwner && rootOwner->focusVisibility();
	}

	void Element::focusVisibilityChangedOverride(bool) { }

	void Element::setEnabled(bool enabled) {
		if (enabledValue == enabled)
			return;

		enabledValue = enabled;
		if (!enabledValue && rootOwner)
			rootOwner->elementBecameDisabled(this);

		markNeedsPaint();
	}

	void Element::setFocused(bool value) {
		if (focused == value)
			return;

		focused = value;
		if (focused)
			focusGainedOverride();
		else
			focusLostOverride();

		markNeedsPaint();
	}

	void Element::dispatchFrame(const FrameEvent& event) {
		if (!mounted)
			return;

		if (!frameUpdatesEnabledValue)
			return;

		frameOverride(event);
	}

	void Element::dispatchFocusVisibilityChanged(bool visible) {
		if (!mounted)
			return;

		focusVisibilityChangedOverride(visible);
	}
}