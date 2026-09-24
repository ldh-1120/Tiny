#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

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
#include <tiny/graphics/Image.h>

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
#include <tiny/ui/widgets/TitleBar.h>

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

		wchar_t executablePath[MAX_PATH] { };

		DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);

		std::shared_ptr<tiny::Image> appIcon;
		if (pathLength > 0 && pathLength < MAX_PATH) {
			std::filesystem::path iconPath = std::filesystem::path(executablePath).parent_path() / L"assets" / L"icon.png";
			appIcon = tiny::Image::load(iconPath.wstring());
		}

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
			tiny::UIBuilder([&state, &uiRoot, &window, &createInfo, &appIcon]() -> std::unique_ptr<tiny::Widget> {
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

			tiny::ButtonStyle toolbarButtonStyle;
			toolbarButtonStyle.textStyle.fontFamily = L"Segoe UI";
			toolbarButtonStyle.textStyle.fontSize = 12.0f;
			toolbarButtonStyle.padding = tiny::Thickness(10.0f, 5.0f);

			std::vector<std::unique_ptr<tiny::Widget>> toolbarChildren;
			toolbarChildren.push_back(std::make_unique<tiny::Button>(U"Toggle", [&state, &uiRoot]() {
				state.addEnabled = !state.addEnabled;
				uiRoot.requestRebuild();
			}, tiny::ButtonStyle(), tiny::Key("title-toolbar-toggle")));

			toolbarChildren.push_back(std::make_unique<tiny::Button>(U"Reset", [&state, &uiRoot]() {
				state.count = 0;
				uiRoot.requestRebuild();
			}, tiny::ButtonStyle(), tiny::Key("toolbar-reset")));

			std::unique_ptr<tiny::Widget> toolbar = std::make_unique<tiny::Row>(std::move(toolbarChildren), 8.0f, tiny::CrossAxisAlignment::Center, tiny::Key("title-toolbar-row"));

			tiny::TitleBarStyle titleBarStyle;
			titleBarStyle.brandWidth = 180.0f;
			titleBarStyle.iconSize = 16.0f;
			titleBarStyle.iconColor = tiny::Color::fromRgb(137, 180, 250);

			return std::make_unique<tiny::TitleBar>(U"Tiny Playground",
				std::make_unique<tiny::Center>(
					std::make_unique<tiny::Padding>(
						tiny::Thickness(24.0f),
						std::make_unique<tiny::Column>(
							std::move(children),
							16.0f,
							tiny::CrossAxisAlignment::Center,
							tiny::Key("content")
						)
					)),
				[&window](tiny::TitleBarAction action) {
				switch (action) {
					case tiny::TitleBarAction::Minimize:
						window.minimize();
						break;

					case tiny::TitleBarAction::Maximize:
						window.toggleMaximize();
						break;

					case tiny::TitleBarAction::Close:
						window.close();
						break;
				}
			}, [&window]() {
				return window.isMaximized();
			}, createInfo.titleBarHeight, tiny::WindowCaptionButtonWidth, titleBarStyle, tiny::Key("window-title-bar"), std::move(toolbar), appIcon);
		}
			));

		tiny::Subscription paintSubscription = window.paintRequested.subscribe([&window, &graphicsContext, &renderer, &uiRoot]() {
			tiny::Size pixelSize = window.clientSize();
			float scale = window.dpiScale();

			tiny::Size logicalSize(pixelSize.width / scale, pixelSize.height / scale);
			uiRoot.layout(graphicsContext, logicalSize);

			renderer.render([&uiRoot](tiny::Canvas& canvas) {
				canvas.clear(tiny::Color::fromRgb(30, 30, 46));
				uiRoot.paint(canvas);
			});
		});

		tiny::Subscription pointerMovedSubscription = window.pointerMoved.subscribe([&window, &uiRoot](const tiny::PointerEvent& event) {
			uiRoot.pointerMoved(event);
		});

		tiny::Subscription pointerPressedSubscription = window.pointerPressed.subscribe([&window, &uiRoot](const tiny::PointerEvent& event) {
			bool handled = uiRoot.pointerPressed(event);
			if (handled)
				window.capturePointer();
		});

		tiny::Subscription pointerReleasedSubscription = window.pointerReleased.subscribe([&window, &uiRoot](const tiny::PointerEvent& event) {
			uiRoot.pointerReleased(event);

			if (window.hasPointerCapture())
				window.releasePointerCapture();
		});

		tiny::Subscription pointerExitedSubscription = window.pointerExited.subscribe([&window, &uiRoot]() {
			uiRoot.pointerExited();
		});

		tiny::Subscription pointerCaptureLostSubscription = window.pointerCaptureLost.subscribe([&window, &uiRoot]() {
			uiRoot.pointerCaptureLost();
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

		window.setCaptionClientHitTest([&uiRoot](const tiny::Point& position) {
			return uiRoot.isInteractiveAt(position);
		});

		window.show();
		window.requestRepaint();

		result = tiny::runMessageLoop();
	}

	tiny::shutdownPlatform();

	return result;
}