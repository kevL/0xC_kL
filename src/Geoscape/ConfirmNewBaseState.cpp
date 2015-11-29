/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif

#include "ConfirmNewBaseState.h"

#include "BaseNameState.h"
#include "Globe.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Confirm New Base window.
 * @param base	- pointer to the Base to place
 * @param globe	- pointer to the Geoscape globe
 */
ConfirmNewBaseState::ConfirmNewBaseState(
		Base* const base,
		Globe* const globe)
	:
		_base(base),
		_globe(globe),
		_cost(0)
{
	_screen = false;

	_window		= new Window(this, 224, 72, 16, 64);
	_txtCost	= new Text(120, 9, 68, 80);
	_txtArea	= new Text(120, 9, 68, 90);
	_btnCancel	= new TextButton(54, 14,  68, 106);
	_btnOk		= new TextButton(54, 14, 138, 106);

	setInterface("geoscape");

	add(_window,	"genericWindow",	"geoscape");
	add(_txtCost,	"genericText",		"geoscape");
	add(_txtArea,	"genericText",		"geoscape");
	add(_btnCancel,	"genericButton2",	"geoscape");
	add(_btnOk,		"genericButton2",	"geoscape");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()))
		{
			_cost = (*i)->getRules()->getBaseCost();
			_txtCost->setText(tr("STR_COST_").arg(Text::formatFunding(_cost)));
			_txtArea->setText(tr("STR_AREA_").arg(tr((*i)->getRules()->getType())));
			break;
		}
	}

	_btnCancel->setText(tr("STR_CANCEL_UC"));
	_btnCancel->onMouseClick((ActionHandler)& ConfirmNewBaseState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ConfirmNewBaseState::btnCancelClick,
					Options::keyCancel);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ConfirmNewBaseState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ConfirmNewBaseState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ConfirmNewBaseState::btnOkClick,
					Options::keyOkKeypad);
}

/**
 * dTor.
 */
ConfirmNewBaseState::~ConfirmNewBaseState()
{}

/**
 * Go to the Place Access Lift screen.
 * @param action - pointer to an Action
 */
void ConfirmNewBaseState::btnOkClick(Action*)
{
	if (_game->getSavedGame()->getFunds() >= _cost)
	{
		_game->getSavedGame()->setFunds(_game->getSavedGame()->getFunds() - _cost);
		_base->setCashSpent(_cost);

		_game->getSavedGame()->getBases()->push_back(_base);
		_game->pushState(new BaseNameState(_base, _globe));
	}
	else
		_game->pushState(new ErrorMessageState(
											tr("STR_NOT_ENOUGH_MONEY"),
											_palette,
											_game->getRuleset()->getInterface("geoscape")->getElement("genericWindow")->color,
											"BACK01.SCR",
											_game->getRuleset()->getInterface("geoscape")->getElement("palette")->color));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ConfirmNewBaseState::btnCancelClick(Action*)
{
	_globe->onMouseOver(nullptr);
	_game->popState();
}

}
