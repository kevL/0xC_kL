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

#include "BuildNewBaseState.h"

//#include <cmath>

//#include "../fmath.h"

#include "BaseNameState.h"
#include "ConfirmNewBaseState.h"
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
 * Initializes all the elements in the BuildNewBase window.
 * @param base		- pointer to the Base to place
 * @param globe		- pointer to the geoscape Globe
 * @param firstBase	- true if this the first base in the game (default false)
 */
BuildNewBaseState::BuildNewBaseState(
		Base* const base,
		Globe* const globe,
		bool firstBase)
	:
		_base(base),
		_globe(globe),
		_firstBase(firstBase),
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
	_hoverTimer->onTimer((StateHandler)& BuildNewBaseState::hoverRedraw);
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

	_globe->onMouseClick((ActionHandler)& BuildNewBaseState::globeClick);

/*	_btnRotateLeft->onMousePress((ActionHandler)&BuildNewBaseState::btnRotateLeftPress);
	_btnRotateLeft->onMouseRelease((ActionHandler)&BuildNewBaseState::btnRotateLeftRelease);
	_btnRotateLeft->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnRotateLeftPress, Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease((ActionHandler)&BuildNewBaseState::btnRotateLeftRelease, Options::keyGeoLeft);

	_btnRotateRight->onMousePress((ActionHandler)&BuildNewBaseState::btnRotateRightPress);
	_btnRotateRight->onMouseRelease((ActionHandler)&BuildNewBaseState::btnRotateRightRelease);
	_btnRotateRight->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnRotateRightPress, Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease((ActionHandler)&BuildNewBaseState::btnRotateRightRelease, Options::keyGeoRight);

	_btnRotateUp->onMousePress((ActionHandler)&BuildNewBaseState::btnRotateUpPress);
	_btnRotateUp->onMouseRelease((ActionHandler)&BuildNewBaseState::btnRotateUpRelease);
	_btnRotateUp->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnRotateUpPress, Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease((ActionHandler)&BuildNewBaseState::btnRotateUpRelease, Options::keyGeoUp);

	_btnRotateDown->onMousePress((ActionHandler)&BuildNewBaseState::btnRotateDownPress);
	_btnRotateDown->onMouseRelease((ActionHandler)&BuildNewBaseState::btnRotateDownRelease);
	_btnRotateDown->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnRotateDownPress, Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease((ActionHandler)&BuildNewBaseState::btnRotateDownRelease, Options::keyGeoDown);

	_btnZoomIn->onMouseClick((ActionHandler)&BuildNewBaseState::btnZoomInLeftClick, SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick((ActionHandler)&BuildNewBaseState::btnZoomInRightClick, SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnZoomInLeftClick, Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick((ActionHandler)&BuildNewBaseState::btnZoomOutLeftClick, SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick((ActionHandler)&BuildNewBaseState::btnZoomOutRightClick, SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress((ActionHandler)&BuildNewBaseState::btnZoomOutLeftClick, Options::keyGeoZoomOut);

	// dirty hacks to get the rotate buttons to work in "classic" style
	_btnRotateLeft->setListButton();
	_btnRotateRight->setListButton();
	_btnRotateUp->setListButton();
	_btnRotateDown->setListButton(); */

	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_SELECT_SITE_FOR_NEW_BASE"));
	_txtTitle->setAlign(ALIGN_CENTER);

	if (_firstBase == true)
		_btnCancel->setVisible(false);
	else
	{
		_btnCancel->setText(tr("STR_CANCEL_UC"));
		_btnCancel->onMouseClick((ActionHandler)& BuildNewBaseState::btnCancelClick);
		_btnCancel->onKeyboardPress(
						(ActionHandler)& BuildNewBaseState::btnCancelClick,
						Options::keyCancel);
	}

	_showRadar = Options::globeRadarLines;
	Options::globeRadarLines = true;
}

/**
 * dTor.
 */
BuildNewBaseState::~BuildNewBaseState()
{
	if (Options::globeRadarLines != _showRadar)
		Options::globeRadarLines = false;

	delete _hoverTimer;
}

/**
 * Stops the Globe and adds radar hover effect.
 */
void BuildNewBaseState::init()
{
	State::init();
	_globe->onMouseOver((ActionHandler)& BuildNewBaseState::globeHover);
	_globe->setNewBaseHover();
}

/**
 * Runs the Globe rotation timer.
 */
void BuildNewBaseState::think()
{
	State::think();
	_globe->think();
	_hoverTimer->think(this, nullptr);
}

/**
 * Handles the Globe.
 * @param action - pointer to an Action
 */
void BuildNewBaseState::handle(Action* action)
{
	State::handle(action);
	_globe->handle(action, this);
}

/**
 * Processes mouse-hover event for Base placement.
 * @param action - pointer to an Action
 */
void BuildNewBaseState::globeHover(Action* action)
{
	_mX = static_cast<int>(std::floor(action->getAbsoluteXMouse()));
	_mY = static_cast<int>(std::floor(action->getAbsoluteYMouse()));

	if (_hoverTimer->isRunning() == false)
		_hoverTimer->start();
}

/**
 * Redraws stuff as the cursor is moved over the Globe.
 */
void BuildNewBaseState::hoverRedraw()
{
	double
		lon,lat;
	_globe->cartToPolar(
					static_cast<Sint16>(_mX),
					static_cast<Sint16>(_mY),
					&lon, &lat);

	if (isNaNorInf(lon,lat) == false)
	{
		_globe->setNewBaseHoverPos(lon,lat);
		_globe->setNewBaseHover();
	}

	if (Options::globeRadarLines == true
		&& !(AreSame(_latPre, lat) && AreSame(_lonPre, lon)))
	{
		_latPre = lat;
		_lonPre = lon;
		_globe->invalidate();
	}
}

/**
 * Processes any left-clicks for Base placement.
 * @param action - pointer to an Action
 */
void BuildNewBaseState::globeClick(Action* action)
{
	const int mouseY (static_cast<int>(std::floor(action->getAbsoluteYMouse())));
	if (mouseY > _window->getY() + _window->getHeight())
	{
		const int mouseX (static_cast<int>(std::floor(action->getAbsoluteXMouse())));
		double
			lon,lat;
		_globe->cartToPolar(
						static_cast<Sint16>(mouseX),
						static_cast<Sint16>(mouseY),
						&lon, &lat);

//		if (action->getDetails()->button.button == SDL_BUTTON_LEFT) // This is called only on LMB.
//		{
		if (_globe->insideLand(lon, lat) == true)
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

			if (_firstBase == true)
				_game->pushState(new BaseNameState(_base, _globe, true));
			else
				_game->pushState(new ConfirmNewBaseState(_base, _globe));
		}
		else
			_game->pushState(new ErrorMessageState(
											tr("STR_XCOM_BASE_CANNOT_BE_BUILT"),
											_palette,
											_game->getRuleset()->getInterface("geoscape")->getElement("genericWindow")->color,
											_game->getResourcePack()->getRandomBackground(),
											_game->getRuleset()->getInterface("geoscape")->getElement("backpal")->color));
//		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BuildNewBaseState::btnCancelClick(Action*)
{
	delete _base;
	_game->popState();
}

/**
 * Starts rotating the globe to the left.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateLeftPress(Action*)
{
	_globe->rotateLeft();
} */
/**
 * Stops rotating the globe to the left.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateLeftRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe to the right.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateRightPress(Action*)
{
	_globe->rotateRight();
} */
/**
 * Stops rotating the globe to the right.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateRightRelease(Action*)
{
	_globe->rotateStopLon();
} */
/**
 * Starts rotating the globe upwards.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateUpPress(Action*)
{
	_globe->rotateUp();
} */
/**
 * Stops rotating the globe upwards.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateUpRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Starts rotating the globe downwards.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateDownPress(Action*)
{
	_globe->rotateDown();
} */
/**
 * Stops rotating the globe downwards.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnRotateDownRelease(Action*)
{
	_globe->rotateStopLat();
} */
/**
 * Zooms into the globe.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnZoomInLeftClick(Action*)
{
	_globe->zoomIn();
} */
/**
 * Zooms the globe maximum.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnZoomInRightClick(Action*)
{
	_globe->zoomMax();
} */
/**
 * Zooms out of the globe.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnZoomOutLeftClick(Action*)
{
	_globe->zoomOut();
} */
/**
 * Zooms the globe minimum.
 * @param action - pointer to an Action
 *
void BuildNewBaseState::btnZoomOutRightClick(Action*)
{
	_globe->zoomMin();
} */

/**
 * Updates the scale.
 * @param dX - x-delta
 * @param dY - y-delta
 */
void BuildNewBaseState::resize(
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
