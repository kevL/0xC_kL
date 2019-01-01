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

#include "ConfirmCydoniaState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Ruleset/RuleAlienDeployment.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes the journey to Cydonia on mars.
 * @param craft - pointer to the Craft taking the journey
 */
ConfirmCydoniaState::ConfirmCydoniaState(Craft* const craft)
	:
		_craft(craft)
{
	_fullScreen = false;

	_window		= new Window(this, 256, 160, 32, 20);
	_txtMessage	= new Text(224, 48, 48, 76);
	_btnNo		= new TextButton(80, 20,  70, 142);
	_btnYes		= new TextButton(80, 20, 170, 142);

	setInterface("confirmCydonia");

	add(_window,		"window",	"confirmCydonia");
	add(_txtMessage,	"text",		"confirmCydonia");
	add(_btnNo,			"button",	"confirmCydonia");
	add(_btnYes,		"button",	"confirmCydonia");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnYes->setText(tr("STR_YES"));
	_btnYes->onMouseClick(		static_cast<ActionHandler>(&ConfirmCydoniaState::btnYesClick));
	_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmCydoniaState::btnYesClick),
								Options::keyOk);
	_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmCydoniaState::btnYesClick),
								Options::keyOkKeypad);

	_btnNo->setText(tr("STR_NO"));
	_btnNo->onMouseClick(	static_cast<ActionHandler>(&ConfirmCydoniaState::btnNoClick));
	_btnNo->onKeyboardPress(static_cast<ActionHandler>(&ConfirmCydoniaState::btnNoClick),
							Options::keyCancel);

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_ARE_YOU_SURE_CYDONIA"));
}

/**
 * dTor.
 */
ConfirmCydoniaState::~ConfirmCydoniaState()
{}

/**
 * Begins Cydonia tactical.
 * @param action - pointer to an Action
 */
void ConfirmCydoniaState::btnYesClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 335);

	_game->popState();
	_game->popState();

	SavedBattleGame* const battleSave (new SavedBattleGame(
													_game->getSavedGame(),
													&_game->getRuleset()->getOperations(),
													_game->getRuleset()));
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen = BattlescapeGenerator(_game);

	for (std::vector<std::string>::const_iterator
			i = _game->getRuleset()->getDeploymentsList().begin();
			i != _game->getRuleset()->getDeploymentsList().end();
			++i)
	{
		const RuleAlienDeployment* const ruleDeploy (_game->getRuleset()->getDeployment(*i));
		if (ruleDeploy->isFinalDestination() == true)
		{
			battleSave->setTacticalType(*i);
			bGen.setAlienRace(ruleDeploy->getRace());
			break;
		}
	}

	bGen.setCraft(_craft);
	bGen.stage();

	_game->pushState(new BriefingState(_craft));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ConfirmCydoniaState::btnNoClick(Action*)
{
	_game->popState();
}

}
