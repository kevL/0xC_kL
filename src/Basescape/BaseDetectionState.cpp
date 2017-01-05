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

#include "BaseDetectionState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the BaseDetection window.
 * @param base - pointer to the Base to get info from
 */
BaseDetectionState::BaseDetectionState(const Base* const base)
	:
		_base(base)
{
	_fullScreen = false;

	_window		= new Window(this, 200, 100, 60, 50, POPUP_BOTH);
	_txtTitle	= new Text(200, 17, 60, 60);

	_txtActivity		= new Text(60, 9,  76, 83);
	_txtFacilitiesVal	= new Text(15, 9, 136, 83);
	_txtShields			= new Text(60, 9,  76, 93);
	_txtShieldsVal		= new Text(15, 9, 136, 93);
//	_txtDifficulty		= new Text(60, 9,  76, 103);
//	_txtDifficultyVal	= new Text(15, 9, 136, 103);
	_txtSpotted			= new Text(200, 9, 60, 109);

	_txtExposure	= new Text(102,  9, 152,  79);
	_txtExposureVal	= new Text(102, 16, 152,  90);
//	_txtTimePeriod	= new Text(102,  9, 152, 107);

	_btnOk = new TextButton(168, 16, 76, 125);

	setPalette(PAL_BASESCAPE, BACKPAL_ORANGE);

	add(_window);
	add(_txtTitle);
	add(_txtActivity);
	add(_txtShields);
//	add(_txtDifficulty);
	add(_txtFacilitiesVal);
	add(_txtShieldsVal);
//	add(_txtDifficultyVal);
	add(_txtSpotted);
	add(_txtExposure);
	add(_txtExposureVal);
//	add(_txtTimePeriod);
	add(_btnOk);

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));
	_window->setColor(PURPLE);

	_txtTitle->setText(tr("STR_BASE_DETECTION"));
	_txtTitle->setColor(PURPLE);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(PURPLE);
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BaseDetectionState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDetectionState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDetectionState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDetectionState::btnOkClick),
							Options::keyOkKeypad);

	_txtActivity->setText(tr("STR_ACTIVITY"));
	_txtActivity->setColor(PURPLE);

	_txtShields->setText(tr("STR_MINDSHIELDS"));
	_txtShields->setColor(PURPLE);

//	_txtDifficulty->setColor(PURPLE);
//	_txtDifficulty->setText(tr("STR_DIFFICULTY"));

	if (_base->getBaseExposed() == true)
	{
		_timerBlink = new Timer(325u);
		_timerBlink->onTimer(static_cast<StateHandler>(&BaseDetectionState::blink));
		_timerBlink->start();

		_txtSpotted->setText(tr("STR_SPOTTED"));
		_txtSpotted->setColor(RED);
		_txtSpotted->setHighContrast();
		_txtSpotted->setAlign(ALIGN_CENTER);
	}
	_txtSpotted->setVisible(false); // wait for blink.

	_txtExposure->setText(tr("STR_EXPOSURE"));
	_txtExposure->setColor(PURPLE);
	_txtExposure->setAlign(ALIGN_CENTER);

//	_txtTimePeriod->setColor(PURPLE);
//	_txtTimePeriod->setAlign(ALIGN_CENTER);
//	_txtTimePeriod->setText(tr("STR_PER10MIN"));

	// TODO: Add gravShield info. And baseDefense power.
	int
		facQty,
		shields,
		det = _base->getExposedChance(
									static_cast<int>(_game->getSavedGame()->getDifficulty()),
									&facQty,
									&shields);

	_txtFacilitiesVal->setColor(PURPLE);
	_txtFacilitiesVal->setText(Text::intWide(facQty));

	if (_game->getSavedGame()->isResearched("STR_MIND_SHIELD") == true)
	{
		std::wostringstream woststr;
		switch (shields)
		{
			case 0:  woststr << L"-"; break;
			default: woststr << shields;
		}
		_txtShieldsVal->setText(woststr.str());
		_txtShieldsVal->setColor(PURPLE);
	}
	else
	{
		_txtShields->setVisible(false);
		_txtShieldsVal->setVisible(false);
	}

//	_txtDifficultyVal->setColor(PURPLE);
//	woststr3 << diff;
//	_txtDifficultyVal->setText(woststr3.str());

	_txtExposureVal->setColor(YELLOW);
	_txtExposureVal->setHighContrast();
	_txtExposureVal->setBig();
	_txtExposureVal->setAlign(ALIGN_CENTER);
	_txtExposureVal->setText(Text::intWide(det));
}

/**
 * dTor.
 */
BaseDetectionState::~BaseDetectionState()
{
	if (_base->getBaseExposed() == true)
		delete _timerBlink;
}

/**
 * Runs the blink Timer.
 */
void BaseDetectionState::think()
{
	if (_window->isPopupDone() == false)
		_window->think();
	else if (_base->getBaseExposed() == true)
		_timerBlink->think(this, nullptr);
}

/**
 * Blinks the message Text.
 */
void BaseDetectionState::blink()
{
	_txtSpotted->setVisible(!_txtSpotted->getVisible());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void BaseDetectionState::btnOkClick(Action*)
{
	_game->popState();
}

}
