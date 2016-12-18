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

#include "TextButton.h"

//#include <SDL/SDL.h>
//#include <SDL/SDL_mixer.h>

#include "ComboBox.h"
#include "Text.h"

#include "../Engine/Action.h"
#include "../Engine/Sound.h"


namespace OpenXcom
{

const Sound* TextButton::soundPress (nullptr); // static.


/**
 * Sets up the TextButton with a specified size and position.
 * The text is centered on the button.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
TextButton::TextButton(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_color(0u),
		_group(nullptr),
		_contrast(1u),
		_silent(false),
		_comboBox(nullptr)
//		_geoscapeButton(false)
{
	_text = new Text(
					width,
					height,
					0,0);
	_text->setSmall();
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
}

/**
 * Deletes this TextButton.
 */
TextButton::~TextButton()
{
	delete _text;
}

/**
 * Checks if a specified mouse-button is handled.
 * @param btn - (default 0)
 */
bool TextButton::isButtonHandled(Uint8 btn) // protected.
{
	if (_comboBox != nullptr)
		return (btn == SDL_BUTTON_LEFT);

	return InteractiveSurface::isButtonHandled(btn);
}

/**
 * Changes the color for this TextButton and Text.
 * @param color - color value
 */
void TextButton::setColor(Uint8 color)
{
	_color = color;
	_text->setColor(color);
	_redraw = true;
}

/**
 * Gets the color for this TextButton and Text.
 * @return, color value
 */
Uint8 TextButton::getColor() const
{
	return _color;
}

/**
 * Sets the secondary color of this TextButton.
 * @param color - the color
 */
void TextButton::setSecondaryColor(Uint8 color)
{
	_text->setSecondaryColor(color);
	_redraw = true;
}

/**
 * Changes the color for the Text only.
 * @param color - color value
 */
void TextButton::setTextColor(Uint8 color)
{
	_text->setColor(color);
	_redraw = true;
}

/**
 * Changes the Text to use the big-size Font.
 */
void TextButton::setBig()
{
	_text->setBig();
	_redraw = true;
}

/**
 * Changes the Text to use the small-size Font.
 */
void TextButton::setSmall()
{
	_text->setSmall();
	_redraw = true;
}

/**
 * Gets the Font currently used by the Text.
 * @return, pointer to Font
 */
Font* TextButton::getFont() const
{
	return _text->getFont();
}

/**
 * Changes the various resources needed for Text rendering.
 * @note The different fonts need to be passed in advance since the text-size
 * can change mid-text and the language affects how the Text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void TextButton::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_text->initText(big, small, lang);
	_redraw = true;
}

/**
 * Enables/disables high-contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - high-contrast setting (default true)
 */
void TextButton::setHighContrast(bool contrast)
{
	_contrast = contrast ? 2u : 1u;
	_text->setHighContrast(contrast);
	_redraw = true;
}

/**
 * Changes the Text of the button-label.
 * @param text - reference to a text-string
 */
void TextButton::setText(const std::wstring& text)
{
	_text->setText(text);
	if (text == L"<") _text->setX(-1); // looks better shifted left a pixel.
	_redraw = true;
}

/**
 * Gets the Text of the button-label.
 * @return, text-string
 *
std::wstring TextButton::getText() const
{
	return _text->getText();
} */

/**
 * Gets a pointer to the Text.
 * @return, pointer to Text
 */
Text* TextButton::getTextPtr() const
{
	return _text;
}

/**
 * Changes the button-group this TextButton belongs to.
 * @note Passing NULL makes it a regular button.
 * @param group - pointer to a pointer to the pressed button in the group
 */
void TextButton::setGroup(TextButton** const group)
{
	_group = group;
	_redraw = true;
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void TextButton::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the labeled button.
 * @note The colors are inverted if this TextButton is pressed.
 */
void TextButton::draw()
{
	Surface::draw();

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(getWidth());
	rect.h = static_cast<Uint16>(getHeight());

	Uint8 color (static_cast<Uint8>(_color + _contrast));

	// limit highest color to darkest hue/shade in its palette-block.
//	const Uint8 topColor = ((_color / 16) * 16) + 15; // exploit INT

	for (int
			i = 0;
			i != 5;
			++i)
	{
//		if (color > topColor) color = topColor;
		if (i == 0)
			color = static_cast<Uint8>(_color + (_contrast * 5u));

		drawRect(&rect, color);

		if ((i & 1) == 0)
		{
			++rect.x;
			++rect.y;
		}
		--rect.w;
		--rect.h;

		switch (i)
		{
			case 0:
//				color = _color + (_contrast * 5);
//				if (color > topColor) color = topColor;
				setPixelColor(
						static_cast<int>(rect.w),
						0,
						color);
				break;

			case 1:
				color = static_cast<Uint8>(_color + (_contrast * 2u));
				break;

			case 2:
				color = static_cast<Uint8>(_color + (_contrast * 4u));
//				if (color > topColor) color = topColor;
				setPixelColor(
						rect.w + 1,
						1,
						color);
				break;

			case 3:
				color = static_cast<Uint8>(_color + (_contrast * 3u));
//				break;
//
//			case 4:
//				if (_geoscapeButton == true)
//				{
//					setPixelColor(0,0, _color);
//					setPixelColor(1,1, _color);
//				}
		}
	}

	bool press;
	if (_group == nullptr)
		press = isButtonPressed();
	else
		press = (*_group == this);

	if (press == true)
	{
//		if (_geoscapeButton == true)
//			this->invert(_color + (_contrast * 2u));
//		else
		this->invert(static_cast<Uint8>(_color + (_contrast * 3u)));
	}

	_text->setInvert(press);
	_text->blit(this);
}

/**
 * Sets this TextButton as the depressed button if it's part of a group.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void TextButton::mousePress(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _group != nullptr)
	{
		TextButton* const pre (*_group);
		*_group = this;

		if (pre != nullptr) pre->draw();
		draw();
	}

	if (isButtonHandled(action->getDetails()->button.button) == true)
	{
		if (_silent == false)
		{
			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_WHEELUP:
				case SDL_BUTTON_WHEELDOWN:
					break;

				default:
					if (_comboBox == nullptr || _comboBox->getVisible() == true) //&& soundPress != nullptr && _group == nullptr
						soundPress->play(Mix_GroupAvailable(0));
			}
		}
		draw();
	}

	InteractiveSurface::mousePress(action, state);
}

/**
 * Sets this TextButton as the released button.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void TextButton::mouseRelease(Action* action, State* state)
{
	if (isButtonHandled(action->getDetails()->button.button) == true)
	{
		if (_comboBox != nullptr && _comboBox->getVisible() == true)
			_comboBox->toggleCbx();

		draw();
	}

	InteractiveSurface::mouseRelease(action, state);
}

/**
 * Hooks up this TextButton to work as part of an existing ComboBox toggling its
 * state when pressed.
 * @param comboBox - pointer to ComboBox
 */
void TextButton::setComboBox(ComboBox* comboBox)
{
	_comboBox = comboBox;

	if (_comboBox != nullptr)
		_text->setX(-6);
//	else _text->setX(0);
}

/**
 * Sets the width of this TextButton.
 * @param width - the width to set
 */
void TextButton::setWidth(int width)
{
	Surface::setWidth(width);
	_text->setWidth(width);
}

/**
 * Sets the height of this TextButton.
 * @param height - the height to set
 */
void TextButton::setHeight(int height)
{
	Surface::setHeight(height);
	_text->setHeight(height);
}

/**
 * Sets this TextButton as silent.
 */
void TextButton::setSilent()
{
	_silent = true;
}

/**
 * Sets whether this TextButton is a Geoscape button.
 * @param geo - true if Geo
 *
void TextButton::setGeoscapeButton(bool geo)
{
	_geoscapeButton = geo;
} */

}
