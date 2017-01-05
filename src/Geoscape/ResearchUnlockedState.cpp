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

#include "ResearchUnlockedState.h"

#include "../Basescape/ResearchState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ResearchUnlocked screen.
 * @param base		- pointer to the Base to get info from
 * @param projects	- reference to a vector of pointers to RuleResearch projects
 * @param allocate	- true to show the allocate research button
 */
ResearchUnlockedState::ResearchUnlockedState(
		Base* const base,
		const std::vector<const RuleResearch*>& projects,
		bool allocate)
	:
		_base(base)
{
	_fullScreen = false;

	_window				= new Window(this, 288, 180, 16, 10);
	_txtTitle			= new Text(288, 17, 16, 20);

	_lstPossibilities	= new TextList(253, 81, 24, 56);

	_btnResearch		= new TextButton(160, 14, 80, 149);
	_btnOk				= new TextButton(160, 14, 80, 165);

	setInterface("geoResearch");

	add(_window,			"window",	"geoResearch");
	add(_txtTitle,			"text1",	"geoResearch");
	add(_lstPossibilities,	"text2",	"geoResearch");
	add(_btnResearch,		"button",	"geoResearch");
	add(_btnOk,				"button",	"geoResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setText(tr((allocate == true) ? "STR_OK" : "STR_MORE"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ResearchUnlockedState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchUnlockedState::btnOkClick),
							Options::keyCancel);

	_btnResearch->setText(tr("STR_ALLOCATE_RESEARCH"));
	_btnResearch->onMouseClick(		static_cast<ActionHandler>(&ResearchUnlockedState::btnResearchClick));
	_btnResearch->onKeyboardPress(	static_cast<ActionHandler>(&ResearchUnlockedState::btnResearchClick),
									Options::keyOk);
	_btnResearch->onKeyboardPress(	static_cast<ActionHandler>(&ResearchUnlockedState::btnResearchClick),
									Options::keyOkKeypad);
	_btnResearch->setVisible(allocate == true);

	_txtTitle->setText(tr("STR_WE_CAN_NOW_RESEARCH"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstPossibilities->setColumns(1, 261);
	_lstPossibilities->setAlign(ALIGN_CENTER);
	_lstPossibilities->setBig();

	for (std::vector<const RuleResearch*>::const_iterator
			i = projects.begin();
			i != projects.end();
			++i)
	{
		_lstPossibilities->addRow(1, tr((*i)->getType()).c_str());
	}
}

/**
 * dTor.
 */
ResearchUnlockedState::~ResearchUnlockedState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchUnlockedState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the ResearchState so the player can dispatch available scientists.
 * @param action - pointer to an Action
 */
void ResearchUnlockedState::btnResearchClick(Action*)
{
	_game->popState();
	_game->pushState(new ResearchState(_base));
}

}
