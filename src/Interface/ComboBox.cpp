/*
 * Copyright 2010-2020 OpenXcom Developers.
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

//#include <algorithm>

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
 * Sets up the ComboBox with the specified size and position.
 * @param state 	- pointer to state the combobox belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 * @param extend	- extra pixels to extend window in x-direction (default 0)
 */
ComboBox::ComboBox(
		State* const state,
		int width,
		int height,
		int x,
		int y,
		int extend)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_change(nullptr),
		_sel(0u),
		_state(state),
		_lang(nullptr),
		_toggled(false),
		_color(0u)
{
	_button = new TextButton(width,height, x,y);
	_arrow	= new Surface(
						11,
						8,
						x + width - BUTTON_WIDTH,
						y + 4);
	_window	= new Window(
						state,
						width + extend,
						((ROWS_DEFAULT * TEXT_HEIGHT) + (MARGIN_VERTICAL << 1u)) + 1,
						x,
						y + height);
	_list	= new TextList(
						width - (MARGIN_HORIZONTAL << 1u) - BUTTON_WIDTH + 3 + extend,
						(ROWS_DEFAULT * TEXT_HEIGHT) + 1,
						x + MARGIN_HORIZONTAL,
						y + height + MARGIN_VERTICAL);


	_button->setComboBox(this);
	_window->setThinBorder();

	_list->setComboBox(this);
	_list->setColumns(1, _list->getWidth());
	_list->setBackground(_window);
	_list->setSelectable();
	_list->setMargin();

	toggleCbx(true);
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
 * Changes the position of the surface in the x-axis.
 * @param x - x-position in pixels
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
 * Changes the position of the surface in the y-axis.
 * @param y - y-position in pixels
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
 * Replaces a specified quantity of colors in the palette of all the text contained in the list.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void ComboBox::setPalette(
		SDL_Color* const colors,
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
 * Changes the resources for the text in this ComboBox.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void ComboBox::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_lang = lang;
	_button->initText(big, small, lang);
	_list->initText(big, small, lang);
}

/**
 * Changes the surface used to draw the background of this ComboBox.
 * @param bg - new background
 */
void ComboBox::setBackground(Surface* const bg)
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
 * Changes the color used to draw this ComboBox.
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
 * Gets the color used to draw this ComboBox.
 * @return, color value
 */
Uint8 ComboBox::getColor() const
{
	return _color;
}

/**
 * Draws the arrow used to indicate this ComboBox.
 */
void ComboBox::drawArrow() // private.
{
	_arrow->clear();

	Uint8 color;
	switch (_color)
	{
		case 255u: color = 1u; break; // TODO: Not sure that is necessary.
		default:   color = static_cast<Uint8>(_color + 1u);
	}

	SDL_Rect rect;

	rect.x = 1; // draw arrow triangle 1
	rect.y = 2;
	rect.w = 9u;
	rect.h = 1u;

	for (
			;
			rect.w > 1u;
			rect.w = static_cast<Uint16>(rect.w - 2u))
	{
		_arrow->drawRect(&rect, static_cast<Uint8>(color + 2u));
		++rect.x;
		++rect.y;
	}
	_arrow->drawRect(&rect, static_cast<Uint8>(color + 2u));

	rect.x = // draw arrow triangle 2
	rect.y = 2;
	rect.w = 7u;
	rect.h = 1u;

	for (
			;
			rect.w > 1u;
			rect.w = static_cast<Uint16>(rect.w - 2u))
	{
		_arrow->drawRect(&rect, color);
		++rect.x;
		++rect.y;
	}

	_arrow->drawRect(&rect, color);
}

/**
 * Enables/disables high-contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - high-contrast setting (default true)
 */
void ComboBox::setHighContrast(bool contrast)
{
	_button->setHighContrast(contrast);
	_window->setHighContrast(contrast);
	_list->setHighContrast(contrast);
}

/**
 * Changes the color of the arrow-buttons.
 * @param color - color value
 */
void ComboBox::setArrowColor(Uint8 color)
{
	_list->setArrowColor(color);
}

/**
 * Gets the currently selected option.
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
	if ((_sel = sel) < _list->getTextsQuantity())
		_button->setText(_list->getCellText(_sel, 0u));
}

/**
 * Updates the vertical size of the dropdown list based on the number of options
 * available.
 * @param rows - quantity of rows/options
 */
void ComboBox::setDropdown(int rows) // private.
{
//	rows = std::min(rows, ROWS_DEFAULT);
	const int
		h (_button->getFont()->getHeight() + _button->getFont()->getSpacing()),
		dY ((Options::baseYResolution - 200) >> 1u);

	while (_window->getY() + rows * h + (MARGIN_VERTICAL << 1u) > 200 + dY)
		--rows;

	_window->setHeight((rows * h + (MARGIN_VERTICAL << 1u)) + 1);
	_list->setHeight((rows * h) + 1);
}

/**
 * Changes the list of available options to choose from.
 * @param options - reference to a vector of string-IDs
 */
void ComboBox::setOptions(const std::vector<std::string>& options)
{
	setDropdown(static_cast<int>(options.size()));
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
	setDropdown(static_cast<int>(options.size()));
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
 * Blits this ComboBox's components.
 * @param srf - pointer to a Surface to blit to
 */
void ComboBox::blit(const Surface* const srf)
{
	Surface::blit(srf);

	_list->invalidate();

	if (_visible == true && _hidden == false)
	{
		_button->blit(srf);
		_arrow->blit(srf);
		_window->blit(srf);
		_list->blit(srf);
	}
}

/**
 * Passes ticks to the arrow-buttons.
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
 * @param state		- State that the ActionHandlers belong to
 */
void ComboBox::handle(Action* action, State* state)
{
	_button->handle(action, state);
	_list->handle(action, state);
	InteractiveSurface::handle(action, state);

	if (_window->getVisible() == true
		&& action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& (   action->getAbsoluteMouseX() <  getX()
			|| action->getAbsoluteMouseX() >= getX() /*+ getWidth()*/ + _window->getWidth() // TODO: Carve out the top-right bit.
			|| action->getAbsoluteMouseY() <  getY()
			|| action->getAbsoluteMouseY() >= getY() + getHeight() + _window->getHeight()))
	{
		toggleCbx();
	}

	if (_toggled == true)
	{
		if (_change != nullptr)
			(state->*_change)(action);

		_toggled = false;
	}
}

/**
 * Opens/closes this ComboBox's TextList.
 * @param init - true if it is the initialization toggle (default false)
 */
void ComboBox::toggleCbx(bool init)
{
	_list->setVisible(!_list->getVisible());
	_window->setVisible(!_window->getVisible());

	InteractiveSurface* modal;
	if (_window->getVisible() == true)
		modal = this;
	else
		modal = nullptr;

	_state->setModal(modal);

	if (init == false && _window->getVisible() == false)
		_toggled = true;

	if (_list->getVisible() == true)
	{
		if (_sel < (_list->getVisibleRows() >> 1u))
			_list->scrollTo();
		else
			_list->scrollTo(_sel - (_list->getVisibleRows() >> 1u));
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
