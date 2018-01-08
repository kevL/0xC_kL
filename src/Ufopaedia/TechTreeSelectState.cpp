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

#include "TechTreeSelectState.h"

//#include <cwctype> // std::towupper()

#include "TechTreeViewerState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 * @param viewer - pointer to TechTreeViewerState
 */
TechTreeSelectState::TechTreeSelectState(TechTreeViewerState* const viewer)
	:
		_viewer(viewer),
		_firstManufactureId(0u)
{
	_fullScreen = false;

	_window			= new Window(this, 230, 140, 45, 30);

	_txtTitle		= new Text(80, 9, 120, 42);
	_edtQuickSearch	= new TextEdit(this, 50, 9, 215, 42);
	_srfSearchField	= new Surface(52, 11, 214, 41);

	_lstTopics		= new TextList(192, 89, 64, 54);

	_btnOk			= new TextButton(206, 16, 57, 145);

	setInterface("allocateResearch");

	add(_window,			"window",	"allocateResearch");
	add(_txtTitle,			"text",		"allocateResearch");
	add(_edtQuickSearch,	"button2",	"allocateResearch");
	add(_srfSearchField);
	add(_lstTopics,			"list",		"researchMenu");
	add(_btnOk,				"button2",	"allocateResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_AVAILABLE_TOPICS"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick),
							SDLK_t);

	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchToggle),
							SDLK_q);

	_lstTopics->setColumns(1, 192);
	_lstTopics->setBackground(_window);
	_lstTopics->setSelectable();
	_lstTopics->setMargin();
	_lstTopics->setAlign(ALIGN_CENTER);
	_lstTopics->onMouseClick(static_cast<ActionHandler>(&TechTreeSelectState::lstTopicClick));

	_edtQuickSearch->setText(L"");
	_edtQuickSearch->setVisible(false);
	_edtQuickSearch->onEnter(			static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchApply));
	_edtQuickSearch->onKeyboardPress(	static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchToggle),
										Options::keyCancel);

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(_srfSearchField->getWidth());
	rect.h = static_cast<Uint16>(_srfSearchField->getHeight());
	_srfSearchField->drawRect(&rect, 5u);

	++rect.x;
	++rect.y;
	rect.w = static_cast<Uint16>(rect.w - 2u);
	rect.h = static_cast<Uint16>(rect.h - 2u);
	_srfSearchField->drawRect(&rect, 0u);

	_srfSearchField->setVisible(false);
}

/**
 * dTor.
 */
TechTreeSelectState::~TechTreeSelectState()
{}

/**
 * Initializes the TextLists.
 */
void TechTreeSelectState::init()
{
	State::init();
	fillTechTreeLists();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void TechTreeSelectState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * QuickSearch toggle.
 * @param action - pointer to an Action
 */
void TechTreeSelectState::keyQuickSearchToggle(Action* action)
{
	if (_edtQuickSearch->getVisible() == true) // clear search-field or turn off.
	{
		if (_edtQuickSearch->getText().empty() == true
			|| action == nullptr
			|| action->getDetails()->key.keysym.sym != Options::keyCancel)
		{
			_edtQuickSearch->setFocusEdit(false);
			_edtQuickSearch->setVisible(false);

			_srfSearchField->setVisible(false);
		}
	}
	else // turn on.
	{
		_srfSearchField->setVisible();

		_edtQuickSearch->setBypass();
		_edtQuickSearch->setVisible();
		_edtQuickSearch->setFocusEdit(true, true);
	}

	_edtQuickSearch->setText(L"");
}

/**
 * Applies the QuickSearch field.
 * @param action - pointer to an Action
 */
void TechTreeSelectState::keyQuickSearchApply(Action*)
{
	fillTechTreeLists();
	keyQuickSearchToggle(nullptr);
}

/**
 * Populates the topics.
 */
void TechTreeSelectState::fillTechTreeLists()
{
	_topics.clear();
	_lstTopics->clearList();


	std::wstring search (_edtQuickSearch->getText());
	std::transform(
				search.begin(),
				search.end(),
				search.begin(),
				std::towupper);

	std::wstring project (tr("STR_UNLOCKED"));
	std::transform(
				project.begin(),
				project.end(),
				project.begin(),
				std::towupper);

	size_t r (0u);
	if (search.empty() == true
		|| project.find(search) != std::string::npos)
	{
		_topics.push_back("STR_UNLOCKED");

		_lstTopics->addRow(1, tr("STR_UNLOCKED").c_str());
		++r;
	}

	const Ruleset* const rules (_game->getRuleset());

	const std::vector<std::string>& allResearch (rules->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		project = tr(*i);
		std::transform(
					project.begin(),
					project.end(),
					project.begin(),
					std::towupper);

		if (search.empty() == true
			|| project.find(search) != std::string::npos)
		{
			_topics.push_back(*i);

			_lstTopics->addRow(1, tr(*i).c_str());
			++r;
		}
	}

	_firstManufactureId = r;
	const std::vector<std::string>& allManufacture (rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = allManufacture.begin();
			i != allManufacture.end();
			++i)
	{
		project = tr(*i);
		std::transform(
					project.begin(),
					project.end(),
					project.begin(),
					std::towupper);

		if (search.empty() == true
			|| project.find(search) != std::string::npos)
		{
			_topics.push_back(*i);

			std::wostringstream woststr;
			woststr << tr(*i) << tr("STR_M_FLAG");

			_lstTopics->addRow(1, woststr.str().c_str());
		}
	}
}

/**
 * Selects a topic.
 * @param action - pointer to an Action
 */
void TechTreeSelectState::lstTopicClick(Action*)
{
	const size_t r (_lstTopics->getSelectedRow());
	if (r < _topics.size())
	{
		_viewer->setSelectedTopic(
								_topics[r],
								r < _firstManufactureId);
		_game->popState();
	}
}

}
