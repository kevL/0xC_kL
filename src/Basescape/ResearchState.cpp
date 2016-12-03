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

#include "ResearchState.h"

//#include <sstream>

#include "AlienContainmentState.h"
#include "BasescapeState.h"
#include "MiniBaseView.h"
#include "ResearchInfoState.h"
#include "ResearchListState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleResearch.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"

#include "../Ufopaedia/TechTreeViewerState.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the main Research screen.
 * @param base	- pointer to the base to get info from
 * @param state	- pointer to the BasescapeState (default nullptr when geoscape-invoked)
 */
ResearchState::ResearchState(
		Base* const base,
		BasescapeState* const state)
	:
		_base(base),
		_state(state),
		_baseList(_game->getSavedGame()->getBases())
{
	_window			= new Window(this);
	_mini			= new MiniBaseView(128, 16, 180, 27, MBV_RESEARCH);

	_txtTitle		= new Text(320, 17,   0, 10);
	_txtBaseLabel	= new Text( 80,  9,  16, 10);
	_txtHoverBase	= new Text( 80,  9, 224, 10);

	_txtAllocated	= new Text(60, 9, 16, 26);
	_txtAvailable	= new Text(60, 9, 16, 35);

	_txtSpace		= new Text(100, 9, 80, 26);

	_txtProject		= new Text(110, 9, 16, 49);
	_txtScientists	= new Text( 55, 9, 161, 49);
	_txtProgress	= new Text( 55, 9, 219, 49);

	_lstResearch	= new TextList(288, 105, 16, 64);

	_btnAliens		= new TextButton(92, 16,  16, 177);
	_btnProjects	= new TextButton(92, 16, 114, 177);
	_btnOk			= new TextButton(92, 16, 212, 177);

	setInterface("researchMenu");

	add(_window,		"window",	"researchMenu");
	add(_mini,			"miniBase",	"basescape"); // <-
	add(_txtTitle,		"text",		"researchMenu");
	add(_txtBaseLabel,	"text",		"researchMenu");
	add(_txtHoverBase,	"numbers",	"baseInfo"); // <-
	add(_txtAvailable,	"text",		"researchMenu");
	add(_txtAllocated,	"text",		"researchMenu");
	add(_txtSpace,		"text",		"researchMenu");
	add(_txtProject,	"text",		"researchMenu");
	add(_txtScientists,	"text",		"researchMenu");
	add(_txtProgress,	"text",		"researchMenu");
	add(_lstResearch,	"list",		"researchMenu");
	add(_btnAliens,		"button",	"researchMenu");
	add(_btnProjects,	"button",	"researchMenu");
	add(_btnOk,			"button",	"researchMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_baseList);
	for (size_t
			i = 0u;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == _base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(static_cast<ActionHandler>(&ResearchState::miniClick));
	_mini->onMouseOver(	static_cast<ActionHandler>(&ResearchState::miniMouseOver));
	_mini->onMouseOut(	static_cast<ActionHandler>(&ResearchState::miniMouseOut));
	// TODO: onKeyboardPress(). See BaseInfoState.

	_txtHoverBase->setAlign(ALIGN_RIGHT);

	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick(static_cast<ActionHandler>(&ResearchState::btnAliensClick));

	_btnProjects->setText(tr("STR_NEW_PROJECT"));
	_btnProjects->onMouseClick(static_cast<ActionHandler>(&ResearchState::btnResearchClick));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ResearchState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstResearch->setColumns(4, 137,58,48,34);
	_lstResearch->setBackground(_window);
	_lstResearch->setSelectable();
	_lstResearch->onMouseClick(	static_cast<ActionHandler>(&ResearchState::lstResearchClick),
								0u);
}

/**
 * dTor.
 */
ResearchState::~ResearchState()
{}

/**
 * Updates the research list after going to other screens.
 */
void ResearchState::init()
{
	State::init();

	_txtBaseLabel->setText(_base->getLabel());

	_lstResearch->clearList();
	_online.clear();

	const std::vector<ResearchProject*>& currentProjects (_base->getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = currentProjects.begin();
			i != currentProjects.end();
			++i)
	{
		if ((*i)->getOffline() == true)
			_online.push_back(false);
		else
		{
			_online.push_back(true);

			std::wstring daysLeft;
			const int assigned ((*i)->getAssignedScientists());
			if (assigned != 0)
			{
				const int days (static_cast<int>(std::ceil(
							   (static_cast<double>((*i)->getCost() - (*i)->getSpent()))
							  / static_cast<double>(assigned))));
				daysLeft = Text::intWide(days);
			}
			else
				daysLeft = Text::intWide((*i)->getCost() - (*i)->getSpent()); //L"-"

			_lstResearch->addRow(
							4,
							tr((*i)->getRules()->getType()).c_str(),
							Text::intWide(assigned).c_str(),
							tr((*i)->getResearchProgress()).c_str(),
							daysLeft.c_str());
		}
	}

	_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE_")
							.arg(_base->getScientists()));
	_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED_")
							.arg(_base->getAllocatedScientists()));
	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE_")
							.arg(_base->getFreeLaboratories()));

	_btnAliens->setVisible(_base->hasContainment() == true);

	bool vis (false);
	for (std::vector<ResearchProject*>::const_iterator
			i = _base->getResearch().begin();
			i != _base->getResearch().end();
			++i)
	{
		if ((*i)->getOffline() == true)
		{
			vis = true;
			break;
		}
	}

	if (vis == false)
	{
		std::vector<const RuleResearch*> unlocked;
		_game->getSavedGame()->tabulateStartableResearch(unlocked, _base);
		vis = (unlocked.empty() == false);
	}

	_btnProjects->setVisible(vis);
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens a list of unlocked Research.
 * @param action - pointer to an Action
 */
void ResearchState::btnResearchClick(Action*)
{
	_game->pushState(new ResearchListState(_base));
}

/**
 * Opens the AlienContainment screen.
 * @param action - pointer to an Action
 */
void ResearchState::btnAliensClick(Action*)
{
	_game->pushState(new AlienContainmentState(_base, OPT_GEOSCAPE));
}

/**
 * Opens the Research settings for a project.
 * @param action - pointer to an Action
 */
void ResearchState::lstResearchClick(Action* action) // private.
{
	size_t sel (_lstResearch->getSelectedRow());

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			size_t j (0u);
			for (std::vector<bool>::const_iterator
					i = _online.begin();
					i != _online.end();
					++i)
			{
				if (*i == true && sel == j)
					break;

				if (*i == false)
					++sel; // advance the 'selected row' to account for offline projects.

				++j;
			}
			_game->pushState(new ResearchInfoState(
												_base,
												_base->getResearch().at(sel)));
			break;
		}

		case SDL_BUTTON_RIGHT:
			_game->pushState(new TechTreeViewerState(_base->getResearch().at(sel)->getRules()));
	}
}

/**
 * Selects a different Base to display.
 * TODO: Implement key-presses to switch bases.
 * @param action - pointer to an Action
 */
void ResearchState::miniClick(Action*)
{
	if (_state != nullptr) // cannot switch bases if origin is Geoscape.
	{
		const size_t baseId (_mini->getHoveredBase());
		if (baseId < _baseList->size())
		{
			Base* const base (_baseList->at(baseId));
			if (base != _base && base->hasResearch() == true)
			{
				_txtHoverBase->setText(L"");

				_mini->setSelectedBase(baseId);
				_state->setBase(_base = base);

				_state->resetStoresWarning();
				init();
			}
		}
	}
}

/**
 * Displays the label of the Base the mouse is over.
 * @param action - pointer to an Action
 */
void ResearchState::miniMouseOver(Action*)
{
	const size_t baseId (_mini->getHoveredBase());
	if (baseId < _baseList->size())
	{
		const Base* const base (_baseList->at(baseId));
		if (base != _base && base->hasResearch() == true)
		{
			_txtHoverBase->setText(base->getLabel());
			return;
		}
	}
	_txtHoverBase->setText(L"");
}

/**
 * Clears the hovered Base label.
 * @param action - pointer to an Action
 */
void ResearchState::miniMouseOut(Action*)
{
	_txtHoverBase->setText(L"");
}

}
