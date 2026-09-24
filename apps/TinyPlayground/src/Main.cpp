#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>

#include <Windows.h>

#include <tiny/core/Color.h>
#include <tiny/core/Rect.h>
#include <tiny/core/Subscription.h>

#include <tiny/core/input/Pointer.h>

#include <tiny/graphics/Canvas.h>
#include <tiny/graphics/GraphicsContext.h>
#include <tiny/graphics/WindowRenderer.h>
#include <tiny/graphics/TextStyle.h>
#include <tiny/graphics/TextLayout.h>

#include <tiny/platform/Platform.h>
#include <tiny/platform/Window.h>
#include <tiny/platform/SystemClipboard.h>
#include <tiny/platform/WindowTextInputContext.h>

#include <tiny/ui/UIRoot.h>
#include <tiny/ui/widgets/Box.h>
#include <tiny/ui/widgets/Button.h>
#include <tiny/ui/widgets/Center.h>
#include <tiny/ui/widgets/Column.h>
#include <tiny/ui/widgets/Padding.h>
#include <tiny/ui/widgets/Row.h>
#include <tiny/ui/widgets/SizedBox.h>
#include <tiny/ui/widgets/Text.h>
#include <tiny/ui/widgets/TextBox.h>

namespace {
	struct PlaygroundState {
		int count = 0;

		std::u32string text;

		bool addEnabled = true;
	};
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int showCommand) {
	tiny::initializePlatform();

	int result = 0;
	{
		tiny::WindowCreateInfo createInfo;
		createInfo.title = L"Tiny Playground";
		createInfo.width = 1280;
		createInfo.height = 720;
		createInfo.resizable = true;

		createInfo.customTitleBar = true;
		createInfo.titleBarHeight = 40.0f;

		tiny::Window window(createInfo);

		tiny::GraphicsContext graphicsContext;
		tiny::WindowRenderer renderer(graphicsContext, window);

		PlaygroundState state;

		tiny::WindowTextInputContext textInputContext(window);
		tiny::SystemClipboard clipboard;

		tiny::UIRoot uiRoot;
		uiRoot.setClipboard(clipboard);
		uiRoot.setTextInputContext(textInputContext);

		tiny::Subscription uiRepaintSubscription = uiRoot.repaintRequested.subscribe([&window]() {
			window.requestRepaint();
		});

		uiRoot.setBuilder(
			tiny::UIBuilder([&state, &uiRoot]() -> std::unique_ptr<tiny::Widget> {
			tiny::TextStyle titleStyle;
			titleStyle.fontFamily = L"Segoe UI";
			titleStyle.fontSize = 32.0f;
			titleStyle.bold = true;

			tiny::TextStyle bodyStyle;
			bodyStyle.fontFamily = L"Segoe UI";
			bodyStyle.fontSize = 18.0f;

			tiny::ButtonStyle buttonStyle;
			buttonStyle.textStyle.fontSize = 16.0f;
			buttonStyle.padding = tiny::Thickness(20.0f, 10.0f);

			tiny::TextBoxStyle textBoxStyle;
			textBoxStyle.width = 300.0f;
			textBoxStyle.textStyle.fontSize = 18.0f;

			std::vector<std::unique_ptr<tiny::Widget>> children;

			children.push_back(std::make_unique<tiny::Text>(U"Tiny UI", tiny::Color::fromRgb(205, 214, 244), titleStyle, tiny::Key("title")));

			std::string countAscii = std::to_string(state.count);
			std::u32string countText = U"Count: ";
			for (char value : countAscii)
				countText.push_back(static_cast<char32_t>(value));

			children.push_back(std::make_unique<tiny::Text>(std::move(countText), tiny::Color::fromRgb(166, 173, 200), bodyStyle, tiny::Key("counter")));

			std::vector<std::unique_ptr<tiny::Widget>> actionChildren;

			actionChildren.push_back(
				std::make_unique<tiny::Button>(U"Add", [&state, &uiRoot]() {
				++state.count;

				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("increment-button"), state.addEnabled));

			children.push_back(
				std::make_unique<tiny::Button>(U"Toggle Add", [&state, &uiRoot]() {
				state.addEnabled = !state.addEnabled;

				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("toggle-add-button")));

			actionChildren.push_back(
				std::make_unique<tiny::Button>(U"Decrease", [&state, &uiRoot]() {
				--state.count;

				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("decrement-button")));

			actionChildren.push_back(
				std::make_unique<tiny::Button>(U"Reset", [&state, &uiRoot]() {
				state.count = 0;

				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("reset-button")));

			children.push_back(std::make_unique<tiny::Row>(std::move(actionChildren), 12.0f, tiny::CrossAxisAlignment::Center, tiny::Key("actions-row")));

			children.push_back(
				std::make_unique<tiny::TextBox>(state.text, [&state, &uiRoot](const std::u32string& text) {
				state.text = text;

				uiRoot.requestRebuild();
			}, textBoxStyle, tiny::Key("main-text-box")));

			return std::make_unique<tiny::Padding>(
				tiny::Thickness(0.0f, 40.0f, 0.0f, 0.0f), 
				std::make_unique<tiny::Center>(
				std::make_unique<tiny::Padding>(
					tiny::Thickness(24.0f),
					std::make_unique<tiny::Column>(
						std::move(children),
						16.0f,
						tiny::CrossAxisAlignment::Center,
						tiny::Key("content")
					)
				)), tiny::Key("window-content-padding"));
		}
			)
		);

		tiny::WindowCaptionButton hoveredCaptionButton = tiny::WindowCaptionButton::None;
		tiny::WindowCaptionButton pressedCaptionButton = tiny::WindowCaptionButton::None;

		tiny::Subscription paintSubscription = window.paintRequested.subscribe([&window, &graphicsContext, &renderer, &uiRoot, &pressedCaptionButton, &hoveredCaptionButton]() {
			tiny::Size pixelSize = window.clientSize();
			float scale = window.dpiScale();

			tiny::Size logicalSize(pixelSize.width / scale, pixelSize.height / scale);
			uiRoot.layout(graphicsContext, logicalSize);

			renderer.render([&uiRoot, &window, &graphicsContext, &pressedCaptionButton, &hoveredCaptionButton](tiny::Canvas& canvas) {
				canvas.clear(tiny::Color::fromRgb(30, 30, 46));
				uiRoot.paint(canvas);

				float scale = window.dpiScale();
				if (scale <= 0.0f)
					scale = 1.0f;

				float width = window.clientSize().width / scale;

				constexpr float titleBarHeight = 40.0f;

				canvas.fillRect(tiny::Rect(0.0f, 0.0f, width, titleBarHeight), tiny::Color::fromRgb(24, 24, 37));

				tiny::TextStyle titleStyle;
				titleStyle.fontFamily = L"Segoe UI";
				titleStyle.fontSize = 14.0f;

				if (width > 200.0f) {
					std::unique_ptr<tiny::TextLayout> titleLayout = graphicsContext.createTextLayout(U"Tiny Playground", titleStyle, width - 170.0f);
					if (titleLayout) {
						float y = (titleBarHeight - titleLayout->size().height) * 0.5f;
						canvas.drawTextLayout(*titleLayout, tiny::Point(14.0f, y), tiny::Color::fromRgb(205, 214, 244));
					}
				}

				tiny::TextStyle iconStyle;
				iconStyle.fontFamily = L"Segoe UI Symbol";
				iconStyle.fontSize = 18.0f;

				float buttonWidth = tiny::WindowCaptionButtonWidth;
				float totalButtonWidth = buttonWidth * 3.0f;

				if (width >= totalButtonWidth) {
					for (int index = 0; index < 3; ++index) {
						tiny::WindowCaptionButton button = tiny::WindowCaptionButton::None;

						std::u32string glyph;
						switch (index) {
							case 0:
								button = tiny::WindowCaptionButton::Minimize;
								glyph = U"\u2212";
								break;

							case 1:
								button = tiny::WindowCaptionButton::Maximize;
								glyph = window.isMaximized() ? U"\u2750" : U"\u25A1";
								break;

							case 2:
								button = tiny::WindowCaptionButton::Close;
								glyph = U"\u00D7";
								break;
						}

						float x = width - totalButtonWidth + static_cast<float>(index) * buttonWidth;

						bool hovered = hoveredCaptionButton == button;
						bool pressed = pressedCaptionButton == button;

						tiny::Color background = tiny::Color::fromRgb(24, 24, 37);
						tiny::Color foreground = tiny::Color::fromRgb(205, 214, 244);

						if (button == tiny::WindowCaptionButton::Close) {
							if (pressed)
								background = tiny::Color::fromRgb(174, 41, 55);
							else if (hovered)
								background = tiny::Color::fromRgb(196, 43, 58);
						} else {
							if (pressed)
								background = tiny::Color::fromRgb(69, 71, 90);
							else if (hovered)
								background = tiny::Color::fromRgb(49, 50, 68);
						}

						canvas.fillRect(tiny::Rect(x, 0.0f, buttonWidth, titleBarHeight), background);

						std::unique_ptr<tiny::TextLayout> iconLayout = graphicsContext.createTextLayout(glyph, iconStyle, buttonWidth);
						if (iconLayout) {
							float iconX = x + (buttonWidth - iconLayout->size().width) * 0.5f;
							float iconY = (titleBarHeight - iconLayout->size().height) * 0.5f;

							canvas.drawTextLayout(*iconLayout, tiny::Point(iconX, iconY), foreground);
						}
					}
				}
			});
		});

		tiny::Subscription pointerMovedSubscription = window.pointerMoved.subscribe([&window, &uiRoot, &hoveredCaptionButton](const tiny::PointerEvent& event) {
			uiRoot.pointerMoved(event);

			tiny::WindowCaptionButton button = window.captionButtonAt(event.position);
			if (hoveredCaptionButton != button) {
				hoveredCaptionButton = button;
				window.requestRepaint();
			}
		});

		tiny::Subscription pointerPressedSubscription = window.pointerPressed.subscribe([&window, &uiRoot, &pressedCaptionButton](const tiny::PointerEvent& event) {
			if (event.button == tiny::PointerButton::Left) {
				tiny::WindowCaptionButton button = window.captionButtonAt(event.position);
				if (button != pressedCaptionButton) {
					pressedCaptionButton = button;

					window.capturePointer();
					window.requestRepaint();

					return;
				}
			}

			bool handled = uiRoot.pointerPressed(event);
			if (handled)
				window.capturePointer();
		});

		tiny::Subscription pointerReleasedSubscription = window.pointerReleased.subscribe([&window, &uiRoot, &pressedCaptionButton, &hoveredCaptionButton](const tiny::PointerEvent& event) {
			if (event.button == tiny::PointerButton::Left && pressedCaptionButton != tiny::WindowCaptionButton::None) {
				tiny::WindowCaptionButton pressed = pressedCaptionButton;
				tiny::WindowCaptionButton released = window.captionButtonAt(event.position);

				pressedCaptionButton = tiny::WindowCaptionButton::None;
				if (window.hasPointerCapture())
					window.releasePointerCapture();

				hoveredCaptionButton = released;

				window.requestRepaint();

				if (pressed != released)
					return;

				switch (pressed) {
					case tiny::WindowCaptionButton::Minimize:
						window.minimize();
						break;

					case tiny::WindowCaptionButton::Maximize:
						window.toggleMaximize();
						break;

					case tiny::WindowCaptionButton::Close:
						window.close();
						break;

					case tiny::WindowCaptionButton::None:
						break;
				}
			}

			uiRoot.pointerReleased(event);

			if (window.hasPointerCapture())
				window.releasePointerCapture();
		});

		tiny::Subscription pointerExitedSubscription = window.pointerExited.subscribe([&window, &uiRoot, &hoveredCaptionButton]() {
			uiRoot.pointerExited();

			hoveredCaptionButton = tiny::WindowCaptionButton::None;

			window.requestRepaint();
		});

		tiny::Subscription pointerCaptureLostSubscription = window.pointerCaptureLost.subscribe([&window, &uiRoot, &pressedCaptionButton]() {
			uiRoot.pointerCaptureLost();

			pressedCaptionButton = tiny::WindowCaptionButton::None;

			window.requestRepaint();
		});

		tiny::Subscription keyPressedSubscription = window.keyPressed.subscribe([&uiRoot](tiny::KeyEvent& event) {
			bool handled = uiRoot.keyPressed(event);
			if (handled)
				event.handled = true;
		});

		tiny::Subscription keyReleasedSubscription = window.keyReleased.subscribe([&uiRoot](tiny::KeyEvent& event) {
			bool handled = uiRoot.keyReleased(event);
			if (handled)
				event.handled = true;
		});

		tiny::Subscription textInputSubscription = window.textInput.subscribe([&uiRoot](tiny::TextInputEvent& event) {
			bool handled = uiRoot.textInput(event);
			if (handled)
				event.handled = true;
		});

		tiny::Subscription textCompositionSubscription = window.textComposition.subscribe([&uiRoot](tiny::TextCompositionEvent& event) {
			bool handled = uiRoot.textComposition(event);
			if (handled)
				event.handled = true;
		});

		tiny::Subscription frameSubscription = window.frame.subscribe([&uiRoot](const tiny::FrameEvent& event) {
			uiRoot.onFrame(event);
		});

		window.setFrameUpdatesEnabled(uiRoot.needsFrameUpdates());
		tiny::Subscription frameDemandSubscription = uiRoot.frameDemandChanged.subscribe([&window](bool needed) {
			window.setFrameUpdatesEnabled(needed);
		});

		window.show();
		window.requestRepaint();

		result = tiny::runMessageLoop();
	}

	tiny::shutdownPlatform();

	return result;
}