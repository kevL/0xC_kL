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

#include "StoresState.h"

#include <iomanip>

#include "TransfersState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Stores window.
 * @param base - pointer to the Base to get info from
 */
StoresState::StoresState(Base* const base)
	:
		_base(base)
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 17,  10, 8);
	_txtBaseLabel	= new Text( 80,  9, 224, 8);

	_txtTotal		= new Text( 50, 9, 200, 18);
	_txtItem		= new Text(162, 9,  16, 25);
	_txtQuantity	= new Text( 84, 9, 178, 25);
	_txtSpaceUsed	= new Text( 26, 9, 262, 25);

	_lstStores		= new TextList(285, 137, 16, 36);

	_btnTransfers	= new TextButton(142, 16,  16, 177);
	_btnOk			= new TextButton(142, 16, 162, 177);

	setInterface("storesInfo");

	add(_window,		"window",	"storesInfo");
	add(_txtTitle,		"text",		"storesInfo");
	add(_txtBaseLabel,	"text",		"storesInfo");
	add(_txtTotal);//,	"text",		"storesInfo");
	add(_txtItem,		"text",		"storesInfo");
	add(_txtQuantity,	"text",		"storesInfo");
	add(_txtSpaceUsed,	"text",		"storesInfo");
	add(_lstStores,		"list",		"storesInfo");
	add(_btnTransfers,	"button",	"storesInfo");
	add(_btnOk,			"button",	"storesInfo");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&StoresState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StoresState::btnOkClick),
							Options::keyCancel);

	_btnTransfers->setText(tr("STR_TRANSIT_LC"));
	_btnTransfers->onMouseClick(	static_cast<ActionHandler>(&StoresState::btnIncTransClick));
	_btnTransfers->onKeyboardPress(	static_cast<ActionHandler>(&StoresState::btnIncTransClick),
									Options::keyOk);
	_btnTransfers->onKeyboardPress(	static_cast<ActionHandler>(&StoresState::btnIncTransClick),
									Options::keyOkKeypad);
	_btnTransfers->setVisible(_base->getTransfers()->empty() == false);

	_txtTitle->setText(tr("STR_STORES"));
	_txtTitle->setBig();

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getLabel());

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY"));

//	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));
	_txtSpaceUsed->setText(tr("STR_VOLUME"));

	_lstStores->setColumns(3, 154,84,26);
	_lstStores->setBackground(_window);
	_lstStores->setSelectable();


	const SavedGame* const playSave (_game->getSavedGame());
	const Ruleset* const rules (_game->getRuleset());
	const RuleItem
		* itRule,
		* clRule;
	const RuleCraftWeapon* cwRule;

	const std::vector<std::string>& cwList (rules->getCraftWeaponsList());
	bool craftOrdnance;

	std::wstring item;

	size_t r (0u);
	int
		baseQty,
		clip;
	Uint8 color;

	const std::vector<std::string>& allItems (rules->getItemsList());
	for (std::vector<std::string>::const_iterator
			i = allItems.begin();
			i != allItems.end();
			++i)
	{
		//Log(LOG_INFO) << *i << " stores listOrder = " << rules->getItemRule(*i)->getListOrder(); // Prints listOrder to LOG.
		if (rules->getItemRule(*i)->isLiveAlien() == false)
		{
			if ((baseQty = _base->getStorageItems()->getItemQuantity(*i)) != 0)
			{
				color = BLUE;
				itRule = rules->getItemRule(*i);

				item = tr(*i);

				craftOrdnance = false;
				for (std::vector<std::string>::const_iterator
						j = cwList.begin();
						j != cwList.end();
						++j)
				{
					cwRule = rules->getCraftWeapon(*j);
					if (rules->getItemRule(cwRule->getLauncherType()) == itRule)
					{
						craftOrdnance = true;
						if ((clip = cwRule->getLoadCapacity()) != 0)
							item += (L" (" + Text::intWide(clip) + L")");
						break;
					}

					if ((clRule = rules->getItemRule(cwRule->getClipType())) == itRule)
					{
						craftOrdnance = true;
						if ((clip = clRule->getFullClip()) > 1)
							item += (L"s (" + Text::intWide(clip) + L")");
						break;
					}
				}

				if (itRule->isFixed() == true // tank w/ Ordnance.
					&& (clip = itRule->getFullClip()) > 0)
				{
					item += (L" (" + Text::intWide(clip) + L")");
				}

				if (    itRule->getBattleType() == BT_AMMO
					|| (itRule->getBattleType() == BT_NONE && itRule->getFullClip() != 0))
				{
					color = PURPLE;
					item.insert(0, L"  ");
					if (itRule->getBattleType() == BT_AMMO
						&& (clip = itRule->getFullClip()) > 1
						&& itRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
					{
						item += (L" (" + Text::intWide(clip) + L")");
					}
				}

				if (playSave->isResearched(itRule->getType()) == false					// not researched or is research exempt
					&& (playSave->isResearched(itRule->getRequiredResearch()) == false	// and has requirements to use that have not been researched
//						|| rules->getItemRule(*i)->isLiveAlien() == true					// or is an alien
						|| itRule->getBattleType() == BT_CORPSE								// or is a corpse
						|| itRule->getBattleType() == BT_NONE)								// or is not a battlefield item
					&& craftOrdnance == false)											// and is not craft ordnance
				{
					// well, that was !NOT! easy.
					color = YELLOW;
				}

				std::wostringstream woststr;
				woststr << std::fixed << std::setprecision(1) << (static_cast<double>(baseQty) * itRule->getStoreSize());
				_lstStores->addRow(
								3,
								item.c_str(),
								Text::intWide(baseQty).c_str(),
								woststr.str().c_str());
				_lstStores->setRowColor(r++, color);

//				std::wostringstream woststr1;
//				woststr1 << baseQty;
//				if (rules->getItemRule(*i)->isLiveAlien() == true)
//				{
//					_lstStores->addRow(
//									3,
//									item.c_str(),
//									woststr1.str().c_str(),
//									L"-");
//				}
//				else
//				{
//					std::wostringstream woststr2;
//					woststr2 << std::fixed << std::setprecision(1) << (static_cast<double>(baseQty) * itRule->getSize());
//					_lstStores->addRow(
//									3,
//									item.c_str(),
//									woststr1.str().c_str(),
//									woststr2.str().c_str());
//				}
			}
		}
	}

	std::wostringstream woststr;
	woststr << _base->getTotalStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtTotal->setText(woststr.str());
	_txtTotal->setAlign(ALIGN_RIGHT);

	if (_base->storesOverfull() == true)
	{
		_txtTotal->setColor(RED);
		_txtTotal->setHighContrast();
		_txtTotal->setVisible(false); // wait for blink.

		_timerBlink = new Timer(325u);
		_timerBlink->onTimer(static_cast<StateHandler>(&StoresState::blink));
		_timerBlink->start();
	}
	else
		_txtTotal->setColor(WHITE);
}

/**
 * dTor.
 */
StoresState::~StoresState()
{
	if (_txtTotal->getColor() == RED)
		delete _timerBlink;
}

/**
 * Runs the blink Timer and scrolling.
 */
void StoresState::think()
{
	State::think();

	if (_txtTotal->getColor() == RED)
		_timerBlink->think(this, nullptr);
}

/**
 * Blinks the Text.
 */
void StoresState::blink()
{
	_txtTotal->setVisible(!_txtTotal->getVisible());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void StoresState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the incoming Transfers window.
 * @param action - pointer to an Action
 */
void StoresState::btnIncTransClick(Action*)
{
	_game->pushState(new TransfersState(_base));
}

}
