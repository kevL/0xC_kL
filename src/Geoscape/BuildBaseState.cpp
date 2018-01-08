/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "BuildBaseState.h"

//#include "../fmath.h"

#include "BaseLabelState.h"
#include "ConfirmBuildBaseState.h"
#include "Globe.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the BuildBase window.
 * @param base			- pointer to the Base to place
 * @param globe			- pointer to the geoscape Globe
 * @param isFirstBase	- true if this the player's first base (default false)
 */
BuildBaseState::BuildBaseState(
		Base* const base,
		Globe* const globe,
		bool isFirstBase)
	:
		_base(base),
		_globe(globe),
		_isFirstBase(isFirstBase),
		_latPre(0.),
		_lonPre(0.),
		_mX(0),
		_mY(0)
{
	_fullScreen = false;

	const int dx (_game->getScreen()->getDX());
//	int dy = _game->getScreen()->getDY();

/*	_btnRotateLeft	= new InteractiveSurface(12, 10, 259 + dx * 2, 176 + dy);
	_btnRotateRight	= new InteractiveSurface(12, 10, 283 + dx * 2, 176 + dy);
	_btnRotateUp	= new InteractiveSurface(13, 12, 271 + dx * 2, 162 + dy);
	_btnRotateDown	= new InteractiveSurface(13, 12, 271 + dx * 2, 187 + dy);
	_btnZoomIn		= new InteractiveSurface(23, 23, 295 + dx * 2, 156 + dy);
	_btnZoomOut		= new InteractiveSurface(13, 17, 300 + dx * 2, 182 + dy); */

	_window = new Window(this, 256, 29);
	_window->setX(dx);
//	_window->setDY(0); // -> default when x= 0.

	_txtTitle	= new Text(180, 9, 8 + dx, 11);
	_btnCancel	= new TextButton(54, 14, 194 + dx, 8);

	_hoverTimer	= new Timer(60u);
	_hoverTimer->onTimer(static_cast<StateHandler>(&BuildBaseState::hoverRedraw));
	_hoverTimer->start();

	setInterface("geoscape");

/*	add(_btnRotateLeft);
	add(_btnRotateRight);
	add(_btnRotateUp);
	add(_btnRotateDown);
	add(_btnZoomIn);
	add(_btnZoomOut); */

	add(_window,	"genericWindow",	"geoscape");
	add(_txtTitle,	"genericText",		"geoscape");
	add(_btnCancel,	"genericButton2",	"geoscape");

	_globe->onMouseClick(static_cast<ActionHandler>(&BuildBaseState::globeClick));

/*	_btnRotateLeft->onMousePress(		static_cast<ActionHandler>(&BuildBaseState::btnRotateLeftPress));
	_btnRotateLeft->onMouseRelease(		static_cast<ActionHandler>(&BuildBaseState::btnRotateLeftRelease));
	_btnRotateLeft->onKeyboardPress(	static_cast<ActionHandler>(&BuildBaseState::btnRotateLeftPress),   Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease(	static_cast<ActionHandler>(&BuildBaseState::btnRotateLeftRelease), Options::keyGeoLeft);

	_btnRotateRight->onMousePress(		static_cast<ActionHandler>(&BuildBaseState::btnRotateRightPress));
	_btnRotateRight->onMouseRelease(	static_cast<ActionHandler>(&BuildBaseState::btnRotateRightRelease));
	_btnRotateRight->onKeyboardPress(	static_cast<ActionHandler>(&BuildBaseState::btnRotateRightPress),   Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease(	static_cast<ActionHandler>(&BuildBaseState::btnRotateRightRelease), Options::keyGeoRight);

	_btnRotateUp->onMousePress(		static_cast<ActionHandler>(&BuildBaseState::btnRotateUpPress));
	_btnRotateUp->onMouseRelease(	static_cast<ActionHandler>(&BuildBaseState::btnRotateUpRelease));
	_btnRotateUp->onKeyboardPress(	static_cast<ActionHandler>(&BuildBaseState::btnRotateUpPress),   Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease(static_cast<ActionHandler>(&BuildBaseState::btnRotateUpRelease), Options::keyGeoUp);

	_btnRotateDown->onMousePress(		static_cast<ActionHandler>(&BuildBaseState::btnRotateDownPress));
	_btnRotateDown->onMouseRelease(		static_cast<ActionHandler>(&BuildBaseState::btnRotateDownRelease));
	_btnRotateDown->onKeyboardPress(	static_cast<ActionHandler>(&BuildBaseState::btnRotateDownPress),   Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease(	static_cast<ActionHandler>(&BuildBaseState::btnRotateDownRelease), Options::keyGeoDown);

	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&BuildBaseState::btnZoomInLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&BuildBaseState::btnZoomInRightClick), SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress(static_cast<ActionHandler>(&BuildBaseState::btnZoomInLeftClick),  Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&BuildBaseState::btnZoomOutLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&BuildBaseState::btnZoomOutRightClick), SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress(	static_cast<ActionHandler>(&BuildBaseState::btnZoomOutLeftClick),  Options::keyGeoZoomOut);

	// dirty hacks to get the rotate buttons to work in "classic" style
	_btnRotateLeft->	setListButton();
	_btnRotateRight->	setListButton();
	_btnRotateUp->		setListButton();
	_btnRotateDown->	setListButton(); */

	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_SELECT_SITE_FOR_NEW_BASE"));
	_txtTitle->setAlign(ALIGN_CENTER);

	if (_isFirstBase == true)
		_btnCancel->setVisible(false);
	else
	{
		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onMouseClick(	static_cast<ActionHandler>(&BuildBaseState::btnCancelClick));
		_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&BuildBaseState::btnCancelClick),
									Options::keyCancel);
	}
}

/**
 * dTor.
 */
BuildBaseState::~BuildBaseState()
{
	delete _hoverTimer;
}

/**
 * Stops the Globe and adds radar-hover effect.
 */
void BuildBaseState::init()
{
	State::init();

	_globe->showBaseBuildRadars();
	_globe->onMouseOver(static_cast<ActionHandler>(&BuildBaseState::globeHover));
}

/**
 * Runs the Globe rotation timer.
 */
void BuildBaseState::think()
{
	State::think();
	_globe->think();
	_hoverTimer->think(this, nullptr);
}

/**
 * Handles the Globe.
 * @param action - pointer to an Action
 */
void BuildBaseState::handle(Action* action)
{
	State::handle(action);
	_globe->handle(action, this);
}

/**
 * Processes mouse-hover event for Base placement.
 * @param action - pointer to an Action
 */
void BuildBaseState::globeHover(Action* action)
{
	_mX = static_cast<int>(std::floor(action->getAbsoluteMouseX()));
	_mY = static_cast<int>(std::floor(action->getAbsoluteMouseY()));

	if (_hoverTimer->isRunning() == false)
		_hoverTimer->start();
}

/**
 * Redraws stuff as the cursor is moved over the Globe.
 */
void BuildBaseState::hoverRedraw() // private.
{
	double
		lon,lat;
	_globe->cartToPolar(
					static_cast<Sint16>(_mX),
					static_cast<Sint16>(_mY),
					&lon, &lat);

	if (isNaNorInf(lon,lat) == false)
	{
		_globe->setBaseBuildHoverCoords(lon,lat);

		if (AreSameTwo(
					_lonPre, lon,
					_latPre, lat) == false)
		{
			_lonPre = lon;
			_latPre = lat;
			_globe->invalidate();
		}
	}
}

/**
 * Processes any left-clicks for Base placement.
 * @param action - pointer to an Action
 */
void BuildBaseState::globeClick(Action* action)
{
	const int mY (static_cast<int>(std::floor(action->getAbsoluteMouseY())));
	if (mY > _window->getY() + _window->getHeight())
	{
		const int mX (static_cast<int>(std::floor(action->getAbsoluteMouseX())));
		double
			lon,lat;
		_globe->cartToPolar(
						static_cast<Sint16>(mX),
						static_cast<Sint16>(mY),
						&lon, &lat);

		if (_globe->insideLand(lon,lat) == true)
		{
			_base->setLongitude(lon);
			_base->setLatitude(lat);

			for (std::vector<Craft*>::const_iterator
					i = _base->getCrafts()->begin();
					i != _base->getCrafts()->end();
					++i)
			{
				(*i)->setLongitude(lon);
				(*i)->setLatitude(lat);
			}

			if (_isFirstBase == true)
				_game->pushState(new BaseLabelState(_base, _globe, true));
			else
				_game->pushState(new ConfirmBuildBaseState(_base, _globe));
		}
		else
		{
			const RuleInterface* const uiRule (_game->getRuleset()->getInterface("geoscape"));
			_game->pushState(new ErrorMessageState(
											tr("STR_BASE_CANNOT_BE_BUILT"),
											_palette,
											uiRule->getElement("genericWindow")->color,
											_game->getResourcePack()->getBackgroundRand(),
											uiRule->getElement("backpal")->color));
		}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void BuildBaseState::btnCancelClick(Action*)
{
	_globe->showBaseBuildRadars(false);

	delete _base;
	_game->popState();
}

/**
 * Starts rotating the globe to the left.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateLeftPress(Action*)
{
	_globe->rotateLeft();
} */
/**
 * Stops rotating the globe to the left.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateLeftRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe to the right.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateRightPress(Action*)
{
	_globe->rotateRight();
} */
/**
 * Stops rotating the globe to the right.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateRightRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe upwards.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateUpPress(Action*)
{
	_globe->rotateUp();
} */
/**
 * Stops rotating the globe upwards.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateUpRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Starts rotating the globe downwards.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateDownPress(Action*)
{
	_globe->rotateDown();
} */
/**
 * Stops rotating the globe downwards.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnRotateDownRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Zooms into the globe.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnZoomInLeftClick(Action*)
{
	_globe->zoomIn();
} */
/**
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */
/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
} */
/**
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 *
void BuildBaseState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */

/**
 * Updates the scale.
 * @param dX - x-delta
 * @param dY - y-delta
 */
void BuildBaseState::resize(
		int& dX,
		int& dY)
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);

		if (*i != _window
			&& *i != _btnCancel
			&& *i != _txtTitle)
		{
			(*i)->setY((*i)->getY() + dY / 2);
		}
	}
}

}
