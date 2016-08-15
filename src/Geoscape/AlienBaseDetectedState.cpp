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

#include "AlienBaseDetectedState.h"

//#include "GeoscapeState.h"
//#include "Globe.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleRegion.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Country.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the AlienBaseDetected window.
 * @param aBase		- pointer to the AlienBase to get info from
// * @param geoState	- pointer to the GeoscapeState
 * @param recon		- true if detected by Craft reconnaissance
 */
AlienBaseDetectedState::AlienBaseDetectedState(
		AlienBase* const aBase,
//		GeoscapeState* const geoState,
		bool recon)
	:
		_aBase(aBase)
//		_geoState(geoState)
{
	_window		= new Window(this, 320, 200);
	_txtTitle	= new Text(300, 180, 10, 0);
	_btnOk		= new TextButton(100, 16, 110, 180);

	setInterface("alienBase");

	add(_window,	"window",	"alienBase");
	add(_txtTitle,	"text",		"alienBase");
	add(_btnOk,		"button",	"alienBase");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();


	_aBase->setId(_game->getSavedGame()->getCanonicalId(_aBase->getAlienBaseDeployed()->getMarkerType())); //Target::stTarget[2u]
	_aBase->setDetected();


	const double
		lon (_aBase->getLongitude()),
		lat (_aBase->getLatitude());

	std::wstring loc; // NOTE: Regions implicitly cover all areas of the Globe. Don't eff it up Lol.
	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(lon,lat))
		{
			loc = tr((*i)->getRules()->getType());
			break;
		}
	}
	for (std::vector<Country*>::const_iterator
			i = _game->getSavedGame()->getCountries()->begin();
			i != _game->getSavedGame()->getCountries()->end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(lon,lat))
		{
			loc += L"> ";
			loc += tr((*i)->getRules()->getType());
			break;
		}
	}
	if (loc.empty() == true)
		loc = tr("STR_UNKNOWN");

	std::string st;
	if (recon == true)
		st = "STR_ALIEN_BASE_DETECT_BY_CRAFT";
	else
		st = "STR_ALIEN_BASE_DETECT_BY_SECRET_AGENTS";
	_txtTitle->setText(tr(st).arg(loc)); // TODO: Add canonical-ID.
}

/**
 * dTor.
 */
AlienBaseDetectedState::~AlienBaseDetectedState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void AlienBaseDetectedState::btnOkClick(Action*)
{
//	_geoState->resetTimer();
//	_geoState->getGlobe()->center(
//								_aBase->getLongitude(),
//								_aBase->getLatitude());
	_game->popState();
}

}
