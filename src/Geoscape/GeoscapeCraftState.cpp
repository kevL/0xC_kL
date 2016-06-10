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

#include "GeoscapeCraftState.h"

//#include <sstream>

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
#include "../Ruleset/RuleCraftWeapon.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the GeoscapeCraft window.
 * @param craft		- pointer to a Craft for display
 * @param geoState	- pointer to the Geoscape
 * @param waypoint	- pointer to the last UFO position for redirecting the Craft (default nullptr)
 * @param doublePop	- true if two windows need to pop on exit (default false)
 * @param transpose	- true to start the state transposed (default false)
 */
GeoscapeCraftState::GeoscapeCraftState(
		Craft* const craft,
		GeoscapeState* const geoState,
		Waypoint* const waypoint,
		bool doublePop,
		bool transpose)
	:
		_craft(craft),
		_geoState(geoState),
		_waypoint(waypoint),
		_doublePop(doublePop),
		_delayPop(true)
{
	_fullScreen = false;

	_window			= new Window(this, 224, 176, 16, 8, POPUP_BOTH);

	_txtTitle		= new Text(192, 17, 32, 15);

	_sprite			= new Surface(
								32,38,
								_window->getX() + _window->getWidth() - 16,
								_window->getY() - 11);

	_txtKills		= new Text(60, 9, _sprite->getX() - 61, 15);

	_txtStatus		= new Text(192, 17,  32, 31);
	_txtBase		= new Text(192,  9,  32, 43);
	_txtRedirect	= new Text(120, 17, 120, 46);
	_txtSpeed		= new Text(192,  9,  32, 55);
	_txtMaxSpeed	= new Text(120,  9,  32, 64);
	_txtSoldier		= new Text( 80,  9, 160, 64);
	_txtAltitude	= new Text(120,  9,  32, 73);
	_txtHWP			= new Text( 80,  9, 160, 73);
	_txtFuel		= new Text(120,  9,  32, 82);
	_txtDamage		= new Text( 80,  9, 160, 82);

	_txtW1Name		= new Text(120,  9,  32,  91);
	_txtW1Ammo		= new Text( 80,  9, 160,  91);
	_txtW2Name		= new Text(120,  9,  32, 100);
	_txtW2Ammo		= new Text( 80,  9, 160, 100);

	_btnTarget		= new TextButton( 90, 16,  32, 114);
	_btnPatrol		= new TextButton( 90, 16, 134, 114);
	_btnCenter		= new TextButton(192, 16,  32, 136);
	_btnRebase		= new TextButton( 90, 16,  32, 158);
	_btnCancel		= new TextButton( 90, 16, 134, 158);

	_srfTarget		= new Surface(29, 29, 114, 86);

	setInterface("geoCraftScreens");

	add(_window,		"window",	"geoCraftScreens");
	add(_txtTitle,		"text1",	"geoCraftScreens");
	add(_txtKills,		"text1",	"geoCraftScreens");
	add(_sprite);
	add(_txtStatus,		"text1",	"geoCraftScreens");
	add(_txtBase,		"text3",	"geoCraftScreens");
	add(_txtRedirect,	"text3",	"geoCraftScreens");
	add(_txtSpeed,		"text3",	"geoCraftScreens");
	add(_txtMaxSpeed,	"text3",	"geoCraftScreens");
	add(_txtSoldier,	"text3",	"geoCraftScreens");
	add(_txtAltitude,	"text3",	"geoCraftScreens");
	add(_txtHWP,		"text3",	"geoCraftScreens");
	add(_txtFuel,		"text3",	"geoCraftScreens");
	add(_txtDamage,		"text3",	"geoCraftScreens");
	add(_txtW1Name,		"text3",	"geoCraftScreens");
	add(_txtW1Ammo,		"text3",	"geoCraftScreens");
	add(_txtW2Name,		"text3",	"geoCraftScreens");
	add(_txtW2Ammo,		"text3",	"geoCraftScreens");

	add(_btnTarget,		"button",	"geoCraftScreens");
	add(_btnPatrol,		"button",	"geoCraftScreens");
	add(_btnCenter,		"button",	"geoCraftScreens");
	add(_btnRebase,		"button",	"geoCraftScreens");
	add(_btnCancel,		"button",	"geoCraftScreens");

	add(_srfTarget);

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCenterClick);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnCenterClick,
					SDLK_c);

	_btnRebase->setText(tr("STR_RETURN_TO_BASE"));
	_btnRebase->onMouseClick((ActionHandler)& GeoscapeCraftState::btnBaseClick);
	_btnRebase->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnBaseClick,
					SDLK_b);

	_btnTarget->setText(tr("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick((ActionHandler)& GeoscapeCraftState::btnTargetClick);
	_btnTarget->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnTargetClick,
					SDLK_t);

	_btnPatrol->setText(tr("STR_PATROL"));
	_btnPatrol->onMouseClick((ActionHandler)& GeoscapeCraftState::btnPatrolClick);
	_btnPatrol->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnPatrolClick,
					SDLK_p);

	_btnCancel->onMouseClick((ActionHandler)& GeoscapeCraftState::btnCancelOrRedirectClick);
	if (_waypoint != nullptr) // can Redirect
	{
		_txtRedirect->setText(tr("STR_REDIRECT_CRAFT"));
		_txtRedirect->setAlign(ALIGN_CENTER);
		_txtRedirect->setBig();

		_btnPatrol->onKeyboardPress(
						(ActionHandler)& GeoscapeCraftState::btnPatrolClick,
						Options::keyCancel);

		_btnCancel->setText(tr("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));
		_btnCancel->onKeyboardPress(
						(ActionHandler)& GeoscapeCraftState::btnCancelOrRedirectClick,
						SDLK_r);
	}
	else
	{
		_txtRedirect->setVisible(false);

		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onKeyboardPress(
						(ActionHandler)& GeoscapeCraftState::btnCancelOrRedirectClick,
						Options::keyCancel);
	}
	_btnCancel->onKeyboardPress( // if (can Redirect) redirect; else Cancel.
					(ActionHandler)& GeoscapeCraftState::btnCancelOrRedirectClick,
					Options::keyOk);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& GeoscapeCraftState::btnCancelOrRedirectClick,
					Options::keyOkKeypad);

	_txtTitle->setText(_craft->getName(_game->getLanguage()));
	_txtTitle->setBig();

	if (_craft->getRules()->getWeaponCapacity() != 0)
	{
		_txtKills->setText(tr("STR_KILLS_LC_").arg(_craft->getKills()));
		_txtKills->setAlign(ALIGN_RIGHT);
	}


	const CraftStatus stat (_craft->getCraftStatus());
	std::wstring status;
	const bool
		lowFuel (_craft->getLowFuel()),
		missionComplete (_craft->getTacticalReturn());
	int speed (_craft->getSpeed());

	// NOTE: Could add "DAMAGED - Return to Base" around here.
	if (stat != CS_OUT)
		status = tr("STR_BASED");
	else if (lowFuel == true)
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
	else if (missionComplete == true)
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
	else if (_craft->getDestination() == dynamic_cast<Target*>(_craft->getBase()))
		status = tr("STR_RETURNING_TO_BASE");
	else if (_craft->getDestination() == nullptr)
		status = tr("STR_PATROLLING");
	else
	{
		const Ufo* const ufo (dynamic_cast<Ufo*>(_craft->getDestination()));
		if (ufo != nullptr)
		{
			if (_craft->inDogfight() == true)
			{
				speed = ufo->getSpeed(); // THIS DOES NOT CHANGE THE SPEED of the xCom CRAFT for Fuel usage (ie. it should).
				status = tr("STR_TAILING_UFO").arg(ufo->getId());
			}
			else if (ufo->getUfoStatus() == Ufo::FLYING)
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
			else
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getName(_game->getLanguage()));
		}
		else
			status = tr("STR_DESTINATION_UC_")
						.arg(_craft->getDestination()->getName(_game->getLanguage()));
	}
	_txtStatus->setText(tr("STR_STATUS_").arg(status));

	_txtBase->setText(tr("STR_BASE_UC").arg(_craft->getBase()->getName()));

	_txtSpeed->setText(tr("STR_SPEED_").arg(speed));
	_txtMaxSpeed->setText(tr("STR_MAXIMUM_SPEED_UC")
							.arg(_craft->getRules()->getMaxSpeed()));

	std::string alt;
	switch (stat)
	{
		case CS_OUT:
			alt = _craft->getAltitude();
			break;

		default:
			alt = MovingTarget::stAltitude[0u];
	}

	_txtAltitude->setText(tr("STR_ALTITUDE_").arg(tr(alt)));

	_txtFuel->setText(tr("STR_FUEL").arg(Text::formatPercent(_craft->getFuelPct())));
	_txtDamage->setText(tr("STR_HULL_").arg(Text::formatPercent(100 - _craft->getCraftDamagePct())));

	std::wostringstream woststr;

	if (_craft->getRules()->getSoldierCapacity() != 0)
	{
		woststr << tr("STR_SOLDIERS") << L" " << L'\x01' << _craft->getQtySoldiers()
				<< L" (" << _craft->getRules()->getSoldierCapacity() << L")";
		_txtSoldier->setText(woststr.str());
	}
	else
		_txtSoldier->setVisible(false);

	if (_craft->getRules()->getVehicleCapacity() != 0)
	{
		woststr.str(L"");
		woststr << tr("STR_HWPS") << L" " << L'\x01' << _craft->getQtyVehicles()
				<< L" (" << _craft->getRules()->getVehicleCapacity() << L")";
		_txtHWP->setText(woststr.str());
	}
	else
		_txtHWP->setVisible(false);


	if (_craft->getRules()->getWeaponCapacity() != 0
		&& _craft->getWeapons()->at(0) != nullptr)
	{
		const CraftWeapon* const w1 (_craft->getWeapons()->at(0));
		_txtW1Name->setText(tr("STR_WEAPON_ONE").arg(tr(w1->getRules()->getType())));

		woststr.str(L"");
		woststr << tr("STR_ROUNDS_").arg(w1->getCwLoad())
				<< L" (" << w1->getRules()->getLoadCapacity() << L")";
		_txtW1Ammo->setText(woststr.str());
	}
	else
	{
		_txtW1Name->setVisible(false);
		_txtW1Ammo->setVisible(false);
	}

	if (_craft->getRules()->getWeaponCapacity() > 1
		&& _craft->getWeapons()->at(1) != nullptr)
	{
		const CraftWeapon* const w2 (_craft->getWeapons()->at(1));
		_txtW2Name->setText(tr("STR_WEAPON_TWO").arg(tr(w2->getRules()->getType())));

		woststr.str(L"");
		woststr << tr("STR_ROUNDS_").arg(w2->getCwLoad())
				<< L" (" << w2->getRules()->getLoadCapacity() << L")";
		_txtW2Ammo->setText(woststr.str());
	}
	else
	{
		_txtW2Name->setVisible(false);
		_txtW2Ammo->setVisible(false);
	}


	// NOTE: These could be set above^ where status was set.
	const bool occupied (lowFuel == true
					  || missionComplete == true
					  || _craft->getTakeoff() == false);

	if (stat != CS_OUT || occupied == true)
	{
		_btnRebase->setVisible(false);
		_btnPatrol->setVisible(false);

		switch (stat)
		{
			case CS_REPAIRS:
			case CS_REFUELLING:
			case CS_REARMING:
				_btnTarget->setVisible(false);
				break;

			case CS_OUT:
				if (occupied == true)
					_btnTarget->setVisible(false);
		}
	}
	else if (_craft->getDestination() == dynamic_cast<Target*>(_craft->getBase()))
		_btnRebase->setVisible(false);
	else if (_craft->getDestination() == nullptr)
		_btnPatrol->setVisible(false);

	SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet("INTICON.PCK"));
	const int craftSprite (_craft->getRules()->getSprite());
	srt->getFrame(craftSprite + 11)->blit(_sprite);

	if (transpose == true)
	{
		_delayPop = false;
		transposeWindow();
	}
}

/**
 * dTor.
 */
GeoscapeCraftState::~GeoscapeCraftState()
{}

/**
 * Centers the Craft on the Globe.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCenterClick(Action*)
{
	_geoState->getGlobe()->center(
								_craft->getLongitude(),
								_craft->getLatitude());

	if (_doublePop == true)
	{
		_game->popState();
		_game->popState();
		_game->pushState(new GeoscapeCraftState(
											_craft,
											_geoState,
											nullptr,
											false,
											true));
		return;
	}

	if (_delayPop == true)
	{
		_delayPop = false;
		targeter();
		transposeWindow();
		return;
	}

	_craft->setDestination();
	_geoState->setPaused();
	_geoState->resetTimer();
	_geoState->getGlobe()->clearCrosshair();
	_game->popState();

	if (_waypoint != nullptr)
		delete _waypoint;
}

/**
 * Returns the Craft back to its Base.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnBaseClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_craft->returnToBase();
	_geoState->getGlobe()->clearCrosshair();

	if (_waypoint != nullptr)
		delete _waypoint;
}

/**
 * Changes the Craft's Target.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnTargetClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	targeter();
	_game->popState();
	_game->pushState(new SelectDestinationState(
											_craft,
											_geoState->getGlobe(),
											_waypoint != nullptr));

	if (_waypoint != nullptr)
		delete _waypoint;
}

/**
 * Sets the Craft to patrol the current location.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnPatrolClick(Action*)
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_craft->setDestination();
	_geoState->getGlobe()->clearCrosshair();

	if (_waypoint != nullptr)
		delete _waypoint;
}

/**
 * Closes the Window.
 * @note The button doubles as the redirect-craft btn.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCancelOrRedirectClick(Action*)
{
	if (_waypoint != nullptr) // Go to the last-known UFO position
	{
		_waypoint->setId(_game->getSavedGame()->getCanonicalId(Target::stTarget[4u]));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setDestination(_waypoint);
		_geoState->getGlobe()->clearCrosshair();
	}
	_game->popState(); // and Cancel.
}

/**
 * Hides various screen-elements to reveal the Globe & Craft.
 */
void GeoscapeCraftState::transposeWindow() // private.
{
	_window->setVisible(false);

	_txtRedirect->setVisible(false);
	_txtSpeed->setVisible(false);
	_txtMaxSpeed->setVisible(false);
	_txtSoldier->setVisible(false);
	_txtAltitude->setVisible(false);
	_txtHWP->setVisible(false);
	_txtFuel->setVisible(false);
	_txtDamage->setVisible(false);
	_txtW1Name->setVisible(false);
	_txtW1Ammo->setVisible(false);
	_txtW2Name->setVisible(false);
	_txtW2Ammo->setVisible(false);

	int dy (26);
	_btnTarget->setY(_btnTarget->getY() + dy);
	_btnPatrol->setY(_btnPatrol->getY() + dy);
	_btnCenter->setY(_btnCenter->getY() + dy);
	_btnRebase->setY(_btnRebase->getY() + dy);
	_btnCancel->setY(_btnCancel->getY() + dy);

	if (_geoState->getPaused() == false)
		_btnCenter->setText(tr("STR_PAUSE"));
	else
	{
		_btnCenter->setVisible(false);

		dy = _btnCenter->getY();
		_btnTarget->setY(dy);
		_btnPatrol->setY(dy);
	}

	Surface* const srf (_game->getResourcePack()->getSurface("Crosshairs"));
	srf->setX(0);
	srf->setY(0);
	srf->blit(_srfTarget);
}

/**
 * Applies the targeter-graphic to the last-known UFO coordinates.
 */
void GeoscapeCraftState::targeter() // private.
{
	if (_waypoint != nullptr)
	{
		_geoState->getGlobe()->setCrosshair(
										_craft->getDestination()->getLongitude(),
										_craft->getDestination()->getLatitude());
		_geoState->getGlobe()->draw();
	}
}

}
