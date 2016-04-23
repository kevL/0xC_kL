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

#include "TextList.h"

//#include <algorithm>
//#include <cmath>
//#include <cstdarg>

#include <limits>

#include "ArrowButton.h"
#include "ComboBox.h"
#include "ScrollBar.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
//#include "../Engine/Language.h" // TEST
#include "../Engine/Options.h"
#include "../Engine/Palette.h"


namespace OpenXcom
{

/**
 * Sets up the blank TextList with a specified size and position.
 * @note A sound policy is to specify the list-height as a multiple-of-8 + 1
 * pixels (8 being the height of the small font -1px).
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
TextList::TextList(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width, height,
			x,y),
		_big(nullptr),
		_small(nullptr),
		_font(nullptr),
		_scroll(0u),
		_visibleRows(0u),
		_selRow(0u),
		_color(0u),
		_color2(0u),
		_dot(false),
		_selectable(false),
		_condensed(false),
		_contrast(false),
		_wrap(false),
		_bg(nullptr),
		_selector(nullptr),
		_margin(8),
		_scrollable(true),
		_arrowPos(-1),
		_scrollPos(0),
		_arrowType(ARROW_VERTICAL),
		_leftClick(nullptr),
		_leftPress(nullptr),
		_leftRelease(nullptr),
		_rightClick(nullptr),
		_rightPress(nullptr),
		_rightRelease(nullptr),
		_arrowsLeftEdge(0),
		_arrowsRightEdge(0),
		_comboBox(nullptr)
{
	_up = new ArrowButton(
						ARROW_BIG_UP,
						13,13,
						getX() + getWidth() + _scrollPos,
						getY());
	_up->setVisible(false);
	_up->setTextList(this);

	_down = new ArrowButton(
						ARROW_BIG_DOWN,
						13,13,
						getX() + getWidth() + _scrollPos,
						getY() + getHeight() - 12);
	_down->setVisible(false);
	_down->setTextList(this);

	const int h (std::max(1,
						 _down->getY() - _up->getY() - _up->getHeight()));
	_scrollbar = new ScrollBar(
							_up->getWidth(),
							h,
							getX() + getWidth() + _scrollPos,
							_up->getY() + _up->getHeight());
	_scrollbar->setVisible(false);
	_scrollbar->setTextList(this);
}

/**
 * Deletes all the stuff contained in this TextList.
 */
TextList::~TextList()
{
	for (std::vector<std::vector<Text*>>::const_iterator
			i = _texts.begin();
			i != _texts.end();
			++i)
		for (std::vector<Text*>::const_iterator
				j = (*i).begin();
				j != (*i).end();
				++j)
			delete *j;

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		delete *i;

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		delete *i;

	delete _selector;
	delete _up;
	delete _down;
	delete _scrollbar;
}

/**
 * Sets the position of the surface in the x-axis.
 * @param x - x-position in pixels
 */
void TextList::setX(int x)
{
	Surface::setX(x);

	_up->setX(getX() + getWidth() + _scrollPos);
	_down->setX(getX() + getWidth() + _scrollPos);
	_scrollbar->setX(getX() + getWidth() + _scrollPos);

	if (_selector != nullptr)
		_selector->setX(getX());
}

/**
 * Sets the position of the surface in the y-axis.
 * @param y - y-position in pixels
 */
void TextList::setY(int y)
{
	Surface::setY(y);

	_up->setY(getY());
	_down->setY(getY() + getHeight() - 12);
	_scrollbar->setY(_up->getY() + _up->getHeight());

	if (_selector != nullptr)
		_selector->setY(getY());
}

/**
 * Gets the ArrowButtons' left-edge.
 * @return, left edge in pixels
 */
int TextList::getArrowsLeftEdge()
{
	return _arrowsLeftEdge;
}

/**
 * Gets the ArrowButtons' right-edge.
 * @return, right edge in pixels
 */
int TextList::getArrowsRightEdge()
{
	return _arrowsRightEdge;
}

/**
 * Unpresses all the arrow-buttons.
 * @param state - pointer to running state
 */
void TextList::unpress(State* state)
{
	InteractiveSurface::unpress(state);

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->unpress(state);

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->unpress(state);
}

/**
 * Sets the color of a specific Text object in this TextList.
 * @param row		- row number
 * @param column	- column number
 * @param color		- text color
 * @param contrast	- true for high-contrast (default false)
 */
void TextList::setCellColor(
		size_t row,
		size_t column,
		Uint8 color,
		bool contrast)
{
	_texts[row][column]->setColor(color);

	if (_texts[row][column]->getHighContrast() != contrast)
		_texts[row][column]->setHighContrast(contrast);

	_redraw = true;
}

/**
 * Sets the text-color of a whole row in this TextList.
 * @param row		- row number
 * @param color		- text color
 * @param contrast	- true for high-contrast (default false)
 */
void TextList::setRowColor(
		size_t row,
		Uint8 color,
		bool contrast)
{
	for (std::vector<Text*>::const_iterator
			i = _texts[row].begin();
			i != _texts[row].end();
			++i)
	{
		(*i)->setColor(color);

		if ((*i)->getHighContrast() != contrast)
			(*i)->setHighContrast(contrast);
	}
	_redraw = true;
}

/**
 * Gets the text of a specific Text object in this TextList.
 * @param row		- row number
 * @param column	- column number
 * @return, text string
 */
std::wstring TextList::getCellText(
		size_t row,
		size_t column) const
{
	return _texts[row][column]->getText();
}

/**
 * Sets the text of a specific Text object in this TextList.
 * @param row		- row number
 * @param column	- column number
 * @param text		- text string
 */
void TextList::setCellText(
		size_t row,
		size_t column,
		const std::wstring& text)
{
	_texts[row][column]->setText(text);
	_redraw = true;
}

/**
 * Gets the x-position of a specific text column in this TextList.
 * @param column - column number
 * @return, x-position in pixels
 */
int TextList::getColumnX(size_t column) const
{
	return getX() + _texts[0][column]->getX();
}

/**
 * Gets the y-position of a specific text-row in this TextList.
 * @param row - row number
 * @return, y-position in pixels
 */
int TextList::getRowY(size_t row) const
{
	return getY() + _texts[row][0u]->getY();
}

/**
 * Gets the height of a specific text-row in this TextList.
 * @param row - row number
 * @return, height in pixels
 *
int TextList::getTextHeight(size_t row) const // myk002
{
	return _texts[row].front()->getTextHeight();
} */

/**
 * Gets the number of lines of a specific text-row in this TextList.
 * @param row - row number
 * @return, number of lines
 *
int TextList::getNumTextLines(size_t row) const // myk002
{
	return _texts[row].front()->getNumLines();
} */

/**
 * Gets the quantity of text-rows stored in this TextList.
 * @return, number of rows
 */
size_t TextList::getTextsQuantity() const
{
	return _texts.size();
}

/**
 * Gets the quantity of physical rows stored in this TextList.
 * @return, number of rows
 */
size_t TextList::getRows() const
{
	return _rows.size();
}

/**
 * Gets the quantity of visible rows stored in this TextList.
 * @return, number of rows
 */
size_t TextList::getVisibleRows() const
{
	return _visibleRows;
}

/**
 * Adds a row of text to this TextList automatically creating the required
 * Text-objects lined up where they need to be.
 * @param cols	- quantity of columns
 * @param ...	- text for each cell in the new row
 */
void TextList::addRow(
		int cols,
		...)
{
	va_list args; // typedef char*
	va_start(args, cols); // avoid g++ compiler warnings.

	int ncols;
	if (cols != 0)
		ncols = cols;
	else
		ncols = 1;

	std::vector<Text*> txtRow;
	int
		rowOffset_x	(0), // x/y-values are relative to the TextList's Surface
		rowOffset_y,
		qtyRows		(1),
		rowHeight	(0);

	if (_texts.empty() == false)
		rowOffset_y = _texts.back().front()->getY()
					+ _texts.back().front()->getHeight()
					+ _font->getSpacing();
	else
		rowOffset_y = 0;

	for (size_t
			i = 0u;
			i != static_cast<size_t>(ncols);
			++i)
	{
		Text* const txt (new Text(
								_columns[i],
								_font->getHeight(),
								rowOffset_x + _margin,
								rowOffset_y));

		txt->setPalette(this->getPalette());
		txt->initText(_big, _small, _lang);
		txt->setColor(_color);
		txt->setSecondaryColor(_color2);

		if (_align[i] != ALIGN_LEFT)
			txt->setAlign(_align[i]);

		txt->setHighContrast(_contrast);

		if (_font == _big)	txt->setBig();
		else				txt->setSmall();

		if (cols != 0)
			txt->setText(va_arg(args, wchar_t*));
//		else
//		{
//			wchar_t d[1];
//			std::mbstowcs(d, "", 1);
//			txt->setText(d);
//		}

		// Grab this before enabling word-wrap so it can be used to calculate the total row-height below.
		const int vertPad (_font->getHeight() - txt->getTextHeight());

		if (_wrap == true && txt->getTextWidth() > txt->getWidth())
		{
//			txt->setHeight(_font->getHeight() * 2 + _font->getSpacing());
			txt->setWordWrap(true, true);
			qtyRows = std::max(qtyRows,
							   txt->getNumLines());
		}

		rowHeight = std::max(rowHeight,
							 txt->getTextHeight() + vertPad);

		if (_dot == true
			&& cols != 0 && i < static_cast<size_t>(cols) - 1u)
		{
			std::wstring buf (txt->getText());

			size_t width (txt->getTextWidth());
			while (width < _columns[i])
			{
				width += static_cast<size_t>(static_cast<int>(_font->getChar('.')->getCrop()->w) + _font->getSpacing());
				buf += '.';
			}
			txt->setText(buf);
		}

		txtRow.push_back(txt);

		if (_condensed == true)
			rowOffset_x += txt->getTextWidth();
		else
			rowOffset_x += static_cast<int>(_columns[i]);
	}

	for (size_t // ensure all elements in this row are the same height
			i = 0u;
			i != static_cast<size_t>(cols);
			++i)
		txtRow[i]->setHeight(rowHeight);

	_texts.push_back(txtRow);
	for (int
			i = 0;
			i != qtyRows;
			++i)
		_rows.push_back(_texts.size() - 1u);


	if (_arrowPos != -1)	// place arrow-buttons
	{						// Position defined wrt main window - *not* the TextList's Surface.
		ArrowShape
			shape1,
			shape2;

		switch (_arrowType)
		{
			case ARROW_VERTICAL:
				shape1 = ARROW_SMALL_UP;
				shape2 = ARROW_SMALL_DOWN;
				break;

			default:
			case ARROW_HORIZONTAL:
				shape1 = ARROW_SMALL_LEFT;
				shape2 = ARROW_SMALL_RIGHT;
		}

		ArrowButton* const a1 (new ArrowButton(
											shape1,
											11,8,
											getX() + _arrowPos,
											getY()));
		a1->setListButton();
		a1->setPalette(this->getPalette());
		a1->setColor(_up->getColor());
		a1->onMouseClick(_leftClick, 0);
		a1->onMousePress(_leftPress);
		a1->onMouseRelease(_leftRelease);
		_arrowLeft.push_back(a1);

		ArrowButton* const a2 (new ArrowButton(
											shape2,
											11,8,
											getX() + _arrowPos + 12,
											getY()));
		a2->setListButton();
		a2->setPalette(this->getPalette());
		a2->setColor(_up->getColor());
		a2->onMouseClick(_rightClick, 0);
		a2->onMousePress(_rightPress);
		a2->onMouseRelease(_rightRelease);
		_arrowRight.push_back(a2);
	}

	_redraw = true;

	va_end(args);
	updateArrows();
}

/**
 * Sets the columns that this TextList contains.
 * @note While rows can be unlimited, columns need to be specified since they
 * can have various widths for lining up the text.
 * @param cols	- number of columns
 * @param ...	- width of each column
 */
void TextList::setColumns(
		int cols,
		...)
{
	va_list args;
	va_start(args, cols);

	for (int
			i = 0;
			i != cols;
			++i)
		_columns.push_back(va_arg(args, int));

	va_end(args);
}

/**
 * Replaces a certain quantity of colors in the palette of all the text contained
 * in this TextList.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- quantity of colors to replace (default 256)
 */
void TextList::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	for (std::vector<std::vector<Text*>>::const_iterator
			i = _texts.begin();
			i != _texts.end();
			++i)
		for (std::vector<Text*>::const_iterator
				j = i->begin();
				j != i->end();
				++j)
			(*j)->setPalette(colors, firstcolor, ncolors);

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->setPalette(colors, firstcolor, ncolors);

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->setPalette(colors, firstcolor, ncolors);

	if (_selector != nullptr)
		_selector->setPalette(colors, firstcolor, ncolors);

	_up->setPalette(colors, firstcolor, ncolors);
	_down->setPalette(colors, firstcolor, ncolors);
	_scrollbar->setPalette(colors, firstcolor, ncolors);
}

/**
 * Changes the resources for the text in this TextList and calculates the selector
 * and visible quantity of rows.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void TextList::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_big = big;
	_small =
	_font = small;
	_lang = lang;

	delete _selector;

	_selector = new Surface(
						getWidth(),
						_font->getHeight() + _font->getSpacing(),
						getX(),
						getY());
	_selector->setPalette(getPalette());
	_selector->setVisible(false);

	updateVisible();
}

/**
 * Sets the height of the TextList.
 * @param height - new height in pixels
 */
void TextList::setHeight(int height)
{
	Surface::setHeight(height);

	setY(getY());

	const int h = std::max(1, _down->getY() - _up->getY() - _up->getHeight());
	_scrollbar->setHeight(h);

	updateVisible();
}

/**
 * Sets the color of the text in this TextList.
 * @note This doesn't change the color of existing text - just the color of text
 * added from then on.
 * @param color - color value
 */
void TextList::setColor(Uint8 color)
{
	_color = color;

	_up->setColor(color);
	_down->setColor(color);
	_scrollbar->setColor(color);

	for (std::vector<std::vector<Text*>>::const_iterator
			i = _texts.begin();
			i != _texts.end();
			++i)
		for (std::vector<Text*>::const_iterator
				j = i->begin();
				j != i->end();
				++j)
			(*j)->setColor(color);
}

/**
 * Gets the color of the text in this TextList.
 * @return, color value
 */
Uint8 TextList::getColor() const
{
	return _color;
}

/**
 * Sets the secondary color of the text in this TextList.
 * @param color - color value
 */
void TextList::setSecondaryColor(Uint8 color)
{
	_color2 = color;
}

/**
 * Gets the secondary color of the text in this TextList.
 * @return, color value
 */
Uint8 TextList::getSecondaryColor() const
{
	return _color2;
}

/**
 * Enables/disables high-contrast color. Mostly used for Battlescape text.
 * @param contrast - high-contrast setting (default true)
 */
void TextList::setHighContrast(bool contrast)
{
	_contrast = contrast;

	for (std::vector<std::vector<Text*>>::const_iterator
			i = _texts.begin();
			i != _texts.end();
			++i)
		for (std::vector<Text*>::const_iterator
				j = i->begin();
				j != i->end();
				++j)
			(*j)->setHighContrast(contrast);

	_scrollbar->setHighContrast(contrast);
}

/**
 * Enables/disables text wordwrapping.
 * @note When enabled rows can take up multiple lines of the list otherwise
 * every row is restricted to one line.
 * @param wrap - wordwrap setting (default true)
 */
void TextList::setWordWrap(bool wrap)
{
	_wrap = wrap;
}

/**
 * Sets the horizontal alignment of the text in this TextList.
 * @note This doesn't change the alignment of existing text just the alignment
 * of text added from then on.
 * @param align	- horizontal alignment
 * @param col	- the column to set the alignment for (defaults to -1 meaning "all")
 */
void TextList::setAlign(
		TextHAlign align,
		int col)
{
	if (col == -1)
	{
		for (size_t
				i = 0u;
				i != _columns.size();
				++i)
			_align[i] = align;
	}
	else
		_align[static_cast<size_t>(col)] = align;
}

/**
 * If enabled the text in different columns will be separated by dots.
 * @note Otherwise it will only be separated by blank space.
 * @param dot - true for dots (default true)
 */
void TextList::setDot(bool dot)
{
	_dot = dot;
}

/**
 * If enabled this TextList will respond to player input highlighting selected rows
 * and receiving clicks.
 * @param selectable - selectable setting (default true)
 */
void TextList::setSelectable(bool selectable)
{
	_selectable = selectable;
}

/**
 * Sets the TextList to use the big-size font.
 */
void TextList::setBig()
{
	_font = _big;
	delete _selector;
	_selector = new Surface(
						getWidth(),
						_font->getHeight() + _font->getSpacing(),
						getX(),
						getY());
	_selector->setPalette(getPalette());
	_selector->setVisible(false);
	updateVisible();
}

/**
 * Sets the TextList to use the small-size font.
 */
void TextList::setSmall()
{
	_font = _small;
	delete _selector;
	_selector = new Surface(
						getWidth(),
						_font->getHeight() + _font->getSpacing(),
						getX(),
						getY());
	_selector->setPalette(getPalette());
	_selector->setVisible(false);
	updateVisible();
}

/**
 * If enabled, the columns will match the text width.
 * Otherwise, they will have a fixed width.
 * @param condensed - true for condensed layout, false for table layout
 */
void TextList::setCondensed(bool condensed)
{
	_condensed = condensed;
}

/**
 * Gets the currently selected row if the TextList is selectable.
 * @return, selected row (-1 if none)
 */
size_t TextList::getSelectedRow() const
{
	if (_rows.empty() == true || _selRow > _rows.size() - 1u)
		return std::numeric_limits<size_t>::max();

	return _rows[_selRow];
}

/**
 * Sets the surface used to draw the background of the selector.
 * @param bg - new background
 */
void TextList::setBackground(Surface* bg)
{
	_bg = bg;
	_scrollbar->setBackground(_bg);
}

/**
 * Sets the horizontal margin placed around the text.
 * @param margin - margin in pixels (default 0)
 */
void TextList::setMargin(int margin)
{
	_margin = margin;
}

/**
 * Gets the margin of the text in this TextList.
 * @return, margin in pixels
 */
int TextList::getMargin() const
{
	return _margin;
}

/**
 * Sets the color of the arrow-buttons in this TextList.
 * @param color - color value
 */
void TextList::setArrowColor(Uint8 color)
{
	_up->setColor(color);
	_down->setColor(color);
	_scrollbar->setColor(color);
}

/**
 * Sets the position of the column of arrow-buttons in the TextList.
 * @param pos	- X in pixels (-1 to disable)
 * @param type	- arrow orientation type
 */
void TextList::setArrowColumn(
		int pos,
		ArrowOrientation type)
{
	_arrowPos = pos;
	_arrowType = type;
	_arrowsLeftEdge = getX() + _arrowPos;
	_arrowsRightEdge = _arrowsLeftEdge + 12 + 11;
}

/**
 * Sets a function to be called every time the left arrows are mouse clicked.
 * @param handler - action handler
 */
void TextList::onLeftArrowClick(ActionHandler handler)
{
	_leftClick = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->onMouseClick(handler, 0u);
}

/**
 * Sets a function to be called every time the left arrows are mouse pressed.
 * @param handler - action handler
 */
void TextList::onLeftArrowPress(ActionHandler handler)
{
	_leftPress = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->onMousePress(handler);
}

/**
 * Sets a function to be called every time the left arrows are mouse released.
 * @param handler - action handler
 */
void TextList::onLeftArrowRelease(ActionHandler handler)
{
	_leftRelease = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->onMouseRelease(handler);
}

/**
 * Sets a function to be called every time the right arrows are mouse clicked.
 * @param handler - action handler
 */
void TextList::onRightArrowClick(ActionHandler handler)
{
	_rightClick = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->onMouseClick(handler, 0u);
}

/**
 * Sets a function to be called every time the right arrows are mouse pressed.
 * @param handler - action handler
 */
void TextList::onRightArrowPress(ActionHandler handler)
{
	_rightPress = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->onMousePress(handler);
}

/**
 * Sets a function to be called every time the right arrows are mouse released.
 * @param handler - action handler
 */
void TextList::onRightArrowRelease(ActionHandler handler)
{
	_rightRelease = handler;
	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->onMouseRelease(handler);
}

/**
 * Removes all the rows currently stored in this TextList.
 */
void TextList::clearList()
{
	for (std::vector<std::vector<Text*>>::iterator
			i = _texts.begin();
			i != _texts.end();
			++i)
	{
		for (std::vector<Text*>::const_iterator
				j = i->begin();
				j != i->end();
				++j)
			delete *j;

		i->clear();
	}

	scrollUp(true);

	_texts.clear();
	_rows.clear();

	_redraw = true;
}

/**
 * Scrolls the text in this TextList up by one row or to the top.
 * @param full	- true scrolls to the top of the list (default false)
 * @param wheel	- true uses wheel-scroll (default false)
 */
void TextList::scrollUp(
		bool full,
		bool wheel)
{
	if (_scrollable == true
		&& _scroll > 0u && _rows.size() > _visibleRows)
	{
		if (full == true)
			scrollTo();
		else if (wheel == true)
			scrollTo(_scroll - std::min(_scroll,
										static_cast<size_t>(Options::mousewheelSpeed)));
		else
			scrollTo(_scroll - 1u);
	}
}

/**
 * Scrolls the text in this TextList down by one row or to the bottom.
 * @param full	- true to scroll to the bottom of the list (default false)
 * @param wheel	- true uses wheel-scroll (default false)
 */
void TextList::scrollDown(
		bool full,
		bool wheel)
{
	if (_scrollable == true
		&& _rows.size() > _visibleRows && _scroll < _rows.size() - _visibleRows)
	{
		if (full == true)
			scrollTo(_rows.size() - _visibleRows);
		else if (wheel == true)
			scrollTo(_scroll + static_cast<size_t>(Options::mousewheelSpeed));
		else
			scrollTo(_scroll + 1u);
	}
}

/**
 * Sets whether this TextList can be scrolled.
 * @param scrolling - true allows scrolling (default true)
 * @param scrollPos - custom +/- x_offset for the scroll buttons (default 0)
 */
void TextList::setScrollable(
		bool scrollable,
		int scrollPos)
{
	_scrollable = scrollable;

	if (_scrollPos != scrollPos)
	{
		_scrollPos = scrollPos;
		_up->setX(getX() + getWidth() + _scrollPos);
		_down->setX(getX() + getWidth() + _scrollPos);
		_scrollbar->setX(getX() + getWidth() + _scrollPos);
	}
}

/**
 * Updates the visibility of the arrow-buttons according to the current
 * scroll-position.
 */
void TextList::updateArrows() // private.
{
	_up->setVisible(_rows.size() > _visibleRows); //&& _scroll > 0);
	_down->setVisible(_rows.size() > _visibleRows); //&& _scroll < _rows.size() - _visibleRows);

	_scrollbar->setVisible(_rows.size() > _visibleRows);
	_scrollbar->invalidate();
	_scrollbar->blit(this);
}

/**
 * Updates the quantity of visible rows according to the current list and
 * font-size.
 */
void TextList::updateVisible() // private.
{
	_visibleRows = 0u;

	const int delta_y (_font->getHeight() + _font->getSpacing());
	for (int
			y = 0;
			y < getHeight();
			y += delta_y)
	{
		++_visibleRows;
	}

	if (getHeight() > static_cast<int>(_visibleRows - 1u) * delta_y)
		--_visibleRows;

	updateArrows();
}

/**
 * Draws the TextList and all the text contained within.
 */
void TextList::draw()
{
	Surface::draw();

	int y (0);
	bool addPixel (false);

	if (_rows.empty() == false)
	{
		// for wrapped items offset the draw height above the visible surface
		// so that the correct row appears at the top
		for (size_t
				row = _scroll;
				row != 0u && _rows[row] == _rows[row - 1u];
				--row)
		{
			y -= _font->getHeight() + _font->getSpacing();
		}

		for (size_t
				i = _rows[_scroll];
				i != _texts.size() && i != _rows[_scroll] + _visibleRows;
				++i)
		{
			if (i == _texts.size() - 1u || i == _rows[_scroll] + _visibleRows - 1u)
				addPixel = true; // add px_Y under last row

			for (std::vector<Text*>::const_iterator
					j = _texts[i].begin();
					j != _texts[i].end();
					++j)
			{
				if (addPixel == true)
					(*j)->addTextHeight();

				(*j)->setY(y);
				(*j)->blit(this);
			}

			if (_texts[i].empty() == false)
				y += _texts[i].front()->getHeight() + _font->getSpacing();
			else
				y += _font->getHeight() + _font->getSpacing();
		}
	}
}

/**
 * Blits the TextList and selector.
 * @param srf - pointer to a Surface to blit to
 */
void TextList::blit(const Surface* const srf)
{
	if (_visible == true && _hidden == false)
		_selector->blit(srf);

	Surface::blit(srf);

	if (_visible == true && _hidden == false)
	{
		if (_arrowPos != -1 && _rows.empty() == false)
		{
			int y (getY());
			for (size_t
					row = _scroll;
					row != 0u && _rows[row] == _rows[row - 1u];
					--row)
			{
				y -= _font->getHeight() + _font->getSpacing();
			}

			int maxY (getY() + getHeight());
			for (size_t
					i = _rows[_scroll];
					i != _texts.size() && i != _rows[_scroll] + _visibleRows && y < maxY;
					++i)
			{
				_arrowLeft[i]->setY(y);
				_arrowRight[i]->setY(y);

				if (y >= getY()) // only blit arrows that belong to texts that have their first row on-screen
				{
					_arrowLeft[i]->blit(srf);
					_arrowRight[i]->blit(srf);
				}

				if (_texts[i].empty() == false)
					y += _texts[i].front()->getHeight() + _font->getSpacing();
				else
					y += _font->getHeight() + _font->getSpacing();
			}
		}

		_up->blit(srf);
		_down->blit(srf);
		_scrollbar->blit(srf);
	}
}

/**
 * Passes events to arrow-buttons.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void TextList::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state);

	_up->handle(action, state);
	_down->handle(action, state);
	_scrollbar->handle(action, state);

	if (_arrowPos != -1 && _rows.empty() == false)
	{
		size_t startId (_rows[_scroll]);
		if (_scroll > 0u && _rows[_scroll] == _rows[_scroll - 1u])
			++startId; // arrows for first partly-visible line of text are offscreen - so don't process them

		size_t
			endId (_rows[_scroll] + 1u),
			endRow (std::min(_rows.size(),
							 _scroll + _visibleRows));

		for (size_t
				i = _scroll + 1u;
				i != endRow;
				++i)
		{
			if (_rows[i] != _rows[i - 1u])
				++endId;
		}

		for (size_t
				i = startId;
				i != endId;
				++i)
		{
			_arrowLeft[i]->handle(action, state);
			_arrowRight[i]->handle(action, state);
		}
	}
}

/**
 * Passes ticks to ArrowButton's and ScrollBar.
 */
void TextList::think()
{
	InteractiveSurface::think();

	_up->think();
	_down->think();
	_scrollbar->think();

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowLeft.begin();
			i != _arrowLeft.end();
			++i)
		(*i)->think();

	for (std::vector<ArrowButton*>::const_iterator
			i = _arrowRight.begin();
			i != _arrowRight.end();
			++i)
		(*i)->think();
}

/**
 * Ignores any mouse clicks that aren't on a row.
 * @param action	- pointer to an Action
 * @param state		- state that the ActionHandlers belong to
 */
void TextList::mousePress(Action* action, State* state)
{
	bool allowScroll;
	if (Options::changeValueByMouseWheel == true)
	{
		allowScroll = (action->getAbsoluteMouseX() < _arrowsLeftEdge
					|| action->getAbsoluteMouseX() > _arrowsRightEdge);
	}
	else
		allowScroll = true;

	if (allowScroll == true)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_WHEELUP:
				scrollUp(false, true);
				break;

			case SDL_BUTTON_WHEELDOWN:
				scrollDown(false, true);
		}
	}

	if (_selectable == true)
	{
		if (_selRow < _rows.size())
			InteractiveSurface::mousePress(action, state);
	}
	else
		InteractiveSurface::mousePress(action, state);
}

/**
 * Ignores any mouse clicks that aren't on a row.
 * @param action	- pointer to an Action
 * @param state		- state that the ActionHandlers belong to
 */
void TextList::mouseRelease(Action* action, State* state)
{
	if (_selectable == true)
	{
		if (_selRow < _rows.size())
			InteractiveSurface::mouseRelease(action, state);
	}
	else
		InteractiveSurface::mouseRelease(action, state);
}

/**
 * Ignores any mouse clicks that aren't on a row.
 * @param action	- pointer to an Action
 * @param state		- state that the ActionHandlers belong to
 */
void TextList::mouseClick(Action* action, State* state)
{
	if (_selectable == true)
	{
		if (_selRow < _rows.size())
		{
			InteractiveSurface::mouseClick(action, state);

			if (_comboBox != nullptr
				&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
			{
				_comboBox->setSelected(_selRow);
				_comboBox->toggle();
			}
		}
	}
	else
		InteractiveSurface::mouseClick(action, state);
}

/**
 * Selects the row the mouse is over.
 * @param action	- pointer to an Action
 * @param state		- state that the ActionHandlers belong to
 */
void TextList::mouseOver(Action* action, State* state)
{
	if (_selectable == true)
	{
		int h (_font->getHeight() + _font->getSpacing());
		_selRow = std::max(0,
						   static_cast<int>(_scroll)
						 + static_cast<int>(std::floor(action->getRelativeMouseY()
						/ (static_cast<double>(h) * action->getScaleY()))));

		if (_selRow < _texts.size()
			&& _selRow < _scroll + _visibleRows
			&& _texts[_selRow][0u]->getText().empty() == false)	// kL_add. Don't highlight rows w/out text in first column.
		{														// This is currently only a special case in Battlescape/CeremonyState(cTor)
																// due to the quirky way it adds titleRows, then lists soldierNames & Awards
																// and finally fills the titleRow w/ the relevant awardName; the last titleRow
																// is added, but there are no soldiers nor awards for it.
			//Log(LOG_INFO) << ". text at [" << _selRow << "] = " << Language::wstrToCp(_texts[_selRow][0]->getText());
			const Text* const selText (_texts[_rows[_selRow]].front());
			int y (getY() + selText->getY());
			h = selText->getHeight() + _font->getSpacing();

			if (y < getY() || y + h > getY() + getHeight())
				h /= 2;

			if (y < getY()) y = getY();

			if (_selector->getHeight() != h)
			{
				delete _selector; // resizing doesn't work, but recreating does. Make it so!
				_selector = new Surface(
									getWidth(),
									h,
									getX(),
									y);
				_selector->setPalette(getPalette());
			}

			_selector->setY(y);
			_selector->copy(_bg);

			if (_contrast == true)
				_selector->offsetBlock(-5);
			else if (_comboBox != nullptr)
				_selector->offset(1, Palette::PAL_bgID);
			else
				_selector->offsetBlock(-10);

			_selector->setVisible();
		}
		else
			_selector->setVisible(false);
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the row.
 * @param action	- pointer to an Action
 * @param state		- state that the ActionHandlers belong to
 */
void TextList::mouseOut(Action* action, State* state)
{
	if (_selectable == true)
		_selector->setVisible(false);

	InteractiveSurface::mouseOut(action, state);
}

/**
 * Gets the scroll-depth.
 * @return, scroll-depth
 */
size_t TextList::getScroll()
{
	return _scroll;
}

/**
 * Sets the scroll-depth.
 * @param scroll - scroll-depth (default 0)
 */
void TextList::scrollTo(size_t scroll)
{
	if (_scrollable == true)
	{
		_scroll = static_cast<size_t>(std::max(0, // super-safety and it works.
											   std::min(static_cast<int>(_rows.size() - _visibleRows),
														static_cast<int>(scroll))));

		draw(); // can't just set '_redraw' here because Reasons!
		updateArrows();
	}
}

/**
 * Hooks up the button to work as part of an existing ComboBox updating the
 * selection when it's pressed.
 * @param box - pointer to a ComboBox
 */
void TextList::setComboBox(ComboBox* box)
{
	_comboBox = box;
}

/**
 * Gets the ComboBox that this TextList is attached to if any.
 * @return, pointer to the associated ComboBox
 */
ComboBox* TextList::getComboBox() const
{
	return _comboBox;
}

/**
 * Sets the border-color.
 * @param color - border-color
 */
void TextList::setBorderColor(Uint8 color)
{
	_up->setColor(color);
	_down->setColor(color);
	_scrollbar->setColor(color);
}

/**
 * Gets the scrollbar-color.
 * @return, scrollbar-color
 */
Uint8 TextList::getScrollbarColor() const
{
	return _scrollbar->getColor();
}

}
