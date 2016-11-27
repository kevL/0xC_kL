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

#include "TechTreeViewerState.h"

//#include <algorithm>
//#include <unordered_set>

#include "TechTreeSelectState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleResearch.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 * @param selectedTopicResearch		- (default nullptr)
 * @param selectedTopicManufacture	- (default nullptr)
 */
TechTreeViewerState::TechTreeViewerState(
		const RuleResearch* const selectedTopicResearch,
		const RuleManufacture* const selectedTopicManufacture)
{
	if (selectedTopicResearch != nullptr)
	{
		_selTopic = selectedTopicResearch->getType();
		_selFlag = 1;
	}
	else if (selectedTopicManufacture != nullptr)
	{
		_selTopic = selectedTopicManufacture->getType();
		_selFlag = 2;
	}

	_window = new Window(this);

	_txtTitle = new Text(304, 17, 8, 7);

	_txtSelectedTopic = new Text(204, 9, 8, 24);

	_txtProgress = new Text(100, 9, 212, 24);

	_lstLeft	= new TextList(132, 128,   8, 40);
	_lstRight	= new TextList(132, 128, 164, 40);

	_btnSelect	= new TextButton(148, 16,   8, 176);
	_btnOk		= new TextButton(148, 16, 164, 176);

	setInterface("researchMenu");

	add(_window,			"window",	"researchMenu");
	add(_txtTitle,			"text",		"researchMenu");
	add(_txtSelectedTopic,	"text",		"researchMenu");
	add(_txtProgress,		"text",		"researchMenu");
	add(_lstLeft,			"list",		"researchMenu");
	add(_lstRight,			"list",		"researchMenu");
	add(_btnSelect,			"button",	"researchMenu");
	add(_btnOk,				"button",	"researchMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_TECH_TREE_VIEWER"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtSelectedTopic->setText(tr("STR_TOPIC").arg(L""));

	_lstLeft->setColumns(1, 132);
	_lstLeft->setBackground(_window);
	_lstLeft->setSelectable();
	_lstLeft->setWordWrap();
	_lstLeft->onMouseClick(static_cast<ActionHandler>(&TechTreeViewerState::lstLeftTopicClick));

	_lstRight->setColumns(1, 132);
	_lstRight->setBackground(_window);
	_lstRight->setSelectable();
	_lstRight->setWordWrap();
	_lstRight->onMouseClick(static_cast<ActionHandler>(&TechTreeViewerState::lstRightTopicClick));

	_btnSelect->setText(tr("STR_SELECT_TOPIC"));
	_btnSelect->onMouseClick(	static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick));
	_btnSelect->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick),
								Options::keyOk);		//Options::keyToggleQuickSearch);
	_btnSelect->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick),
								Options::keyOkKeypad);	//Options::keyToggleQuickSearch);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TechTreeViewerState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnOkClick),
							Options::keyCancel);


	int
		costDiscovered	(0),
		costTotal		(0);

	const std::vector<ResearchGeneral*>& discovered (_game->getSavedGame()->getResearchGenerals());
	for (std::vector<ResearchGeneral*>::const_iterator
			i = discovered.begin();
			i != discovered.end();
			++i)
	{
		if ((*i)->getStatus() == RG_DISCOVERED)
		{
			_discovered.insert((*i)->getType());
			costDiscovered += (*i)->getRules()->getCost();
		}
	}


	const RuleResearch* resRule;

	const std::vector<std::string>& allResearch (_game->getRuleset()->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		if ((resRule = _game->getRuleset()->getResearch(*i)) != nullptr)
			costTotal += resRule->getCost();
	}

	const RuleManufacture* mfRule;
	const std::vector<std::string>& allManufacture (_game->getRuleset()->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = allManufacture.begin();
			i != allManufacture.end();
			++i)
	{
		mfRule = _game->getRuleset()->getManufacture(*i);
		if (_game->getSavedGame()->isResearched(mfRule->getRequiredResearch()))
			_discovered.insert(mfRule->getType());
	}

	_txtProgress->setText(tr("STR_RESEARCH_PROGRESS").arg(costDiscovered * 100 / costTotal)); // TODO: Format percent.
	_txtProgress->setAlign(ALIGN_RIGHT);
}

/**
 * dTor.
 */
TechTreeViewerState::~TechTreeViewerState()
{}

/**
 * Initializes the Lists.
 */
void TechTreeViewerState::init()
{
	State::init();
	fillTechTreeLists();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void TechTreeViewerState::btnOkClick(Action *)
{
	_game->popState();
}

/**
* Opens the Select Topic screen.
 * @param action - pointer to an Action
*/
void TechTreeViewerState::btnSelectClick(Action *)
{
	_game->pushState(new TechTreeSelectState(this));
}

/**
 * Populates the topics.
 */
void TechTreeViewerState::fillTechTreeLists()
{
	std::wostringstream woststr;
	woststr << tr(_selTopic);
	if (_selFlag == 2)
		woststr << tr("STR_M_FLAG");

	_txtSelectedTopic->setText(tr("STR_TOPIC").arg(woststr.str()));

	_topicsLeft	.clear();
	_topicsRight.clear();
	_flagsLeft	.clear();
	_flagsRight	.clear();

	_lstLeft	->clearList();
	_lstRight	->clearList();

	switch (_selFlag)
	{
		case 1:
		{
			size_t r (0u);

			const RuleResearch* const resRule (_game->getRuleset()->getResearch(_selTopic));
			if (resRule != nullptr)
			{
				std::vector<std::string> unlockedBy;
				std::vector<std::string> getForFreeFrom;
				std::vector<std::string> requiredByResearch;
				std::vector<std::string> requiredByManufacture;
				std::vector<std::string> leadsTo;

				const std::vector<std::string>& allManufacture (_game->getRuleset()->getManufactureList());
				for (std::vector<std::string>::const_iterator
						i = allManufacture.begin();
						i != allManufacture.end();
						++i)
				{
					const RuleManufacture* const mfRule (_game->getRuleset()->getManufacture(*i));

					const std::vector<std::string>& reqResearch (mfRule->getRequiredResearch());
					for (std::vector<std::string>::const_iterator
							j = reqResearch.begin();
							j != reqResearch.end();
							++j)
					{
						if (*j == resRule->getType())
							requiredByManufacture.push_back(*i);
					}
				}

				const std::vector<std::string>& allResearch (_game->getRuleset()->getResearchList());
				for (std::vector<std::string>::const_iterator
						i = allResearch.begin();
						i != allResearch.end();
						++i)
				{
					const RuleResearch* const otherRule (_game->getRuleset()->getResearch(*i));

					const std::vector<std::string>& required (otherRule->getRequiredResearch());
					for (std::vector<std::string>::const_iterator
							j = required.begin();
							j != required.end();
							++j)
					{
						if (*j == resRule->getType())
							requiredByResearch.push_back(*i);
					}

					const std::vector<std::string>& requisite (otherRule->getRequisiteResearch());
					for (std::vector<std::string>::const_iterator
							j = requisite.begin();
							j != requisite.end();
							++j)
					{
						if (*j == resRule->getType())
							leadsTo.push_back(*i);
					}
					const std::vector<std::string>& requested (otherRule->getRequestedResearch());
					for (std::vector<std::string>::const_iterator
							j = requested.begin();
							j != requested.end();
							++j)
					{
						if (*j == resRule->getType())
							unlockedBy.push_back(*i);
					}

					const std::vector<std::string>& gof (otherRule->getGetOneFree());
					for (std::vector<std::string>::const_iterator
							j = gof.begin();
							j != gof.end();
							++j)
					{
						if (*j == resRule->getType())
							getForFreeFrom.push_back(*i);
					}

				}

				if (resRule->needsItem() == true) // 1. item required
				{
					_lstLeft->addRow(1, tr("STR_ITEM_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					std::wstring wst (tr(_selTopic));
					wst.insert(0, L"  ");
					_lstLeft->addRow(1, wst.c_str());

					if (isDiscovered(_selTopic) == false)
						_lstLeft->setRowColor(r, PINK);

					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;
				}

				const std::vector<std::string>& required (resRule->getRequiredResearch());
				if (required.empty() == false) // 2. requires
				{
					_lstLeft->addRow(1, tr("STR_REQUIRES").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = required.begin();
							i != required.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back(*i);
						_flagsLeft.push_back(1);

						++r;
					}
				}

				const std::vector<std::string>& requisite (resRule->getRequisiteResearch());
				if (requisite.empty() == false) // 3. depends on
				{
					_lstLeft->addRow(1, tr("STR_DEPENDS_ON").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = requisite.begin();
							i != requisite.end();
							++i)
					{
						if (std::find( // if the same item is also in the "Unlocked by" section, skip it
									unlockedBy.begin(),
									unlockedBy.end(),
									*i) == unlockedBy.end())
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstLeft->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstLeft->setRowColor(r, PINK);

							_topicsLeft.push_back(*i);
							_flagsLeft.push_back(1);

							++r;
						}
					}
				}

				if (unlockedBy.empty() == false) // 4. unlocked by
				{
					_lstLeft->addRow(1, tr("STR_UNLOCKED_BY").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = unlockedBy.begin();
							i != unlockedBy.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back(*i);
						_flagsLeft.push_back(1);

						++r;
					}
				}

				if (getForFreeFrom.empty() == false) // 5. get for free from
				{
					_lstLeft->addRow(1, tr("STR_GET_FOR_FREE_FROM").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = getForFreeFrom.begin();
							i != getForFreeFrom.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back(*i);
						_flagsLeft.push_back(1);

						++r;
					}
				}

				r = 0u;

				if (requiredByResearch.empty() == false || requiredByManufacture.empty() == false) // 6. required by
				{
					_lstRight->addRow(1, tr("STR_REQUIRED_BY").c_str());
					_lstRight->setRowColor(r, BLUE);
					_topicsRight.push_back("-");
					_flagsRight.push_back(0);

					++r;

					if (requiredByResearch.empty() == false) // 6a. required by research
					{
						for (std::vector<std::string>::const_iterator
								i = requiredByResearch.begin();
								i != requiredByResearch.end();
								++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(1);

							++r;
						}
					}

					if (requiredByManufacture.empty() == false) // 6b. required by manufacture
					{
						for (std::vector<std::string>::const_iterator
								i = requiredByManufacture.begin();
								i != requiredByManufacture.end();
								++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							wst.append(tr("STR_M_FLAG"));
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(2);

							++r;
						}
					}
				}

				const std::vector<std::string>& requested (resRule->getRequestedResearch());
				if (leadsTo.empty() == false) // 7. leads to
				{
					_lstRight->addRow(1, tr("STR_LEADS_TO").c_str());
					_lstRight->setRowColor(r, BLUE);
					_topicsRight.push_back("-");
					_flagsRight.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = leadsTo.begin();
							i != leadsTo.end();
							++i)
					{
						if (std::find( // if the same topic is also in the 'requested' section skip it
									requested.begin(),
									requested.end(),
									*i) == requested.end())
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(1);

							++r;
						}
					}
				}

				if (requested.empty() == false) // 8. requested
				{
					_lstRight->addRow(1, tr("STR_UNLOCKS").c_str());
					_lstRight->setRowColor(r, BLUE);
					_topicsRight.push_back("-");
					_flagsRight.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = requested.begin();
							i != requested.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstRight->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstRight->setRowColor(r, PINK);

						_topicsRight.push_back(*i);
						_flagsRight.push_back(1);

						++r;
					}
				}

				const std::vector<std::string>& gof (resRule->getGetOneFree());
				if (gof.empty() == false) // 9. gives one for free
				{
					_lstRight->addRow(1, tr("STR_GIVES_ONE_FOR_FREE").c_str());
					_lstRight->setRowColor(r, BLUE);
					_topicsRight.push_back("-");
					_flagsRight.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = gof.begin();
							i != gof.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstRight->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstRight->setRowColor(r, PINK);

						_topicsRight.push_back(*i);
						_flagsRight.push_back(1);

						++r;
					}
				}
			}
			break;
		}

		case 2:
		{
			size_t r (0u);
			const RuleManufacture* const mfRule (_game->getRuleset()->getManufacture(_selTopic));
			if (mfRule != nullptr)
			{
				const std::vector<std::string>& reqResearch (mfRule->getRequiredResearch()); // 1. requires
				if (reqResearch.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_RESEARCH_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::vector<std::string>::const_iterator
							i = reqResearch.begin();
							i != reqResearch.end();
							++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back(*i);
						_flagsLeft.push_back(1);

						++r;
					}
				}

				const std::map<std::string, int>& reqFacilities (mfRule->getRequiredFacilities()); // 2. requires buildings
				if (reqFacilities.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_FACILITIES_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::map<std::string, int>::const_iterator
							i = reqFacilities.begin();
							i != reqFacilities.end();
							++i)
					{
						std::wostringstream woststr;
						woststr << L"  ";
						woststr << tr((*i).first);
						woststr << L": ";
						woststr << (*i).second;
						_lstLeft->addRow(1, woststr.str().c_str());
						_lstLeft->setRowColor(r, GOLD);
						_topicsLeft.push_back("-");
						_flagsLeft.push_back(0);

						++r;
					}
				}

				const std::map<std::string, int>& reqParts (mfRule->getPartsRequired()); // 3. inputs
				if (reqParts.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_MATERIALS_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);
					_topicsLeft.push_back("-");
					_flagsLeft.push_back(0);

					++r;

					for (std::map<std::string, int>::const_iterator
							i = reqParts.begin();
							i != reqParts.end();
							++i)
					{
						std::wostringstream woststr;
						woststr << L"  ";
						woststr << tr((*i).first);
						woststr << L": ";
						woststr << (*i).second;
						_lstLeft->addRow(1, woststr.str().c_str());
						_lstLeft->setRowColor(r, WHITE);
						_topicsLeft.push_back("-");
						_flagsLeft.push_back(0);

						++r;
					}
				}

				r = 0u;

				const std::map<std::string, int>& partsProduced (mfRule->getPartsProduced()); // 4. outputs
				if (partsProduced.empty() == false)
				{
					_lstRight->addRow(1, tr("STR_ITEMS_PRODUCED").c_str());
					_lstRight->setRowColor(r, BLUE);
					_topicsRight.push_back("-");
					_flagsRight.push_back(0);

					++r;

					for (std::map<std::string, int>::const_iterator
							i = partsProduced.begin();
							i != partsProduced.end();
							++i)
					{
						std::wostringstream woststr;
						woststr << L"  ";
						woststr << tr((*i).first);
						woststr << L": ";
						woststr << (*i).second;
						_lstRight->addRow(1, woststr.str().c_str());
						_lstRight->setRowColor(r, WHITE);
						_topicsRight.push_back("-");
						_flagsRight.push_back(0);

						++r;
					}
				}
			}
		}
	}
}

/**
 * Selects a topic on the left.
 * @param action - pointer to an Action
 */
void TechTreeViewerState::lstLeftTopicClick(Action*)
{
	const size_t r (_lstLeft->getSelectedRow());
	if (_flagsLeft[r] > 0)
	{
		_selTopic = _topicsLeft[r];
		_selFlag  = _flagsLeft[r];

		fillTechTreeLists();
	}
}

/**
 * Selects a topic on the right.
 * @param action - pointer to an Action
 */
void TechTreeViewerState::lstRightTopicClick(Action*)
{
	const size_t r (_lstRight->getSelectedRow());
	if (_flagsRight[r] > 0)
	{
		_selTopic = _topicsRight[r];
		_selFlag  = _flagsRight[r];

		fillTechTreeLists();
	}
}

/**
 * Changes the selected topic.
 * @param selectedTopic			- reference to a selected research-topic
 * @param isManufacturingTopic	- true if topic is Manufacture
 */
void TechTreeViewerState::setSelectedTopic(
		const std::string& selectedTopic,
		bool isManufacturingTopic)
{
	_selTopic = selectedTopic;
	_selFlag  = isManufacturingTopic ? 2 : 1;
}

/**
 * Checks if a specified topic is discovered.
 * @param topic - reference to a research-topic
 * @return, true if discovered
 */
bool TechTreeViewerState::isDiscovered(const std::string& topic) const
{
	if (_discovered.find(topic) != _discovered.end())
		return true;

	return false;
}

}
