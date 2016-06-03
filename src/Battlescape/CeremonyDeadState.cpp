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

#include "CeremonyDeadState.h"

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

#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements for Lost in the Medals screen post-mission.
 * @param soldiersLost - vector of pointers to SoldierDead objects
 */
CeremonyDeadState::CeremonyDeadState(std::vector<SoldierDead*> soldiersLost)
{
	_window				= new Window(this, 320, 200);
	_txtTitle			= new Text(300, 16, 10, 8);
	_lstSoldiersLost	= new TextList(285, 145, 16, 29); // 18 rows.
//	_lstSoldiersLost	= new TextList(285,   9, 16, 26);
//	_lstAwards			= new TextList(285, 113, 16, 36);
//	_txtMedalInfo		= new Text(280, 25, 20, 150);
	_btnOk				= new TextButton(288, 16, 16, 177);

	setPalette(PAL_GEOSCAPE, BACKPAL_RED_D);

	add(_window);
	add(_txtTitle);
	add(_lstSoldiersLost);
//	add(_lstAwards);
//	add(_txtMedalInfo);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(GREEN);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	if (soldiersLost.size() == 1u)
		_txtTitle->setText(tr("STR_LOST"));
	else
		_txtTitle->setText(tr("STR_LOST_PL"));
	_txtTitle->setColor(CYAN);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstSoldiersLost->setColor(BROWN);
	_lstSoldiersLost->setHighContrast(true);
	_lstSoldiersLost->setColumns(2, 180,50);
	_lstSoldiersLost->setMargin(16);
//	_lstSoldiersLost->setBackground(_window);
//	_lstSoldiersLost->setSelectable();

//	_lstAwards->setColor(GREEN); // note, is Olive in CeremonyState
//	_lstAwards->setColumns(2, 200,77);
//	_lstAwards->setBackground(_window);
//	_lstAwards->setSelectable();
//	_lstAwards->onMouseOver((ActionHandler)& CeremonyDeadState::lstInfoMouseOver);
//	_lstAwards->onMouseOut((ActionHandler)& CeremonyDeadState::lstInfoMouseOut);

//	_txtMedalInfo->setColor(SLATE);
//	_txtMedalInfo->setHighContrast();
//	_txtMedalInfo->setWordWrap();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(GREEN);
	_btnOk->onMouseClick((ActionHandler)& CeremonyDeadState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CeremonyDeadState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CeremonyDeadState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CeremonyDeadState::btnOkClick,
					Options::keyCancel);


//	const int rowsLost (std::min(8, // the soldiersLost list has maximum 8 rows so that there's room below it for the Awards List
//								 static_cast<int>(soldiersLost.size())));
//	_lstSoldiersLost->setHeight(rowsLost * 8 + 1);

//	_lstAwards->setY(_lstAwards->getY() + (rowsLost - 1) * 8);
//	_lstAwards->setHeight(_lstAwards->getHeight() - (rowsLost - 1) * 8);


	size_t row (0u);
	for (std::vector<SoldierDead*>::const_iterator
			i = soldiersLost.begin();
			i != soldiersLost.end();
			++i, ++row)
	{
		_lstSoldiersLost->addRow(
							2,
							(*i)->getName().c_str(),
							tr((*i)->getDiary()->getKiaOrMia()).c_str());
		_lstSoldiersLost->setCellColor(row, 1u, SLATE, true);
	}


//	std::string qualifier;
//	bool
//		titleChosen (true),
//		qualifiedAward;
//	size_t
//		row (0u),
//		titleRow;
//
//	std::map<std::string, RuleAward*> awardList (_game->getRuleset()->getAwardsList());
//	for (std::map<std::string, RuleAward*>::const_iterator
//			i = awardList.begin();
//			i != awardList.end();
//			)
//	{
//		qualifiedAward = false;
//		qualifier = "noQual";
//
//		if (titleChosen == true)
//		{
//			_lstAwards->addRow(2, L"",L""); // Blank row, will be filled in later -> unless it's the last row ......
//			_titleRows.insert(std::pair<size_t, std::string>(row++, ""));
//		}
//
//		titleChosen = false;
//		titleRow = row - 1u;
//
//		for (std::vector<SoldierDead*>::const_iterator
//				j = soldiersLost.begin();
//				j != soldiersLost.end();
//				++j)
//		{
//			for (std::vector<SoldierAward*>::const_iterator
//					k = (*j)->getDiary()->getSoldierAwards()->begin();
//					k != (*j)->getDiary()->getSoldierAwards()->end();
//					++k)
//			{
//				if ((*k)->getType() == (*i).first
//					&& (*k)->isAwardRecent() == true
//					&& qualifier == "noQual")
//				{
//					(*k)->clearRecent();
//					++row;
//
//					if ((*k)->getQualifier() != "noQual")
//					{
//						qualifier = (*k)->getQualifier();
//						qualifiedAward = true;
//					}
//
//					std::wostringstream woststr;
//					woststr << L"  ";
//					woststr << (*j)->getName();
//
//					int
//						skip (0),
//						lastInt (-2),
//						thisInt;
//
//					size_t nextLevel (0u);
//					for (std::vector<int>::const_iterator
//							l = (*i).second->getCriteria()->begin()->second.begin();
//							l != (*i).second->getCriteria()->begin()->second.end();
//							++l)
//					{
//						if (nextLevel == (*k)->getClassLevel() + 1u)
//							break;
//
//						thisInt = *l;
//						if (l != (*i).second->getCriteria()->begin()->second.begin())
//							lastInt = *(l - 1);
//
//						if (thisInt == lastInt)
//							++skip;
//
//						++nextLevel;
//					}
//
//					_lstAwards->addRow(
//									2,
//									woststr.str().c_str(),
//									tr((*k)->getClassType(skip)).c_str());
//					break;
//				}
//			}
//		}
//
//		if (titleRow != row - 1u)
//		{
//			if (qualifiedAward == true)
//				_lstAwards->setCellText(
//									titleRow, 0u,
//									tr((*i).first).arg(tr(qualifier)));
//			else
//				_lstAwards->setCellText(
//									titleRow, 0u,
//									tr((*i).first));
//
//			_lstAwards->setRowColor(titleRow, BROWN, true);
//
//			const std::string info ((*i).second->getDescriptionGeneral()); // look for Generic Desc first.
//			if (info.empty() == false)
//				_titleRows[titleRow] = info;
//			else
//				_titleRows[titleRow] = (*i).second->getDescription();
//
//			titleChosen = true;
//		}
//
//		if (qualifier == "noQual")
//			++i;
//	}
}

/**
 * dTor.
 */
CeremonyDeadState::~CeremonyDeadState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CeremonyDeadState::btnOkClick(Action*)
{
	if (_game->getQtyStates() == 2 // ie: (1) this, (2) Geoscape
		&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_TAC_AWARDS))
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
//		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
	_game->popState();
}

/**
 * Shows the Medal description.
 *
void CeremonyDeadState::lstInfoMouseOver(Action*)
{
	const size_t row (_lstAwards->getSelectedRow());
	if (_titleRows.find(row) != _titleRows.end())
		_txtMedalInfo->setText(tr(_titleRows[row]));
	else
		_txtMedalInfo->setText(L"");
} */
/**
 * Clears the Medal description.
 *
void CeremonyDeadState::lstInfoMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
} */

}
