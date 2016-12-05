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

const std::string TechTreeViewerState::START_PLAY ("STR_UNLOCKED"); // static.


/**
 * Creates a TechTreeViewer state.
 */
TechTreeViewerState::TechTreeViewerState()
	:
		_rules(_game->getRuleset()),
		_selTopic(START_PLAY),
		_selFlag(TECH_RESEARCH)
{
	build();
}

/**
 * Creates a TechTreeViewer state with a specified topic of Research.
 * @param topicResearch - pointer to a RuleResearch to show
 */
TechTreeViewerState::TechTreeViewerState(const RuleResearch* const topicResearch)
	:
		_rules(_game->getRuleset()),
		_selTopic(topicResearch->getType()),
		_selFlag(TECH_RESEARCH)
{
	build();
}

/**
 * Creates a TechTreeViewer state with a specified topic of Manufacture.
 * @param topicManufacture - pointer to a RuleManufacture to show
 */
TechTreeViewerState::TechTreeViewerState(const RuleManufacture* const topicManufacture)
	:
		_rules(_game->getRuleset()),
		_selTopic(topicManufacture->getType()),
		_selFlag(TECH_MANUFACTURE)
{
	build();
}

/**
 * dTor.
 */
TechTreeViewerState::~TechTreeViewerState()
{}

/**
 * Builds all the elements of the UI.
 */
void TechTreeViewerState::build()
{
	_window = new Window(
					this,
					320,200,
					0,0, POPUP_VERTICAL);

	_txtTitle = new Text(300, 16, 10, 7);

	_txtSelTopic = new Text(180, 9,  16, 24);
	_txtProgress = new Text(100, 9, 204, 24);

	_lstLeft	= new TextList(132, 128,  16, 40);
	_lstRight	= new TextList(132, 128, 169, 40);

	_btnSelect	= new TextButton(148, 16,   8, 176);
	_btnOk		= new TextButton(148, 16, 164, 176);

	setInterface("researchMenu");

	add(_window,		"window",	"researchMenu");
	add(_txtTitle,		"text",		"researchMenu");
	add(_txtSelTopic,	"text",		"researchMenu");
	add(_txtProgress,	"text",		"researchMenu");
	add(_lstLeft,		"list",		"researchMenu");
	add(_lstRight,		"list",		"researchMenu");
	add(_btnSelect,		"button",	"researchMenu");
	add(_btnOk,			"button",	"researchMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_TECH_TREE_VIEWER"));
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_txtSelTopic->setText(tr("STR_TOPIC_").arg(L""));
	_txtSelTopic->setColor(TOPIC_BLUE);
	_txtSelTopic->setHighContrast();

	_lstLeft->setColumns(1, 132);
	_lstLeft->setBackground(_window);
	_lstLeft->setSelectable();
	_lstLeft->setMargin();
	_lstLeft->setWordWrap();
	_lstLeft->onMouseClick(static_cast<ActionHandler>(&TechTreeViewerState::lstLeftTopicClick));

	_lstRight->setColumns(1, 132);
	_lstRight->setBackground(_window);
	_lstRight->setSelectable();
	_lstRight->setMargin();
	_lstRight->setWordWrap();
	_lstRight->onMouseClick(static_cast<ActionHandler>(&TechTreeViewerState::lstRightTopicClick));

	_btnSelect->setText(tr("STR_SELECT_TOPIC"));
	_btnSelect->onMouseClick(	static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick));
	_btnSelect->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick),
								Options::keyOk);
	_btnSelect->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick),
								Options::keyOkKeypad);
	_btnSelect->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnSelectClick),
								SDLK_t);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TechTreeViewerState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeViewerState::btnOkClick),
							Options::keyCancel);


	int
		costDiscovered	(0),
		costTotal		(0);

	_discovered.insert(START_PLAY);

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

	const std::vector<std::string>& allResearch (_rules->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = allResearch.begin();
			i != allResearch.end();
			++i)
	{
		if ((resRule = _rules->getResearch(*i)) != nullptr)
			costTotal += resRule->getCost();
	}

	const RuleManufacture* mfRule;
	const std::vector<std::string>& allManufacture (_rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = allManufacture.begin();
			i != allManufacture.end();
			++i)
	{
		mfRule = _rules->getManufacture(*i);
		if (_game->getSavedGame()->isResearched(mfRule->getRequiredResearch()))
			_discovered.insert(mfRule->getType());
	}

	_txtProgress->setText(tr("STR_RESEARCH_PROGRESS_")
							.arg(Text::formatPercent(costDiscovered * 100 / costTotal)));
	_txtProgress->setAlign(ALIGN_RIGHT);
}

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
void TechTreeViewerState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Select Topic screen.
 * @param action - pointer to an Action
 */
void TechTreeViewerState::btnSelectClick(Action*)
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
	if (_selFlag == TECH_MANUFACTURE)
		woststr << tr("STR_M_FLAG");

	_txtSelTopic->setText(tr("STR_TOPIC_").arg(woststr.str()));
	Uint8 color;
	if (isDiscovered(_selTopic) == true)
		color = TOPIC_GREEN;
	else
		color = TOPIC_YELLOW;
	_txtSelTopic->setSecondaryColor(color);

	_topicsLeft	.clear();
	_topicsRight.clear();
	_flagsLeft	.clear();
	_flagsRight	.clear();

	_lstLeft	->clearList();
	_lstRight	->clearList();

	switch (_selFlag)
	{
		case TECH_RESEARCH:
		{
			if (_selTopic == START_PLAY)
			{
				std::vector<std::string>
					requiredBy_mf,
					requisiteTo;

				const std::vector<std::string>& allManufacture (_rules->getManufactureList());
				for (std::vector<std::string>::const_iterator
						i  = allManufacture.begin();
						i != allManufacture.end();
					  ++i)
				{
					const RuleManufacture* const mfRule (_rules->getManufacture(*i));
					if (mfRule->getRequiredResearch().empty() == true)
						requiredBy_mf.push_back(*i);
				}

				const std::vector<std::string>& allResearch (_rules->getResearchList());
				for (std::vector<std::string>::const_iterator
						i  = allResearch.begin();
						i != allResearch.end();
					  ++i)
				{
					const RuleResearch* const otherRule (_rules->getResearch(*i));
					if (   otherRule->getRequisiteResearch().empty() == false
						&& otherRule->getRequisiteResearch().front() == START_PLAY)
					{
						requisiteTo.push_back(*i);
					}
				}

				size_t r (0u);
				if (requiredBy_mf.empty() == false)
				{
					_lstRight->addRow(1, tr("STR_REQUIRED_BY").c_str());
					_lstRight->setRowColor(r, BLUE);

					_topicsRight.push_back("-");
					_flagsRight.push_back(TECH_NONE);

					++r;

					for (std::vector<std::string>::const_iterator
							i  = requiredBy_mf.begin();
							i != requiredBy_mf.end();
						  ++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						wst.append(tr("STR_M_FLAG"));
						_lstRight->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstRight->setRowColor(r, PINK);

						_topicsRight.push_back(*i);
						_flagsRight.push_back(TECH_MANUFACTURE);

						++r;
					}
				}

				if (requisiteTo.empty() == false)
				{
					_lstRight->addRow(1, tr("STR_REQUISITE_TO").c_str());
					_lstRight->setRowColor(r, BLUE);

					_topicsRight.push_back("-");
					_flagsRight.push_back(TECH_NONE);

					++r;

					for (std::vector<std::string>::const_iterator
							i  = requisiteTo.begin();
							i != requisiteTo.end();
						  ++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstRight->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstRight->setRowColor(r, PINK);

						_topicsRight.push_back(*i);
						_flagsRight.push_back(TECH_RESEARCH);

						++r;
					}
				}
			}
			else
			{
				const RuleResearch* const resRule (_rules->getResearch(_selTopic));
				if (resRule != nullptr)
				{
					std::vector<std::string>
						requiredBy_mf,
						requiredBy,
						requisiteTo,
						requestedBy,
						gofBy;

					const std::vector<std::string>& allManufacture (_rules->getManufactureList());
					for (std::vector<std::string>::const_iterator
							i  = allManufacture.begin();
							i != allManufacture.end();
						  ++i)
					{
						const RuleManufacture* const mfRule (_rules->getManufacture(*i));

						const std::vector<std::string>& reqResearch (mfRule->getRequiredResearch());
						for (std::vector<std::string>::const_iterator
								j  = reqResearch.begin();
								j != reqResearch.end();
							  ++j)
						{
							if (*j == resRule->getType())
								requiredBy_mf.push_back(*i);
						}
					}

					const std::vector<std::string>& allResearch (_rules->getResearchList());
					for (std::vector<std::string>::const_iterator
							i  = allResearch.begin();
							i != allResearch.end();
						  ++i)
					{
						const RuleResearch* const otherRule (_rules->getResearch(*i));

						const std::vector<std::string>& required (otherRule->getRequiredResearch());
						for (std::vector<std::string>::const_iterator
								j  = required.begin();
								j != required.end();
							  ++j)
						{
							if (*j == resRule->getType())
								requiredBy.push_back(*i);
						}

						const std::vector<std::string>& requisite (otherRule->getRequisiteResearch());
						for (std::vector<std::string>::const_iterator
								j  = requisite.begin();
								j != requisite.end();
							  ++j)
						{
							if (*j == resRule->getType())
								requisiteTo.push_back(*i);
						}

						const std::vector<std::string>& requested (otherRule->getRequestedResearch());
						for (std::vector<std::string>::const_iterator
								j  = requested.begin();
								j != requested.end();
							  ++j)
						{
							if (*j == resRule->getType())
								requestedBy.push_back(*i);
						}

						const std::vector<std::string>& gof (otherRule->getGetOneFree());
						for (std::vector<std::string>::const_iterator
								j  = gof.begin();
								j != gof.end();
							  ++j)
						{
							if (*j == resRule->getType())
								gofBy.push_back(*i);
						}
					}

// LEFT LIST TOPICS ->
					size_t r (0u);
					if (resRule->needsItem() == true)
					{
						_lstLeft->addRow(1, tr("STR_PART_REQUIRED").c_str());
						_lstLeft->setRowColor(r, BLUE);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;

						std::wstring wst (tr(_selTopic));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(_selTopic) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;
					}

					const std::vector<std::string>& required (resRule->getRequiredResearch());
					if (required.empty() == false)
					{
						_lstLeft->addRow(1, tr("STR_REQUIRED").c_str());
						_lstLeft->setRowColor(r, BLUE);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = required.begin();
								i != required.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstLeft->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstLeft->setRowColor(r, PINK);

							_topicsLeft.push_back(*i);
							_flagsLeft.push_back(TECH_RESEARCH);

							++r;
						}
					}

					const std::vector<std::string>& requisite (resRule->getRequisiteResearch());
					if (requisite.empty() == false)
					{
						_lstLeft->addRow(1, tr("STR_REQUISITE").c_str());
						_lstLeft->setRowColor(r, BLUE);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = requisite.begin();
								i != requisite.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstLeft->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstLeft->setRowColor(r, PINK);

							_topicsLeft.push_back(*i);
							_flagsLeft.push_back(TECH_RESEARCH);

							++r;
						}
					}

					if (requestedBy.empty() == false)
					{
						_lstLeft->addRow(1, tr("STR_REQUESTED_BY").c_str());
						_lstLeft->setRowColor(r, BLUE);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = requestedBy.begin();
								i != requestedBy.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstLeft->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstLeft->setRowColor(r, PINK);

							_topicsLeft.push_back(*i);
							_flagsLeft.push_back(TECH_RESEARCH);

							++r;
						}
					}

					if (gofBy.empty() == false)
					{
						_lstLeft->addRow(1, tr("STR_FREE_BY").c_str());
						_lstLeft->setRowColor(r, BLUE);

						_topicsLeft.push_back("-");
						_flagsLeft.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = gofBy.begin();
								i != gofBy.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstLeft->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstLeft->setRowColor(r, PINK);

							_topicsLeft.push_back(*i);
							_flagsLeft.push_back(TECH_RESEARCH);

							++r;
						}
					}

// RIGHT LIST TOPICS ->
					r = 0u;
					if (requiredBy.empty() == false || requiredBy_mf.empty() == false)
					{
						_lstRight->addRow(1, tr("STR_REQUIRED_BY").c_str());
						_lstRight->setRowColor(r, BLUE);

						_topicsRight.push_back("-");
						_flagsRight.push_back(TECH_NONE);

						++r;

						if (requiredBy.empty() == false)
						{
							for (std::vector<std::string>::const_iterator
									i  = requiredBy.begin();
									i != requiredBy.end();
								  ++i)
							{
								std::wstring wst (tr(*i));
								wst.insert(0, L"  ");
								_lstRight->addRow(1, wst.c_str());

								if (isDiscovered(*i) == false)
									_lstRight->setRowColor(r, PINK);

								_topicsRight.push_back(*i);
								_flagsRight.push_back(TECH_RESEARCH);

								++r;
							}
						}

						if (requiredBy_mf.empty() == false)
						{
							for (std::vector<std::string>::const_iterator
									i  = requiredBy_mf.begin();
									i != requiredBy_mf.end();
								  ++i)
							{
								std::wstring wst (tr(*i));
								wst.insert(0, L"  ");
								wst.append(tr("STR_M_FLAG"));
								_lstRight->addRow(1, wst.c_str());

								if (isDiscovered(*i) == false)
									_lstRight->setRowColor(r, PINK);

								_topicsRight.push_back(*i);
								_flagsRight.push_back(TECH_MANUFACTURE);

								++r;
							}
						}
					}

					if (requisiteTo.empty() == false)
					{
						_lstRight->addRow(1, tr("STR_REQUISITE_TO").c_str());
						_lstRight->setRowColor(r, BLUE);

						_topicsRight.push_back("-");
						_flagsRight.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = requisiteTo.begin();
								i != requisiteTo.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(TECH_RESEARCH);

							++r;
						}
					}

					const std::vector<std::string>& requested (resRule->getRequestedResearch());
					if (requested.empty() == false)
					{
						_lstRight->addRow(1, tr("STR_REQUESTED").c_str());
						_lstRight->setRowColor(r, BLUE);

						_topicsRight.push_back("-");
						_flagsRight.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = requested.begin();
								i != requested.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(TECH_RESEARCH);

							++r;
						}
					}

					const std::vector<std::string>& gof (resRule->getGetOneFree());
					if (gof.empty() == false)
					{
						_lstRight->addRow(1, tr("STR_FREE").c_str());
						_lstRight->setRowColor(r, BLUE);

						_topicsRight.push_back("-");
						_flagsRight.push_back(TECH_NONE);

						++r;

						for (std::vector<std::string>::const_iterator
								i  = gof.begin();
								i != gof.end();
							  ++i)
						{
							std::wstring wst (tr(*i));
							wst.insert(0, L"  ");
							_lstRight->addRow(1, wst.c_str());

							if (isDiscovered(*i) == false)
								_lstRight->setRowColor(r, PINK);

							_topicsRight.push_back(*i);
							_flagsRight.push_back(TECH_RESEARCH);

							++r;
						}
					}
				}
			}
			break;
		}

		case TECH_MANUFACTURE:
		{
			const RuleManufacture* const mfRule (_rules->getManufacture(_selTopic));
			if (mfRule != nullptr)
			{
// LEFT LIST TOPICS ->
				size_t r (0u);
				const std::vector<std::string>& resRequired (mfRule->getRequiredResearch());
				if (resRequired.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_RESEARCH_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);

					_topicsLeft.push_back("-");
					_flagsLeft.push_back(TECH_NONE);

					++r;

					for (std::vector<std::string>::const_iterator
							i  = resRequired.begin();
							i != resRequired.end();
						  ++i)
					{
						std::wstring wst (tr(*i));
						wst.insert(0, L"  ");
						_lstLeft->addRow(1, wst.c_str());

						if (isDiscovered(*i) == false)
							_lstLeft->setRowColor(r, PINK);

						_topicsLeft.push_back(*i);
						_flagsLeft.push_back(TECH_RESEARCH);

						++r;
					}
				}

				const std::map<std::string, int>& facRequired (mfRule->getRequiredFacilities());
				if (facRequired.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_FACILITIES_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);

					_topicsLeft.push_back("-");
					_flagsLeft.push_back(TECH_NONE);

					++r;

					for (std::map<std::string, int>::const_iterator
							i  = facRequired.begin();
							i != facRequired.end();
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
						_flagsLeft.push_back(TECH_NONE);

						++r;
					}
				}

				const std::map<std::string, int>& partsRequired (mfRule->getPartsRequired());
				if (partsRequired.empty() == false)
				{
					_lstLeft->addRow(1, tr("STR_PARTS_REQUIRED").c_str());
					_lstLeft->setRowColor(r, BLUE);

					_topicsLeft.push_back("-");
					_flagsLeft.push_back(TECH_NONE);

					++r;

					for (std::map<std::string, int>::const_iterator
							i  = partsRequired.begin();
							i != partsRequired.end();
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
						_flagsLeft.push_back(TECH_NONE);

						++r;
					}
				}

// RIGHT LIST TOPICS ->
				r = 0u;
				const std::map<std::string, int>& partsProduced (mfRule->getPartsProduced());
				if (partsProduced.empty() == false)
				{
					_lstRight->addRow(1, tr("STR_PARTS_PRODUCED").c_str());
					_lstRight->setRowColor(r, BLUE);

					_topicsRight.push_back("-");
					_flagsRight.push_back(TECH_NONE);

					++r;

					for (std::map<std::string, int>::const_iterator
							i  = partsProduced.begin();
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
						_flagsRight.push_back(TECH_NONE);

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
	if (_flagsLeft[r] != TECH_NONE)
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
	if (_flagsRight[r] != TECH_NONE)
	{
		_selTopic = _topicsRight[r];
		_selFlag  = _flagsRight[r];

		fillTechTreeLists();
	}
}

/**
 * Changes the selected topic.
 * @param selectedTopic	- reference to a selected topic
 * @param isResearch	- true if topic is Research, false for Manufacture
 */
void TechTreeViewerState::setSelectedTopic(
		const std::string& selTopic,
		bool isResearch)
{
	_selTopic = selTopic;
	_selFlag  = isResearch ? TECH_RESEARCH
						   : TECH_MANUFACTURE;
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
