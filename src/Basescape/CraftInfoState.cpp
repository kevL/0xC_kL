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

#include "CraftInfoState.h"

//#include <cmath>
//#include <sstream>

#include "CraftArmorState.h"
#include "CraftEquipmentState.h"
#include "CraftSoldiersState.h"
#include "CraftWeaponsState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the CraftInfo screen.
 * @param base		- pointer to the Base to get info from
 * @param craftId	- ID of the selected craft
 */
CraftInfoState::CraftInfoState(
		Base* const base,
		size_t craftId)
	:
		_base(base),
		_craftId(craftId),
		_craft(base->getCrafts()->at(craftId)),
		_isQuickBattle(_game->getSavedGame()->getMonthsPassed() == -1)
{
	if (_isQuickBattle == false)
		_window		= new Window(this, 320, 200, 0, 0, POPUP_BOTH);
	else
		_window		= new Window(this, 320, 200);

	_edtCraft		= new TextEdit(this, 160, 16, 80, 10);
	_txtBaseLabel	= new Text(80, 9,  16, 10);
	_txtStatus		= new Text(80, 9, 224, 10);

	_txtRadar		= new Text(80, 9, 120, 28);
	_txtKills		= new Text(60, 9, 130, 38);

	_txtFuel		= new Text(80, 17,  16, 28);
	_txtDamage		= new Text(80, 17, 226, 28);

	_btnW1			= new TextButton(24, 32,  16, 48);
	_btnW2			= new TextButton(24, 32, 282, 48);
	_txtW1Name		= new Text(78,  9,  46, 48);
	_txtW2Name		= new Text(78,  9, 204, 48);
	_txtW1Ammo		= new Text(60, 25,  46, 64);
	_txtW2Ammo		= new Text(60, 25, 204, 64);

	_btnCrew		= new TextButton( 64, 16, 16,  96);
	_btnEquip		= new TextButton( 64, 16, 16, 120);
	_btnArmor		= new TextButton( 64, 16, 16, 144);
	_btnInventory	= new TextButton(220, 16, 84, 144);

	_sprite			= new Surface( 32, 38, 144,  50);
	_weapon1		= new Surface( 15, 17, 121,  63);
	_weapon2		= new Surface( 15, 17, 184,  63);
	_crew			= new Surface(220, 18,  85,  95);
	_equip			= new Surface(220, 18,  85, 119);

	_txtCost		= new Text(150, 9, 24, 165);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("craftInfo");

	add(_window,		"window",	"craftInfo");
	add(_edtCraft,		"text1",	"craftInfo");
	add(_txtBaseLabel,	"text1",	"craftInfo");
//	add(_txtStatus,		"text2",	"craftInfo");
	add(_txtStatus);
	add(_txtRadar,		"text2",	"craftInfo");
	add(_txtKills,		"text2",	"craftInfo");
	add(_txtFuel,		"text2",	"craftInfo");
	add(_txtDamage,		"text2",	"craftInfo");
	add(_btnW1,			"button",	"craftInfo");
	add(_btnW2,			"button",	"craftInfo");
	add(_txtW1Name,		"text2",	"craftInfo");
	add(_txtW2Name,		"text2",	"craftInfo");
	add(_txtW1Ammo,		"text2",	"craftInfo");
	add(_txtW2Ammo,		"text2",	"craftInfo");
	add(_btnCrew,		"button",	"craftInfo");
	add(_btnEquip,		"button",	"craftInfo");
	add(_btnArmor,		"button",	"craftInfo");
	add(_btnInventory,	"button",	"craftInfo");
	add(_sprite);
	add(_weapon1);
	add(_weapon2);
	add(_crew);
	add(_equip);
	add(_txtCost,		"text1",	"craftInfo");
	add(_btnOk,			"button",	"craftInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_edtCraft->setBig();
	_edtCraft->setAlign(ALIGN_CENTER);
	_edtCraft->onTextChange(static_cast<ActionHandler>(&CraftInfoState::edtCraftChange));

	_txtBaseLabel->setText(_base->getName());
	_txtStatus->setAlign(ALIGN_RIGHT);

	_txtRadar->setAlign(ALIGN_CENTER);

	if (_craft->getKills() != 0) //&& _craft->getRules()->getWeapons() != 0
	{
		_txtKills->setText(tr("STR_KILLS_LC_").arg(_craft->getKills()));
		_txtKills->setAlign(ALIGN_CENTER);
	}

	_txtDamage->setAlign(ALIGN_RIGHT);

	_btnW1->setText(L"1");
	_btnW1->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnW1Click));
	_btnW1->onKeyboardPress(static_cast<ActionHandler>(&CraftInfoState::btnW1Click),
							SDLK_1);

	_btnW2->setText(L"2");
	_btnW2->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnW2Click));
	_btnW2->onKeyboardPress(static_cast<ActionHandler>(&CraftInfoState::btnW2Click),
							SDLK_2);

	_btnCrew->setText(tr("STR_CREW"));
	_btnCrew->onMouseClick(		static_cast<ActionHandler>(&CraftInfoState::btnCrewClick));
	_btnCrew->onKeyboardPress(	static_cast<ActionHandler>(&CraftInfoState::btnCrewClick),
								SDLK_s);

	_btnEquip->setText(tr("STR_EQUIPMENT_UC"));
	_btnEquip->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnEquipClick));
	_btnEquip->onKeyboardPress(	static_cast<ActionHandler>(&CraftInfoState::btnEquipClick),
								SDLK_e);

	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnArmorClick));
	_btnArmor->onKeyboardPress(	static_cast<ActionHandler>(&CraftInfoState::btnArmorClick),
								SDLK_a);

	_btnInventory->setText(tr("STR_LOADOUT"));
	_btnInventory->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnInventoryClick));
	_btnInventory->onKeyboardPress(	static_cast<ActionHandler>(&CraftInfoState::btnInventoryClick),
									SDLK_i);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftInfoState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftInfoState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftInfoState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftInfoState::btnOkClick),
							Options::keyCancel);

	_timerBlink = new Timer(325u);
	_timerBlink->onTimer(static_cast<StateHandler>(&CraftInfoState::blinkStatus));
}

/**
 * dTor.
 */
CraftInfoState::~CraftInfoState()
{
	delete _timerBlink;
}

/**
 * The Craft's info can change after going to other screens.
 */
void CraftInfoState::init()
{
	State::init();

	// Reset stuff when coming back from pre-battle Inventory.
	if (_game->getSavedGame()->getBattleSave() != nullptr)
	{
		_game->getSavedGame()->setBattleSave();
		_craft->setTactical(false);
	}

	_btnInventory->setVisible(_craft->getQtySoldiers() != 0
						   && _craft->getCraftItems()->getTotalQuantity() != 0
						   && _isQuickBattle == false);

	_edtCraft->setText(_craft->getName(_game->getLanguage()));

	const RuleCraft* const crRule (_craft->getRules());

	if (_isQuickBattle == true)
	{
		_txtStatus->setText(L"");
		_craft->setFuel(crRule->getMaxFuel()); // top up Craft for insta-Battle mode.
	}
	else
	{
		Uint8 color;

		const CraftStatus status (_craft->getCraftStatus());
		switch (status)
		{
			case CS_READY: color = GREEN; break;
			default:
			{
				if (_timerBlink->isRunning() == false)
					_timerBlink->start();

				switch (status)
				{
					case CS_REFUELLING:	color = YELLOW; break;
					case CS_REARMING:	color = ORANGE; break;
					default: // shuttup, g++
					case CS_REPAIRS:	color = RED;
				}
			}
		}
		_txtStatus->setText(tr(_craft->getCraftStatusString()));
		_txtStatus->setColor(color);
		_txtStatus->setHighContrast();
	}


	int hrs;
	std::wostringstream
		woststr1, // fuel
		woststr2; // hull

	woststr1 << tr("STR_FUEL").arg(Text::formatPercent(_craft->getFuelPct()));
	if (crRule->getMaxFuel() - _craft->getFuel() > 0)
	{
		hrs = static_cast<int>(std::ceil(
			  static_cast<double>(crRule->getMaxFuel() - _craft->getFuel()) / static_cast<double>(crRule->getRefuelRate())
			  / 2.)); // refuel every half-hour.
		woststr1 << L"\n" << _game->getSavedGame()->formatCraftDowntime(
																	hrs,
																	_craft->getWarning() == CW_CANTREFUEL,
																	_game->getLanguage());
	}
	_txtFuel->setText(woststr1.str());

	_txtRadar->setText(tr("STR_RADAR_RANGE_")
						.arg(crRule->getRadarRange()));

	woststr2 << tr("STR_HULL_").arg(Text::formatPercent(100 - _craft->getCraftDamagePct()));
	if (_craft->getCraftDamage() != 0)
	{
		hrs = static_cast<int>(std::ceil(
			  static_cast<double>(_craft->getCraftDamage()) / static_cast<double>(crRule->getRepairRate())
			  / 2.)); // repair every half-hour.
		woststr2 << L"\n" << _game->getSavedGame()->formatCraftDowntime(
																	hrs,
																	false, // ... unless item is required to repair Craft.
																	_game->getLanguage());
	}
	_txtDamage->setText(woststr2.str());


	_sprite->clear(); // clear off sprites/icons
	_crew->clear();
	_equip->clear();
	_weapon1->clear();
	_weapon2->clear();

	SurfaceSet* const baseBits (_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	const int sprite (crRule->getSprite() + 33);
	baseBits->getFrame(sprite)->setX(0); // BaseView::draw() changes x&y
	baseBits->getFrame(sprite)->setY(0);
	baseBits->getFrame(sprite)->blit(_sprite);

	Surface* bit;
	const int icon_width (10);

	if (crRule->getSoldierCapacity() != 0)
	{
		int x (0);
		for (std::vector<Soldier*>::const_iterator // soldier graphic
				i = _base->getSoldiers()->begin();
				i != _base->getSoldiers()->end();
				++i)
		{
			if ((*i)->getCraft() == _craft)
			{
				if ((*i)->getArmor()->isBasic() == true)
					bit = baseBits->getFrame(70);
				else if ((*i)->getArmor()->getType() == "STR_PERSONAL_ARMOR_UC")
					bit = baseBits->getFrame(71);
				else if ((*i)->getArmor()->getMoveTypeArmor() != MT_FLY)
					bit = baseBits->getFrame(72);
				else // MT_FLY
					bit = baseBits->getFrame(73);

				bit->setX(x);
				bit->blit(_crew);

				x += icon_width;
			}
		}

		bit = baseBits->getFrame(40); // vehicle graphic
		bit->setY(2);
		x = 0;
		for (int
				i = 0;
				i != _craft->getQtyVehicles();
				++i, x += icon_width)
		{
			bit->setX(x);
			bit->blit(_equip);
		}

		bit = baseBits->getFrame(39); // equip't gra'hic
		bit->setY(0);

		const int
			totalIcons (((_equip->getWidth() - x) + (icon_width - 1)) / icon_width),
			loadCap (_craft->getLoadCapacity() - (_craft->getSpaceUsed() * 10)),
			qtyIcons (((totalIcons * _craft->getQtyEquipment()) + (loadCap - 1)) / loadCap);

		for (int
				i = 0;
				i != qtyIcons;
				++i, x += icon_width)
		{
			bit->setX(x);
			bit->blit(_equip);
		}

		if (_isQuickBattle == false)
			tacticalCost();
		else
			_txtCost->setVisible(false);
	}
	else
	{
		_crew->setVisible(false);
		_equip->setVisible(false);
		_btnCrew->setVisible(false);
		_btnEquip->setVisible(false);
		_btnArmor->setVisible(false);
		_txtCost->setVisible(false);
	}


	const RuleCraftWeapon* cwRule;
	const CraftWeapon* cw;

	if (crRule->getWeaponCapacity() > 0 && _isQuickBattle == false)
	{
		if ((cw = _craft->getWeapons()->at(0u)) != nullptr)
		{
			cwRule = cw->getRules();

			bit = baseBits->getFrame(cwRule->getSprite() + 48);
			bit->blit(_weapon1);

			std::wostringstream woststr;

			woststr << L'\x01' << tr(cwRule->getType());
			_txtW1Name->setText(woststr.str());

			woststr.str(L"");
			woststr << tr("STR_AMMO_").arg(cw->getCwLoad())
					<< L"\n\x01"
					<< tr("STR_MAX_").arg(cwRule->getLoadCapacity());
			if (cw->getCwLoad() < cwRule->getLoadCapacity())
			{
				hrs = static_cast<int>(std::ceil(
					  static_cast<double>(cwRule->getLoadCapacity() - cw->getCwLoad()) / static_cast<double>(cwRule->getRearmRate())
					  / 2.)); // rearm every half-hour.
				woststr << L"\n" << _game->getSavedGame()->formatCraftDowntime(
																			hrs,
																			cw->getCantLoad(),
																			_game->getLanguage());
			}
			_txtW1Ammo->setText(woststr.str());
		}
		else
		{
			_txtW1Name->setText(L"");
			_txtW1Ammo->setText(L"");
		}
	}
	else
	{
		_weapon1->setVisible(false);
		_btnW1->setVisible(false);
		_txtW1Name->setVisible(false);
		_txtW1Ammo->setVisible(false);
	}

	if (crRule->getWeaponCapacity() > 1 && _isQuickBattle == false)
	{
		if ((cw = _craft->getWeapons()->at(1u)) != nullptr)
		{
			cwRule = cw->getRules();

			bit = baseBits->getFrame(cwRule->getSprite() + 48);
			bit->blit(_weapon2);

			std::wostringstream woststr;

			woststr << L'\x01' << tr(cwRule->getType());
			_txtW2Name->setText(woststr.str());

			woststr.str(L"");
			woststr << tr("STR_AMMO_").arg(cw->getCwLoad())
					<< L"\n\x01"
					<< tr("STR_MAX_").arg(cwRule->getLoadCapacity());
			if (cw->getCwLoad() < cwRule->getLoadCapacity())
			{
				hrs = static_cast<int>(std::ceil(
					  static_cast<double>(cwRule->getLoadCapacity() - cw->getCwLoad()) / static_cast<double>(cwRule->getRearmRate())
					  / 2.)); // rearm every half-hour.
				woststr << L"\n" << _game->getSavedGame()->formatCraftDowntime(
																			hrs,
																			cw->getCantLoad(),
																			_game->getLanguage());
			}
			_txtW2Ammo->setText(woststr.str());
		}
		else
		{
			_txtW2Name->setText(L"");
			_txtW2Ammo->setText(L"");
		}
	}
	else
	{
		_weapon2->setVisible(false);
		_btnW2->setVisible(false);
		_txtW2Name->setVisible(false);
		_txtW2Ammo->setVisible(false);
	}
}

/**
 * Runs the blink timer.
 */
void CraftInfoState::think()
{
	if (_window->isPopupDone() == false)
		_window->think();
	else
	{
		_edtCraft->think();

		if (_timerBlink->isRunning() == true)
			_timerBlink->think(this, nullptr);
	}
}

/**
 * Blinks the status text.
 */
void CraftInfoState::blinkStatus()
{
	_txtStatus->setVisible(!_txtStatus->getVisible());
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnOkClick(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->popState();
}

/**
 * Goes to the Select Armament window for the first weapon.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnW1Click(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->pushState(new CraftWeaponsState(_base, _craftId, 0u));
}

/**
 * Goes to the Select Armament window for the second weapon.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnW2Click(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->pushState(new CraftWeaponsState(_base, _craftId, 1u));
}

/**
 * Goes to the Select Squad screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnCrewClick(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->pushState(new CraftSoldiersState(_base, _craftId));
}

/**
 * Goes to the Select Equipment screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnEquipClick(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->pushState(new CraftEquipmentState(_base, _craftId));
}

/**
 * Goes to the Select Armor screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnArmorClick(Action*)
{
	if (_edtCraft->isFocused() == false)
		_game->pushState(new CraftArmorState(_base, _craftId));
}

/**
 * Goes to the soldier-inventory screen.
 * @param action - pointer to an Action
 */
void CraftInfoState::btnInventoryClick(Action*)
{
	if (_edtCraft->isFocused() == false)
	{
		SavedBattleGame* const battleSave (new SavedBattleGame());
		_game->getSavedGame()->setBattleSave(battleSave);

		BattlescapeGenerator bGen = BattlescapeGenerator(_game);
		bGen.runFakeInventory(_craft);

		_game->getScreen()->clear();
		_game->pushState(new InventoryState());
	}
}

/**
 * Changes the Craft's name.
 * @param action - pointer to an Action
 */
void CraftInfoState::edtCraftChange(Action*)
{
	_craft->setName(_edtCraft->getText());
}

/**
 * Sets current cost to send the Craft on a mission.
 */
void CraftInfoState::tacticalCost() // private.
{
	const int cost (_base->soldierBonuses(_craft)
				  + _craft->getRules()->getSoldierCapacity() * 1000);
	_txtCost->setText(tr("STR_COST_").arg(Text::formatCurrency(cost)));
}

}
