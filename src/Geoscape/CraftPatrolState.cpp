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

#include "CraftPatrolState.h"

#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"
#include "SelectDestinationState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the CraftPatrol window.
 * @param craft		- pointer to a Craft
 * @param geoState	- pointer to the GeoscapeState
 */
CraftPatrolState::CraftPatrolState(
		Craft* const craft,
		GeoscapeState* const geoState)
	:
		_craft(craft),
		_geoState(geoState),
		_delayPop(true)
{
	_fullScreen = false;

	_window = new Window(this, 224, 168, 16, 16, POPUP_BOTH);

	_sprite = new Surface(
						32,38,
						_window->getX() + _window->getWidth() - 16,
						_window->getY() - 11);

	_txtDestination	= new Text(224, 78, 16, 40); // was 32 w/ _txtPatrolling
//	_txtPatrolling	= new Text(224, 17, 16, 100);

	_btnOk			= new TextButton(94, 16,  32, 121);
	_btn5s			= new TextButton(94, 16, 130, 121);

	_btnInfo		= new TextButton(192, 16, 32, 140);

	_btnBase		= new TextButton(62, 16,  32, 159);
	_btnCenter		= new TextButton(62, 16,  97, 159);
	_btnRedirect	= new TextButton(62, 16, 162, 159);

	_srfTarget		= new Surface(29, 29, 114, 86);

	setInterface("geoCraftScreens");

	add(_window,			"window",	"geoCraftScreens");
	add(_sprite);
	add(_txtDestination,	"text1",	"geoCraftScreens");
//	add(_txtPatrolling,		"text1",	"geoCraftScreens");
	add(_btnOk,				"button",	"geoCraftScreens");
	add(_btn5s,				"button",	"geoCraftScreens");
	add(_btnInfo,			"button",	"geoCraftScreens");
	add(_btnBase,			"button",	"geoCraftScreens");
	add(_btnCenter,			"button",	"geoCraftScreens");
	add(_btnRedirect,		"button",	"geoCraftScreens");

	add(_srfTarget);

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_txtDestination->setText(tr("STR_CRAFT_HAS_REACHED_DESTINATION")
								 .arg(_craft->getLabel(_game->getLanguage()))
								 .arg(_craft->getDestination()->getLabel(_game->getLanguage())));
	_txtDestination->setAlign(ALIGN_CENTER);
	_txtDestination->setBig();
	_txtDestination->setWordWrap();

//	_txtPatrolling->setBig();
//	_txtPatrolling->setAlign(ALIGN_CENTER);
//	_txtPatrolling->setText(tr("STR_NOW_PATROLLING"));

	_btnOk->setText(tr("STR_PATROL"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftPatrolState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftPatrolState::btnOkClick),
							Options::keyCancel);

	_btn5s->setText(tr("STR_5_SECONDS"));
	_btn5s->onMouseClick(static_cast<ActionHandler>(&CraftPatrolState::btn5sClick));

	_btnInfo->setText(tr("STR_EQUIP_CRAFT"));
	_btnInfo->onMouseClick(static_cast<ActionHandler>(&CraftPatrolState::btnInfoClick));

	_btnBase->setText(tr("STR_RETURN_TO_BASE"));
	_btnBase->onMouseClick(static_cast<ActionHandler>(&CraftPatrolState::btnBaseClick));

	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick(static_cast<ActionHandler>(&CraftPatrolState::btnCenterClick));

	_btnRedirect->setText(tr("STR_REDIRECT_CRAFT"));
	_btnRedirect->onMouseClick(		static_cast<ActionHandler>(&CraftPatrolState::btnRedirectClick));
	_btnRedirect->onKeyboardPress(	static_cast<ActionHandler>(&CraftPatrolState::btnRedirectClick),
									Options::keyOk);
	_btnRedirect->onKeyboardPress(	static_cast<ActionHandler>(&CraftPatrolState::btnRedirectClick),
									Options::keyOkKeypad);

	SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet("INTICON.PCK"));
	const int craftSprite (_craft->getRules()->getSprite());
	srt->getFrame(craftSprite + 11)->blit(_sprite);
}

/**
 * dTor.
 */
CraftPatrolState::~CraftPatrolState()
{}

/**
 * Initializes the state.
 */
void CraftPatrolState::init()
{
	State::init();
	_btn5s->setVisible(_geoState->is5Sec() == false);
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Closes the window and slows time-compression to 5 sec.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btn5sClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
}

/**
 * Opens the Craft info-window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnInfoClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
	_game->pushState(new GeoscapeCraftState(_craft, _geoState));
}

/**
 * Sends the Craft back to its Base.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnBaseClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
	_craft->returnToBase();
}

/**
 * Centers the Craft on the Globe.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnCenterClick(Action*)
{
	_geoState->getGlobe()->center(
							_craft->getLongitude(),
							_craft->getLatitude());

	if (_delayPop == true)
	{
		_delayPop = false;
		transposeWindow();
		return;
	}

	_geoState->setPaused();
	_geoState->resetTimer();
	_game->popState();
}

/**
 * Opens the SelectDestination window.
 * @param action - pointer to an Action
 */
void CraftPatrolState::btnRedirectClick(Action*)
{
	_geoState->resetTimer();
	_game->popState();
	_game->pushState(new SelectDestinationState(_craft, _geoState->getGlobe()));
}

/**
 * Hides various screen-elements to reveal the Globe & Craft.
 */
void CraftPatrolState::transposeWindow() // private.
{
	_window->setVisible(false);

	const int dy (25);
	_btnOk->setY(_btnOk->getY() + dy);
	_btn5s->setY(_btn5s->getY() + dy);
	_btnInfo->setY(_btnInfo->getY() + dy);
	_btnBase->setY(_btnBase->getY() + dy);
	_btnCenter->setY(_btnCenter->getY() + dy);
	_btnRedirect->setY(_btnRedirect->getY() + dy);

	_txtDestination->setVisible(false);
	_btnCenter->setText(tr("STR_PAUSE"));

	Surface* const srf (_game->getResourcePack()->getSurface("Crosshairs"));
	srf->setX(0);
	srf->setY(0);
	srf->blit(_srfTarget);
}

}
