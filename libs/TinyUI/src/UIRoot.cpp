#include <tiny/ui/UIRoot.h>

#include <utility>
#include <cstddef>
#include <vector>

#include <tiny/core/Rect.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/Widget.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>

namespace tiny {
	UIRoot::UIRoot() = default;
	UIRoot::~UIRoot() {
		clearPointerState();

		if (rootElement)
			rootElement->unmount();
	}

	void UIRoot::setWidget(std::unique_ptr<Widget> widget) {
		uiBuilder = UIBuilder();
		rebuildDirty = false;

		reconcileRootWidget(std::move(widget));

		invalidateLayout();
	}

	void UIRoot::setBuilder(UIBuilder builder) {
		uiBuilder = std::move(builder);

		rebuildDirty = true;

		invalidateLayout();
	}

	void UIRoot::requestRebuild() {
		if (!uiBuilder.valid())
			return;

		if (rebuildDirty)
			return;

		rebuildDirty = true;

		invalidateLayout();
	}

	void UIRoot::layout(GraphicsContext& graphicsContext, const Size& size) {
		rebuildIfNeeded();

		Size safeSize(size.width > 0.0f ? size.width : 0.0f, size.height > 0.0f ? size.height : 0.0f);
		if (!layoutDirty && hasLayoutSize && lastLayoutSize == safeSize)
			return;

		lastLayoutSize = safeSize;
		hasLayoutSize = true;

		if (!rootElement) {
			layoutDirty = true;
			return;
		}

		Constraints constraints = Constraints::tight(safeSize);

		LayoutContext context(graphicsContext);
		rootElement->measure(context, constraints);
		rootElement->arrange(Rect(0.0f, 0.0f, safeSize.width, safeSize.height));

		layoutDirty = false;
	}

	void UIRoot::paint(Canvas& canvas) {
		if (!rootElement)
			return;

		rootElement->paint(canvas);
	}

	bool UIRoot::empty() const {
		return rootElement == nullptr;
	}

	bool UIRoot::keyPressed(const KeyEvent& event) {
		if (event.key == KeyCode::Tab && !event.modifiers.control && !event.modifiers.alt && !event.repeated)
			return moveFocus(!event.modifiers.shift);

		Element* focused = focusManager.focusedElement();
		if (!focused)
			return false;

		return focused->keyDown(event);
	}

	bool UIRoot::keyReleased(const KeyEvent& event) {
		if (event.key == KeyCode::Tab && !event.modifiers.control && !event.modifiers.alt)
			return focusManager.focusedElement() != nullptr;

		Element* focused = focusManager.focusedElement();
		if (!focused)
			return false;

		return focused->keyUp(event);
	}

	bool UIRoot::textInput(const TextInputEvent& event) {
		Element* focused = focusManager.focusedElement();
		if (!focused)
			return false;

		return focused->textInput(event);
	}

	bool UIRoot::textComposition(const TextCompositionEvent& event) {
		Element* focused = focusManager.focusedElement();
		if (!focused)
			return false;

		return focused->textComposition(event);
	}

	void UIRoot::setClipboard(Clipboard& clipboard) {
		clipboardValue = &clipboard;
	}

	void UIRoot::setTextInputContext(TextInputContext& context) {
		textInputContextValue = &context;
	}

	void UIRoot::onFrame(const FrameEvent& event) {
		std::vector<Element*> snapshot(frameElements.begin(), frameElements.end());
		for (Element* element : snapshot) {
			if (!frameElements.contains(element))
				continue;

			element->dispatchFrame(event);
		}
	}

	bool UIRoot::needsFrameUpdates() const {
		return !frameElements.empty();
	}

	bool UIRoot::focusVisibility() const {
		return focusVisibilityValue;
	}

	bool UIRoot::isInteractiveAt(const Point& position) {
		if (!rootElement || !hasLayoutSize || layoutDirty)
			return false;

		return rootElement->hitTest(position) != nullptr;
	}

	Element* UIRoot::hitTest(const Point& position) {
		if (!rootElement)
			return nullptr;

		return rootElement->hitTest(position);
	}

	void UIRoot::updateHoveredElement(Element* element, const PointerEvent& event) {
		if (hoveredElement == element)
			return;

		if (hoveredElement)
			hoveredElement->pointerLeave();

		hoveredElement = element;
		if (hoveredElement)
			hoveredElement->pointerEnter(event);
	}

	void UIRoot::pointerMoved(const PointerEvent& event) {
		Element* target = hitTest(event.position);
		updateHoveredElement(target, event);

		Element* deliveryTarget = capturedElement ? capturedElement : hoveredElement;
		if (!deliveryTarget)
			return;

		deliveryTarget->pointerMove(event);
	}

	bool UIRoot::pointerPressed(const PointerEvent& event) {
		Element* target = hitTest(event.position);
		updateHoveredElement(target, event);

		requestFocus(target, FocusReason::Pointer);

		if (!target) {
			if (event.button == PointerButton::Left)
				focusManager.clearFocus();

			return false;
		}

		if (capturedElement && capturedElement != target) {
			capturedElement->pointerCancel();
			capturedElement = nullptr;
		}

		bool handled = target->pointerDown(event);
		if (!handled)
			return false;

		capturedElement = target;

		return true;
	}

	void UIRoot::pointerReleased(const PointerEvent& event) {
		Element* target = hitTest(event.position);
		updateHoveredElement(target, event);

		Element* deliveryTarget = capturedElement ? capturedElement : target;
		if (deliveryTarget)
			deliveryTarget->pointerUp(event);

		capturedElement = nullptr;
	}

	bool UIRoot::pointerWheel(const PointerWheelEvent& event) {
		Element* target = hitTest(event.position);
		if (!target)
			return false;

		return target->pointerWheel(event);
	}

	void UIRoot::pointerExited() {
		if (!hoveredElement)
			return;

		hoveredElement->pointerLeave();
		hoveredElement = nullptr;
	}

	void UIRoot::pointerCaptureLost() {
		if (!capturedElement)
			return;

		capturedElement->pointerCancel();
		capturedElement = nullptr;
	}

	void UIRoot::clearPointerState() {
		if (capturedElement) {
			capturedElement->pointerCancel();
			capturedElement = nullptr;
		}

		if (hoveredElement) {
			hoveredElement->pointerLeave();
			hoveredElement = nullptr;
		}
	}

	void UIRoot::invalidatePaint() {
		repaintRequested.emit();
	}

	void UIRoot::invalidateLayout() {
		layoutDirty = true;

		repaintRequested.emit();
	}

	bool UIRoot::requestFocus(Element* element, FocusReason reason) {
		applyFocusReason(reason);

		return focusManager.requestFocus(element);
	}

	bool UIRoot::moveFocus(bool forward) {
		applyFocusReason(FocusReason::Keyboard);

		if (!rootElement)
			return false;

		std::vector<Element*> focusableElements;
		rootElement->collectFocusableElements(focusableElements);

		if (focusableElements.empty())
			return false;

		Element* current = focusManager.focusedElement();

		std::size_t currentIndex = focusableElements.size();
		for (std::size_t index = 0; index < focusableElements.size(); ++index) {
			if (focusableElements[index] == current) {
				currentIndex = index;
				break;
			}
		}

		std::size_t targetIndex = 0;
		if (currentIndex >= focusableElements.size())
			targetIndex = forward ? 0 : focusableElements.size() - 1;
		else if (forward)
			targetIndex = (currentIndex + 1) % focusableElements.size();
		else
			targetIndex = currentIndex == 0 ? focusableElements.size() - 1 : currentIndex - 1;

		return focusManager.requestFocus(focusableElements[targetIndex]);
	}

	void UIRoot::elementWillUnmount(Element* element) {
		if (!element)
			return;

		if (capturedElement == element) {
			capturedElement->pointerCancel();
			capturedElement = nullptr;
		}

		if (hoveredElement == element) {
			hoveredElement->pointerLeave();
			hoveredElement = nullptr;
		}

		focusManager.elementWillUnmount(element);
	}

	void UIRoot::reconcileRootWidget(std::unique_ptr<Widget> widget) {
		if (!widget) {
			if (rootElement)
				rootElement->unmount();

			rootElement.reset();
			rootWidget.reset();

			return;
		}

		if (rootElement && rootElement->canUpdate(*widget)) {
			rootElement->update(*widget);

			rootWidget = std::move(widget);

			return;
		}

		if (rootElement)
			rootElement->unmount();

		rootElement.reset();

		rootElement = widget->createElement();
		if (rootElement)
			rootElement->mount(*this, nullptr);

		rootWidget = std::move(widget);
	}

	void UIRoot::rebuildIfNeeded() {
		if (!rebuildDirty)
			return;

		rebuildDirty = false;
		
		if (!uiBuilder.valid())
			return;

		std::unique_ptr<Widget> widget = uiBuilder.build();
		reconcileRootWidget(std::move(widget));
	}

	Clipboard* UIRoot::clipboardService() {
		return clipboardValue;
	}

	TextInputContext* UIRoot::textInputContextService() {
		return textInputContextValue;
	}

	void UIRoot::registerFrameElement(Element* element) {
		if (!element)
			return;

		if (frameElements.contains(element))
			return;

		bool wasEmpty = frameElements.empty();
		frameElements.insert(element);

		if (wasEmpty)
			frameDemandChanged.emit(true);
	}

	void UIRoot::unregisterFrameElement(Element* element) {
		std::unordered_set<Element*>::iterator iterator = frameElements.find(element);
		if (iterator == frameElements.end())
			return;

		frameElements.erase(iterator);
		if (frameElements.empty())
			frameDemandChanged.emit(false);
	}

	void UIRoot::setFocusVisibility(bool visible) {
		if (focusVisibilityValue == visible)
			return;

		focusVisibilityValue = visible;

		Element* focusedElement = focusManager.focusedElement();
		if (focusedElement)
			focusedElement->dispatchFocusVisibilityChanged(visible);
	}

	void UIRoot::applyFocusReason(FocusReason reason) {
		switch (reason)		{
			case tiny::FocusReason::Pointer:
				setFocusVisibility(false);
				break;

			case tiny::FocusReason::Keyboard:
				setFocusVisibility(true);
				break;

			case tiny::FocusReason::Programmatic:
				break;
		}
	}

	void UIRoot::elementBecameDisabled(Element* element) {
		if (!element)
			return;

		if (capturedElement == element) {
			capturedElement = nullptr;

			element->pointerCancel();
		}

		if (hoveredElement == element) {
			hoveredElement = nullptr;

			element->pointerLeave();
		}

		if (focusManager.focusedElement() == element)
			focusManager.clearFocus();
	}
}