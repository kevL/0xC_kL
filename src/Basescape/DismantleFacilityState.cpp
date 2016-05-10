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

#include "DismantleFacilityState.h"

#include "../Basescape/BaseView.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

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
 * Initializes all the elements in the DismantleFacility window.
 * @param base	- pointer to the Base to get info from
 * @param view	- pointer to the baseview to update
 * @param fac	- pointer to the facility to dismantle
 */
DismantleFacilityState::DismantleFacilityState(
		Base* const base,
		BaseView* const view,
		const BaseFacility* const fac)
	:
		_base(base),
		_view(view),
		_fac(fac)
{
	_fullScreen = false;

	_window			= new Window(this, 152, 80, 20, 60);

	_txtTitle		= new Text(142, 9, 25, 75);
	_txtFacility	= new Text(142, 9, 25, 85);
	_txtRefund		= new Text(142, 9, 25, 98);

	_btnCancel		= new TextButton(44, 16,  36, 115);
	_btnOk			= new TextButton(44, 16, 112, 115);

	setInterface("dismantleFacility");

	add(_window,		"window",	"dismantleFacility");
	add(_txtTitle,		"text",		"dismantleFacility");
	add(_txtFacility,	"text",		"dismantleFacility");
	add(_txtRefund,		"text",		"dismantleFacility");
	add(_btnCancel,		"button",	"dismantleFacility");
	add(_btnOk,			"button",	"dismantleFacility");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DismantleFacilityState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnOkClick,
					SDLK_y);

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& DismantleFacilityState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnCancelClick,
					Options::keyCancel);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnCancelClick,
					SDLK_c);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& DismantleFacilityState::btnCancelClick,
					SDLK_n);

	_txtTitle->setText(tr("STR_DISMANTLE"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtFacility->setText(tr(_fac->getRules()->getType()));
	_txtFacility->setAlign(ALIGN_CENTER);

	calcRefund();

	_txtRefund->setText(tr("STR_REFUND_")
						.arg(Text::formatCurrency(_refund)));
	_txtRefund->setAlign(ALIGN_CENTER);
}

/**
 * dTor.
 */
DismantleFacilityState::~DismantleFacilityState()
{}

/**
 * Dismantles the facility and returns to the previous screen.
 * @param action - pointer to an Action
 */
void DismantleFacilityState::btnOkClick(Action*)
{
	SavedGame* const gameSave (_game->getSavedGame());

	if (_fac->getRules()->isLift() == false)
	{
		gameSave->setFunds(gameSave->getFunds() + static_cast<int64_t>(_refund));
		_base->addCashIncome(_refund);

		for (std::vector<BaseFacility*>::const_iterator
				i = _base->getFacilities()->begin();
				i != _base->getFacilities()->end();
				++i)
		{
			if (*i == _fac)
			{
				_base->getFacilities()->erase(i);
				_view->resetSelectedFacility();
				delete _fac;

				if (Options::allowBuildingQueue == true)
					_view->reCalcQueuedBuildings();

				break;
			}
		}
	}
	else // Remove whole base if it's the access lift. TODO: And there's not another 'connected' access lift.
	{
		for (std::vector<Base*>::const_iterator
				i = gameSave->getBases()->begin();
				i != gameSave->getBases()->end();
				++i)
		{
			if (*i == _base)
			{
				gameSave->getBases()->erase(i);
				delete _base;
				break;
			}
		}
	}

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DismantleFacilityState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Calculates the refund value.
 * TODO: Zero refund if Base is currently targeted by a UFO Retaliation Run.
 */
void DismantleFacilityState::calcRefund() // private.
{
	const int buildCost (_fac->getRules()->getBuildCost());
	if (_fac->buildFinished() == false)
	{
		if (_fac->getBuildTime() > _fac->getRules()->getBuildTime()) // queued facilities
			_refund = buildCost;
		else
		{
			const float factor (static_cast<float>(_fac->getBuildTime())
							  / static_cast<float>(_fac->getRules()->getBuildTime()));
			_refund = static_cast<int>(ceil(
					  static_cast<float>(buildCost) * factor));
			if (_refund < buildCost / 10)
				_refund = buildCost / 10;
		}
	}
	else
		_refund = buildCost / 10;
}

}
