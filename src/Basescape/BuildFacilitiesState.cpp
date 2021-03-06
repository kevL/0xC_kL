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

#include "BuildFacilitiesState.h"

#include "PlaceFacilityState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the BuildFacilities window.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the base State to update
 */
BuildFacilitiesState::BuildFacilitiesState(
		Base* const base,
		State* const state)
	:
		_base(base),
		_state(state)
{
	_fullScreen = false;

	_window			= new Window(
								this,
								128,165,
								192,33,
								POPUP_VERTICAL);
	_txtTitle		= new Text(118, 16, 197, 41);
	_lstFacilities	= new TextList(101, 113, 200, 57);
	_btnOk			= new TextButton(112, 16, 200, 176);

	setInterface("selectFacility");

	add(_window,		"window",	"selectFacility");
	add(_txtTitle,		"text",		"selectFacility");
	add(_lstFacilities,	"list",		"selectFacility");
	add(_btnOk,			"button",	"selectFacility");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BuildFacilitiesState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BuildFacilitiesState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BuildFacilitiesState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BuildFacilitiesState::btnOkClick),
							Options::keyOkKeypad);

	_txtTitle->setText(tr("STR_INSTALLATION"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstFacilities->setColumns(1, 99);
	_lstFacilities->setArrowColor(YELLOW);
	_lstFacilities->setBackground(_window);
	_lstFacilities->setSelectable();
	_lstFacilities->setMargin(2);
	_lstFacilities->onMouseClick(static_cast<ActionHandler>(&BuildFacilitiesState::lstFacilitiesClick));

	populateBuildList();
}

/**
 * dTor.
 */
BuildFacilitiesState::~BuildFacilitiesState()
{}

/**
 * Populates the build list from the current "available" facilities.
 */
void BuildFacilitiesState::populateBuildList() // virtual. Cf, SelectStartFacilityState::populateBuildList()
{
	RuleBaseFacility* facRule;
	const std::vector<std::string>& allFacs (_game->getRuleset()->getBaseFacilitiesList());
	for (std::vector<std::string>::const_iterator
			i = allFacs.begin();
			i != allFacs.end();
			++i)
	{
		facRule = _game->getRuleset()->getBaseFacility(*i);
		if (_game->getSavedGame()->isResearched(facRule->getRequiredResearch()) == true
			&& facRule->isLift() == false)
		{
			_facilities.push_back(facRule);
		}
	}

	for (std::vector<RuleBaseFacility*>::const_iterator
			i = _facilities.begin();
			i != _facilities.end();
			++i)
	{
		_lstFacilities->addRow(1, tr((*i)->getType()).c_str());
	}
}

/**
 * The player can change the selected Base or change info on other screens.
 */
void BuildFacilitiesState::init()
{
	_state->init();
	State::init();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void BuildFacilitiesState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Places the selected Facility.
 * @param action - pointer to an Action
 */
void BuildFacilitiesState::lstFacilitiesClick(Action*) // virtual.
{
	_game->pushState(new PlaceFacilityState(
										_base,
										_facilities[_lstFacilities->getSelectedRow()]));
}

}
