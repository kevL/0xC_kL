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

#include "SelectStartFacilityState.h"

#include "BasescapeState.h" // resetStoresWarning
#include "PlaceLiftState.h"
#include "PlaceStartFacilityState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SelectStartFacility state.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the base State to refresh
 * @param globe	- pointer to the Globe to refresh
 */
SelectStartFacilityState::SelectStartFacilityState(
		Base* const base,
		State* const state,
		Globe* const globe)
	:
		BuildFacilitiesState(base, state),
		_globe(globe)
{
	_facilities = _game->getRuleset()->getCustomBaseFacilities();

	_btnOk->setText(tr("STR_RESET"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SelectStartFacilityState::btnResetClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SelectStartFacilityState::btnResetClick),
							Options::keyCancel);
//	_btnOk->onKeyboardPress(nullptr, Options::keyCancel);

	_lstFacilities->onMouseClick(static_cast<ActionHandler>(&SelectStartFacilityState::lstFacilitiesClick));

	populateBuildList();
}

/**
 * dTor.
 */
SelectStartFacilityState::~SelectStartFacilityState()
{}

/**
 * Populates the build-list from the currently available Facilities.
 */
void SelectStartFacilityState::populateBuildList() // virtual. Cf, BuildFacilitiesState::populateBuildList()
{
	_lstFacilities->clearList();
	for (std::vector<RuleBaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		_lstFacilities->addRow(1, tr((*i)->getType()).c_str());
	}
}

/**
 * Resets the base-building procedure.
 * @param action - pointer to an Action
 */
//void SelectStartFacilityState::btnOkClick(Action*)
void SelectStartFacilityState::btnResetClick(Action*)
{
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		delete *i;
	}

	_base->getFacilities()->clear();

	_game->popState();
	_game->popState();

	_game->pushState(new PlaceLiftState(_base, _globe, true));
}

/**
 * Places the selected Facility.
 * @param action - pointer to an Action
 */
void SelectStartFacilityState::lstFacilitiesClick(Action*)
{
	_game->pushState(new PlaceStartFacilityState(
											_base, this,
											_facilities[_lstFacilities->getSelectedRow()]));
}

/**
 * Callback from PlaceStartFacilityState.
 * @note Erases a placed Facility from the list available for building.
 */
void SelectStartFacilityState::facilityBuilt()
{
	_facilities.erase(_facilities.begin() + static_cast<std::ptrdiff_t>(_lstFacilities->getSelectedRow()));

	if (_facilities.empty() == false)
		populateBuildList();
	else // return to Geoscape, force time-compression to start.
	{
		_game->popState(); // this
		_game->popState(); // PlaceLiftState
	}
}

}
