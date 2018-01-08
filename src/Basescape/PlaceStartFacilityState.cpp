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

#include "PlaceStartFacilityState.h"

#include "BaseView.h"
#include "SelectStartFacilityState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"

#include "../Interface/Text.h"

#include "../Menu/ErrorMessageState.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the PlaceStartFacility state.
 * @param base		- pointer to the base to get info from
 * @param select	- pointer to the selection state
 * @param facRule	- pointer to the facility ruleset to build
 */
PlaceStartFacilityState::PlaceStartFacilityState(
		Base* const base,
		SelectStartFacilityState* const select,
		const RuleBaseFacility* const facRule)
	:
		PlaceFacilityState(base, facRule),
		_select(select)
{
	_baseLayout->onMouseClick(static_cast<ActionHandler>(&PlaceStartFacilityState::baseLayoutClick));

	_txtCostAmount->setText(tr("STR_NONE"));
	_txtTimeAmount->setText(tr("STR_NONE"));
}

/**
 * dTor.
 */
PlaceStartFacilityState::~PlaceStartFacilityState()
{}

/**
 * Processes clicking on Facilities.
 * @param action - pointer to an Action
 */
void PlaceStartFacilityState::baseLayoutClick(Action*)
{
	if (_baseLayout->isPlaceable(_facRule) == true)
	{
		BaseFacility* const fac (new BaseFacility(_facRule, _base));
		fac->setX(_baseLayout->getGridX());
		fac->setY(_baseLayout->getGridY());

		_base->getFacilities()->push_back(fac);
		_game->popState();
		_select->facilityBuilt();
	}
	else
	{
		_game->popState();
		const RuleInterface* const uiRule (_game->getRuleset()->getInterface("basescape"));
		_game->pushState(new ErrorMessageState(
											tr("STR_CANNOT_BUILD_HERE"),
											_palette,
											uiRule->getElement("errorMessage")->color,
											"BACK01.SCR",
											uiRule->getElement("errorPalette")->color));
	}
}

}
