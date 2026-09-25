#include <tiny/ui/widgets/TextBox.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <string_view>

#include <tiny/core/Point.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Size.h>

#include <tiny/core/services/Clipboard.h>
#include <tiny/core/services/TextInputContext.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/TextLayout.h>
#include <tiny/graphics/FontMetrics.h>

#include <tiny/ui/Element.h>
#include <tiny/ui/LayoutContext.h>
#include <tiny/ui/layout/Constraints.h>
#include <tiny/ui/animation/AnimationController.h>
#include <tiny/ui/visual/FocusRing.h>

#include <tiny/ui/text/TextEditingModel.h>
#include <tiny/ui/text/TextSelection.h>

namespace tiny {
	namespace {
		std::u32string sanitizeSingleLineText(std::u32string_view text) {
			std::u32string result;
			result.reserve(text.size());

			bool previousWasCarriageReturn = false;
			for (char32_t codePoint : text) {
				if (codePoint == U'\r') {
					result.push_back(U' ');

					previousWasCarriageReturn = true;
					continue;
				}

				if (codePoint == U'\n') {
					if (!previousWasCarriageReturn)
						result.push_back(U' ');

					previousWasCarriageReturn = false;
					continue;
				}

				previousWasCarriageReturn = false;
				if (codePoint == U'\t') {
					result.push_back(U' ');
					continue;
				}

				if (codePoint < 0x20)
					continue;

				if (codePoint == 0x7F)
					continue;

				result.push_back(codePoint);
			}

			return result;
		}

		class TextBoxElement final : public Element {
		public:
			explicit TextBoxElement(const TextBox& widget) : Element(widget), editingModel(widget.text()), changedCallback(widget.onChanged()), textBoxStyle(widget.style()) {
				setEnabled(widget.enabled());
			}

		protected:
			void updateOverride(const Widget& widget) override {
				const TextBox& textBox = static_cast<const TextBox&>(widget);

				bool textChangedExternally = editingModel.text() != textBox.text();

				changedCallback = textBox.onChanged();
				textBoxStyle = textBox.style();
				
				if (textChangedExternally && compositionActive) {
					compositionActive = false;
					compositionText.clear();
					compositionCaret = std::nullopt;
				}

				compositionStart = std::min(compositionStart, editingModel.text().size());

				setEnabled(textBox.enabled());

				if (!isEnabled()) {
					focusAnimation.setValue(0.0f);
					updateFrameDemand();
				}
			}

			bool focusable() const override {
				return true;
			}

			bool acceptsPointerEvents() const override {
				return true;
			}

			Size measureOverride(LayoutContext& context, const Constraints& constraints) override {
				GraphicsContext& graphicsContext = context.graphicsContext();

				fontMetrics = graphicsContext.getFontMetrics(textBoxStyle.textStyle);
				lineHeight = fontMetrics.lineHeight;
				if (lineHeight <= 0.0f)
					lineHeight = textBoxStyle.textStyle.fontSize;

				Size desiredSize(textBoxStyle.width, lineHeight + textBoxStyle.padding.vertical());
				Size measuredSize = constraints.constrain(desiredSize);

				float contentWidth = std::max(measuredSize.width - textBoxStyle.padding.horizontal(), 0.0f);

				std::u32string displayText = buildDisplayText();
				textLayout = graphicsContext.createTextLayout(displayText, textBoxStyle.textStyle, std::numeric_limits<float>::infinity());

				return measuredSize;
			}

			void arrangeOverride(const Rect& finalBounds) override {
				Rect contentBounds = getContentBounds();

				clampHorizontalScroll(contentBounds);

				ensureCaretVisible(contentBounds);
			}

			void paintOverride(Canvas& canvas) override {
				canvas.fillRect(bounds(), isEnabled() ? textBoxStyle.background : textBoxStyle.disabledBackground);
				canvas.drawRect(bounds(), isEnabled() ? textBoxStyle.borderColor : textBoxStyle.disabledBorderColor, textBoxStyle.borderWidth);

				Rect contentBounds = getContentBounds();
				ensureCaretVisible(contentBounds);

				Point textOrigin = getTextOrigin(contentBounds);

				canvas.pushClip(contentBounds);

				paintSelection(canvas, contentBounds);

				Point textDrawOrigin = getTextDrawOrigin(contentBounds);
				if (textLayout)
					canvas.drawTextLayout(*textLayout, textDrawOrigin, isEnabled() ? textBoxStyle.textColor : textBoxStyle.disabledTextColor);

				paintCompositionUnderline(canvas, contentBounds);

				if (hasFocus() && caretVisible) {
					std::size_t caretIndex = displayCaretIndex();

					TextPositionMetrics caretMetrics { };
					if (textLayout)
						caretMetrics = textLayout->hitTestTextPosition(caretIndex);

					Rect visualBounds = getTextVisualBounds(contentBounds);

					float caretX = textOrigin.x + caretMetrics.position.x;
					canvas.fillRect(Rect(caretX, visualBounds.y, textBoxStyle.caretWidth, visualBounds.height), textBoxStyle.caretColor);
				}

				canvas.popClip();

				paintFocusRing(canvas, bounds(), textBoxStyle.focusedBorderColor, textBoxStyle.focusedBorderWidth, focusAnimation.value());

				if (hasFocus())
					updateNativeCaretRect(contentBounds); //temporary
			}

			bool pointerDownOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return false;

				requestFocus();

				std::size_t newCaretIndex = hitTestCaret(event.position);
				bool selectionChanged = editingModel.moveCaretTo(newCaretIndex, event.modifiers.shift);
				if (selectionChanged)
					markNeedsPaint();

				pointerSelecting = true;

				return true;
			}

			void pointerMoveOverride(const PointerEvent& event) override {
				if (!pointerSelecting)
					return;

				std::size_t newCaretIndex = hitTestCaret(event.position);
				bool selectionChanged = editingModel.moveCaretTo(newCaretIndex, true);
				if (selectionChanged)
					markNeedsPaint();
			}

			void pointerUpOverride(const PointerEvent& event) override {
				if (event.button != PointerButton::Left)
					return;

				if (!pointerSelecting)
					return;

				std::size_t newCaretIndex = hitTestCaret(event.position);
				bool selectionChanged = editingModel.moveCaretTo(newCaretIndex, true);
				if (selectionChanged)
					markNeedsPaint();

				pointerSelecting = false;
			}

			void pointerCancelOverride() override {
				pointerSelecting = false;
			}

			PointerCursor pointerCursorOverride() const override {
				return PointerCursor::IBeam;
			}

			void focusGainedOverride() override {
				resetCaretBlink();
				updateFocusAnimation(isFocusVisible());
			}

			void focusLostOverride() override {
				caretVisible = false;
				caretBlinkElapsed = 0.0f;

				updateFocusAnimation(false);

				pointerSelecting = false;

				compositionActive = false;
				compositionText.clear();
				compositionCaret = 0;

				updateFrameDemand();
				markNeedsLayout();
			}

			void focusVisibilityChangedOverride(bool visible) override {
				if (!hasFocus())
					return;

				updateFocusAnimation(visible);
			}

			bool textInputOverride(const TextInputEvent& event) override {
				if (event.text.empty())
					return false;

				if (compositionActive) {
					editingModel.moveCaretTo(std::min(compositionStart, editingModel.text().size()), false);

					compositionActive = false;
					compositionText.clear();
					compositionCaret = std::nullopt;
				}

				insertText(event.text);

				return true;
			}

			bool textCompositionOverride(const TextCompositionEvent& event) override {
				switch (event.type) {
					case TextCompositionEventType::Started:
						beginComposition();
						return true;

					case TextCompositionEventType::Updated:
						updateComposition(event);
						return true;

					case TextCompositionEventType::Ended:
						endComposition();
						return true;
				}

				return false;
			}

			bool keyDownOverride(const KeyEvent& event) override {
				if (compositionActive && isCompositionEditingKey(event.key))
					return false;

				bool commandModifier = event.modifiers.control && !event.modifiers.alt;
				if (commandModifier) {
					switch (event.key) {
						case KeyCode::A:
							return selectAll();

						case KeyCode::C:
							return copySelection();

						case KeyCode::X:
							return cutSelection();

						case KeyCode::V:
							return pasteClipboard();

						default:
							break;
					}
				}

				switch (event.key) {
					case KeyCode::Backspace:
						return handleBackspace();

					case KeyCode::DeleteKey:
						return handleDelete();

					case KeyCode::Left:
						return moveCaretLeft(event.modifiers.shift);

					case KeyCode::Right:
						return moveCaretRight(event.modifiers.shift);

					case KeyCode::Home:
						return moveCaretHome(event.modifiers.shift);

					case KeyCode::End:
						return moveCaretEnd(event.modifiers.shift);

					default:
						return false;
				}
			}

			void frameOverride(const FrameEvent& event) override {
				bool needsPaint = false;
				if (hasFocus()) {
					caretBlinkElapsed += event.delta.count();
					while (caretBlinkElapsed >= CaretBlinkInterval) {
						caretBlinkElapsed -= CaretBlinkInterval;
						caretVisible = !caretVisible;

						needsPaint = true;
					}
				}

				if (focusAnimation.advance(event.delta.count()))
					needsPaint = true;

				if (needsPaint)
					markNeedsPaint();

				updateFrameDemand();
			}

		private:
			Rect getContentBounds() const {
				float width = std::max(bounds().width - textBoxStyle.padding.horizontal(), 0.0f);
				float height = std::max(bounds().height - textBoxStyle.padding.vertical(), 0.0f);

				return Rect(bounds().x + textBoxStyle.padding.left, bounds().y + textBoxStyle.padding.top, width, height);
			}

			std::size_t hitTestCaret(const Point& point) const {
				if (!textLayout)
					return 0;

				Rect contentBounds = getContentBounds();

				Point textOrigin = getTextOrigin(contentBounds);
				Point localPoint(point.x - textOrigin.x, point.y - textOrigin.y);
				TextHitTestResult result = textLayout->hitTestPoint(localPoint);

				std::size_t displayIndex = result.textPosition;

				return displayIndexToModelIndex(displayIndex);
			}

			std::size_t displayIndexToModelIndex(std::size_t displayIndex) const {
				if (!compositionActive)
					return std::min(displayIndex, editingModel.text().size());

				std::size_t compositionEnd = compositionStart + compositionText.size();
				if (displayIndex <= compositionStart)
					return displayIndex;

				if (displayIndex <= compositionEnd)
					return compositionStart;

				return std::min(displayIndex - compositionText.size(), editingModel.text().size());
			}

			void notifyTextChanged() {
				resetCaretBlink();
				markNeedsLayout();

				TextBox::ChangedCallback callback = changedCallback;
				if (callback)
					callback(editingModel.text());
			}

			bool handleBackspace() {
				const TextSelection& selection = editingModel.selection();

				if (!selection.isCollapsed()) {
					if (editingModel.eraseSelection())
						notifyTextChanged();

					return true;
				}

				if (selection.caret == 0)
					return true;

				std::size_t previous = 0;
				if (textLayout)
					previous = textLayout->previousCaretPosition(selection.caret);
				else
					previous = selection.caret - 1;

				if (editingModel.eraseRange(previous, selection.caret))
					notifyTextChanged();

				return true;
			}

			bool handleDelete() {
				const TextSelection& selection = editingModel.selection();

				if (!selection.isCollapsed()) {
					if (editingModel.eraseSelection())
						notifyTextChanged();

					return true;
				}

				if (selection.caret >= editingModel.text().size())
					return true;

				std::size_t next = editingModel.text().size();
				if (textLayout)
					next = textLayout->nextCaretPosition(selection.caret);
				else
					next = selection.caret + 1;

				if (editingModel.eraseRange(selection.caret, next))
					notifyTextChanged();

				return true;
			}

			bool moveCaretLeft(bool extendSelection) {
				resetCaretBlink();

				const TextSelection& selection = editingModel.selection();

				if (!extendSelection) {
					if (!selection.isCollapsed()) {
						if (editingModel.moveCaretTo(selection.start(), false))
							markNeedsPaint();

						return true;
					}
				}

				std::size_t target = 0;
				if (textLayout)
					target = textLayout->previousCaretPosition(selection.caret);
				else if (selection.caret > 0)
					target = selection.caret - 1;

				if (editingModel.moveCaretTo(target, extendSelection))
					markNeedsPaint();

				return true;
			}

			bool moveCaretRight(bool extendSelection) {
				resetCaretBlink();

				const TextSelection& selection = editingModel.selection();

				if (!extendSelection) {
					if (!selection.isCollapsed()) {
						if (editingModel.moveCaretTo(selection.end(), false))
							markNeedsPaint();

						return true;
					}
				}

				std::size_t target = editingModel.text().size();
				if (textLayout)
					target = textLayout->nextCaretPosition(selection.caret);
				else if (selection.caret < editingModel.text().size())
					target = selection.caret + 1;

				if (editingModel.moveCaretTo(target, extendSelection))
					markNeedsPaint();

				return true;
			}

			bool moveCaretHome(bool extendSelection) {
				resetCaretBlink();

				if (editingModel.moveHome(extendSelection))
					markNeedsPaint();

				return true;
			}

			bool moveCaretEnd(bool extendSelection) {
				resetCaretBlink();

				if (editingModel.moveEnd(extendSelection))
					markNeedsPaint();

				return true;
			}

			bool isCompositionEditingKey(KeyCode code) const {
				switch (code) {
					case KeyCode::Backspace:
					case KeyCode::DeleteKey:
					case KeyCode::Left:
					case KeyCode::Right:
					case KeyCode::Home:
					case KeyCode::End:
						return true;

					default:
						return false;
				}
			}

			void paintSelection(Canvas& canvas, const Rect& contentBounds) {
				const TextSelection& selection = editingModel.selection();
				if (selection.isCollapsed())
					return;

				if (!hasFocus())
					return;

				if (!textLayout)
					return;

				Point textOrigin = getTextOrigin(contentBounds);

				Rect visualBounds = getTextVisualBounds(contentBounds);

				std::vector<Rect> rectangles = textLayout->hitTestRange(selection.start(), selection.length());
				for (const Rect& rectangle : rectangles)
					canvas.fillRect(Rect(textOrigin.x + rectangle.x, visualBounds.y, rectangle.width, visualBounds.height), textBoxStyle.selectionColor);
			}

			bool selectAll() {
				if (editingModel.selectAll())
					markNeedsPaint();

				return true;
			}

			bool copySelection() {
				const TextSelection& selection = editingModel.selection();

				if (selection.isCollapsed())
					return true;

				Clipboard* service = clipboard();
				if (!service)
					return true;

				std::u32string text = editingModel.selectedText();
				service->writeText(text);

				return true;
			}

			bool cutSelection() {
				const TextSelection& selection = editingModel.selection();

				if (selection.isCollapsed())
					return true;

				Clipboard* service = clipboard();
				if (!service)
					return true;

				std::u32string text = editingModel.selectedText();

				bool copied = service->writeText(text);
				if (!copied)
					return true;

				if (editingModel.eraseSelection())
					notifyTextChanged();

				return true;
			}

			bool pasteClipboard() {
				Clipboard* service = clipboard();
				if (!service)
					return false;

				std::optional<std::u32string> clipboardText = service->readText();
				if (!clipboardText.has_value())
					return true;

				insertText(clipboardText.value());

				return true;
			}

			bool insertText(std::u32string_view text) {
				std::u32string sanitized = sanitizeSingleLineText(text);
				if (sanitized.empty())
					return false;

				bool changed = editingModel.replaceSelection(sanitized);
				if (changed)
					notifyTextChanged();

				return true;
			}

			void beginComposition() {
				if (compositionActive)
					return;

				if (!editingModel.eraseSelection())
					notifyTextChanged();

				compositionActive = true;

				compositionText.clear();

				compositionStart = editingModel.selection().caret;
				compositionCaret = std::nullopt;

				markNeedsLayout();
			}

			void updateComposition(const TextCompositionEvent& event) {
				if (!compositionActive) {
					compositionActive = true;

					compositionStart = editingModel.selection().caret;
				}

				compositionText = event.text;

				if (event.caretIndex.has_value())
					compositionCaret = std::min(event.caretIndex.value(), compositionText.size());
				else
					compositionCaret = std::nullopt;

				markNeedsLayout();
			}

			void endComposition() {
				if (!compositionActive)
					return;

				compositionActive = false;
				compositionText.clear();
				compositionCaret = std::nullopt;

				editingModel.moveCaretTo(std::min(compositionStart, editingModel.text().size()), false);

				markNeedsLayout();
			}

			std::u32string buildDisplayText() const {
				if (!compositionActive || compositionText.empty())
					return editingModel.text();

				std::u32string result = editingModel.text();
				
				std::size_t insertIndex = std::min(compositionStart, result.size());
				result.insert(insertIndex, compositionText);

				return result;
			}

			std::size_t displayCaretIndex() const {
				if (!compositionActive)
					return editingModel.selection().caret;

				return compositionStart + compositionText.size();
			}

			void paintCompositionUnderline(Canvas& canvas, const Rect& contentBounds) {
				if (!compositionActive)
					return;

				if (compositionText.empty())
					return;

				if (!textLayout)
					return;

				Point textOrigin = getTextOrigin(contentBounds);

				Rect visualBounds = getTextVisualBounds(contentBounds);

				std::vector<Rect> rectangles = textLayout->hitTestRange(compositionStart, compositionText.size());
				for (const Rect& rectangle : rectangles) {
					float y = textOrigin.y + rectangle.y + rectangle.height - 1.0f;
					canvas.fillRect(Rect(textOrigin.x + rectangle.x, visualBounds.y + visualBounds.height - 1.0f, rectangle.width, 1.0f), textBoxStyle.textColor);
				}
			}

			void updateNativeCaretRect(const Rect& contentBounds) {
				if (!hasFocus())
					return;

				TextInputContext* context = textInputContext();
				if (!context)
					return;

				std::size_t caretIndex = displayCaretIndex();

				TextPositionMetrics metrics = textLayout->hitTestTextPosition(caretIndex);
				Rect visualBounds = getTextVisualBounds(contentBounds);
					
				Point textOrigin = getTextOrigin(contentBounds);
				context->setCaretRect(Rect(textOrigin.x + metrics.position.x, textOrigin.y + metrics.position.y, textBoxStyle.caretWidth, visualBounds.height));
			}

			Point getTextOrigin(const Rect& contentBounds) const {
				Rect visualBounds = getTextVisualBounds(contentBounds);

				float baselineY = visualBounds.y + fontMetrics.ascent;

				float layoutBaseline = fontMetrics.ascent;
				if (textLayout && textLayout->baseline() > 0.0f)
					layoutBaseline = textLayout->baseline();

				return Point(contentBounds.x - horizontalScroll, baselineY - layoutBaseline);
			}

			Point getTextDrawOrigin(const Rect& contentBounds) const {
				if (!textLayout)
					return Point(contentBounds.x - horizontalScroll, contentBounds.y);

				float layoutHeight = textLayout->size().height;

				float y = contentBounds.y + (contentBounds.height - layoutHeight) * 0.5f;

				return Point(contentBounds.x - horizontalScroll, y);
			}

			Rect getTextVisualBounds(const Rect& contentBounds) const {
				float ascent = fontMetrics.ascent;
				float descent = fontMetrics.descent;

				float height = ascent + descent;
				if (height <= 0.0f)
					height = std::min(lineHeight, contentBounds.height);

				height = std::min(height, contentBounds.height);

				float y = contentBounds.y + (contentBounds.height - height) * 0.5f;
				return Rect(contentBounds.x - horizontalScroll, y, contentBounds.width, height);
			}

			float maximumHorizontalScroll(const Rect& contentBounds) const {
				if (!textLayout)
					return 0.0f;

				float textWidth = textLayout->size().width;

				return std::max(textWidth - contentBounds.width + 4.0f, 0.0f);
			}

			void clampHorizontalScroll(const Rect& contentBounds) {
				horizontalScroll = std::clamp(horizontalScroll, 0.0f, maximumHorizontalScroll(contentBounds));
			}

			void ensureCaretVisible(const Rect& contentBounds) {
				if (!textLayout)
					return;

				std::size_t caretIndex = displayCaretIndex();

				TextPositionMetrics metrics = textLayout->hitTestTextPosition(caretIndex);

				float caretLeft = metrics.position.x;
				float caretRight = caretLeft + textBoxStyle.caretWidth;
				
				float viewportLeft = horizontalScroll;
				float viewportRight = horizontalScroll + contentBounds.width;

				if (caretLeft < viewportLeft)
					horizontalScroll = caretLeft;
				else if (caretRight > viewportRight)
					horizontalScroll = caretRight - contentBounds.width;

				clampHorizontalScroll(contentBounds);
			}

			void resetCaretBlink() {
				caretVisible = true;
				caretBlinkElapsed = 0.0f;
				
				updateFrameDemand();
				markNeedsPaint();
			}

			void updateFrameDemand() {
				bool caretBlinkNeeded = hasFocus();
				bool focusAnimationNeeded = focusAnimation.isRunning();

				setFrameUpdatesEnabled(caretBlinkNeeded || focusAnimationNeeded);
			}

			void updateFocusAnimation(bool focused) {
				focusAnimation.animateTo(focused ? 1.0f : 0.0f, 0.16f, Easing::EaseOutCubic);

				updateFrameDemand();
				markNeedsPaint();
			}

		private:
			TextEditingModel editingModel;

			TextBox::ChangedCallback changedCallback;

			TextBoxStyle textBoxStyle;

			FontMetrics fontMetrics;
			
			bool pointerSelecting = false;

			bool compositionActive = false;
			std::u32string compositionText;
			std::size_t compositionStart = 0;
			std::optional<std::size_t> compositionCaret = 0;

			std::unique_ptr<TextLayout> textLayout;

			float lineHeight = 0.0f;

			float horizontalScroll = 0.0f;

			bool caretVisible = true;
			float caretBlinkElapsed = 0.0f;

			static constexpr float CaretBlinkInterval = 0.5f;

			AnimationController focusAnimation;
		};
	}

	TextBox::TextBox(std::u32string text, ChangedCallback onChanged, TextBoxStyle style, Key key, bool enabled)
		: Widget(std::move(key)), textValue(std::move(text)), changedCallback(std::move(onChanged)), textBoxStyle(std::move(style)), enabledValue(enabled) { }

	const std::u32string& TextBox::text() const {
		return textValue;
	}

	const TextBox::ChangedCallback& TextBox::onChanged() const {
		return changedCallback;
	}

	const TextBoxStyle& TextBox::style() const {
		return textBoxStyle;
	}

	std::unique_ptr<Element> TextBox::createElement() const {
		return std::make_unique<TextBoxElement>(*this);
	}

	bool TextBox::enabled() const {
		return enabledValue;
	}
}