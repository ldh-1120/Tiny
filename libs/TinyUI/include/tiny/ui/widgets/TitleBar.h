#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <tiny/core/Color.h>

#include <tiny/ui/Key.h>
#include <tiny/ui/Widget.h>

#include <tiny/graphics/PngIcon.h>

namespace tiny {
	enum class TitleBarAction {
		Minimize,
		Maximize,
		Close
	};

	struct TitleBarStyle {
		Color background = Color::fromRgb(24, 24, 37);
		Color borderColor = Color::fromRgb(49, 50, 68);

		Color textColor = Color::fromRgb(205, 214, 244);

		Color hoveredBackground = Color::fromRgb(49, 50, 68);
		Color pressedBackground = Color::fromRgb(69, 71, 90);

		Color closeHoveredBackground = Color::fromRgb(196, 43, 58);
		Color closePressedBackground = Color::fromRgb(174, 41, 55);

		Color iconColor = Color::fromRgb(137, 180, 250);

		float brandWidth = 180.0f;
		float iconSize = 16.0f;
		float iconLeftPadding = 14.0f;
		float titleLeftPadding = 40.0f;
	};

	class TitleBar : public Widget {
	public:
		using ActionCallback = std::function<void(TitleBarAction)>;
		using MaximizedCallback = std::function<bool()>;

		TitleBar(std::u32string title, std::unique_ptr<Widget> child, ActionCallback onAction, MaximizedCallback isMaximized, float height = 40.0f, float buttonWidth = 46.0f, 
			TitleBarStyle style = TitleBarStyle(), Key key = Key(), std::unique_ptr<Widget> toolbar = nullptr, std::shared_ptr<PngIcon> icon = nullptr);

		const std::u32string& title() const;

		const Widget* child() const;
		const Widget* toolbar() const;

		const ActionCallback& onAction() const;
		const MaximizedCallback& isMaximized() const;

		float height() const;
		float buttonWidth() const;

		const TitleBarStyle& style() const;

		std::unique_ptr<Element> createElement() const override;
		
		const std::shared_ptr<PngIcon>& icon() const;

	private:
		std::u32string titleValue;

		std::unique_ptr<Widget> childWidget;
		std::unique_ptr<Widget> toolbarWidget;

		ActionCallback actionCallback;
		MaximizedCallback maximizedCallback;

		float heightValue = 40.0f;
		float buttonWidthValue = 46.0f;

		TitleBarStyle styleValue;

		std::shared_ptr<PngIcon> iconValue;
	};
}