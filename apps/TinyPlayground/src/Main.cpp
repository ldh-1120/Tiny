#include <cstdio>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <cmath>

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

#include <tiny/ui/UI.h>
#include <tiny/ui/UIRoot.h>
#include <tiny/ui/UIBuilder.h>

#include <tiny/ui/widgets/Box.h>
#include <tiny/ui/widgets/TitleBar.h>
#include <tiny/ui/widgets/ImageViewer.h>

namespace {
	struct PlaygroundState {
		int count = 0;

		std::u32string text;

		bool addEnabled = true;

		int viewerZoomPercent = 100;
	};

	struct PlaygroundStyles {
		tiny::TextStyle title;
		tiny::TextStyle body;
		tiny::TextStyle overlay;
		tiny::TextStyle status;

		tiny::ButtonStyle button;
		tiny::ButtonStyle toolbarButton;

		tiny::TextBoxStyle textBox;

		tiny::TitleBarStyle titleBar;

		PlaygroundStyles() {
			title.fontFamily = L"Segoe UI";
			title.fontSize = 32.0f;
			title.bold = true;

			body.fontFamily = L"Segoe UI";
			body.fontSize = 18.0f;

			overlay.fontFamily = L"Segoe UI";
			overlay.fontSize = 12.0f;

			status.fontFamily = L"Segoe UI";
			status.fontSize = 12.0f;

			button.textStyle.fontFamily = L"Segoe UI";
			button.textStyle.fontSize = 16.0f;
			button.padding = tiny::Thickness(20.0f, 10.0f);

			toolbarButton.textStyle.fontFamily = L"Segoe UI";
			toolbarButton.textStyle.fontSize = 12.0f;
			toolbarButton.background = tiny::Color::fromRgb(30, 30, 46);
			toolbarButton.padding = tiny::Thickness(10.0f, 5.0f);

			textBox.width = 240.0f;
			textBox.textStyle.fontFamily = L"Segoe UI";
			textBox.textStyle.fontSize = 18.0f;

			titleBar.brandWidth = 180.0f;
			titleBar.titleLeftPadding = 42.0f;
			titleBar.iconLeftPadding = 10.0f;
			titleBar.iconSize = 22.0f;
			titleBar.iconColor = tiny::Color::fromRgb(137, 180, 250);
		}
	};
}

std::u32string toU32String(int value) {
	std::string ascii = std::to_string(value);

	std::u32string result;
	for (char character : ascii)
		result.push_back(static_cast<char32_t>(character));

	return result;
}

tiny::WidgetPtr buildControls(PlaygroundState& state, tiny::UIRoot& uiRoot, const PlaygroundStyles& styles) {
	using namespace tiny;
	using namespace tiny::ui;

	std::u32string countText = U"Count: ";
	countText += toU32String(state.count);

	return fixedWidth(280.0f,
		padding(16.0f,
			scroll(
				column(children(
					text(U"Controls", Color::fromRgb(137, 180, 250), styles.title),
					text(std::move(countText), Color::fromRgb(166, 173, 200), styles.body),
					button(U"Add", [&state, &uiRoot]() {
		++state.count;
		uiRoot.requestRebuild();
	}, styles.button, state.addEnabled),
					button(U"Decrease", [&state, &uiRoot]() {
		--state.count;
		uiRoot.requestRebuild();
	}, styles.button),
					button(U"Reset", [&state, &uiRoot]() {
		state.count = 0;
		uiRoot.requestRebuild();
	}, styles.button),
					textBox(state.text, [&state, &uiRoot](const std::u32string& value) {
		state.text = value;
		uiRoot.requestRebuild();
	}, styles.textBox)
				), 12.0f, CrossAxisAlignment::Stretch))));
}

tiny::WidgetPtr buildViewer(PlaygroundState& state, tiny::UIRoot& uiRoot, const PlaygroundStyles& styles, const std::shared_ptr<tiny::Image>& previewImage) {
	using namespace tiny;
	using namespace tiny::ui;

	std::u32string zoomText = U"Zoom: ";
	zoomText += toU32String(state.viewerZoomPercent);
	zoomText += U"%  |  Ctrl+0 Fit  |  Ctrl+1 100%";

	ImageViewer::ZoomChangedCallback zoomChanged = [&state, &uiRoot](float zoom) {
		state.viewerZoomPercent = static_cast<int>(std::lround(zoom * 100.0f));
		uiRoot.requestRebuild();
	};

	return expanded(
		padding(16.0f,
			stack(children(
				make<ImageViewer>(previewImage, Size(360.0f, 220.0f), ImageInterpolation::Linear, Key("image-viewer"), std::move(zoomChanged)),
				make<Box>(Size(1.0f, 1.0f), Color::fromRgba(137, 180, 250, 18)),
				align(Alignment::BottomRight,
					padding(12.0f,
						text(std::move(zoomText), Color::fromRgb(205, 214, 144), styles.overlay)))))));
}

tiny::WidgetPtr buildScrollTest(const PlaygroundStyles& styles) {
	using namespace tiny;
	using namespace tiny::ui;

	std::vector<WidgetPtr> items;
	for (int index = 0; index < 40; ++index) {
		std::u32string label = U"Scroll item";
		label += toU32String(index);

		items.push_back(text(std::move(label), Color::fromRgb(205, 214, 244), styles.body));
	}

	return fixedWidth(260.0f,
		padding(16.0f,
			column(children(
				text(U"Scroll Text", Color::fromRgb(137, 180, 250), styles.body),
				expanded(
					scroll(
						column(std::move(items), 12.0f, CrossAxisAlignment::Stretch)))), 12.0f, CrossAxisAlignment::Stretch)));
}

tiny::WidgetPtr buildStatusBar(const PlaygroundStyles& styles) {
	using namespace tiny::ui;

	return fixedHeight(30.0f,
		padding(6.0f,
			text(U"Playground  |  Flex / Scroll / Stack / Align", tiny::Color::fromRgb(108, 112, 134), styles.status)));
}

tiny::WidgetPtr buildToolbar(PlaygroundState& state, tiny::UIRoot& uiRoot, const PlaygroundStyles& styles) {
	using namespace tiny;
	using namespace tiny::ui;

	return row(children(
		button(U"Toggle Add", [&state, &uiRoot]() {
		state.addEnabled = !state.addEnabled;
		uiRoot.requestRebuild();
	}, styles.toolbarButton),
		button(U"Reset", [&state, &uiRoot]() {
		state.count = 0;
		uiRoot.requestRebuild();
	}, styles.toolbarButton)
	), 8.0f, CrossAxisAlignment::Center);
}

tiny::WidgetPtr buildContent(PlaygroundState& state, tiny::UIRoot& uiRoot, const std::shared_ptr<tiny::Image>& previewImage, const PlaygroundStyles& styles) {
	using namespace tiny;
	using namespace tiny::ui;

	WidgetPtr main = row(children(
		buildControls(state, uiRoot, styles),
		buildViewer(state, uiRoot, styles, previewImage),
		buildScrollTest(styles)
	), 1.0f, CrossAxisAlignment::Stretch);

	return column(children(
		expanded(std::move(main)),
		buildStatusBar(styles)
	), 0.0f, CrossAxisAlignment::Stretch);
}

void handleTitleBarAction(tiny::Window& window, tiny::TitleBarAction action) {
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
}

tiny::WidgetPtr buildPlayground(PlaygroundState& state, tiny::UIRoot& uiRoot, tiny::Window& window, float titleBarHeight, const std::shared_ptr<tiny::Image>& appIcon, const std::shared_ptr<tiny::Image>& previewImage, const PlaygroundStyles& styles) {
	using namespace tiny;
	using namespace tiny::ui;

	WidgetPtr content = padding(8.0f,
		buildContent(state, uiRoot, previewImage, styles));

	WidgetPtr toolbar = buildToolbar(state, uiRoot, styles);

	return make<TitleBar>(U"Tiny Playground", std::move(content), [&window](TitleBarAction action) {
		handleTitleBarAction(window, action);
	}, [&window]() {
		return window.isMaximized();
	}, titleBarHeight, WindowCaptionButtonWidth, styles.titleBar, Key(), std::move(toolbar), appIcon);
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
		PlaygroundStyles styles;

		tiny::WindowTextInputContext textInputContext(window);
		tiny::SystemClipboard clipboard;

		tiny::UIRoot uiRoot;
		uiRoot.setClipboard(clipboard);
		uiRoot.setTextInputContext(textInputContext);

		tiny::Subscription uiRepaintSubscription = uiRoot.repaintRequested.subscribe([&window]() {
			window.requestRepaint();
		});

		uiRoot.setBuilder(
			tiny::UIBuilder([&state, &uiRoot, &window, &createInfo, &appIcon, &previewImage, &styles]() -> std::unique_ptr<tiny::Widget> {
			return buildPlayground(state, uiRoot, window, createInfo.titleBarHeight, appIcon, previewImage, styles);
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