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

#include "ConfirmDestinationState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ConfirmDestination window.
 * @param craft		- pointer to the Craft to retarget
 * @param target	- pointer to the selected Target (nullptr if it's just a point on the globe)
 */
ConfirmDestinationState::ConfirmDestinationState(
		Craft* const craft,
		Target* const target)
	:
		_craft(craft),
		_target(target)
{
	_fullScreen = false;

	_window		= new Window(this, 224, 72, 16, 64);
	_txtTarget	= new Text(192, 32, 32, 75);

	_btnCancel	= new TextButton(75, 16,  51, 111);
	_btnOk		= new TextButton(75, 16, 130, 111);

	_waypoint = dynamic_cast<Waypoint*>(_target);
	if (_waypoint != nullptr && _waypoint->getId() != 0)
		_waypoint = nullptr;

	setInterface(
			"confirmDestination",
			_waypoint != nullptr);

	add(_window,	"window",	"confirmDestination");
	add(_txtTarget,	"text",		"confirmDestination");
	add(_btnCancel,	"button",	"confirmDestination");
	add(_btnOk,		"button",	"confirmDestination");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtTarget->setBig();
	_txtTarget->setAlign(ALIGN_CENTER);
	_txtTarget->setVerticalAlign(ALIGN_MIDDLE);
	if (_waypoint != nullptr)
		_txtTarget->setText(tr("STR_TARGET_WAY_POINT"));
	else
		_txtTarget->setText(tr("STR_TARGET_").arg(_target->getLabel(_game->getLanguage())));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ConfirmDestinationState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ConfirmDestinationState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ConfirmDestinationState::btnOkClick),
							Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&ConfirmDestinationState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ConfirmDestinationState::btnCancelClick),
								Options::keyCancel);
}

/**
 * dTor.
 */
ConfirmDestinationState::~ConfirmDestinationState()
{}

/**
 * Confirms the selected Target for the Craft.
 * @param action - pointer to an Action
 */
void ConfirmDestinationState::btnOkClick(Action*)
{
	if (_waypoint != nullptr)
	{
		_waypoint->setId(_game->getSavedGame()->getCanonicalId(Target::stTarget[4u]));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
	}

	_craft->setTarget(_target);
	_craft->setCraftStatus(CS_OUT);

	_game->popState();
	_game->popState();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ConfirmDestinationState::btnCancelClick(Action*)
{
	if (_waypoint != nullptr)
		delete _waypoint;

	_game->popState();
}

}
