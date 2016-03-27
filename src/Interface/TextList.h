/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_TEXTLIST_H
#define OPENXCOM_TEXTLIST_H

//#include <map>
//#include <vector>

#include "Text.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

enum ArrowOrientation
{
	ARROW_VERTICAL,		// 0
	ARROW_HORIZONTAL	// 1
};


class ArrowButton;
class ComboBox;
class ScrollBar;


/**
 * A TextList is a list of Text's split into rows & columns.
 * @note Contains a set of Text's that are automatically lined up by rows and
 * columns like a big table making it easy to manage them together.
 */
class TextList final
	:
		public InteractiveSurface
{

private:
	bool
		_condensed,
		_contrast,
		_dot,
		_scrollable,
		_selectable,
		_wrap;
	int
		_arrowPos,
		_arrowsLeftEdge,
		_arrowsRightEdge,
		_margin,
		_scrollPos;
	size_t
		_scroll,
		_selRow,
		_visibleRows;
	Uint8
		_color,
		_color2;

	ActionHandler
		_leftClick,
		_leftPress,
		_leftRelease,
		_rightClick,
		_rightPress,
		_rightRelease;
	ArrowButton
		* _up,
		* _down;
	ArrowOrientation _arrowType;
	ComboBox* _comboBox;
	Font
		* _big,
		* _font,
		* _small;
	const Language* _lang;
	ScrollBar* _scrollbar;
	Surface
		* _bg,
		* _selector;

	std::map<int, TextHAlign> _align;

	std::vector<size_t>
		_columns,
		_rows;
	std::vector<std::vector<Text*>> _texts;
	std::vector<ArrowButton*>
		_arrowLeft,
		_arrowRight;

	/// Updates the arrow buttons.
	void updateArrows();
	/// Updates the visible rows.
	void updateVisible();


	public:
		/// Creates a TextList with the specified size and position.
		TextList(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the TextList.
		~TextList();

		/// Sets the x-position of the Surface.
		void setX(int x) override;
		/// Sets the y-position of the Surface.
		void setY(int y) override;

		/// Gets the ArrowButtons' left-edge.
		int getArrowsLeftEdge();
		/// Gets the ArrowButtons' right-edge.
		int getArrowsRightEdge();

		/// Unpresses the Surface.
		void unpress(State* state) override;

		/// Sets the text-color of a certain cell.
		void setCellColor(
				size_t row,
				size_t column,
				Uint8 color,
				bool contrast = false);
		/// Sets the text-color of a certain row.
		void setRowColor(
				size_t row,
				Uint8 color,
				bool contrast = false);

		/// Gets the text-string of a certain cell.
		std::wstring getCellText(
				size_t row,
				size_t column) const;
		/// Sets the text-string of a certain cell.
		void setCellText(
				size_t row,
				size_t column,
				const std::wstring& text);

		/// Gets the x-position of a certain column.
		int getColumnX(size_t column) const;
		/// Gets the y-position of a certain row.
		int getRowY(size_t row) const;

		/// Gets the height of a text-row in pixels
//		int getTextHeight(size_t row) const;
		/// Gets the number of lines in the wrapped text for a specified row.
//		int getNumTextLines(size_t row) const;

		/// Gets the quantity of Texts in the list.
		size_t getTexts() const;
		/// Gets the quantity of rows in the list.
		size_t getRows() const;
		/// Gets the quantity of visible rows in the list.
		size_t getVisibleRows() const;

		/// Adds a new row to the TextList.
		void addRow(
				int cols,
				...);
		/// Sets the columns in the TextList.
		void setColumns(
				int cols,
				...);

		/// Sets the palette of the TextList.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;
		/// Initializes the resources for the TextList.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override;

		/// Sets the height of the Surface.
		void setHeight(int height) override;

		/// Sets the text-color of the TextList.
		void setColor(Uint8 color) override;
		/// Gets the text-color of the TextList.
		Uint8 getColor() const;
		/// Sets the secondary color of the TextList.
		void setSecondaryColor(Uint8 color) override;
		/// Gets the secondary color of the TextList.
		Uint8 getSecondaryColor() const;

		/// Sets the TextList's high-contrast setting.
		void setHighContrast(bool contrast = true);
		/// Sets the TextList's wordwrap setting.
		void setWordWrap(bool wrap = true);

		/// Sets the horizontal alignment of the TextList.
		void setAlign(
				TextHAlign align,
				int col = -1);

		/// Sets whether to add dots to rows.
		void setDot(bool dot = true);

		/// Sets whether the TextList's rows are selectable and/or highlight on mouse-overs.
		void setSelectable(bool selectable = true);

		/// Sets the font-size to big.
		void setBig();
		/// Sets the font-size to small.
		void setSmall();

		/// Sets whether to condense columns instead of a table like layout.
		void setCondensed(bool condensed);

		/// Sets the background for the selector.
		void setBackground(Surface* bg);

		/// Gets the selected row in the TextList.
		size_t getSelectedRow() const;

		/// Sets the margin of the TextList.
		void setMargin(int margin = 0);
		/// Gets the margin of the TextList.
		int getMargin() const;

		/// Sets the arrow-color of the TextList.
		void setArrowColor(Uint8 color);
		/// Sets the arrow-column of the TextList.
		void setArrowColumn(
				int pos,
				ArrowOrientation type);

		/// Hooks an action handler to a mouse-click on the left arrows.
		void onLeftArrowClick(ActionHandler handler);
		/// Hooks an action handler to a mouse-press over the left arrows.
		void onLeftArrowPress(ActionHandler handler);
		/// Hooks an action handler to a mouse-release over the left arrows.
		void onLeftArrowRelease(ActionHandler handler);
		/// Hooks an action handler to a mouse-click on the right arrows.
		void onRightArrowClick(ActionHandler handler);
		/// Hooks an action handler to a mouse-press over the right arrows.
		void onRightArrowPress(ActionHandler handler);
		/// Hooks an action handler to a mouse-release over the right arrows.
		void onRightArrowRelease(ActionHandler handler);

		/// Clears the TextList.
		void clearList();

		/// Scrolls the TextList up.
		void scrollUp(
				bool toMax = false,
				bool scrollByWheel = false);
		/// Scrolls the TextList down.
		void scrollDown(
				bool toMax = false,
				bool scrollByWheel = false);
		/// Sets the TextList scrollable.
		void setScrollable(
				bool scrollable = true,
				int scrollPos = 0);

		/// Draws the text onto the TextList.
		void draw() override;
		/// Blits the TextList onto another Surface.
		void blit(Surface* surface) override;
		/// Thinks the arrow-buttons.
		void think() override;

		/// Handles the arrow-buttons.
		void handle(Action* action, State* state) override;
		/// Special handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse-releases.
		void mouseRelease(Action* action, State* state) override;
		/// Special handling for mouse-clicks.
		void mouseClick(Action* action, State* state) override;
		/// Special handling for mouse-hovering.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse-hovering out.
		void mouseOut(Action* action, State* state) override;

		/// Gets the current scroll-depth.
		size_t getScroll();
		/// Sets the scroll-depth.
		void scrollTo(size_t scroll = 0);

		/// Attaches a button to a ComboBox.
		void setComboBox(ComboBox* box);
		/// Checks for a ComboBox.
		ComboBox* getComboBox() const;

		/// Sets the border-color.
		void setBorderColor(Uint8 color) override;
		/// Gets the border-color.
		Uint8 getScrollbarColor() const;
};

}

#endif
