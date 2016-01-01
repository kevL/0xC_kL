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

#include "ComboBox.h"

#include "TextButton.h"
#include "TextList.h"
#include "Window.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/State.h"


namespace OpenXcom
{

/**
 * Sets up a combobox with the specified size and position.
 * @param state 	- pointer to state the combobox belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
ComboBox::ComboBox(
		State* state,
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_change(0),
		_sel(0),
		_state(state),
		_lang(0),
		_toggled(false)
{
	_button = new TextButton(width,height, x,y);
	_arrow	= new Surface(
						11,
						8,
						x + width - BUTTON_WIDTH,
						y + 4);
	_window	= new Window(
						state,
						width,
						(ROWS_DEFAULT * 8 + MARGIN_VERTICAL * 2) + 1,
						x,
						y + height);
	_list	= new TextList(
						width - MARGIN_HORIZONTAL * 2 - BUTTON_WIDTH + 1,
						(ROWS_DEFAULT * TEXT_HEIGHT) + 1,
						x + MARGIN_HORIZONTAL,
						y + height + MARGIN_VERTICAL);


	_button->setComboBox(this);
	_window->setThinBorder();

	_list->setComboBox(this);
	_list->setColumns(1, _list->getWidth());
	_list->setBackground(_window);
	_list->setSelectable();
//	_list->setAlign(ALIGN_CENTER);
	_list->setScrollable();

	toggle(true);
}

/**
 * Deletes all the stuff contained in the list.
 */
ComboBox::~ComboBox()
{
	delete _button;
	delete _arrow;
	delete _window;
	delete _list;
}

/**
* Changes the position of the surface in the X axis.
* @param x - X position in pixels
*/
void ComboBox::setX(int x)
{
	Surface::setX(x);
	_button->setX(x);
	_arrow->setX(x + getWidth() - BUTTON_WIDTH);
	_window->setX(x);
	_list->setX(x + MARGIN_HORIZONTAL);
}

/**
* Changes the position of the surface in the Y axis.
* @param y - Y position in pixels
*/
void ComboBox::setY(int y)
{
	Surface::setY(y);

	_button->setY(y);
	_arrow->setY(y + 4);
	_window->setY(y + getHeight());
	_list->setY(y + getHeight() + MARGIN_VERTICAL);
}

/**
 * Replaces a certain amount of colors in the palette of all the text contained in the list.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void ComboBox::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_button->setPalette(colors, firstcolor, ncolors);
	_arrow->setPalette(colors, firstcolor, ncolors);
	_window->setPalette(colors, firstcolor, ncolors);
	_list->setPalette(colors, firstcolor, ncolors);
}

/**
 * Changes the resources for the text in the combo box.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void ComboBox::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_lang = lang;
	_button->initText(big, small, lang);
	_list->initText(big, small, lang);
}

/**
 * Changes the surface used to draw the background of the combo box.
 * @param bg - new background
 */
void ComboBox::setBackground(Surface* bg)
{
	_window->setBackground(bg);
}

/**
 * Sets the color to fill the background.
 * @note A background picture will override a fill color.
 * @param color - fill color (0 is transparent)
 */
void ComboBox::setBackgroundFill(Uint8 color)
{
	_window->setBackgroundFill(color);
}

/**
 * Changes the color used to draw the combo box.
 * @param color - color value
 */
void ComboBox::setColor(Uint8 color)
{
	_color = color;
	drawArrow();
	_button->setColor(_color);
	_window->setColor(_color);
	_list->setColor(_color);
}

/**
 * Returns the color used to draw the combo box.
 * @return, color value
 */
Uint8 ComboBox::getColor() const
{
	return _color;
}

/**
 * Draws the arrow used to indicate the combo box.
 */
void ComboBox::drawArrow() // private.
{
	_arrow->clear();

	SDL_Rect square;

	Uint8 color;
	if (_color == 255)
		color = 1;
	else
		color = _color + 1;

	// Draw arrow triangle 1
	square.x = 1;
	square.y = 2;
	square.w = 9;
	square.h = 1;

	for (
			;
			square.w > 1;
			square.w -= 2)
	{
		_arrow->drawRect(&square, color + 2);
		++square.x;
		++square.y;
	}
	_arrow->drawRect(&square, color + 2);

	// Draw arrow triangle 2
	square.x =
	square.y = 2;
	square.w = 7;
	square.h = 1;

	for (
			;
			square.w > 1;
			square.w -= 2)
	{
		_arrow->drawRect(&square, color);
		++square.x;
		++square.y;
	}

	_arrow->drawRect(&square, color);
}

/**
 * Enables/disables high contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - high contrast setting (default true)
 */
void ComboBox::setHighContrast(bool contrast)
{
	_button->setHighContrast(contrast);
	_window->setHighContrast(contrast);
	_list->setHighContrast(contrast);
}

/**
 * Changes the color of the arrow buttons in the list.
 * @param color - color value
 */
void ComboBox::setArrowColor(Uint8 color)
{
	_list->setArrowColor(color);
}

/**
 * Returns the currently selected option.
 * @return, selected row
 */
size_t ComboBox::getSelected() const
{
	return _sel;
}

/**
 * Changes the currently selected option.
 * @param sel - selected row
 */
void ComboBox::setSelected(size_t sel)
{
	_sel = sel;

	if (_sel < _list->getTexts())
		_button->setText(_list->getCellText(_sel, 0));
}

/**
 * Updates the vertical size of the dropdown list based on the number of options
 * available.
 * @param rows - quantity of rows/options
 */
void ComboBox::setDropdown(int rows) // private.
{
	int
//		rows = std::min(rows, ROWS_DEFAULT),
		h = _button->getFont()->getHeight() + _button->getFont()->getSpacing(),
		dY = (Options::baseYResolution - 200) / 2;

	while (_window->getY() + rows * h + MARGIN_VERTICAL * 2 > 200 + dY)
		--rows;

	_window->setHeight((rows * h + MARGIN_VERTICAL * 2) + 1);
	_list->setHeight((rows * h) + 1);
}

/**
 * Changes the list of available options to choose from.
 * @param options - reference to a vector of string IDs
 */
void ComboBox::setOptions(const std::vector<std::string>& options)
{
	setDropdown(options.size());
	_list->clearList();

	for (std::vector<std::string>::const_iterator
			i = options.begin();
			i != options.end();
			++i)
	{
		_list->addRow(1, _lang->getString(*i).c_str());
	}

	setSelected(_sel);
	_list->draw();
}

/**
 * Changes the list of available options to choose from.
 * @param options - reference to a vector of localized strings
 */
void ComboBox::setOptions(const std::vector<std::wstring>& options)
{
	setDropdown(options.size());
	_list->clearList();

	for (std::vector<std::wstring>::const_iterator
			i = options.begin();
			i != options.end();
			++i)
	{
		_list->addRow(1, i->c_str());
	}

	setSelected(_sel);
}

/**
 * Blits the combo box components.
 * @param surface - pointer to surface to blit onto
 */
void ComboBox::blit(Surface* surface)
{
	Surface::blit(surface);
	_list->invalidate();

	if (_visible == true && _hidden == false)
	{
		_button->blit(surface);
		_arrow->blit(surface);
		_window->blit(surface);
		_list->blit(surface);
	}
}

/**
 * Passes ticks to arrow buttons.
 */
void ComboBox::think()
{
	_button->think();
	_arrow->think();
	_window->think();
	_list->think();

	InteractiveSurface::think();
}

/**
 * Passes events to internal components.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void ComboBox::handle(Action* action, State* state)
{
	_button->handle(action, state);
	_list->handle(action, state);
	InteractiveSurface::handle(action, state);

	if (_window->getVisible() == true
		&& action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& (action->getAbsoluteXMouse() < getX()
			|| action->getAbsoluteXMouse() >= getX() + getWidth()
			|| action->getAbsoluteYMouse() < getY()
			|| action->getAbsoluteYMouse() >= getY() + getHeight() + _window->getHeight()))
	{
		toggle();
	}

	if (_toggled == true)
	{
		if (_change != nullptr)
			(state->*_change)(action);

		_toggled = false;
	}
}

/**
 * Opens/closes the combo box list.
 * @param init - true if it is the initialization toggle (default false)
 */
void ComboBox::toggle(bool init)
{
	_list->setVisible(!_list->getVisible());
	_window->setVisible(!_window->getVisible());

	InteractiveSurface* modal;
	if (_window->getVisible() == true)
		modal = this;
	else
		modal = nullptr;

	_state->setModal(modal);

	if (init == false
		&& _window->getVisible() == false)
	{
		_toggled = true;
	}

	if (_list->getVisible() == true)
	{
		if (_sel < _list->getVisibleRows() / 2)
			_list->scrollTo(0);
		else
			_list->scrollTo(_sel - _list->getVisibleRows() / 2);
	}
}

/**
 * Sets a function to be called every time the box's content changes.
 * @param handler - ActionHandler
 */
void ComboBox::onComboChange(ActionHandler handler)
{
	_change = handler;
}

}
