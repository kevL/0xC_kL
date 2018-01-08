/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "ErrorMessageState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ErrorMessage window.
 * @param wst		- reference to the LocalizedText (wide-string) to be displayed
 * @param palette	- pointer to the parent-state palette
 * @param color		- color of the UI-controls
 * @param bgType	- reference to the background image-string
 * @param bgColor	- background color (-1 for Battlescape)
 */
ErrorMessageState::ErrorMessageState(
		const std::wstring& wst,
		SDL_Color* const palette,
		int color,
		const std::string& bgType,
		int bgColor)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 160, 32, 20, POPUP_BOTH);
	_txtMessage	= new Text(200, 120, 60, 30);
	_btnOk		= new TextButton(120, 18, 100, 154);

	setPalette(palette);
	if (bgColor != -1)
		setPalette(
				_game->getResourcePack()->getPalette(PAL_BACKPALS)
						->getColors(static_cast<int>(Palette::blockOffset(static_cast<Uint8>(bgColor)))),
				Palette::PAL_bgID,
				16);

	add(_window);
	add(_btnOk);
	add(_txtMessage);

	centerSurfaces();


	_window->setColor(color);
	_window->setBackground(_game->getResourcePack()->getSurface(bgType));

	_btnOk->setColor(color);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ErrorMessageState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ErrorMessageState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ErrorMessageState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ErrorMessageState::btnOkClick),
							Options::keyCancel);

	_txtMessage->setText(wst);
	_txtMessage->setColor(color);
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();

	if (bgColor == -1)
	{
		_window		->setHighContrast();
		_btnOk		->setHighContrast();
		_txtMessage	->setHighContrast();
	}
}

/**
 * dTor.
 */
ErrorMessageState::~ErrorMessageState()
{}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void ErrorMessageState::btnOkClick(Action*)
{
	_game->popState();
}

}
