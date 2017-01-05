/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "SelectDestinationState.h"

//#include <cmath>

#include "ConfirmCydoniaState.h"
#include "CraftErrorState.h"
#include "GeoscapeState.h"
#include "Globe.h"
#include "SelectTargetState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Select Destination window.
 * @param craft		- pointer to the Craft
 * @param globe		- pointer to the Globe
 * @param ufoLost	- true if called after ufo-detection was lost
 */
SelectDestinationState::SelectDestinationState(
		Craft* const craft,
		Globe* const globe,
		bool ufoLost)
	:
		_craft(craft),
		_globe(globe),
		_ufoLost(ufoLost)
{
	//Log(LOG_INFO) << "SelectDestinationState cTor";
	_fullScreen = false;

	const int dX (_game->getScreen()->getDX());
//	int dy = _game->getScreen()->getDY();

/*	_btnRotateLeft	= new InteractiveSurface(12, 10, 259 + dX * 2, 176 + dy);
	_btnRotateRight	= new InteractiveSurface(12, 10, 283 + dX * 2, 176 + dy);
	_btnRotateUp	= new InteractiveSurface(13, 12, 271 + dX * 2, 162 + dy);
	_btnRotateDown	= new InteractiveSurface(13, 12, 271 + dX * 2, 187 + dy);
	_btnZoomIn		= new InteractiveSurface(23, 23, 295 + dX * 2, 156 + dy);
	_btnZoomOut		= new InteractiveSurface(13, 17, 300 + dX * 2, 182 + dy); */

	_window = new Window(this, 256, 30);
	_window->setX(dX);
//	_window->setDY(0); // -> default when x= 0.

//	_txtTitle	= new Text(100, 9, 16 + dX, 10);

	_btnCancel	= new TextButton(60, 14, 120 + dX, 8);
	_btnCydonia	= new TextButton(60, 14, 180 + dX, 8);

	_txtTooFar	= new Text(100, 9, 12 + dX, 11);

	setInterface("geoscape");

/*	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut); */

	add(_window,		"genericWindow",	"geoscape");
//	add(_txtTitle,		"genericText",		"geoscape");
	add(_txtTooFar,		"genericText",		"geoscape");
	add(_btnCancel,		"genericButton1",	"geoscape");
	add(_btnCydonia,	"genericButton1",	"geoscape");


	_txtTooFar->setText(tr("STR_OUTSIDE_CRAFT_RANGE"));
	_txtTooFar->setVisible(false);

	// NOTE: Replacing onMouseClick() w/ onMousePress() causes a bizarre CTD situation.
	_globe->onMouseClick(static_cast<ActionHandler>(&SelectDestinationState::globeClick));

/*	_btnRotateLeft->onMousePress(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateLeftPress));
	_btnRotateLeft->onMouseRelease(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateLeftRelease));
	_btnRotateLeft->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateLeftPress),   Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateLeftRelease), Options::keyGeoLeft);

	_btnRotateRight->onMousePress(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateRightPress));
	_btnRotateRight->onMouseRelease(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateRightRelease));
	_btnRotateRight->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateRightPress),   Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateRightRelease), Options::keyGeoRight);

	_btnRotateUp->onMousePress(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateUpPress));
	_btnRotateUp->onMouseRelease(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateUpRelease));
	_btnRotateUp->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateUpPress),   Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease(static_cast<ActionHandler>(&SelectDestinationState::btnRotateUpRelease), Options::keyGeoUp);

	_btnRotateDown->onMousePress(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateDownPress));
	_btnRotateDown->onMouseRelease(		static_cast<ActionHandler>(&SelectDestinationState::btnRotateDownRelease));
	_btnRotateDown->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateDownPress),   Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease(	static_cast<ActionHandler>(&SelectDestinationState::btnRotateDownRelease), Options::keyGeoDown);

	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&SelectDestinationState::btnZoomInLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&SelectDestinationState::btnZoomInRightClick), SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress(static_cast<ActionHandler>(&SelectDestinationState::btnZoomInLeftClick),  Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&SelectDestinationState::btnZoomOutLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&SelectDestinationState::btnZoomOutRightClick), SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnZoomOutLeftClick),  Options::keyGeoZoomOut);

	// dirty hacks to get the rotate buttons to work in "classic" style
	_btnRotateLeft	->setListButton();
	_btnRotateRight	->setListButton();
	_btnRotateUp	->setListButton();
	_btnRotateDown	->setListButton(); */

	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&SelectDestinationState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&SelectDestinationState::btnCancelClick),
								Options::keyCancel);

//	_txtTitle->setText(tr("STR_SELECT_DESTINATION"));
//	_txtTitle->setVerticalAlign(ALIGN_MIDDLE);
//	_txtTitle->setWordWrap();

	if (_craft->getRules()->getSpacecraft() == true
		&& _craft->getFuel() == _craft->getRules()->getFuelCapacity()
		&& _craft->getQtySoldiers() != 0 //|| _craft->getQtyVehicles() > 0
		&& _game->getSavedGame()->isResearched(_game->getRuleset()->getFinalResearch()) == true)
	{
		for (std::vector<Soldier*>::const_iterator // if all Soldiers have Power or Flight suits .......
			i = _craft->getBase()->getSoldiers()->begin();
			i != _craft->getBase()->getSoldiers()->end();
			++i)
		{
			if ((*i)->getCraft() == _craft						// TODO: Allow click but then give warning in btnCydoniaClick()
				&& (*i)->getArmor()->isSpacesuit() == false)	// - if no soldiers aboard,
			{													// - if not suited appropriately,
				_btnCydonia->setVisible(false);					// - if fuel not full.
				break;
			}
		}
	}
	else
		_btnCydonia->setVisible(false);

	if (_btnCydonia->getVisible() == true)
	{
		_btnCydonia->setText(tr("STR_CYDONIA"));
		_btnCydonia->onMouseClick(		static_cast<ActionHandler>(&SelectDestinationState::btnCydoniaClick));
		_btnCydonia->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnCydoniaClick),
										Options::keyOk);
		_btnCydonia->onKeyboardPress(	static_cast<ActionHandler>(&SelectDestinationState::btnCydoniaClick),
										Options::keyOkKeypad);
	}
	else
	{
		_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&SelectDestinationState::btnCancelClick),
									Options::keyOk);
		_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&SelectDestinationState::btnCancelClick),
									Options::keyOkKeypad);
	}
}

/**
 * dTor.
 */
SelectDestinationState::~SelectDestinationState()
{
	//Log(LOG_INFO) << "SelectDestinationState dTor";
}

/**
 * Initializes the State.
 */
void SelectDestinationState::init()
{
	//Log(LOG_INFO) << "SelectDestinationState::init()";
	State::init();
}

/**
 * Runs the Globe's rotation Timer.
 *
void SelectDestinationState::think()
{
	State::think();
	_globe->think();
} */

/**
 * Handles the Globe.
 * @param action - pointer to an Action
 */
void SelectDestinationState::handle(Action* action)
{
	//Log(LOG_INFO) << "SelectDestinationState::handle()";
	State::handle(action);

	if (static_cast<int>(std::floor(action->getAbsoluteMouseY())) > _window->getY() + _window->getHeight()) // ignore window clicks
		_globe->handle(action, this);
}

/**
 * Processes left-clicks for picking a Target and ensures that a targeting
 * crosshair if visible gets cleared from the Globe.
 * @param action - pointer to an Action
 */
void SelectDestinationState::globeClick(Action* action)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "SelectDestinationState::globeClick()";
	const int mY (static_cast<int>(std::floor(action->getAbsoluteMouseY())));
	if (mY > _window->getY() + _window->getHeight()) // ignore window clicks
	{
		if (_ufoLost == true)
		{
			_ufoLost = false;
			_globe->clearCrosshair();
			_craft->setTarget();
			_globe->draw();
		}

		const int mX (static_cast<int>(std::floor(action->getAbsoluteMouseX())));
		double
			lon,lat;
		_globe->cartToPolar(
						static_cast<Sint16>(mX),
						static_cast<Sint16>(mY),
						&lon,&lat);

		Waypoint* const wp (new Waypoint());
		wp->setLongitude(lon);
		wp->setLatitude(lat);

		//Log(LOG_INFO) << ". getDistanceLeft= " << _craft->getDistanceLeft();
		//Log(LOG_INFO) << ". getDistanceReserved= " << _craft->getDistanceReserved(wp);

		if (_craft->getDistanceLeft(true) < _craft->getDistanceReserved(wp))
		{
			_txtTooFar->setVisible();
			delete wp;
		}
		else
		{
			_txtTooFar->setVisible(false);

			std::vector<Target*> targets (_globe->getTargets(mX, mY));
			std::vector<Target*>::const_iterator i (std::find( // do not show Craft's current Target
															targets.begin(),
															targets.end(),
															_craft->getTarget()));
			if (i != targets.end())
				targets.erase(i);

			if (targets.empty() == false)
				delete wp;
			else
				targets.push_back(wp);

			_game->pushState(new SelectTargetState(targets, _craft));
		}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SelectDestinationState::btnCancelClick(Action*)
{
	if (_ufoLost == true)
	{
		_globe->clearCrosshair();
		_craft->setTarget();
	}
	_game->popState();
}

/**
 * Goes to Cydonia on Mars.
 * @param action - pointer to an Action
 */
void SelectDestinationState::btnCydoniaClick(Action*)
{
	if (_ufoLost == true) // NOTE: This is kinda pointless ... you ain't comin' back to Earth, son.
	{
		_globe->clearCrosshair();
		_craft->setTarget();
	}
	_game->pushState(new ConfirmCydoniaState(_craft));
}

/**
 * Updates the scale.
 * @param dX - x-delta
 * @param dY - y-delta
 */
void SelectDestinationState::resize(
		int& dX,
		int& dY)
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);

		if (   *i != _window
			&& *i != _btnCancel
//			&& *i != _txtTitle
			&& *i != _btnCydonia)
		{
			(*i)->setY((*i)->getY() + dY / 2);
		}
	}
}

}
/**
 * Starts rotating the globe to the left.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateLeftPress(Action*)
{
	_globe->rotateLeft();
} */
/**
 * Stops rotating the globe to the left.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateLeftRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe to the right.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateRightPress(Action*)
{
	_globe->rotateRight();
} */
/**
 * Stops rotating the globe to the right.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateRightRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe upwards.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateUpPress(Action*)
{
	_globe->rotateUp();
} */
/**
 * Stops rotating the globe upwards.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateUpRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Starts rotating the globe downwards.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateDownPress(Action*)
{
	_globe->rotateDown();
} */
/**
 * Stops rotating the globe downwards.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnRotateDownRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Zooms into the globe.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnZoomInLeftClick(Action*)
{
	_globe->zoomIn();
} */
/**
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */
/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
} */
/**
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 *
void SelectDestinationState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */
