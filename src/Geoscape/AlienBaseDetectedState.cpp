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

#include "AlienBaseDetectedState.h"

#include "GeoscapeState.h"
#include "Globe.h"

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
 * @param alienBase	- pointer to the AlienBase to get info from
 * @param geoState	- pointer to the GeoscapeState
 * @param agents	- true if detected by end-of-month secret operatives (default false)
 */
AlienBaseDetectedState::AlienBaseDetectedState(
		AlienBase* const alienBase,
		GeoscapeState* const geoState,
		bool agents)
	:
		_alienBase(alienBase),
		_geoState(geoState)
{
	_window		= new Window(this);

	_txtTitle	= new Text(288, 180, 16, 0);

	_btnCenter	= new TextButton(100, 16,  50, 180);
	_btnOk		= new TextButton(100, 16, 170, 180);

	setInterface("alienBase");

	add(_window,	"window",	"alienBase");
	add(_txtTitle,	"text",		"alienBase");
	add(_btnCenter,	"button",	"alienBase");
	add(_btnOk,		"button",	"alienBase");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	_btnCenter->setText(tr("STR_CENTER_ON_SITE_TIME_5_SECONDS"));
	_btnCenter->onMouseClick(	static_cast<ActionHandler>(&AlienBaseDetectedState::btnCenterClick));
	_btnCenter->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnCenterClick),
								Options::keyOk);
	_btnCenter->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnCenterClick),
								Options::keyOkKeypad);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&AlienBaseDetectedState::btnOkClick),
							Options::keyCancel);


	_alienBase->setId(_game->getSavedGame()->getCanonicalId(_alienBase->getAlienBaseDeployed()->getMarkerType())); //Target::stTarget[2u]
	_alienBase->setDetected();

	const double
		lon (_alienBase->getLongitude()),
		lat (_alienBase->getLatitude());

	std::wstring wst; // NOTE: Regions implicitly cover all areas of the Globe. Don't eff it up Lol.
	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(lon,lat))
		{
			wst = tr((*i)->getRules()->getType());
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
			wst += L"> ";
			wst += tr((*i)->getRules()->getType());
			break;
		}
	}

	if (wst.empty() == true)
		wst = tr("STR_UNKNOWN");

	std::string st;
	if (agents == true)
		st = "STR_ALIEN_BASE_DETECT_BY_SECRET_AGENTS";
	else
		st = "STR_ALIEN_BASE_DETECT_BY_CRAFT";
	_txtTitle->setText(tr(st).arg(wst)); // TODO: Add canonical-ID.
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
	_game->popState();
}

/**
 * Exits to the Globe with the AlienBase centered.
 * @param action - pointer to an Action
 */
void AlienBaseDetectedState::btnCenterClick(Action*)
{
	_geoState->getGlobe()->center(
								_alienBase->getLongitude(),
								_alienBase->getLatitude());
	_game->popState();
}

}
