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

#include "DebriefExtraState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../SaveGame/Base.h"
#include "../SaveGame/SavedGame.h"


namespace OpenXcom
{

/**
 * DebriefExtraState cTor.
 * @param base			- pointer to the Base that was in tactical
 * @param operation		- the operation title
 * @param itemsLost		- a map of pointers to RuleItems & ints for lost items
 * @param itemsGained	- a map of pointers to RuleItems & ints for gained items
 */
DebriefExtraState::DebriefExtraState(
		const Base* const base,
		std::wstring operation,
		std::map<const RuleItem*, int> itemsLost,
		std::map<const RuleItem*, int> itemsGained,
		std::map<std::wstring, std::vector<int>> soldierStatInc)
	:
		_itemsLost(itemsLost),
		_itemsGained(itemsGained),
		_soldierStatInc(soldierStatInc),
		_curScreen(DES_SOL_STATS)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(280, 16, 16, 8);
	_txtBaseLabel	= new Text( 80,  9, 16, 8);

	_lstSolStats	= new TextList(285, 145, 16, 32);
	_lstGained		= new TextList(285, 145, 16, 32);
	_lstLost		= new TextList(285, 145, 16, 32);

	_btnOk			= new TextButton(136, 16, 92, 177);

	setInterface("debriefing");

	add(_window,		"window",	"debriefing");
	add(_txtTitle,		"heading",	"debriefing");
	add(_txtBaseLabel,	"text",		"debriefing");
	add(_lstSolStats,	"list",		"debriefing");
	add(_lstGained,		"list",		"cannotReequip");
	add(_lstLost,		"list",		"debriefing");
	add(_btnOk,			"button",	"debriefing");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& DebriefExtraState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& DebriefExtraState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(operation);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(base->getName(_game->getLanguage()));

	_lstSolStats->setColumns(12, 90,17,17,17,17,17,17,17,17,17,17,17);
	_lstSolStats->setBackground(_window);
	_lstSolStats->setSelectable();

	_lstGained->setColumns(2, 242,35);
	_lstGained->setBackground(_window);
	_lstGained->setSelectable();
	_lstGained->setVisible(false);

	_lstLost->setColumns(2, 242,35);
	_lstLost->setBackground(_window);
	_lstLost->setSelectable();
	_lstLost->setVisible(false);

	buildSoldierStats();
	styleList(_itemsGained, _lstGained);
	styleList(_itemsLost, _lstLost);
}

/**
 * dTor.
 */
DebriefExtraState::~DebriefExtraState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void DebriefExtraState::btnOkClick(Action*)
{
	switch (_curScreen)
	{
		case DES_SOL_STATS:
			_lstSolStats->setVisible(false);
			if (_itemsGained.empty() == false)
			{
				_curScreen = DES_LOOT_GAINED;
				_lstGained->scrollTo();
				_lstGained->setVisible();
				break;
			} // no break;

		case DES_LOOT_GAINED:
			_lstGained->setVisible(false);
			if (_itemsLost.empty() == false)
			{
				_curScreen = DES_LOOT_LOST;
				_lstLost->scrollTo();
				_lstLost->setVisible();
				break;
			} // no break;

		case DES_LOOT_LOST:
			_game->popState();
	}
}

/**
 * Builds the Soldier Stat increases screen.
 */
void DebriefExtraState::buildSoldierStats() // private.
{
	_lstSolStats->addRow( // vid. BattleUnit::postMissionProcedures().
					12,
					L"",
					L"BR",
					L"FR",
					L"RA",
					L"ML",
					L"PA",
					L"PD",
					L"TR",
					L"TU",
					L"HL",
					L"ST",
					L"EN");

	size_t row (1u);
	for (std::map<std::wstring, std::vector<int>>::const_iterator
			i = _soldierStatInc.begin();
			i != _soldierStatInc.end();
			++i, ++row)
	{
		_lstSolStats->addRow(
						12,
						i->first.c_str(),
						i->second[0]  ? Text::intWide(i->second[0]).c_str()  : L"",
						i->second[1]  ? Text::intWide(i->second[1]).c_str()  : L"",
						i->second[2]  ? Text::intWide(i->second[2]).c_str()  : L"",
						i->second[3]  ? Text::intWide(i->second[3]).c_str()  : L"",
						i->second[4]  ? Text::intWide(i->second[4]).c_str()  : L"",
						i->second[5]  ? Text::intWide(i->second[5]).c_str()  : L"",
						i->second[6]  ? Text::intWide(i->second[6]).c_str()  : L"",
						i->second[7]  ? Text::intWide(i->second[7]).c_str()  : L"",
						i->second[8]  ? Text::intWide(i->second[8]).c_str()  : L"",
						i->second[9]  ? Text::intWide(i->second[9]).c_str()  : L"",
						i->second[10] ? Text::intWide(i->second[10]).c_str() : L"");
		_lstSolStats->setRowColor(row, YELLOW);
	}
}

/**
 * Formats mapped input to a TextList.
 * @param input	- reference to the mapped-input of pointers-to-RuleItems & quantities
 * @param list	- pointer to a 2-column TextList to format
 */
void DebriefExtraState::styleList( // private.
		const std::map<const RuleItem*, int>& input,
		TextList* const list)
{
	std::string type;
	std::wstring wst;
	Uint8 color;
	int row (0);

	for (std::map<const RuleItem*, int>::const_iterator
			i = input.begin();
			i != input.end();
			++i)
	{
		type = i->first->getType();
		wst = tr(type);
		color = YELLOW;

		if (i->first->getBattleType() == BT_AMMO)
		{
			wst.insert(0, L"  ");
			color = PURPLE;
		}

		if (_game->getSavedGame()->isResearched(type) == false								// not researched or is research exempt
			&& (_game->getSavedGame()->isResearched(i->first->getRequirements()) == false		// and has requirements to use that have not been researched
//				|| rules->getItemRule(*i)->isAlien() == true									// or is an alien
				|| i->first->getBattleType() == BT_CORPSE))										// or is a corpse
//				|| i->first->getBattleType() == BT_NONE)										// or is not a battlefield item
//			&& craftOrdnance == false)														// and is not craft ordnance
		{
			// well, that was !NOT! easy.
			color = GREEN;
		}

		list->addRow(
				2,
				wst.c_str(),
				Text::intWide(i->second).c_str());
		list->setRowColor(row++, color /*(color == PURPLE)*/);
	}
}

}
