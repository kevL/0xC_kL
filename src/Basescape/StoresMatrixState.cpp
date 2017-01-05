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

#include "StoresMatrixState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Vehicle.h"


namespace OpenXcom
{

static size_t recallRow;


/**
 * Initializes all the elements in the Matrix window.
 * @param base - pointer to the accessing Base
 */
StoresMatrixState::StoresMatrixState(const Base* base)
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 17,  10, 8);
	_txtBaseLabel	= new Text( 80,  9, 224, 8);

//	_txtItem		= new Text(100, 9, 16, 25);
	_txtFreeStore	= new Text(100, 9, 16, 33);

	_txtBase_0		= new Text(23, 17, 116, 25);
	_txtBase_1		= new Text(23, 17, 139, 25);
	_txtBase_2		= new Text(23, 17, 162, 25);
	_txtBase_3		= new Text(23, 17, 185, 25);
	_txtBase_4		= new Text(23, 17, 208, 25);
	_txtBase_5		= new Text(23, 17, 231, 25);
	_txtBase_6		= new Text(23, 17, 254, 25);
	_txtBase_7		= new Text(23, 17, 277, 25);

	_lstMatrix		= new TextList(285, 129, 16, 44);

	_btnOk			= new TextButton(268, 16, 26, 177);

	setPalette(PAL_BASESCAPE, BACKPAL_OLIVE);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
//	add(_txtItem);
	add(_txtFreeStore);
	add(_txtBase_0);
	add(_txtBase_1);
	add(_txtBase_2);
	add(_txtBase_3);
	add(_txtBase_4);
	add(_txtBase_5);
	add(_txtBase_6);
	add(_txtBase_7);
	add(_lstMatrix);
	add(_btnOk);

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));
	_window->setColor(BLUE);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->setColor(BLUE);
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&StoresMatrixState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StoresMatrixState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StoresMatrixState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StoresMatrixState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_MATRIX"));
	_txtTitle->setColor(BLUE);
	_txtTitle->setBig();

	_txtBaseLabel->setText(base->getLabel());
	_txtBaseLabel->setColor(BLUE);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

//	_txtItem->setText(tr("STR_ITEM"));
//	_txtItem->setColor(WHITE);

	_txtFreeStore->setText(tr("STR_FREESTORE"));
	_txtFreeStore->setColor(WHITE);

	_lstMatrix->setColumns(9, 100,23,23,23,23,23,23,23,23);
	_lstMatrix->setColor(BLUE);
	_lstMatrix->setBackground(_window);
	_lstMatrix->setSelectable();
	_lstMatrix->setMargin();


	const SavedGame* const playSave (_game->getSavedGame());
	std::wstring wst;
	int
		qty[MTX_BASES]			{0,0,0,0,0,0,0,0},
		qtyScientist[MTX_BASES]	{0,0,0,0,0,0,0,0},
		qtyEngineer[MTX_BASES]	{0,0,0,0,0,0,0,0},
		freeStorage;

	for (size_t
		i = 0u;
		i != playSave->getBases()->size();
		++i)
	{
		if ((base = playSave->getBases()->at(i)) != nullptr) // safety.
		{
			qty[i]			= base->getTotalSoldiers();
			qtyScientist[i]	= base->getTotalScientists();
			qtyEngineer[i]	= base->getTotalEngineers();

			wst = base->getLabel().substr(0,4);
			freeStorage = static_cast<int>(static_cast<double>(base->getTotalStores()) - base->getUsedStores() + 0.5);

			std::wostringstream woststr;
			woststr	<< wst
					<< L"\n"
					<< freeStorage;

			switch (i)
			{
				case 0u: _txtBase_0->setText(woststr.str()); break;
				case 1u: _txtBase_1->setText(woststr.str()); break;
				case 2u: _txtBase_2->setText(woststr.str()); break;
				case 3u: _txtBase_3->setText(woststr.str()); break;
				case 4u: _txtBase_4->setText(woststr.str()); break;
				case 5u: _txtBase_5->setText(woststr.str()); break;
				case 6u: _txtBase_6->setText(woststr.str()); break;
				case 7u: _txtBase_7->setText(woststr.str());
			}
		}
	}

	_txtBase_0->setColor(WHITE);
	_txtBase_1->setColor(WHITE);
	_txtBase_2->setColor(WHITE);
	_txtBase_3->setColor(WHITE);
	_txtBase_4->setColor(WHITE);
	_txtBase_5->setColor(WHITE);
	_txtBase_6->setColor(WHITE);
	_txtBase_7->setColor(WHITE);

	std::wostringstream
		woststr0,
		woststr1,
		woststr2,
		woststr3,
		woststr4,
		woststr5,
		woststr6,
		woststr7;

	size_t r (0u);

	if (  qty[0u]
		+ qty[1u]
		+ qty[2u]
		+ qty[3u]
		+ qty[4u]
		+ qty[5u]
		+ qty[6u]
		+ qty[7u] != 0)
	{
		if (qty[0u] != 0) woststr0 << qty[0u];
		if (qty[1u] != 0) woststr1 << qty[1u];
		if (qty[2u] != 0) woststr2 << qty[2u];
		if (qty[3u] != 0) woststr3 << qty[3u];
		if (qty[4u] != 0) woststr4 << qty[4u];
		if (qty[5u] != 0) woststr5 << qty[5u];
		if (qty[6u] != 0) woststr6 << qty[6u];
		if (qty[7u] != 0) woststr7 << qty[7u];

		_lstMatrix->addRow(
						9,
						tr("STR_SOLDIERS").c_str(),
						woststr0.str().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						woststr3.str().c_str(),
						woststr4.str().c_str(),
						woststr5.str().c_str(),
						woststr6.str().c_str(),
						woststr7.str().c_str());
		_lstMatrix->setRowColor(r++, YELLOW);
	}

	if (  qtyScientist[0u]
		+ qtyScientist[1u]
		+ qtyScientist[2u]
		+ qtyScientist[3u]
		+ qtyScientist[4u]
		+ qtyScientist[5u]
		+ qtyScientist[6u]
		+ qtyScientist[7u] != 0)
	{
		woststr0.str(L"");
		woststr1.str(L"");
		woststr2.str(L"");
		woststr3.str(L"");
		woststr4.str(L"");
		woststr5.str(L"");
		woststr6.str(L"");
		woststr7.str(L"");

		if (qtyScientist[0u] != 0) woststr0 << qtyScientist[0u];
		if (qtyScientist[1u] != 0) woststr1 << qtyScientist[1u];
		if (qtyScientist[2u] != 0) woststr2 << qtyScientist[2u];
		if (qtyScientist[3u] != 0) woststr3 << qtyScientist[3u];
		if (qtyScientist[4u] != 0) woststr4 << qtyScientist[4u];
		if (qtyScientist[5u] != 0) woststr5 << qtyScientist[5u];
		if (qtyScientist[6u] != 0) woststr6 << qtyScientist[6u];
		if (qtyScientist[7u] != 0) woststr7 << qtyScientist[7u];

		_lstMatrix->addRow(
						9,
						tr("STR_SCIENTISTS").c_str(),
						woststr0.str().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						woststr3.str().c_str(),
						woststr4.str().c_str(),
						woststr5.str().c_str(),
						woststr6.str().c_str(),
						woststr7.str().c_str());
		_lstMatrix->setRowColor(r++, YELLOW);
	}

	if (  qtyEngineer[0u]
		+ qtyEngineer[1u]
		+ qtyEngineer[2u]
		+ qtyEngineer[3u]
		+ qtyEngineer[4u]
		+ qtyEngineer[5u]
		+ qtyEngineer[6u]
		+ qtyEngineer[7u] != 0)
	{
		woststr0.str(L"");
		woststr1.str(L"");
		woststr2.str(L"");
		woststr3.str(L"");
		woststr4.str(L"");
		woststr5.str(L"");
		woststr6.str(L"");
		woststr7.str(L"");

		if (qtyEngineer[0u] != 0) woststr0 << qtyEngineer[0u];
		if (qtyEngineer[1u] != 0) woststr1 << qtyEngineer[1u];
		if (qtyEngineer[2u] != 0) woststr2 << qtyEngineer[2u];
		if (qtyEngineer[3u] != 0) woststr3 << qtyEngineer[3u];
		if (qtyEngineer[4u] != 0) woststr4 << qtyEngineer[4u];
		if (qtyEngineer[5u] != 0) woststr5 << qtyEngineer[5u];
		if (qtyEngineer[6u] != 0) woststr6 << qtyEngineer[6u];
		if (qtyEngineer[7u] != 0) woststr7 << qtyEngineer[7u];

		_lstMatrix->addRow(
						9,
						tr("STR_ENGINEERS").c_str(),
						woststr0.str().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						woststr3.str().c_str(),
						woststr4.str().c_str(),
						woststr5.str().c_str(),
						woststr6.str().c_str(),
						woststr7.str().c_str());
		_lstMatrix->setRowColor(r++, YELLOW);
	}

	const Ruleset* const rules (_game->getRuleset());
	const RuleItem* itRule;
	const RuleCraftWeapon* cwRule;
	size_t baseId;
	std::string type;
	std::wstring item;
	Uint8 color;

	const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
	bool craftOrdnance;

	const std::vector<std::string>& allItems (rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		itRule = rules->getItemRule(*i);
		type = itRule->getType();

		woststr0.str(L"");
		woststr1.str(L"");
		woststr2.str(L"");
		woststr3.str(L"");
		woststr4.str(L"");
		woststr5.str(L"");
		woststr6.str(L"");
		woststr7.str(L"");

		baseId = 0u;
		for (std::vector<Base*>::const_iterator
				j = playSave->getBases()->begin();
				j != playSave->getBases()->end();
				++j, ++baseId)
		{
			qty[baseId] = (*j)->getStorageItems()->getItemQuantity(*i);

			for (std::vector<Transfer*>::const_iterator // Add qty of items in transit to theMatrix.
					k = (*j)->getTransfers()->begin();
					k != (*j)->getTransfers()->end();
					++k)
			{
				if ((*k)->getTransferItems() == type)
					qty[baseId] += (*k)->getQuantity();
			}

			for (std::vector<Craft*>::const_iterator // Add qty of items & vehicles on transport craft to da-Matrix.
					k = (*j)->getCrafts()->begin();
					k != (*j)->getCrafts()->end();
					++k)
			{
				if ((*k)->getRules()->getSoldierCapacity() != 0) // is transport craft
				{
					for (std::map<std::string, int>::const_iterator
							l = (*k)->getCraftItems()->getContents()->begin();
							l != (*k)->getCraftItems()->getContents()->end();
							++l)
					{
						if (l->first == type)
							qty[baseId] += l->second;
					}
				}

				if ((*k)->getRules()->getVehicleCapacity() != 0) // is transport craft capable of vehicles
				{
					for (std::vector<Vehicle*>::const_iterator
							l = (*k)->getVehicles()->begin();
							l != (*k)->getVehicles()->end();
							++l)
					{
						if ((*l)->getRules()->getType() == type)
							++qty[baseId];
						else if ((*l)->getLoad() > 0
							&& (*l)->getRules()->getAcceptedLoadTypes()->front() == type)
						{
							qty[baseId] += (*l)->getLoad();
						}
					}
				}
			}
		}

		if (  qty[0u]
			+ qty[1u]
			+ qty[2u]
			+ qty[3u]
			+ qty[4u]
			+ qty[5u]
			+ qty[6u]
			+ qty[7u] != 0)
		{
			if (qty[0u] != 0) woststr0 << qty[0u];
			if (qty[1u] != 0) woststr1 << qty[1u];
			if (qty[2u] != 0) woststr2 << qty[2u];
			if (qty[3u] != 0) woststr3 << qty[3u];
			if (qty[4u] != 0) woststr4 << qty[4u];
			if (qty[5u] != 0) woststr5 << qty[5u];
			if (qty[6u] != 0) woststr6 << qty[6u];
			if (qty[7u] != 0) woststr7 << qty[7u];


			craftOrdnance = false;
			for (std::vector<std::string>::const_iterator
					j = cwList.begin();
					j != cwList.end();
					++j)
			{
				cwRule = rules->getCraftWeapon(*j);
				if (   itRule == rules->getItemRule(cwRule->getLauncherType())
					|| itRule == rules->getItemRule(cwRule->getClipType()))
				{
					craftOrdnance = true;
					break;
				}
			}

			item = tr(*i);
			color = BLUE;

			if (    itRule->getBattleType() == BT_AMMO
				|| (itRule->getBattleType() == BT_NONE && itRule->getFullClip() != 0))
			{
				item.insert(0, L"  ");
				color = PURPLE;
			}

			if (playSave->isResearched(itRule->getType()) == false					// not researched or is research exempt
				&& (playSave->isResearched(itRule->getRequiredResearch()) == false	// and has requirements to use but not been researched
					|| rules->getItemRule(*i)->isLiveAlien() == true					// or is an alien
					|| itRule->getBattleType() == BT_CORPSE								// or is a corpse
					|| itRule->getBattleType() == BT_NONE)								// or is not a battlefield item
				&& craftOrdnance == false)											// and is not craft ordnance
			{
				// well, that was !NOT! easy.
				color = YELLOW;
			}

			_lstMatrix->addRow(
							9,
							item.c_str(),
							woststr0.str().c_str(),
							woststr1.str().c_str(),
							woststr2.str().c_str(),
							woststr3.str().c_str(),
							woststr4.str().c_str(),
							woststr5.str().c_str(),
							woststr6.str().c_str(),
							woststr7.str().c_str());
			_lstMatrix->setRowColor(r++, color);
		}
	}

	_lstMatrix->scrollTo(recallRow);
//	_lstMatrix->draw(); // only needed if list changes while state is active. Eg, on re-inits
}

/**
 * dTor.
 */
StoresMatrixState::~StoresMatrixState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void StoresMatrixState::btnOkClick(Action*)
{
	recallRow = _lstMatrix->getScroll();
	_game->popState();
}

}
