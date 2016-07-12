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

#include "BaseDestroyedState.h"

#include "Globe.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Informs player that an undefended Base was destroyed by aLien attack.
 * @param base	- pointer to the base to destroy
 * @param globe	- pointer to the geoscape globe
 */
BaseDestroyedState::BaseDestroyedState(
		const Base* const base,
		Globe* const globe)
	:
		_base(base),
		_globe(globe)
{
	_fullScreen = false;

	_window		= new Window(this, 246, 160, 35, 20); // NOTE: These are offset a few px left.
	_txtMessage	= new Text(224, 106, 46, 26);
	_btnCenter	= new TextButton(140, 16, 88, 133);
	_btnOk		= new TextButton(140, 16, 88, 153);

	setInterface("UFOInfo");

	add(_window,		"window",	"UFOInfo");
	add(_txtMessage,	"text",		"UFOInfo");
	add(_btnCenter,		"button",	"UFOInfo");
	add(_btnOk,			"button",	"UFOInfo");

	centerAllSurfaces();


	_window->setBackground(
						_game->getResourcePack()->getSurface("BACK15.SCR"),
						37); // x-offset

	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setVerticalAlign(ALIGN_MIDDLE);
	_txtMessage->setBig();
	_txtMessage->setWordWrap();
	_txtMessage->setText(tr("STR_BASE_DESTROYED_BY_ALIENS")
							.arg(_base->getName()));

	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick(static_cast<ActionHandler>(&BaseDestroyedState::btnCenterClick));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BaseDestroyedState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDestroyedState::btnCenterClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDestroyedState::btnCenterClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDestroyedState::btnOkClick),
							Options::keyCancel);

	const double
		lon (base->getLongitude()),
		lat (base->getLatitude());
	const std::vector<Region*>* regionList (_game->getSavedGame()->getRegions());
	std::vector<Region*>::const_iterator pRegion (regionList->begin());
	for (
			;
			pRegion != regionList->end();
			++pRegion)
	{
		if ((*pRegion)->getRules()->insideRegion(lon,lat))
			break;
	}

	const AlienMission* const mission (_game->getSavedGame()->findAlienMission(
																			(*pRegion)->getRules()->getType(),
																			alm_RETAL));

	for (std::vector<Ufo*>::const_iterator
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();)
	{
		if ((*i)->getAlienMission() == mission)
		{
			delete *i;
			i = _game->getSavedGame()->getUfos()->erase(i);
		}
		else
			++i;
	}

	for (std::vector<AlienMission*>::const_iterator
			i = _game->getSavedGame()->getAlienMissions().begin();
			i != _game->getSavedGame()->getAlienMissions().end();
			++i)
	{
		if (*i == mission)
		{
			delete *i;
			_game->getSavedGame()->getAlienMissions().erase(i);
			break;
		}
	}
}

/**
 * dTor.
 */
BaseDestroyedState::~BaseDestroyedState()
{}

/**
 * Scores aLien-points and destroys the Base.
 */
void BaseDestroyedState::finish()
{
	_game->popState();

//	const int score ((static_cast<int>(_game->getSavedGame()->getDifficulty()) + 1) * 200); // was 235 in DebriefingState.
	const int score (_game->getRuleset()->getBaseLostScore()
				  *  _base->getQuantityFacilities()
				  * (static_cast<int>(_game->getSavedGame()->getDifficulty()) + 1));

	_game->getSavedGame()->scorePoints(
									_base->getLongitude(),
									_base->getLatitude(),
									score, true);

	std::vector<Base*>* const baseList (_game->getSavedGame()->getBases());
	for (std::vector<Base*>::const_iterator
			i = baseList->begin();
			i != baseList->end();
			++i)
	{
		if (*i == _base)
		{
			delete *i;
			baseList->erase(i);
			break;
			// SHOULD PUT IN A SECTION FOR TRANSFERING AIRBORNE CRAFT TO ANOTHER BASE/S
			// if Option:: transfer airborne craft == true
			// & hangar and stores available (else sell)
			// & living quarters available (if soldiers) else Sack.
		}
	}
}

/**
 * Deletes the Base and scores aLien victory points.
 * @param action - pointer to an Action
 */
void BaseDestroyedState::btnOkClick(Action*)
{
	finish();
}

/**
 * Deletes the Base and scores aLien victory points, and centers the Globe.
 * @param action - pointer to an Action
 */
void BaseDestroyedState::btnCenterClick(Action*)
{
	_globe->center(
				_base->getLongitude(),
				_base->getLatitude());
	finish();
}

}
