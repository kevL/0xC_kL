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

#include "HiddenMovement.h"

//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/Window.h"


namespace OpenXcom
{

/**
 * Sets up a blank HiddenMovement screen with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
HiddenMovement::HiddenMovement(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y)
{
	_window = new Window(
						nullptr,
						width,
						height,
						x,y,
						POPUP_NONE);
	_window->setHighContrast();

	_text = new Text(150, 16); // x/y are set in Map cTor.
	_text->setHighContrast();
}

/**
 * Deletes Surfaces.
 */
HiddenMovement::~HiddenMovement()
{
	delete _window;
	delete _text;
}

/**
 * Changes the position of this HiddenMovement in the x-axis.
 * @param x - x-position in pixels
 */
void HiddenMovement::setX(int x) // override
{
	Surface::setX(x);

	_window->setX(x);
	_text  ->setX(x + 71);
}

/**
 * Changes the position of this HiddenMovement in the y-axis.
 * @param y - y-position in pixels
 */
void HiddenMovement::setY(int y) // override
{
	Surface::setY(y);

	_window->setY(y);
	_text  ->setY(y + 88);
}

/**
 * Changes this HiddenMovement's background.
 * @param background - pointer to background surface
 */
void HiddenMovement::setBackground(Surface* const bg)
{
	_window->setBackground(bg);
}

/**
 * Changes this HiddenMovement's text.
 * @param st - reference to a string
 */
void HiddenMovement::setText(const std::wstring& st)
{
	_text->setText(st);
}

/**
 * Changes the various resources needed for text-rendering.
 * @note The different fonts need to be passed in advance since the text-size
 * can change mid-text and the language affects how the Text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void HiddenMovement::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_text->initText(big, small, lang);
	_text->setBig();
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void HiddenMovement::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_window->setPalette(colors, firstcolor, ncolors);
	_text  ->setPalette(colors, firstcolor, ncolors);
}

/**
 * Blits the battlescape message.
 * @param srf - pointer to a Surface
 */
void HiddenMovement::blit(const Surface* const srf)
{
	Surface::blit(srf);

	_window->blit(srf);
	_text  ->blit(srf);
}

/**
 * Special handling for setting the height of this HiddenMovement.
 * @param height - the height
 */
void HiddenMovement::setHeight(int height)
{
	Surface::setHeight(height);

	_window->setHeight(height);
	_text  ->setHeight(height);
}

/**
 * Sets the text-color of this HiddenMovement.
 * @param color - the color
 */
void HiddenMovement::setTextColor(Uint8 color)
{
	_text->setColor(color);
}

}
