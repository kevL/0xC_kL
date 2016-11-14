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

#include "ResearchListState.h"

#include "ResearchInfoState.h"

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

#include "../Savegame/Base.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * List that allows player to choose what to research.
 * @note Initializes all the elements in the ResearchList screen.
 * @param base - pointer to the Base to get info from
 */
ResearchListState::ResearchListState(
		Base* const base)
	:
		_base(base),
		_cutoff(-1),
		_scroll(0u)
{
	_fullScreen = false;

	_window			= new Window(this, 230, 160, 45, 25, POPUP_BOTH);
	_txtTitle		= new Text(214, 16, 53, 33);
	_lstResearch	= new TextList(190, 113, 61, 44);
	_btnCancel		= new TextButton(214, 16, 53, 162);

	setInterface("selectNewResearch");

	add(_window,		"window",	"selectNewResearch");
	add(_txtTitle,		"text",		"selectNewResearch");
	add(_lstResearch,	"list",		"selectNewResearch");
	add(_btnCancel,		"button",	"selectNewResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_NEW_RESEARCH_PROJECTS"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_lstResearch->setColumns(1, 180);
	_lstResearch->setBackground(_window);
	_lstResearch->setSelectable();
	_lstResearch->setAlign(ALIGN_CENTER);
	_lstResearch->onMouseClick(static_cast<ActionHandler>(&ResearchListState::lstStartClick));

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&ResearchListState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ResearchListState::btnCancelClick),
								Options::keyCancel);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ResearchListState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&ResearchListState::btnCancelClick),
								Options::keyOkKeypad);
}

/**
 * dTor.
 */
ResearchListState::~ResearchListState()
{}

/**
 * Initializes the screen and fills the list with ResearchProjects.
 */
void ResearchListState::init()
{
	State::init();

	fillProjectList();
	_lstResearch->scrollTo(_scroll);
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Selects the RuleResearch to work on.
 * @note If an offline ResearchProject is selected the spent & cost values are preserved.
 * @param action - pointer to an Action
 */
void ResearchListState::lstStartClick(Action*) // private.
{
	_scroll = _lstResearch->getScroll();

	if (static_cast<int>(_lstResearch->getSelectedRow()) > _cutoff)	// new project
		_game->pushState(new ResearchInfoState(
											_base,
											_unlocked[static_cast<size_t>(static_cast<int>(_lstResearch->getSelectedRow()) - (_cutoff + 1))]));
	else
		_game->pushState(new ResearchInfoState(						// offline project reactivation.
											_base,
											_offlineProjects[_lstResearch->getSelectedRow()]));
//	_game->popState();
}

/**
 * Fills the list with ResearchProjects.
 */
void ResearchListState::fillProjectList() // private.
{
	_cutoff = -1;
	_offlineProjects.clear();
	_unlocked.clear();
	_lstResearch->clearList();

	size_t r (0u);

	for (std::vector<ResearchProject*>::const_iterator
			i = _base->getResearch().begin();
			i != _base->getResearch().end();
			++i)
	{
		// If cancelled projects are not marked 'offline' they'd lose spent
		// research time; if they are not marked 'offline' even though no
		// spent time has accumulated, they still need to be tracked so player
		// can't exploit the system by forcing a recalculation of totalCost ....
		if ((*i)->getOffline() == true)
		{
			std::wstring wst (tr((*i)->getRules()->getType()));
			if ((*i)->getSpent() != 0)
				wst += L" (" + Text::intWide((*i)->getSpent()) + L")";

			_lstResearch->addRow(1, wst.c_str());
			_lstResearch->setRowColor(r++, GRAY);

			_offlineProjects.push_back(*i);
			++_cutoff;
		}
	}

	const Uint8 color (_lstResearch->getSecondaryColor());
	std::string type;

	_game->getSavedGame()->tabulateStartableResearch(_unlocked, _base);
	for (std::vector<const RuleResearch*>::const_iterator
			i = _unlocked.begin();
			i != _unlocked.end();
			)
	{
		if ((*i)->getCost() != 0)
		{
			type = (*i)->getType();
			_lstResearch->addRow(1, tr(type).c_str());

			if (_game->getRuleset()->getUnitRule(type) != nullptr // mark researched aliens yellow.
				&& _game->getSavedGame()->isResearched(type) == true)
			{
				_lstResearch->setRowColor(r, color, true);
			}
			++r;
			++i;
		}
		else						// fake project.
			i = _unlocked.erase(i);	// erase it so it doesn't show up in ResearchInfoState list either.
	}
}

}
