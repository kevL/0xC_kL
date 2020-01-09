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
 * @param ufo		- pointer to Ufo to intercept a freshly detected UFO (default nullptr)
 */
GeoscapeCraftState::GeoscapeCraftState(
		Craft* const craft,
		GeoscapeState* const geoState,
		Waypoint* const waypoint,
		bool doublePop,
		bool transpose,
		Ufo* const ufo)
	:
		_craft(craft),
		_geoState(geoState),
		_waypoint(waypoint),
		_doublePop(doublePop),
		_delayPop(true),
		_ufo(ufo)
{
	_fullScreen = false;

	int h (168);
	if (_ufo != nullptr) h += 17; // for the auto-intercept btn.

	_window       = new Window(this, 224, h, 16, 8, POPUP_BOTH);
	_txtTitle     = new Text(192, 16, 24, 15);
	_sprite       = new Surface(
							32,38,
							_window->getX() + _window->getWidth() - 16,
							_window->getY() - 11);

	_txtKills     = new Text(60, 9, _sprite->getX() - 62, 15);

	_txtStatus    = new Text(208, 16,  32,  33);
	_txtBase      = new Text( 88,  9,  32,  43);
	_txtRedirect  = new Text(120, 16, 120,  43);
	_txtSpeed     = new Text( 88,  9,  32,  55);
	_txtMaxSpeed  = new Text(128,  9,  32,  65);
	_txtSoldier   = new Text( 80,  9, 160,  65);
	_txtAltitude  = new Text(128,  9,  32,  75);
	_txtHwp       = new Text( 80,  9, 160,  75);
	_txtFuel      = new Text(128,  9,  32,  85);
	_txtHull      = new Text( 80,  9, 160,  85);
	_txtWeapon1   = new Text(128,  9,  32,  95);
	_txtLoad1     = new Text( 80,  9, 160,  95);
	_txtWeapon2   = new Text(128,  9,  32, 105);
	_txtLoad2     = new Text( 80,  9, 160, 105);

	_btnTarget    = new TextButton( 90, 16,  32, 118);
	_btnPatrol    = new TextButton( 90, 16, 134, 118);
	_btnCenter    = new TextButton(192, 16,  32, 135);
	_btnRebase    = new TextButton( 90, 16,  32, 152);
	_btnCancel    = new TextButton( 90, 16, 134, 152);
	_btnIntercept = new TextButton(192, 16,  32, 169);

	_srfTarget    = new Surface(29, 29, 114, 86); // the surface that crosshairs will be blitted to.

	setInterface("geoCraftScreens");

	add(_window,       "window", "geoCraftScreens");
	add(_txtTitle,     "text1",  "geoCraftScreens");
	add(_txtKills,     "text1",  "geoCraftScreens");
	add(_sprite);
	add(_txtStatus,    "text1",  "geoCraftScreens");
	add(_txtBase,      "text3",  "geoCraftScreens");
	add(_txtRedirect,  "text3",  "geoCraftScreens");
	add(_txtSpeed,     "text3",  "geoCraftScreens");
	add(_txtMaxSpeed,  "text3",  "geoCraftScreens");
	add(_txtSoldier,   "text3",  "geoCraftScreens");
	add(_txtAltitude,  "text3",  "geoCraftScreens");
	add(_txtHwp,       "text3",  "geoCraftScreens");
	add(_txtFuel,      "text3",  "geoCraftScreens");
	add(_txtHull,      "text3",  "geoCraftScreens");
	add(_txtWeapon1,   "text3",  "geoCraftScreens");
	add(_txtLoad1,     "text3",  "geoCraftScreens");
	add(_txtWeapon2,   "text3",  "geoCraftScreens");
	add(_txtLoad2,     "text3",  "geoCraftScreens");

	add(_btnTarget,    "button", "geoCraftScreens");
	add(_btnPatrol,    "button", "geoCraftScreens");
	add(_btnCenter,    "button", "geoCraftScreens");
	add(_btnRebase,    "button", "geoCraftScreens");
	add(_btnCancel,    "button", "geoCraftScreens");
	add(_btnIntercept, "button", "geoCraftScreens");

	add(_srfTarget);

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"));

	_btnCenter->setText(tr("STR_CENTER"));
	_btnCenter->onMouseClick(	static_cast<ActionHandler>(&GeoscapeCraftState::btnCenterPauseClick));
	_btnCenter->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnCenterPauseClick),
								SDLK_c);

	_btnRebase->setText(tr("STR_RETURN_TO_BASE"));
	_btnRebase->onMouseClick(	static_cast<ActionHandler>(&GeoscapeCraftState::btnBaseClick));
	_btnRebase->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnBaseClick),
								SDLK_b);

	_btnTarget->setText(tr("STR_SELECT_NEW_TARGET"));
	_btnTarget->onMouseClick(	static_cast<ActionHandler>(&GeoscapeCraftState::btnTargetClick));
	_btnTarget->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnTargetClick),
								SDLK_t);

	_btnPatrol->setText(tr("STR_PATROL"));
	_btnPatrol->onMouseClick(	static_cast<ActionHandler>(&GeoscapeCraftState::btnPatrolClick));
	_btnPatrol->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnPatrolClick),
								SDLK_p);

	_btnCancel->onMouseClick(static_cast<ActionHandler>(&GeoscapeCraftState::btnCancelOrRedirectClick));
	if (_waypoint != nullptr) // can Redirect
	{
		_txtRedirect->setText(tr("STR_REDIRECT_CRAFT"));
		_txtRedirect->setAlign(ALIGN_CENTER);
		_txtRedirect->setBig();

		_btnPatrol->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnPatrolClick),
									Options::keyCancel);

		_btnCancel->setText(tr("STR_GO_TO_LAST_KNOWN_UFO_POSITION"));
		_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnCancelOrRedirectClick),
									SDLK_r);
	}
	else
	{
		_txtRedirect->setVisible(false);

		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnCancelOrRedirectClick),
									Options::keyCancel);
	}
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnCancelOrRedirectClick), // if (can Redirect) redirect; else Cancel.
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeCraftState::btnCancelOrRedirectClick),
								Options::keyOkKeypad);

	if (_ufo != nullptr)
	{
		_btnIntercept->setText(tr("STR_INTERCEPT"));
		_btnIntercept->onMouseClick(	static_cast<ActionHandler>(&GeoscapeCraftState::btnInterceptClick));
		_btnIntercept->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeCraftState::btnInterceptClick),
										SDLK_i);
	}
	else
		_btnIntercept->setVisible(false);

	_txtTitle->setText(_craft->getLabel(_game->getLanguage()));
	_txtTitle->setBig();

	if (_craft->getRules()->getWeaponCapacity() != 0u)
	{
		_txtKills->setText(tr("STR_KILLS_LC_").arg(_craft->getAces()));
		_txtKills->setAlign(ALIGN_RIGHT);
	}


	const CraftStatus stat (_craft->getCraftStatus());
	std::wstring status;
	const bool
		lowFuel (_craft->isLowFuel()),
		tacticalReturn (_craft->isTacticalReturn());
	int speed (_craft->getSpeed());

	// NOTE: Could add "DAMAGED - Return to Base" around here.
	if (stat != CS_OUT)
		status = tr("STR_BASED");
	else if (lowFuel == true)
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
	else if (tacticalReturn == true)
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
	else if (_craft->getTarget() == dynamic_cast<Target*>(_craft->getBase()))
		status = tr("STR_RETURNING_TO_BASE");
	else if (_craft->getTarget() == nullptr)
		status = tr("STR_PATROLLING");
	else
	{
		const Ufo* const ufo (dynamic_cast<Ufo*>(_craft->getTarget()));
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
							.arg(ufo->getLabel(_game->getLanguage()));
		}
		else
			status = tr("STR_DESTINATION_UC_")
						.arg(_craft->getTarget()->getLabel(_game->getLanguage()));
	}
	_txtStatus->setText(tr("STR_STATUS_").arg(status));

	_txtBase->setText(tr("STR_BASE_UC_").arg(_craft->getBase()->getLabel()));

	_txtSpeed->setText(tr("STR_SPEED_").arg(speed));
	_txtMaxSpeed->setText(tr("STR_MAXIMUM_SPEED_")
							.arg(_craft->getRules()->getTopSpeed()));

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

	_txtFuel->setText(tr("STR_FUEL_").arg(Text::formatPercent(_craft->getFuelPct())));
	_txtHull->setText(tr("STR_HULL_").arg(Text::formatPercent(_craft->getCraftHullPct())));

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
		_txtHwp->setText(woststr.str());
	}
	else
		_txtHwp->setVisible(false);


	const CraftWeapon* cw;
	const RuleCraftWeapon* cwRule;
	Text
		* cwLabel,
		* cwLoad;
	std::string prefix;

	const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
	for (size_t
			i = 0u;
			i != hardpoints;
			++i)
	{
		switch (i)
		{
			default:
			case 0u:
				prefix  = "STR_WEAPON_POD1_";
				cwLabel = _txtWeapon1;
				cwLoad  = _txtLoad1;
				break;
			case 1u:
				prefix  = "STR_WEAPON_POD2_";
				cwLabel = _txtWeapon2;
				cwLoad  = _txtLoad2;
		}

		if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr)
		{
			cwRule = cw->getRules();
			cwLabel->setText(tr(prefix).arg(tr(cwRule->getType())));

			woststr.str(L"");
			woststr << tr("STR_ROUNDS_").arg(cw->getCwLoad())
					<< L" (" << cwRule->getLoadCapacity() << L")";
			cwLoad->setText(woststr.str());
		}
		else
		{
			cwLabel->setVisible(false);
			cwLoad->setVisible(false);
		}
	}


	// NOTE: These could be set above^ where status was set.
	const bool occupied (lowFuel == true
					  || tacticalReturn == true
					  || _craft->hasLeftGround() == false);

	if ((stat != CS_OUT || occupied == true)
		&& _waypoint == nullptr) // <- toss player a bone if UFO-contact is lost while Craft is still firing up for flight.
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
	else if (_craft->getTarget() == dynamic_cast<Target*>(_craft->getBase()))
		_btnRebase->setVisible(false);
	else if (_craft->getTarget() == nullptr)
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
 * Centers the Globe on the current Craft.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCenterPauseClick(Action*) // private.
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
											true,
											_ufo));
	}
	else if (_delayPop == true)
	{
		_delayPop = false;
		targeter();
		transposeWindow();
	}
	else // pauseHard.
	{
		_geoState->setPaused();

//		_craft->setTarget();
//		_geoState->getGlobe()->clearCrosshair();

		_game->popState();

		if (_waypoint != nullptr)
			delete _waypoint;
	}
}

/**
 * Sends the Craft back to its Base.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnBaseClick(Action*) // private.
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
void GeoscapeCraftState::btnTargetClick(Action*) // private.
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
 * Sets the Craft to patrol its current location.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnPatrolClick(Action*) // private.
{
	if (_doublePop == true)
		_game->popState();

	_game->popState();
	_craft->setTarget();
	_geoState->getGlobe()->clearCrosshair();

	if (_waypoint != nullptr)
		delete _waypoint;
}

/**
 * Closes this State.
 * @note The button doubles as the redirect-craft btn. This can allow
 * redirection toward a waypoint that is outside the Craft's range.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnCancelOrRedirectClick(Action*) // private.
{
	if (_waypoint != nullptr) // Go to the last-known UFO position
	{
		_waypoint->setId(_game->getSavedGame()->getCanonicalId(Target::stTarget[4u]));
		_game->getSavedGame()->getWaypoints()->push_back(_waypoint);
		_craft->setTarget(_waypoint);
		_geoState->getGlobe()->clearCrosshair();
	}
	_game->popState(); // and Cancel.
}

/**
 * Targets the Craft on a freshly detected UFO if it exists/if button is visible.
 * @param action - pointer to an Action
 */
void GeoscapeCraftState::btnInterceptClick(Action*) // private.
{
	_game->popState(); // since this btn is shown only when this State is invoked by InterceptState
	_game->popState(); // '_doublePop' will always be true.

	_craft->setTarget(_ufo);
	_geoState->getGlobe()->clearCrosshair();
}

/**
 * Hides various screen-elements to reveal the Globe & Craft.
 */
void GeoscapeCraftState::transposeWindow() // private.
{
	_window     ->setVisible(false);
	_txtRedirect->setVisible(false);
	_txtSpeed   ->setVisible(false);
	_txtMaxSpeed->setVisible(false);
	_txtSoldier ->setVisible(false);
	_txtAltitude->setVisible(false);
	_txtHwp     ->setVisible(false);
	_txtFuel    ->setVisible(false);
	_txtHull    ->setVisible(false);
	_txtWeapon1 ->setVisible(false);
	_txtLoad1   ->setVisible(false);
	_txtWeapon2 ->setVisible(false);
	_txtLoad2   ->setVisible(false);

	int dy (26);
	_btnTarget->setY(_btnTarget->getY() + dy);
	_btnPatrol->setY(_btnPatrol->getY() + dy);
	_btnCenter->setY(_btnCenter->getY() + dy);
	_btnRebase->setY(_btnRebase->getY() + dy);
	_btnCancel->setY(_btnCancel->getY() + dy);

	if (_geoState->isPaused() == false)
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
 * Applies the Globe's targeter-graphic to the last-known UFO coordinates.
 */
void GeoscapeCraftState::targeter() // private.
{
	if (_waypoint != nullptr)
		_geoState->getGlobe()->setCrosshair(
										_craft->getTarget()->getLongitude(),
										_craft->getTarget()->getLatitude());
}

}
