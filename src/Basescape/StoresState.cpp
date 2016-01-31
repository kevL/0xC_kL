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

#include "StoresState.h"

#include <iomanip>
//#include <sstream>

#include "TransfersState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

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
StoresState::StoresState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17, 10, 8);

	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtTotal		= new Text(50, 9, 200, 18);

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

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& StoresState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& StoresState::btnOkClick,
					Options::keyCancel);

	_btnTransfers->setText(tr("STR_TRANSIT_LC"));
	_btnTransfers->onMouseClick((ActionHandler)& StoresState::btnIncTransClick);
	_btnTransfers->onKeyboardPress(
					(ActionHandler)& StoresState::btnIncTransClick,
					Options::keyOk);
	_btnTransfers->onKeyboardPress(
					(ActionHandler)& StoresState::btnIncTransClick,
					Options::keyOkKeypad);
	_btnTransfers->setVisible(_base->getTransfers()->empty() == false);

	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_STORES"));

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtTotal->setText(tr("STR_QUANTITY_UC"));
	_txtTotal->setColor(208); // white
	_txtTotal->setAlign(ALIGN_RIGHT);

	_txtItem->setText(tr("STR_ITEM"));

	_txtQuantity->setText(tr("STR_QUANTITY_UC"));

//	_txtSpaceUsed->setText(tr("STR_SPACE_USED_UC"));
	_txtSpaceUsed->setText(tr("STR_VOLUME"));

	_lstStores->setColumns(3, 154,84,26);
	_lstStores->setBackground(_window);
	_lstStores->setSelectable();


	const SavedGame* const gameSave = _game->getSavedGame();
	const Ruleset* const rules = _game->getRuleset();
	const RuleItem
		* itRule = nullptr,
		* laRule = nullptr,
		* clRule = nullptr;
	const RuleCraftWeapon* cwRule = nullptr;

	int
		row = 0,
		qty,
		clipSize;
	Uint8 color;

	const std::vector<std::string>& items = rules->getItemsList();
	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		//Log(LOG_INFO) << *i << " stores listOrder = " << rules->getItem(*i)->getListOrder(); // Prints listOrder to LOG.
		if (rules->getItem(*i)->isAlien() == false)
		{
			qty = _base->getStorageItems()->getItemQuantity(*i);
			if (qty != 0)
			{
				color = BLUE;
				itRule = rules->getItem(*i);

				std::wstring item = tr(*i);

				bool craftOrdnance = false;
				const std::vector<std::string>& cwList = rules->getCraftWeaponsList();
				for (std::vector<std::string>::const_iterator
						j = cwList.begin();
						j != cwList.end()
							&& craftOrdnance == false;
						++j)
				{
					// Special handling for treating craft weapons as items
					cwRule = rules->getCraftWeapon(*j);

					laRule = rules->getItem(cwRule->getLauncherItem());
					clRule = rules->getItem(cwRule->getClipItem());

					if (laRule == itRule)
					{
						craftOrdnance = true;
						clipSize = cwRule->getAmmoMax(); // Launcher capacity
						if (clipSize != 0)
							item += (L" (" + Text::intWide(clipSize) + L")");
					}
					else if (clRule == itRule)
					{
						craftOrdnance = true;
						clipSize = clRule->getClipSize(); // launcher Ammo quantity
						if (clipSize > 1)
							item += (L"s (" + Text::intWide(clipSize) + L")");
					}
				}

				if (itRule->isFixed() == true // tank w/ Ordnance.
					&& itRule->getCompatibleAmmo()->empty() == false)
				{
					clRule = rules->getItem(itRule->getCompatibleAmmo()->front());
					clipSize = clRule->getClipSize();
					if (clipSize != 0)
						item += (L" (" + Text::intWide(clipSize) + L")");
				}

				if ((itRule->getBattleType() == BT_AMMO
						|| (itRule->getBattleType() == BT_NONE
							&& itRule->getClipSize() > 0))
					&& itRule->getType() != _game->getRuleset()->getAlienFuelType())
				{
					if (itRule->getBattleType() == BT_AMMO
						&& itRule->getType().substr(0, 8) != "STR_HWP_") // *cuckoo** weapon clips
					{
						clipSize = itRule->getClipSize();
						if (clipSize > 1)
							item += (L" (" + Text::intWide(clipSize) + L")");
					}
					item.insert(0, L"  ");

					color = PURPLE;
				}

				if (gameSave->isResearched(itRule->getType()) == false				// not researched or is research exempt
					&& (gameSave->isResearched(itRule->getRequirements()) == false	// and has requirements to use that have not been researched
//						|| rules->getItem(*i)->isAlien() == true						// or is an alien
						|| itRule->getBattleType() == BT_CORPSE							// or is a corpse
						|| itRule->getBattleType() == BT_NONE)							// or is not a battlefield item
					&& craftOrdnance == false)										// and is not craft ordnance
				{
					// well, that was !NOT! easy.
					color = YELLOW;
				}

				std::wostringstream
					woststr1,
					woststr2;

				woststr1 << qty;
				woststr2 << std::fixed << std::setprecision(1) << (static_cast<double>(qty) * itRule->getSize());
				_lstStores->addRow(
								3,
								item.c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());

				_lstStores->setRowColor(
									row++,
									color);
/*			std::wostringstream woststr1;
			woststr1 << qty;
			if (rules->getItem(*i)->isAlien() == true)
			{
				_lstStores->addRow(
								3,
								item.c_str(),
								woststr1.str().c_str(),
								L"-");
			}
			else
			{
				std::wostringstream woststr2;
				woststr2 << std::fixed << std::setprecision(1) << (static_cast<double>(qty) * itRule->getSize());
				_lstStores->addRow(
								3,
								item.c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str());
			} */
			}
		}
	}

	std::wostringstream woststr;
	woststr << _base->getTotalStores() << ":" << std::fixed << std::setprecision(1) << _base->getUsedStores();
	_txtTotal->setText(woststr.str());
}

/**
 * dTor.
 */
StoresState::~StoresState()
{}

/**
 * Returns to the previous screen.
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
