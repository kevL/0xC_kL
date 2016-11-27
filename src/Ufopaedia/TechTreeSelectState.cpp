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

	_window = new Window(this, 230, 140, 45, 32);

	_txtTitle = new Text(182, 9, 53, 42);

	_edtQuickSearch = new TextEdit(this, 48, 9, 219, 42);

	_lstTopics = new TextList(198, 88, 53, 54);

	_btnOk = new TextButton(206, 16, 57, 145);

	setInterface("allocateResearch");

	add(_window,			"window",	"allocateResearch");
	add(_txtTitle,			"text",		"allocateResearch");
	add(_edtQuickSearch,	"button2",	"allocateResearch");
	add(_lstTopics,			"list",		"researchMenu");
	add(_btnOk,				"button2",	"allocateResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_AVAILABLE_TOPICS"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(		static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick));
	_btnOk->onKeyboardPress(	static_cast<ActionHandler>(&TechTreeSelectState::btnOkClick),
								Options::keyCancel);
	_btnOk->onKeyboardRelease(	static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchToggle),
								Options::keyOk);		//Options::keyToggleQuickSearch
	_btnOk->onKeyboardRelease(	static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchToggle),
								Options::keyOkKeypad);	//Options::keyToggleQuickSearch

	_lstTopics->setColumns(1, 182);
	_lstTopics->setBackground(_window);
	_lstTopics->setSelectable();
	_lstTopics->setMargin();
	_lstTopics->setAlign(ALIGN_CENTER);
	_lstTopics->onMouseClick(static_cast<ActionHandler>(&TechTreeSelectState::lstTopicClick));

	_edtQuickSearch->setText(L"");
	_edtQuickSearch->setFocus();
	_edtQuickSearch->onEnter(static_cast<ActionHandler>(&TechTreeSelectState::keyQuickSearchApply));
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
void TechTreeSelectState::keyQuickSearchToggle(Action*)
{
	if (_edtQuickSearch->getVisible())
	{
		_edtQuickSearch->setText(L"");
		_edtQuickSearch->setVisible(false);
		keyQuickSearchApply(nullptr);
	}
	else
	{
		_edtQuickSearch->setVisible(true);
		_edtQuickSearch->setFocus(true);
	}
}

/**
 * Applies QuickSearch field.
 * @param action - pointer to an Action
 */
void TechTreeSelectState::keyQuickSearchApply(Action*)
{
	fillTechTreeLists();
}

/**
 * Populates the topics.
 */
void TechTreeSelectState::fillTechTreeLists()
{
	std::wstring search (_edtQuickSearch->getText());
	std::transform(
				search.begin(),
				search.end(),
				search.begin(),
				std::towupper);

	_availableTopics.clear();
	_lstTopics->clearList();

	if (search.length() < 3u)
	{
		_lstTopics->addRow(1, tr("STR_QS_THREE_LETTERS_A").c_str());
		_lstTopics->addRow(1, tr("STR_QS_THREE_LETTERS_B").c_str());
		return;
	}

	size_t r (0u);
	const std::vector<std::string>& allResearch (_game->getRuleset()->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		std::wstring project (tr(*i));
		std::transform(
					project.begin(),
					project.end(),
					project.begin(),
					std::towupper);

		if (project.find(search) != std::string::npos
			|| (search == L"HOCUSPOCUS" && _viewer->isDiscovered(*i) == false))
		{
			_availableTopics.push_back(*i);

			_lstTopics->addRow(1, tr(*i).c_str());
			++r;
		}
	}

	_firstManufactureId = r;
	const std::vector<std::string>& allManufacture (_game->getRuleset()->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = allManufacture.begin();
			i != allManufacture.end();
			++i)
	{
		std::wstring project (tr(*i));
		std::transform(
					project.begin(),
					project.end(),
					project.begin(),
					std::towupper);

		if (project.find(search) != std::string::npos
			|| (search == L"HOCUSPOCUS" && _viewer->isDiscovered(*i) == false))
		{
			_availableTopics.push_back(*i);

			std::wostringstream woststr;
			woststr << tr(*i);
			woststr << tr("STR_M_FLAG");

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
	if (r < _availableTopics.size())
	{
		_viewer->setSelectedTopic(
							_availableTopics[r],
							r >= _firstManufactureId);
		_game->popState();
	}
}

}
