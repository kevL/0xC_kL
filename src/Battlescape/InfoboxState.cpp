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

#include "InfoboxState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Timer.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"


namespace OpenXcom
{

/**
 * Initializes all the elements of the InfoboxState.
 * @param wst - reference to a wide-string
 */
InfoboxState::InfoboxState(const std::wstring& wst)
{
	_fullScreen = false;

	_frame = new Frame(260, 90, 30, 110);
	_text  = new Text(250, 80, 35, 115);

	setPalette(PAL_BATTLESCAPE);

	add(_frame, "infoBox", "battlescape");
	add(_text,  "infoBox", "battlescape");

	centerSurfaces();


	_frame->setThickness(8);
	_frame->setHighContrast();

	_text->setText(wst);

	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setHighContrast();
	_text->setWordWrap();
	_text->setBig();

	_timer = new Timer(INFOBOX_DURATION);
	_timer->onTimer(static_cast<StateHandler>(&InfoboxState::exit));
	_timer->start();
}

/**
 * dTor.
 */
InfoboxState::~InfoboxState()
{
	delete _timer;
}

/**
 * This InfoboxState can be closed early with a key- or mouse-down event.
 * @param action - pointer to an Action
 */
void InfoboxState::handle(Action* action)
{
	State::handle(action);

	switch (action->getDetails()->type)
	{
		case SDL_KEYDOWN:
		case SDL_MOUSEBUTTONDOWN:
			exit();
	}
}

/**
 * Exits this InfoboxState on a single timer-tick.
 */
void InfoboxState::think()
{
	_timer->think(this, nullptr);
}

/**
 * Exits this InfoboxState.
 */
void InfoboxState::exit()
{
	_game->popState();
}

}
