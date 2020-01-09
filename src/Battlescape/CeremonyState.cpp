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

#include "CeremonyState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Ceremony screen post-tactical.
 * @param soldiersMedalled - vector of pointers to Soldier objects
 */
CeremonyState::CeremonyState(std::vector<Soldier*> soldiersMedalled)
{
	_window			= new Window(this);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstSoldiers	= new TextList(285, 121, 16, 28);
	_txtMedalInfo	= new Text(280, 25, 20, 150);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette(PAL_GEOSCAPE);

	add(_window);
	add(_txtTitle);
	add(_lstSoldiers);
	add(_txtMedalInfo);
	add(_btnOk);

	centerSurfaces();


	_window->setColor(GREEN);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(CYAN);
	_txtTitle->setText(tr("STR_MEDALS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstSoldiers->setColor(OLIVE); // NOTE: Green in CeremonyDeadState.
	_lstSoldiers->setColumns(2, 200,77);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMouseOver(	static_cast<ActionHandler>(&CeremonyState::lstInfoMouseOver));
	_lstSoldiers->onMouseOut(	static_cast<ActionHandler>(&CeremonyState::lstInfoMouseOut));

	_txtMedalInfo->setColor(SLATE);
	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();

	_btnOk->setColor(GREEN);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CeremonyState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CeremonyState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CeremonyState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CeremonyState::btnOkClick),
							Options::keyCancel);


	std::string qualifier;
	bool
		titleChosen (true),
		qualifiedAward;
	size_t
		r (0u),
		titleRow;

	const std::map<std::string, const RuleAward*>& allAwards (_game->getRuleset()->getAwardsList());
	for (std::map<std::string, const RuleAward*>::const_iterator
			i = allAwards.begin();
			i != allAwards.end();
			)
	{
		qualifiedAward = false;
		qualifier = "noQual";

		if (titleChosen == true)
		{
			titleChosen = false;
			_lstSoldiers->addRow(2, L"", L""); // Blank row, will be filled in later -> unless it's the last row ......
			_titleRows.insert(std::pair<size_t, std::string>(r++, ""));
		}

		titleRow = r - 1u; // NOTE: Does not underflow since r always increments^ on the 1st iteration.

		for (std::vector<Soldier*>::const_iterator
				j = soldiersMedalled.begin();
				j != soldiersMedalled.end();
				++j)
		{
			for (std::vector<SoldierAward*>::const_iterator
					k = (*j)->getDiary()->getSoldierAwards().begin();
					k != (*j)->getDiary()->getSoldierAwards().end();
					++k)
			{
				if ((*k)->getType() == i->first
					&& (*k)->isAwardRecent() == true
					&& qualifier == "noQual")
				{
					(*k)->clearRecent();
					++r;

					if ((*k)->getQualifier() != "noQual")
					{
						qualifier = (*k)->getQualifier();
						qualifiedAward = true;
					}

					std::wostringstream woststr;
					woststr << L"  ";
					woststr << (*j)->getLabel();

					int
						qtyPre (-2),
						qtyCur;
					size_t
						nextLevel (0u),
						skip      (0u);

					for (std::vector<int>::const_iterator
							l = i->second->getCriteria()->begin()->second.begin();
							l != i->second->getCriteria()->begin()->second.end();
							++l)
					{
						if (nextLevel == (*k)->getAwardLevel() + 1u)
							break;

						qtyCur = *l;
						if (l != i->second->getCriteria()->begin()->second.begin())
							qtyPre = *(l - 1);

						if (qtyCur == qtyPre)
							++skip;

						++nextLevel;
					}

					_lstSoldiers->addRow(
									2,
									woststr.str().c_str(),
									tr((*k)->GetLevelString(skip)).c_str());
					break;
				}
			}
		}

		if (titleRow != r - 1u)
		{
			if (qualifiedAward == true)
				_lstSoldiers->setCellText(
										titleRow, 0u,
										tr((*i).first).arg(tr(qualifier)));
			else
				_lstSoldiers->setCellText(
										titleRow, 0u,
										tr((*i).first));

			_lstSoldiers->setRowColor(titleRow, GREEN);

			const std::string info (i->second->getDescriptionGeneric()); // look for Generic Description first.
			if (info.empty() == false)
				_titleRows[titleRow] = info;
			else
				_titleRows[titleRow] = i->second->getDescription();

			titleChosen = true;
		}

		if (qualifier == "noQual")
			++i;
	}
}

/**
 * dTor.
 */
CeremonyState::~CeremonyState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CeremonyState::btnOkClick(Action*)
{
	if (_game->getQtyStates() == 2u // ie: (1) this, (2) Geoscape
		&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
//		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
	_game->popState();
}

/**
 * Shows the Award description.
 * @param action - pointer to an Action
 */
void CeremonyState::lstInfoMouseOver(Action*)
{
	const size_t r (_lstSoldiers->getSelectedRow());
	if (_titleRows.find(r) != _titleRows.end())
		_txtMedalInfo->setText(tr(_titleRows[r]));
	else
		_txtMedalInfo->setText(L"");
}

/**
 * Clears the Award description.
 * @param action - pointer to an Action
 */
void CeremonyState::lstInfoMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
}

}
