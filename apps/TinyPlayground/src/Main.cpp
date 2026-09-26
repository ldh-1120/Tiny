#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <string>

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
#include <tiny/ui/widgets/Column.h>
#include <tiny/ui/widgets/Padding.h>
#include <tiny/ui/widgets/Row.h>
#include <tiny/ui/widgets/SizedBox.h>
#include <tiny/ui/widgets/Text.h>
#include <tiny/ui/widgets/TextBox.h>
#include <tiny/ui/widgets/TitleBar.h>
#include <tiny/ui/widgets/ImageView.h>
#include <tiny/ui/widgets/ImageViewer.h>
#include <tiny/ui/widgets/ScrollView.h>
#include <tiny/ui/widgets/Flexible.h>
#include <tiny/ui/widgets/ConstrainedBox.h>
#include <tiny/ui/widgets/Stack.h>

namespace {
	struct PlaygroundState {
		int count = 0;

		std::u32string text;

		bool addEnabled = true;

		int viewerZoomPercent = 100;
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
		std::shared_ptr<tiny::Image> previewImage;

		if (pathLength > 0 && pathLength < MAX_PATH) {
			std::filesystem::path assetsPath = std::filesystem::path(executablePath).parent_path() / L"assets";

			appIcon = tiny::Image::load((assetsPath / L"icon.png").wstring());
			previewImage = tiny::Image::load((assetsPath / L"preview.png").wstring());
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
			tiny::UIBuilder([&state, &uiRoot, &window, &createInfo, &appIcon, &previewImage]() -> std::unique_ptr<tiny::Widget> {
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

			std::string countAscii = std::to_string(state.count);
			std::u32string countText = U"Count: ";
			for (char value : countAscii)
				countText.push_back(static_cast<char32_t>(value));

			std::vector<std::unique_ptr<tiny::Widget>> controlChildren;
			controlChildren.push_back(std::make_unique<tiny::Text>(U"Controls", tiny::Color::fromRgb(137, 180, 250), titleStyle, tiny::Key("controls-title")));
			controlChildren.push_back(std::make_unique<tiny::Text>(std::move(countText), tiny::Color::fromRgb(166, 173, 200), bodyStyle, tiny::Key("counter")));

			controlChildren.push_back(std::make_unique<tiny::Button>(U"Add", [&state, &uiRoot]() {
				++state.count;
				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("increasement-button"), state.addEnabled));
			controlChildren.push_back(
				std::make_unique<tiny::Button>(U"Decrease", [&state, &uiRoot]() {
				--state.count;
				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("decrement-button")));
			controlChildren.push_back(
				std::make_unique<tiny::Button>(U"Reset", [&state, &uiRoot]() {
				state.count = 0;
				uiRoot.requestRebuild();
			}, buttonStyle, tiny::Key("reset-button")));

			textBoxStyle.width = 240.0f;
			controlChildren.push_back(
				std::make_unique<tiny::TextBox>(state.text, [&state, &uiRoot](const std::u32string& text) {
				state.text = text;
				uiRoot.requestRebuild();
			}, textBoxStyle, tiny::Key("main-text-box")));

			std::unique_ptr<tiny::Widget> controlContent =
				std::make_unique<tiny::Column>(std::move(controlChildren), 12.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("control-content"));
			std::unique_ptr<tiny::Widget> controlPanel =
				std::make_unique<tiny::ConstrainedBox>(tiny::Constraints::fixedWidth(280.0f),
					std::make_unique<tiny::Padding>(tiny::Thickness(16.0f),
						std::make_unique<tiny::ScrollView>(std::move(controlContent), 48.0f, tiny::Key("control-scroll"))), tiny::Key("control-panel"));

			std::string zoomNumber = std::to_string(state.viewerZoomPercent);
			std::u32string zoomText = U"Zoom: ";
			for (char digit : zoomNumber)
				zoomText.push_back(static_cast<char32_t>(digit));

			zoomText += U"%  |  Ctrl+0 Fit  |  Ctrl+1 100%";

			tiny::TextStyle zoomTextStyle;
			zoomTextStyle.fontFamily = L"Segoe UI";
			zoomTextStyle.fontSize = 12.0f;

			std::vector<std::unique_ptr<tiny::Widget>> viewerChildren;
			viewerChildren.push_back(std::make_unique<tiny::Text>(zoomText, tiny::Color::fromRgb(205, 214, 244), zoomTextStyle, tiny::Key("viewer-zoom-label")));

			std::vector<std::unique_ptr<tiny::Widget>> viewerStackChildren;
			viewerStackChildren.push_back(std::make_unique<tiny::ImageViewer>(previewImage, tiny::Size(360.0f, 220.0f), tiny::ImageInterpolation::Linear, tiny::Key("image-viewer"), [&state, &uiRoot](float zoom) {
				state.viewerZoomPercent = static_cast<int>(std::lround(zoom * 100.0f));
				uiRoot.requestRebuild();
			}));
			viewerStackChildren.push_back(std::make_unique<tiny::Box>(tiny::Size(1.0f, 1.0f), tiny::Color::fromRgba(137, 180, 250, 18), tiny::Key("viewer-overlay")));

			std::unique_ptr<tiny::Widget> viewerStack = std::make_unique<tiny::Stack>(std::move(viewerStackChildren), tiny::Key("viewer-stack"));
			viewerChildren.push_back(std::make_unique<tiny::Expanded>(std::move(viewerStack), 1.0f, tiny::Key("viewer-expanded")));

			std::unique_ptr<tiny::Widget> viewerContent = std::make_unique<tiny::Column>(std::move(viewerChildren), 10.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("viewer-content"));
			std::unique_ptr<tiny::Widget> viewerPanel = std::make_unique<tiny::Expanded>(std::make_unique<tiny::Padding>(tiny::Thickness(16.0f), std::move(viewerContent)), 1.0f, tiny::Key("viewer-panel"));

			std::vector<std::unique_ptr<tiny::Widget>> scrollChildren;
			for (int index = 1; index <= 40; ++index) {
				std::u32string text = U"Scroll item ";

				std::string number = std::to_string(index);
				for (char character : number)
					text.push_back(static_cast<char32_t>(character));

				scrollChildren.push_back(std::make_unique<tiny::Text>(std::move(text), tiny::Color::fromRgb(205, 214, 244), bodyStyle));
			}

			std::unique_ptr<tiny::Widget> scrollContent = std::make_unique<tiny::Column>(std::move(scrollChildren), 12.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("scroll-content"));

			std::vector<std::unique_ptr<tiny::Widget>> rightChildren;
			rightChildren.push_back(std::make_unique<tiny::Text>(U"Scroll Test", tiny::Color::fromRgb(137, 180, 250), bodyStyle, tiny::Key("scroll-title")));
			rightChildren.push_back(std::make_unique<tiny::Expanded>(std::make_unique<tiny::ScrollView>(std::move(scrollContent), 48.0f, tiny::Key("demo-scroll-view")), 1.0f, tiny::Key("scroll-expanded")));

			std::unique_ptr<tiny::Widget> rightPanel =
				std::make_unique<tiny::ConstrainedBox>(
					tiny::Constraints::fixedWidth(260.0f),
					std::make_unique<tiny::Padding>(tiny::Thickness(16.0f),
						std::make_unique<tiny::Column>(std::move(rightChildren), 12.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("right-content"))), tiny::Key("right-panel"));

			std::vector<std::unique_ptr<tiny::Widget>> mainChildren;
			mainChildren.push_back(std::move(controlPanel));
			mainChildren.push_back(std::move(viewerPanel));
			mainChildren.push_back(std::move(rightPanel));

			std::unique_ptr<tiny::Widget> mainContent = std::make_unique<tiny::Row>(std::move(mainChildren), 1.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("main-layout"));

			tiny::TextStyle statusStyle;
			statusStyle.fontFamily = L"Segoe UI";
			statusStyle.fontSize = 12.0f;

			std::unique_ptr<tiny::Widget> statusBar =
				std::make_unique<tiny::ConstrainedBox>(tiny::Constraints::fixedHeight(30.0f),
					std::make_unique<tiny::Padding>(tiny::Thickness(6.0f),
						std::make_unique<tiny::Text>(U"Playground  |  Expanded / ConstrainedBox / ScrollView", tiny::Color::fromRgb(108, 112, 134), statusStyle, tiny::Key("status-text"))), tiny::Key("status-bar"));

			std::vector<std::unique_ptr<tiny::Widget>> rootChildren;
			rootChildren.push_back(std::make_unique<tiny::Expanded>(std::move(mainContent), 1.0f, tiny::Key("main-expanded")));
			rootChildren.push_back(std::move(statusBar));

			std::unique_ptr<tiny::Widget> rootContent = std::make_unique<tiny::Column>(std::move(rootChildren), 0.0f, tiny::CrossAxisAlignment::Stretch, tiny::Key("root-content"));

			tiny::ButtonStyle toolbarButtonStyle;
			toolbarButtonStyle.textStyle.fontFamily = L"Segoe UI";
			toolbarButtonStyle.textStyle.fontSize = 12.0f;
			toolbarButtonStyle.background = tiny::Color::fromRgb(30, 30, 46);
			toolbarButtonStyle.padding = tiny::Thickness(10.0f, 5.0f);

			std::vector<std::unique_ptr<tiny::Widget>> toolbarChildren;
			toolbarChildren.push_back(std::make_unique<tiny::Button>(U"Toggle Add", [&state, &uiRoot]() {
				state.addEnabled = !state.addEnabled;
				uiRoot.requestRebuild();
			}, toolbarButtonStyle, tiny::Key("title-toolbar-toggle")));
			toolbarChildren.push_back(std::make_unique<tiny::Button>(U"Reset", [&state, &uiRoot]() {
				state.count = 0;
				uiRoot.requestRebuild();
			}, toolbarButtonStyle, tiny::Key("toolbar-reset")));

			std::unique_ptr<tiny::Widget> toolbar = std::make_unique<tiny::Row>(std::move(toolbarChildren), 8.0f, tiny::CrossAxisAlignment::Center, tiny::Key("title-toolbar-row"));

			tiny::TitleBarStyle titleBarStyle;
			titleBarStyle.brandWidth = 180.0f;
			titleBarStyle.titleLeftPadding = 42.0f;
			titleBarStyle.iconLeftPadding = 10.0f;
			titleBarStyle.iconSize = 22.0f;
			titleBarStyle.iconColor = tiny::Color::fromRgb(137, 180, 250);

			return std::make_unique<tiny::TitleBar>(U"Tiny Playground",
				std::make_unique<tiny::Padding>(tiny::Thickness(8.0f), std::move(rootContent)), [&window](tiny::TitleBarAction action) {
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
				}}, [&window]() {
					return window.isMaximized();
				}, createInfo.titleBarHeight, tiny::WindowCaptionButtonWidth, titleBarStyle, tiny::Key("window-title-bar"), std::move(toolbar), appIcon);
		}));

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

			window.setPointerCursor(uiRoot.pointerCursor());
		});

		tiny::Subscription pointerPressedSubscription = window.pointerPressed.subscribe([&window, &uiRoot](const tiny::PointerEvent& event) {
			bool handled = uiRoot.pointerPressed(event);
			if (handled)
				window.capturePointer();

			window.setPointerCursor(uiRoot.pointerCursor());
		});

		tiny::Subscription pointerReleasedSubscription = window.pointerReleased.subscribe([&window, &uiRoot](const tiny::PointerEvent& event) {
			uiRoot.pointerReleased(event);

			if (window.hasPointerCapture())
				window.releasePointerCapture();

			window.setPointerCursor(uiRoot.pointerCursor());
		});

		tiny::Subscription pointerWheelSubscription = window.pointerWheel.subscribe([&uiRoot](tiny::PointerWheelEvent& event) {
			event.handled = uiRoot.pointerWheel(event);
		});

		tiny::Subscription pointerExitedSubscription = window.pointerExited.subscribe([&window, &uiRoot]() {
			uiRoot.pointerExited();

			window.setPointerCursor(uiRoot.pointerCursor());
		});

		tiny::Subscription pointerCaptureLostSubscription = window.pointerCaptureLost.subscribe([&window, &uiRoot]() {
			uiRoot.pointerCaptureLost();

			window.setPointerCursor(uiRoot.pointerCursor());
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