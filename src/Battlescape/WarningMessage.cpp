/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "WarningMessage.h"

//#include <string>
//#include <SDL/SDL.h>

#include "../Engine/Timer.h"

#include "../Interface/Text.h"


namespace OpenXcom
{

/**
 * Sets up a WarningMessage with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
WarningMessage::WarningMessage(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_color(0u),
		_fadeStep(0u)
{
	_text = new Text(
					width,
					height,
					0,0);
	_text->setHighContrast();
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setWordWrap();

	_tr = new Timer(80u);
	_tr->onTimer(static_cast<SurfaceHandler>(&WarningMessage::fade));

	_visible = false;
}

/**
 * Deletes text and timers.
 */
WarningMessage::~WarningMessage()
{
	delete _tr;
	delete _text;
}

/**
 * Changes the color for the message background.
 * @param color - color value
 */
void WarningMessage::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Changes the color for the message text.
 * @param color - color value
 */
void WarningMessage::setTextColor(Uint8 color)
{
	_text->setColor(color);
}

/**
 * Changes the various resources needed for text rendering.
 * @note The different fonts need to be passed in advance since the text size
 * can change mid-text and the language affects how the text is rendered.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void WarningMessage::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_text->initText(big, small, lang);
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor 	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void WarningMessage::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Displays this WarningMessage.
 * @param wst - reference to a widestring
 */
void WarningMessage::showMessage(const std::wstring& wst)
{
	_text->setText(wst);
	_fadeStep = 0u;
	_visible =
	_redraw = true;

	_tr->start();
}

/**
 * Keeps the animation timers running.
 */
void WarningMessage::think()
{
	_tr->think(nullptr, this);
}

/**
 * Plays the message fade animation.
 */
void WarningMessage::fade()
{
	if (++_fadeStep == 15u)
	{
		_visible = false;
		_tr->stop();
	}
	else
		_redraw = true;
}

/**
 * Draws this WarningMessage.
 */
void WarningMessage::draw()
{
	Surface::draw();

	Uint8 color (static_cast<Uint8>(_color + _fadeStep));
	if (_fadeStep != 15u) color = static_cast<Uint8>(color + 1u);

	drawRect(
			0,0,
			static_cast<Sint16>(getWidth()),
			static_cast<Sint16>(getHeight()),
			6u);
	drawRect(
			1,1,
			static_cast<Sint16>(getWidth()  - 2),
			static_cast<Sint16>(getHeight() - 2),
			10u);
	drawRect(
			2,2,
			static_cast<Sint16>(getWidth()  - 3),
			static_cast<Sint16>(getHeight() - 3),
			15u);
	drawRect(
			2,2,
			static_cast<Sint16>(getWidth()  - 4),
			static_cast<Sint16>(getHeight() - 4),
			color);

	_text->blit(this);
}

}
