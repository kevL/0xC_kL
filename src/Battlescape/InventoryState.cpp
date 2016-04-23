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

#include "InventoryState.h"

//#include <algorithm>

#include "BattlescapeState.h"
#include "Inventory.h"
#include "Map.h"
#include "TileEngine.h"
#include "UnitInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
//#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SoldierLayout.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

//static const int _templateBtnX		= 288;
//static const int _createTemplateBtnY	= 67;	//90
//static const int _applyTemplateBtnY	= 90;	//113
//static const int _clearInventoryBtnY	= 113;


/**
 * Initializes all the elements in the Inventory screen.
 * @param tuMode - true if in battle when inventory usage costs time units (default false)
 * @param parent - pointer to parent BattlescapeState (default nullptr)
 */
InventoryState::InventoryState(
		bool tuMode,
		BattlescapeState* const parent)
	:
		_tuMode(tuMode),
		_parent(parent)
//		_flarePower(0)
{
	_battleSave = _game->getSavedGame()->getBattleSave();

	if (_battleSave->getBattleGame() != nullptr)
		_battleSave->getBattleGame()->getMap()->setNoDraw();

/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	}
	else if (!_battleSave->getTileEngine())
	{
		Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
		_game->getScreen()->resetDisplay(false);
	} */

	_bg			= new Surface(320, 200);
	_paper		= new Surface(320, 200);

	_txtName	= new Text(200, 17, 36, 6);
	_gender		= new Surface(7, 7, 28, 1);

	_txtWeight	= new Text(70, 9, 237, 24);
	_txtTUs		= new Text(40, 9, 237, 32);
	_txtFAcc	= new Text(40, 9, 237, 32);
	_txtReact	= new Text(40, 9, 237, 40);
	_txtThrow	= new Text(40, 9, 237, 48);
	_txtMelee	= new Text(40, 9, 237, 56);
	_txtPStr	= new Text(40, 9, 237, 64);
	_txtPSkill	= new Text(40, 9, 237, 72);

	_txtUseTU	= new Text(45, 9, 245, 123);
	_txtThrowTU	= new Text(40, 9, 245, 132);
	_txtPsiTU	= new Text(40, 9, 245, 141);

	_numOrder	= new NumberText(7, 5, 228,  4);
	_numTuCost	= new NumberText(7, 5, 310, 60);

	_wndHead		= new NumberText(7, 5,  79,  31);
	_wndTorso		= new NumberText(7, 5,  79, 144);
	_wndRightArm	= new NumberText(7, 5,  40,  80);
	_wndLeftArm		= new NumberText(7, 5, 117,  80);
	_wndRightLeg	= new NumberText(7, 5,  40, 120);
	_wndLeftLeg		= new NumberText(7, 5, 117, 120);

	_txtItem	= new Text(160, 9, 128, 140);

	_btnOk		= new BattlescapeButton(35, 23, 237, 0);
	_btnPrev	= new BattlescapeButton(23, 23, 273, 0);
	_btnNext	= new BattlescapeButton(23, 23, 297, 0);

	_btnRank	= new BattlescapeButton(26, 23,   0,   0);
	_btnUnload	= new BattlescapeButton(32, 25, 288,  32);
	_btnGroundL	= new BattlescapeButton(32, 15,   0, 137);
	_btnGroundR	= new BattlescapeButton(32, 15, 288, 137);

//	_btnCreateTemplate = new BattlescapeButton(32,22, _templateBtnX, _createTemplateBtnY);
//	_btnApplyTemplate = new BattlescapeButton(32,22, _templateBtnX, _applyTemplateBtnY);
//	_btnClearInventory = new BattlescapeButton(32,22, _templateBtnX, _clearInventoryBtnY);

	_txtAmmo = new Text(40, 24, 288, 64);
	_srfAmmo = new Surface(
						RuleInventory::HAND_W * RuleInventory::SLOT_W,
						RuleInventory::HAND_H * RuleInventory::SLOT_H,
						288,88);

	_inventoryPanel	= new Inventory(
								_game,
								320,200,
								0,0,
								_parent == nullptr);

	setPalette(PAL_BATTLESCAPE);

	add(_bg);
	_game->getResourcePack()->getSurface("Inventory")->blit(_bg);

	add(_gender);
	add(_paper);
	add(_txtName,		"textName",			"inventory", _bg);

	add(_txtWeight,		"textWeight",		"inventory", _bg);
	add(_txtTUs,		"textTUs",			"inventory", _bg);
	add(_txtFAcc,		"textFiring",		"inventory", _bg);
	add(_txtReact,		"textReaction",		"inventory", _bg);
	add(_txtThrow,		"textThrowing",		"inventory", _bg);
	add(_txtMelee,		"textMelee",		"inventory", _bg);
	add(_txtPStr,		"textPsiStrength",	"inventory", _bg);
	add(_txtPSkill,		"textPsiSkill",		"inventory", _bg);

	add(_numOrder);
	add(_numTuCost);

	add(_txtItem,		"textItem",			"inventory", _bg);
	add(_txtAmmo,		"textAmmo",			"inventory", _bg);
	add(_btnOk,			"buttonOK",			"inventory", _bg);
	add(_btnPrev,		"buttonPrev",		"inventory", _bg);
	add(_btnNext,		"buttonNext",		"inventory", _bg);
	add(_btnUnload,		"buttonUnload",		"inventory", _bg);
	add(_btnGroundL,	"buttonGround",		"inventory", _bg);
	add(_btnGroundR,	"buttonGround",		"inventory", _bg);
	add(_btnRank,		"rank",				"inventory", _bg);

//	add(_btnCreateTemplate,	"buttonCreate",	"inventory", _bg);
//	add(_btnApplyTemplate,	"buttonApply",	"inventory", _bg);
//	add(_btnClearInventory);

	add(_txtUseTU,		"textTUs",			"inventory", _bg);
	add(_txtThrowTU,	"textTUs",			"inventory", _bg);
	add(_txtPsiTU,		"textTUs",			"inventory", _bg);

	add(_wndHead);
	add(_wndTorso);
	add(_wndRightArm);
	add(_wndLeftArm);
	add(_wndRightLeg);
	add(_wndLeftLeg);

	add(_srfAmmo);
	add(_inventoryPanel);

	centerAllSurfaces();


	_txtName->setBig();
	_txtName->setHighContrast();

	_txtWeight->setHighContrast();
	_txtTUs->setHighContrast();
	_txtFAcc->setHighContrast();
	_txtReact->setHighContrast();
	_txtThrow->setHighContrast();
	_txtMelee->setHighContrast();
	_txtPStr->setHighContrast();
	_txtPSkill->setHighContrast();
	_txtUseTU->setHighContrast();
	_txtThrowTU->setHighContrast();
	_txtPsiTU->setHighContrast();

	_numOrder->setColor(WHITE);
	_numOrder->setVisible(false);

	_numTuCost->setColor(WHITE);
	_numTuCost->setVisible(false);

	_wndHead->setColor(RED);
	_wndHead->setVisible(false);
	_wndTorso->setColor(RED);
	_wndTorso->setVisible(false);
	_wndRightArm->setColor(RED);
	_wndRightArm->setVisible(false);
	_wndLeftArm->setColor(RED);
	_wndLeftArm->setVisible(false);
	_wndRightLeg->setColor(RED);
	_wndRightLeg->setVisible(false);
	_wndLeftLeg->setColor(RED);
	_wndLeftLeg->setVisible(false);

	_txtItem->setHighContrast();

	_txtAmmo->setAlign(ALIGN_LEFT);
	_txtAmmo->setHighContrast();

	_btnOk->onMouseClick((ActionHandler)& InventoryState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InventoryState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InventoryState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InventoryState::btnOkClick,
					Options::keyCancel);
	_btnOk->onKeyboardPress(
					(ActionHandler)& InventoryState::btnOkClick,
					Options::keyBattleInventory);
//	_btnOk->setTooltip("STR_OK");
//	_btnOk->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnOk->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnPrev->onMouseClick((ActionHandler)& InventoryState::btnPrevClick);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& InventoryState::btnPrevClick,
					Options::keyBattlePrevUnit);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& InventoryState::btnPrevClick,
					SDLK_LEFT);
	_btnPrev->onKeyboardPress(
					(ActionHandler)& InventoryState::btnPrevClick,
					SDLK_KP4);
//	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");
//	_btnPrev->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnPrev->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnNext->onMouseClick((ActionHandler)& InventoryState::btnNextClick);
	_btnNext->onKeyboardPress(
					(ActionHandler)& InventoryState::btnNextClick,
					Options::keyBattleNextUnit);
	_btnNext->onKeyboardPress(
					(ActionHandler)& InventoryState::btnNextClick,
					SDLK_RIGHT);
	_btnNext->onKeyboardPress(
					(ActionHandler)& InventoryState::btnNextClick,
					SDLK_KP6);
//	_btnNext->setTooltip("STR_NEXT_UNIT");
//	_btnNext->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnNext->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnUnloadClick,
					SDL_BUTTON_LEFT);
	_btnUnload->onMouseClick(
					(ActionHandler)& InventoryState::btnSaveLayouts,
					SDL_BUTTON_RIGHT);
//	_btnUnload->onMouseClick((ActionHandler)& InventoryState::btnUnloadClick);
//	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");
//	_btnUnload->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnUnload->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnGroundL->onMouseClick(
					(ActionHandler)& InventoryState::btnGroundClick,
					SDL_BUTTON_LEFT);
	_btnGroundL->onMouseClick(
					(ActionHandler)& InventoryState::btnUnequipUnitClick,
					SDL_BUTTON_RIGHT);

	_btnGroundR->onMouseClick(
					(ActionHandler)& InventoryState::btnGroundClick,
					SDL_BUTTON_LEFT);
	_btnGroundR->onMouseClick(
					(ActionHandler)& InventoryState::btnUnequipUnitClick,
					SDL_BUTTON_RIGHT);
	_btnGroundR->onKeyboardPress(
					(ActionHandler)& InventoryState::btnUnequipUnitClick,
					Options::keyInvClear);
//	_btnGroundR->setTooltip("STR_SCROLL_RIGHT");
//	_btnGroundR->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnGroundR->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

	_btnRank->onMouseClick((ActionHandler)& InventoryState::btnRankClick);
//	_btnRank->setTooltip("STR_UNIT_STATS");
//	_btnRank->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnRank->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


/*	_btnCreateTemplate->onMouseClick((ActionHandler)& InventoryState::btnCreateTemplateClick);
	_btnCreateTemplate->onKeyboardPress(
					(ActionHandler)& InventoryState::btnCreateTemplateClick,
					Options::keyInvCreateTemplate); */
//	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");
//	_btnCreateTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnCreateTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnApplyTemplate->onMouseClick((ActionHandler)& InventoryState::btnApplyTemplateClick);
	_btnApplyTemplate->onKeyboardPress(
					(ActionHandler)& InventoryState::btnApplyTemplateClick,
					Options::keyInvApplyTemplate); */
//	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");
//	_btnApplyTemplate->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnApplyTemplate->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);

/*	_btnClearInventory->onMouseClick((ActionHandler)& InventoryState::btnUnequipUnitClick);
	_btnClearInventory->onKeyboardPress(
					(ActionHandler)& InventoryState::btnUnequipUnitClick,
					Options::keyInvClear); */
//	_btnClearInventory->setTooltip("STR_CLEAR_INVENTORY");
//	_btnClearInventory->onMouseIn((ActionHandler)& InventoryState::txtTooltipIn);
//	_btnClearInventory->onMouseOut((ActionHandler)& InventoryState::txtTooltipOut);


	// only use copy/paste layout-template buttons in setup (i.e. non-tu) mode
/*	if (_tuMode)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
		_btnClearInventory->setVisible(false);
	}
	else
		_updateTemplateButtons(true); */

	_inventoryPanel->setSelectedUnitInventory(_battleSave->getSelectedUnit());
	_inventoryPanel->setTuMode(_tuMode);
	_inventoryPanel->draw();
	_inventoryPanel->onMouseClick((ActionHandler)& InventoryState::inClick, 0);
	_inventoryPanel->onMouseOver((ActionHandler)& InventoryState::inMouseOver);
	_inventoryPanel->onMouseOut((ActionHandler)& InventoryState::inMouseOut);

	_txtTUs->setVisible(_tuMode);
	_txtUseTU->setVisible(_tuMode);

	const bool vis (_tuMode == false);
	_txtFAcc->setVisible(vis);
	_txtReact->setVisible(vis);
	_txtThrow->setVisible(vis);
	_txtMelee->setVisible(vis);
	_txtPStr->setVisible(vis);
	_txtPSkill->setVisible(vis);

//	_timer = new Timer(300u);
//	_timer->onTimer((StateHandler)& InventoryState::keyRepeat);
//	_timer->start();
}

/**
 * Helper for the dTor.
 *
static void _clearInventoryTemplate(std::vector<SoldierLayout*>& inventoryTemplate)
{
	for (std::vector<SoldierLayout*>::iterator i = inventoryTemplate.begin(); i != inventoryTemplate.end();)
		delete *i;
} */

/**
 * dTor.
 */
InventoryState::~InventoryState()
{
//	_clearInventoryTemplate(_curInventoryTemplate);
//	delete _timer;
	if (_parent != nullptr)
	{
		Tile* const tile (_battleSave->getSelectedUnit()->getTile());

//		int flarePower (0);
//		for (std::vector<BattleItem*>::const_iterator
//				i = tile->getInventory()->begin();
//				i != tile->getInventory()->end();
//				++i)
//		{
//			if ((*i)->getRules()->getBattleType() == BT_FLARE
//				&& (*i)->getFuse() != -1
//				&& (*i)->getRules()->getPower() > flarePower)
//			{
//				flarePower = (*i)->getRules()->getPower();
//			}
//		}

		TileEngine* const te (_battleSave->getTileEngine());
		te->applyGravity(tile);

//		if (flarePower > _flarePower)
//		{
		te->calculateTerrainLighting();
		te->calcFovAll(true); // <- done in BattlescapeGame::init() -> but without 'spotSound'
//		}

		_battleSave->getBattleGame()->getMap()->setNoDraw(false);
	}
}
//	if (_battleSave->getTileEngine())
//	{
//		if (Options::maximizeInfoScreens)
//		{
//			Screen::updateScale(
//							Options::battlescapeScale,
//							Options::battlescapeScale,
//							Options::baseXBattlescape,
//							Options::baseYBattlescape,
//							true);
//			_game->getScreen()->resetDisplay(false);
//		}

//kL	Tile* tile = _battleSave->getSelectedUnit()->getTile();
//kL	_battleSave->getTileEngine()->applyGravity(tile);
//kL	_battleSave->getTileEngine()->calculateTerrainLighting(); // dropping/picking up flares
//kL	_battleSave->getTileEngine()->calcFovAll();

//	}
//	else
//	{
//		Screen::updateScale(
//						Options::geoscapeScale,
//						Options::geoscapeScale,
//						Options::baseXGeoscape,
//						Options::baseYGeoscape,
//						true);
//		_game->getScreen()->resetDisplay(false);
//	}

/**
 * Hits the think timer.
 *
void InventoryState::think()
{
	State::think();
	_timer->think(this, nullptr);
} */

/**
 * Advances to the next/previous Unit when right/left key is depressed.
 *
void InventoryState::keyRepeat() // private.
{
	Uint8* keystate = SDL_GetKeyState(nullptr);
	if (keystate[Options::keyBattleNextUnit] == 1
		|| keystate[SDLK_KP6] == 1)
	{
		btnNextClick(nullptr);
	}
	else if (keystate[Options::keyBattlePrevUnit] == 1
		|| keystate[SDLK_KP4] == 1)
	{
		btnPrevClick(nullptr);
	}
} */

/**
 * Updates all soldier stats when the soldier changes.
 * @note parent BattlescapeState is invalid @ BaseEquip screen
 */
void InventoryState::init()
{
	State::init();

	BattleUnit* unit (_battleSave->getSelectedUnit());
	if (unit == nullptr) // no selected unit, close inventory
	{
		btnOkClick(nullptr);
		return;
	}

	if (unit->hasInventory() == false) // skip to the first unit with inventory
	{
		if (_parent != nullptr)
			_parent->selectNextPlayerUnit(false,false,true);
		else
			_battleSave->selectNextFactionUnit(false,false,true);

		if ((unit = _battleSave->getSelectedUnit()) == nullptr
			 || unit->hasInventory() == false)
		{
			btnOkClick(nullptr);
			return; // starting a mission with just vehicles. kL_note: DISALLOWED!!!
		}
	}

//	if (_parent) _parent->getMap()->getCamera()->centerOnPosition(unit->getPosition(), false);

	unit->flagCache();

	_paper->clear();
	_btnRank->clear();
	_gender->clear();

	_txtName->setText(unit->getName(_game->getLanguage()));

	_inventoryPanel->setSelectedUnitInventory(unit);

//	_flarePower = 0;										// unfortunately this can screw up when accessing the UfoPaedia
//	for (std::vector<BattleItem*>::const_iterator			// or when picking up a flare-item, so just go ahead and run
//			i = unit->getTile()->getInventory()->begin();	// both calcLight/Fov in the dTor regardless.
//			i != unit->getTile()->getInventory()->end();
//			++i)
//	{
//		if ((*i)->getRules()->getBattleType() == BT_FLARE
//			&& (*i)->getFuse() != -1
//			&& (*i)->getRules()->getPower() > _flarePower)
//		{
//			_flarePower = (*i)->getRules()->getPower();
//		}
//	}


	const Soldier* const sol (unit->getGeoscapeSoldier());
	if (sol != nullptr)
	{
		SurfaceSet* const srtRank (_game->getResourcePack()->getSurfaceSet("SMOKE.PCK"));
		srtRank->getFrame(20 + sol->getRank())->blit(_btnRank);

		std::string look (sol->getArmor()->getSpriteInventory());
		Surface* srfGender;
		switch (sol->getGender())
		{
			default:
			case GENDER_MALE:
				srfGender = _game->getResourcePack()->getSurface("GENDER_M");
				look += "M";
				break;

			case GENDER_FEMALE:
				srfGender = _game->getResourcePack()->getSurface("GENDER_F");
				look += "F";
		}
		srfGender->blit(_gender);

		switch (sol->getLook())
		{
			default:
			case LOOK_BLONDE:		look += "0"; break;
			case LOOK_BROWNHAIR:	look += "1"; break;
			case LOOK_ORIENTAL:		look += "2"; break;
			case LOOK_AFRICAN:		look += "3";
		}
		look += ".SPK";

		if (CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false
			&& _game->getResourcePack()->getSurface(look) == nullptr)
		{
			look = sol->getArmor()->getSpriteInventory() + ".SPK";
		}

		_game->getResourcePack()->getSurface(look)->blit(_paper);
	}
	else
	{
		Surface* const dolphins (_game->getResourcePack()->getSurface("DOLPHINS"));
		dolphins->blit(_btnRank);

		Surface* const srfPaper (_game->getResourcePack()->getSurface(unit->getArmor()->getSpriteInventory()));
		if (srfPaper != nullptr)
			srfPaper->blit(_paper);
	}

	updateStats();
	updateWounds();
	_battleSave->getBattleState()->refreshMousePosition();
}

/**
 * Updates the selected unit's info - weight, TU, etc.
 */
void InventoryState::updateStats() // private.
{
	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());

	if (selUnit->getGeoscapeSoldier() != nullptr)
	{
		_numOrder->setVisible();
		_numOrder->setValue(selUnit->getBattleOrder());
	}
	else
		_numOrder->setVisible(false);

	if (_tuMode == true)
		_txtTUs->setText(tr("STR_TIME_UNITS_SHORT").arg(selUnit->getTimeUnits()));

	const int
		weight (selUnit->getCarriedWeight(_inventoryPanel->getSelectedItem())),
		strength (selUnit->getStrength());

	_txtWeight->setText(tr("STR_WEIGHT").arg(weight).arg(strength));
	if (weight > strength)
		_txtWeight->setSecondaryColor(static_cast<Uint8>(
					_game->getRuleset()->getInterface("inventory")->getElement("weight")->color2));
	else
		_txtWeight->setSecondaryColor(static_cast<Uint8>(
					_game->getRuleset()->getInterface("inventory")->getElement("weight")->color));

	const int psiSkill (selUnit->getBattleStats()->psiSkill);

	if (_tuMode == true)
	{
		if (selUnit->getBattleStats()->throwing != 0)
		{
			_txtThrowTU->setVisible();
			_txtThrowTU->setText(tr("STR_THROW_").arg(selUnit->getActionTu(BA_THROW)));
		}
		else
			_txtThrowTU->setVisible(false);

		if (selUnit->getOriginalFaction() == FACTION_HOSTILE
			&& psiSkill != 0)
		{
			_txtPsiTU->setVisible();
			_txtPsiTU->setText(tr("STR_PSI_")
						.arg(selUnit->getActionTu(
											BA_PSIPANIC,
											_parent->getBattleGame()->getAlienPsi())));
		}
		else
			_txtPsiTU->setVisible(false);
	}
	else
	{
		_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(selUnit->getBattleStats()->firing));
		_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(selUnit->getBattleStats()->reactions));
		_txtThrow->setText(tr("STR_THROWACC_SHORT").arg(selUnit->getBattleStats()->throwing));
		_txtMelee->setText(tr("STR_MELEEACC_SHORT").arg(selUnit->getBattleStats()->melee));

		if (psiSkill != 0)
		{
			_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(selUnit->getBattleStats()->psiStrength));
			_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
		}
		else
		{
			_txtPStr->setText(L"");
			_txtPSkill->setText(L"");
		}
	}
}

/**
 * Shows woundage values.
 */
void InventoryState::updateWounds() // private.
{
	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	UnitBodyPart bodyPart;
	unsigned wound;

	for (size_t
			i = 0u;
			i != BattleUnit::PARTS_BODY;
			++i)
	{
		bodyPart = static_cast<UnitBodyPart>(i);
		wound = static_cast<unsigned>(selUnit->getFatalWound(bodyPart));

		switch (bodyPart)
		{
			case BODYPART_HEAD:
				if (wound != 0u)
				{
					_wndHead->setValue(wound);
					_wndHead->setVisible();
				}
				else
					_wndHead->setVisible(false);
				break;

			case BODYPART_TORSO:
				if (wound != 0u)
				{
					_wndTorso->setValue(wound);
					_wndTorso->setVisible();
				}
				else
					_wndTorso->setVisible(false);
				break;

			case BODYPART_RIGHTARM:
				if (wound != 0u)
				{
					_wndRightArm->setValue(wound);
					_wndRightArm->setVisible();
				}
				else
					_wndRightArm->setVisible(false);
				break;

			case BODYPART_LEFTARM:
				if (wound != 0u)
				{
					_wndLeftArm->setValue(wound);
					_wndLeftArm->setVisible();
				}
				else
					_wndLeftArm->setVisible(false);
				break;

			case BODYPART_RIGHTLEG:
				if (wound != 0u)
				{
					_wndRightLeg->setValue(wound);
					_wndRightLeg->setVisible();
				}
				else
					_wndRightLeg->setVisible(false);
				break;

			case BODYPART_LEFTLEG:
				if (wound != 0u)
				{
					_wndLeftLeg->setValue(wound);
					_wndLeftLeg->setVisible();
				}
				else
					_wndLeftLeg->setVisible(false);
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnOkClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
	{
		_game->popState();

		if (_tuMode == false && _parent != nullptr) // pre-Battle but going into Battlescape!
		{
			_battleSave->resetUnitsOnTiles();

			Tile* const inTile (_battleSave->getBattleInventory());
			_battleSave->distributeEquipment(inTile);	// This doesn't seem to happen on second stage of Multi-State MISSIONS.
														// In fact, none of this !_tuMode InventoryState appears to run for 2nd staged missions.
														// and BattlescapeGenerator::nextStage() has its own bu->prepUnit() call ....
														// but Leaving this out could be troublesome for Multi-Stage MISSIONS.
//			if (_battleSave->getTurn() == 1)
//			{
//				_battleSave->distributeEquipment(inTile);
//				if (inTile->getTileUnit())
//					_battleSave->setSelectedUnit(inTile->getTileUnit()); // make sure the unit closest to the ramp is selected.
//			}

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
					(*i)->prepTu(true);
			}
		}
	}
}

/**
 * Selects the previous eligible unit.
 * @param action - pointer to an Action
 */
void InventoryState::btnPrevClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
	{
		if (_parent != nullptr)
			_parent->selectPreviousPlayerUnit(false,false,true);
		else
			_battleSave->selectPreviousFactionUnit(false,false,true);

		init();
	}
}

/**
 * Selects the next eligible unit.
 * @param action - pointer to an Action
 */
void InventoryState::btnNextClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
	{
		if (_parent != nullptr)
			_parent->selectNextPlayerUnit(false,false,true);
		else
			_battleSave->selectNextFactionUnit(false,false,true);

		init();
	}
}

/**
 * Unloads the selected weapon or saves a Soldier's equipment-layout.
 * @param action - pointer to an Action
 */
void InventoryState::btnUnloadClick(Action*)
{
	if (_inventoryPanel->unload() == true)
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_srfAmmo->clear();

		updateStats();
		_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_UNLOAD_HQ)->play();
	}
	else if (_tuMode == false
		&& _inventoryPanel->getSelectedItem() == nullptr
		&& saveLayout(_battleSave->getSelectedUnit()) == true)
	{
		_inventoryPanel->showWarning(tr("STR_EQUIP_LAYOUT_SAVED"));
	}
}

/**
 * Saves all Soldiers' equipment-layouts in pre-battle.
 * @param action - pointer to an Action
 */
void InventoryState::btnSaveLayouts(Action*)
{
	if (_tuMode == false
		&& _inventoryPanel->getSelectedItem() == nullptr
		&& saveAllLayouts() == true)
	{
		_inventoryPanel->showWarning(tr("STR_EQUIP_LAYOUTS_SAVED"));
	}
}

/**
 * Saves all Soldiers' equipment-layouts.
 * @note Helper for btnSaveLayouts().
 * @return, true if a layout is saved
 */
bool InventoryState::saveAllLayouts() const // private.
{
	bool ret (false);
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (saveLayout(*i) == true)
			ret = true;
	}
	return ret;
}

/**
 * Saves a Soldier's equipment layout.
 * @note Called from btnUnloadClick() if in pre-battle.
 * @param unit - pointer to a BattleUnit
 * @return, true if layout saved
 */
bool InventoryState::saveLayout(BattleUnit* const unit) const // private.
{
	if (unit->getGeoscapeSoldier() != nullptr)
	{
		std::vector<SoldierLayout*>* const layoutItems (unit->getGeoscapeSoldier()->getLayout());
		if (layoutItems->empty() == false) // clear Soldier's items
		{
			for (std::vector<SoldierLayout*>::const_iterator
					i = layoutItems->begin();
					i != layoutItems->end();
					++i)
			{
				delete *i;
			}
			layoutItems->clear();
		}

		// Note when using getInventory() the loaded ammos are skipped because
		// they're not owned by the unit; ammo is handled separately by the weapon.
		for (std::vector<BattleItem*>::const_iterator // save Soldier's items
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			std::string st;
			if ((*i)->selfPowered() == false && (*i)->getAmmoItem() != nullptr)
				st = (*i)->getAmmoItem()->getRules()->getType();

			layoutItems->push_back(new SoldierLayout(
												(*i)->getRules()->getType(),
												(*i)->getInventorySection()->getInventoryType(),
												(*i)->getSlotX(),
												(*i)->getSlotY(),
												st,
												(*i)->getFuse()));
		}
		return true;
	}
	return false;
}

/**
 * Shows more ground items / rearranges them.
 * @param action - pointer to an Action
 */
void InventoryState::btnGroundClick(Action* action)
{
	if (action->getSender() == dynamic_cast<InteractiveSurface*>(_btnGroundR))
		_inventoryPanel->arrangeGround(+1);
	else
		_inventoryPanel->arrangeGround(-1);
}

/**
 * Clears the current unit's inventory and places all items on the ground.
 * @param action - pointer to an Action
 */
void InventoryState::btnUnequipUnitClick(Action*)
{
	if (_tuMode == false									// don't accept clicks in battlescape because this doesn't cost TU.
		&& _inventoryPanel->getSelectedItem() == nullptr)	// or if mouse is holding an item
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		std::vector<BattleItem*>* const equipt (unit->getInventory());
		if (equipt->empty() == false)
		{
			_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();

			const RuleInventory* const grdRule (_game->getRuleset()->getInventoryRule(ST_GROUND));
			Tile* const tile (unit->getTile());
			for (std::vector<BattleItem*>::const_iterator
					i = equipt->begin();
					i != equipt->end();
					++i)
			{
				(*i)->setOwner();
				(*i)->setInventorySection(grdRule);
				tile->addItem(*i);
			}
			equipt->clear();

			_inventoryPanel->arrangeGround();
			updateStats();
			_battleSave->getBattleState()->refreshMousePosition();
		}
	}
}
/* void InventoryState::btnUnequipUnitClick(Action*)
{
	if (_tuMode == true						// don't accept clicks in battlescape
		&& _inventoryPanel->getSelectedItem() == nullptr) // or when moving items
	{
		BattleUnit* const unit = _battleSave->getSelectedUnit();
		std::vector<BattleItem*>* const unitInv = unit->getInventory();
		Tile* const groundTile = unit->getTile();
		clearInventory(_game, unitInv, groundTile);
		_inventoryPanel->arrangeGround(); // refresh ui
		updateStats();
		refreshMouse();

		_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();
	}
} */
/*
 * Clears unit's inventory - move everything to the ground.
 * @note Helper for btnUnequipUnitClick().
 * @param game			- pointer to the core Game to get the Ruleset
 * @param unitInv		- pointer to a vector of pointers to BattleItems
 * @param groundTile	- pointer to the ground Tile
 *
void InventoryState::clearInventory( // private.
		Game* game,
		std::vector<BattleItem*>* unitInv,
		Tile* groundTile)
{
	RuleInventory* const groundRule = game->getRuleset()->getInventory("STR_GROUND");
	for (std::vector<BattleItem*>::const_iterator i = unitInv->begin(); i != unitInv->end();)
	{
		(*i)->setOwner();
		groundTile->addItem(*i, groundRule);
		i = unitInv->erase(i);
	}
} */

/**
 * Shows the unit info screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnRankClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
		_game->pushState(new UnitInfoState(
										_battleSave->getSelectedUnit(),
										_parent,
										true, false,
										_tuMode == false));
}

/**
 * Updates item info.
 * @param action - pointer to an Action
 */
void InventoryState::inClick(Action*)
{
	_numTuCost->setVisible(false);

	// kL_note: This function has only updateStats() in the stock code,
	// since induction of Copy/Paste Inventory Layouts ... that is, the
	// vast majority of this function has been subsumed into inMouseOver().
	// But i'm leaving it in anyway ...

	const BattleItem* const selItem (_inventoryPanel->getSelectedItem());
	if (selItem != nullptr)
	{
		setExtraInfo(selItem);

		std::wstring wst;
		switch (selItem->getRules()->getBattleType())
		{
			case BT_MEDIKIT:
				wst = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
						.arg(selItem->getPainKillerQuantity())
						.arg(selItem->getStimulantQuantity())
						.arg(selItem->getHealQuantity());
				break;

			case BT_AMMO:
				wst = tr("STR_AMMO_ROUNDS_LEFT").arg(selItem->getAmmoQuantity());
				break;

			case BT_FIREARM:
//			case BT_MELEE:
				if (selItem->selfPowered() == false || selItem->selfExpended() == true)
				{
					const BattleItem* const aItem (selItem->getAmmoItem());
					if (aItem != nullptr)
					{
						wst = tr("STR_AMMO_ROUNDS_LEFT").arg(aItem->getAmmoQuantity());

						SDL_Rect rect;
						rect.x =
						rect.y = 0;
						rect.w = static_cast<Sint16>(RuleInventory::HAND_W * RuleInventory::SLOT_W);
						rect.h = static_cast<Sint16>(RuleInventory::HAND_H * RuleInventory::SLOT_H);
						_srfAmmo->drawRect(
										&rect,
										static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("grid")->color));

						++rect.x;
						++rect.y;
						rect.w -= 2;
						rect.h -= 2;
						_srfAmmo->drawRect(&rect, 15);

						aItem->getRules()->drawHandSprite(
													_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
													_srfAmmo);
					}
				}
		}
		_txtAmmo->setText(wst);
	}

	updateStats();
}

/**
 * Shows item info.
 * @param action - pointer to an Action
 */
void InventoryState::inMouseOver(Action*)
{
	if (_inventoryPanel->getSelectedItem() != nullptr)
	{
//		_numTuCost->setValue(static_cast<unsigned>(_inventoryPanel->getTuCostInventory()));
//		_numTuCost->setVisible(_tuMode == true
//							&& _inventoryPanel->getTuCostInventory() > 0);
		if (_tuMode == true)
		{
			const int costTu (_inventoryPanel->getTuCostInventory());
			if (costTu > 0)
			{
				_numTuCost->setValue(static_cast<unsigned>(costTu));
				_numTuCost->setVisible();
			}
			else
				_numTuCost->setVisible(false);
		}
//		_updateTemplateButtons(false);
	}
	else // no item on cursor.
	{
		_srfAmmo->clear();

		const BattleItem* const overItem (_inventoryPanel->getMouseOverItem());
		if (overItem != nullptr)
		{
			setExtraInfo(overItem);
//			_updateTemplateButtons(false);

			std::wstring wst;
			switch (overItem->getRules()->getBattleType())
			{
				case BT_MEDIKIT:
					wst = tr("STR_MEDI_KIT_QUANTITIES_LEFT")
							.arg(overItem->getPainKillerQuantity())
							.arg(overItem->getStimulantQuantity())
							.arg(overItem->getHealQuantity());
					break;

				case BT_AMMO:
					wst = tr("STR_AMMO_ROUNDS_LEFT").arg(overItem->getAmmoQuantity());
					break;

				case BT_FIREARM:
//				case BT_MELEE:
					if (overItem->selfPowered() == false || overItem->selfExpended() == true)
					{
						const BattleItem* const aItem (overItem->getAmmoItem());
						if (aItem != nullptr)
						{
							wst = tr("STR_AMMO_ROUNDS_LEFT").arg(aItem->getAmmoQuantity());

							SDL_Rect rect;
							rect.x =
							rect.y = 0;
							rect.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
							rect.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
							_srfAmmo->drawRect(&rect, static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("grid")->color));

							++rect.x;
							++rect.y;
							rect.w -= 2;
							rect.h -= 2;
							_srfAmmo->drawRect(&rect, 15);

							aItem->getRules()->drawHandSprite(
														_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
														_srfAmmo);
//							_updateTemplateButtons(false);
						}
//						else _updateTemplateButtons(!_tuMode);
					}
			}
			_txtAmmo->setText(wst);
		}
		else // no Item under cursor.
		{
//			if (_currentTooltip.empty())
			_txtItem->setText(L"");
			_txtAmmo->setText(L"");
			_txtUseTU->setText(L"");
//			_updateTemplateButtons(!_tuMode);
		}
	}
}

/**
 * Hides item info.
 * @param action - pointer to an Action
 */
void InventoryState::inMouseOut(Action*)
{
	_numTuCost->setVisible(false);

	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_txtUseTU->setText(L"");

	_srfAmmo->clear();
//	_updateTemplateButtons(!_tuMode);
}

/**
 * Takes care of any events from the core game engine.
 * @param action - pointer to an Action
 */
void InventoryState::handle(Action* action)
{
	State::handle(action);
/*
#ifndef __MORPHOS__
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_X1)
			btnNextClick(action);
		else if (action->getDetails()->button.button == SDL_BUTTON_X2)
			btnPrevClick(action);
	}
#endif
*/
}

/**
 * Sets the extra-info fields on mouseover and mouseclicks.
 * @param selOver - pointer to a BattleItem (selected or mouseover)
 */
void InventoryState::setExtraInfo(const BattleItem* const selOver) // private.
{
	const RuleItem* const itRule (selOver->getRules());
	std::wostringstream label;
	bool isArt;

	if (selOver->getRules()->getBattleType() == BT_CORPSE)
	{
		isArt = false;
		const BattleUnit* const corpseUnit (selOver->getUnit());
		if (corpseUnit != nullptr)
		{
			if (corpseUnit->getType().compare(0,11, "STR_FLOATER") == 0 // special handling for Floaters.
				&& (_game->getSavedGame()->isResearched("STR_FLOATER") == false
					|| _game->getSavedGame()->isResearched("STR_FLOATER_AUTOPSY") == false))
			{
				label << tr("STR_FLOATER"); // STR_FLOATER_CORPSE
			}
			else
			{
				switch (corpseUnit->getUnitStatus())
				{
					case STATUS_DEAD:
						label << tr(itRule->getType());
						if (corpseUnit->getOriginalFaction() == FACTION_PLAYER)
							label << L" (" + corpseUnit->getName(_game->getLanguage()) + L")";
						break;

					case STATUS_UNCONSCIOUS:
						label << corpseUnit->getName(_game->getLanguage());
				}
			}
		}
		else
			label << tr(itRule->getType());
	}
	else if (_game->getSavedGame()->isResearched(itRule->getRequirements()) == true)
	{
		isArt = false;
		label << tr(itRule->getType());
	}
	else
	{
		label << tr("STR_ALIEN_ARTIFACT");
		isArt = true;
	}

	int weight (itRule->getWeight());
	if (selOver->selfPowered() == false && selOver->getAmmoItem() != nullptr)
		weight += selOver->getAmmoItem()->getRules()->getWeight();

	label << L" (" << weight << L")";
	_txtItem->setText(label.str());

	_txtUseTU->setText(L"");
	if (isArt == false)
	{
		const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
		if (selUnit != nullptr)
		{
			const BattleActionType bat (itRule->getDefaultAction(selOver->getFuse() != -1));
			if (bat != BA_NONE || itRule->getBattleType() == BT_AMMO)
			{
				int tu;
				std::string actionType;
				switch (itRule->getBattleType())
				{
					case BT_AMMO:
						tu = itRule->getReloadTu();
						actionType = "STR_RELOAD_";
						break;

					default:
						tu = selUnit->getActionTu(bat, selOver);
						switch (bat)
						{
							case BA_LAUNCH:		actionType = "STR_LAUNCH_";	break;
							case BA_SNAPSHOT:	actionType = "STR_SNAP_";	break;
							case BA_AUTOSHOT:	actionType = "STR_BURST_";	break;
							case BA_AIMEDSHOT:	actionType = "STR_SCOPE_";	break;
							case BA_PRIME:		actionType = "STR_PRIME_";	break;
							case BA_DEFUSE:		actionType = "STR_DEFUSE_";	break;
							case BA_USE:		actionType = "STR_USE_";	break;
							case BA_PSIPANIC:	actionType = "STR_PSI_";	break;
							case BA_MELEE:		actionType = "STR_ATTACK_";
						}
				}
				_txtUseTU->setText(tr(actionType).arg(tu));
			}
		}
	}
}

}

/**
 * Shows a tooltip for the appropriate button.
 * @param action - pointer to an Action
 *
void InventoryState::txtTooltipIn(Action* action)
{
	if (_inventoryPanel->getSelectedItem() == 0 && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtItem->setText(tr(_currentTooltip));
	}
} */
/**
 * Clears the tooltip text.
 * @param action - pointer to an Action
 *
void InventoryState::txtTooltipOut(Action* action)
{
	if (_inventoryPanel->getSelectedItem() == 0 && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_currentTooltip = "";
			_txtItem->setText(L"");
		}
	}
} */

/**
 *
 *
void InventoryState::_updateTemplateButtons(bool isVisible)
{
	if (isVisible)
	{
		_game->getResourcePack()->getSurface("InvClear")->blit(_btnClearInventory);

		if (_curInventoryTemplate.empty()) // use "empty template" icons
		{
			_game->getResourcePack()->getSurface("InvCopy")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPasteEmpty")->blit(_btnApplyTemplate);
		}
		else // use "active template" icons
		{
			_game->getResourcePack()->getSurface("InvCopyActive")->blit(_btnCreateTemplate);
			_game->getResourcePack()->getSurface("InvPaste")->blit(_btnApplyTemplate);
		}

		_btnCreateTemplate->initSurfaces();
		_btnApplyTemplate->initSurfaces();
		_btnClearInventory->initSurfaces();
	}
	else
	{
		_btnCreateTemplate->clear();
		_btnApplyTemplate->clear();
		_btnClearInventory->clear();
	}
} */
/**
 *
 *
void InventoryState::btnCreateTemplateClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() != nullptr) // don't accept clicks when moving items
		return;

	_clearInventoryTemplate(_curInventoryTemplate); // clear current template

	// Copy inventory instead of just keeping a pointer to it. This way
	// create/apply can be used as an undo button for a single unit and will
	// also work as expected if inventory is modified after 'create' is clicked.
	std::vector<BattleItem*>* unitInv = _battleSave->getSelectedUnit()->getInventory();
	for (std::vector<BattleItem*>::iterator
			j = unitInv->begin();
			j != unitInv->end();
			++j)
	{
		std::string ammo;
		if ((*j)->usesAmmo()
			&& (*j)->getAmmoItem())
		{
			ammo = (*j)->getAmmoItem()->getRules()->getType();
		}
		else
			ammo = "NONE";

		_curInventoryTemplate.push_back(new SoldierLayout(
															(*j)->getRules()->getType(),
															(*j)->getInventorySection()->getId(),
															(*j)->getSlotX(),
															(*j)->getSlotY(),
															ammo,
															(*j)->getFuse()));
	}

	_game->getResourcePack()->getSoundByDepth(
											_battleSave->getDepth(),
											ResourcePack::ITEM_DROP)->play();
	refreshMouse();
} */
/**
 *
 *
void InventoryState::btnApplyTemplateClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() != nullptr)	// don't accept clicks when moving items
//		|| _curInventoryTemplate.empty())	// or when the template is empty ->
											// if the template is empty -- it will just result in clearing the unit's inventory
	{
		return;
	}

	BattleUnit* unit = _battleSave->getSelectedUnit();
	std::vector<BattleItem*>* unitInv = unit->getInventory();

	Tile* groundTile = unit->getTile();
	std::vector<BattleItem*>* groundInv = groundTile->getInventory();

	RuleInventory* groundRuleInv = _game->getRuleset()->getInventory("STR_GROUND");

	_clearInventory(
				_game,
				unitInv,
				groundTile);

	// attempt to replicate inventory template by grabbing corresponding items
	// from the ground. if any item is not found on the ground, display warning
	// message, but continue attempting to fulfill the template as best we can
	bool itemMissing = false;

	std::vector<SoldierLayout*>::iterator templateIt;
	for (
			templateIt = _curInventoryTemplate.begin();
			templateIt != _curInventoryTemplate.end();
			++templateIt)
	{
		// search for template item in ground inventory
		std::vector<BattleItem*>::iterator groundItem;
		const bool usesAmmo = !_game->getRuleset()->getItemRule((*templateIt)->getItemType())->getCompatibleAmmo()->empty();
		bool
			found = false,
			rescan = true;

		while (rescan)
		{
			rescan = false;

			const std::string targetAmmo = (*templateIt)->getAmmoItem();
			BattleItem* matchedWeapon = nullptr;
			BattleItem* matchedAmmo = nullptr;

			for (
					groundItem = groundInv->begin();
					groundItem != groundInv->end();
					++groundItem)
			{
				// if we find the appropriate ammo, remember it for later for if we find
				// the right weapon but with the wrong ammo
				const std::string groundItemName = (*groundItem)->getRules()->getType();
				if (usesAmmo
					&& targetAmmo == groundItemName)
				{
					matchedAmmo = *groundItem;
				}

				if ((*templateIt)->getItemType() == groundItemName)
				{
					// if the loaded ammo doesn't match the template item's,
					// remember the weapon for later and continue scanning
					BattleItem* loadedAmmo = (*groundItem)->getAmmoItem();
					if ((usesAmmo
							&& loadedAmmo
							&& targetAmmo != loadedAmmo->getRules()->getType())
						|| (usesAmmo
							&& !loadedAmmo))
					{
						// remember the last matched weapon for simplicity (but prefer empty weapons if any are found)
						if (!matchedWeapon
							|| matchedWeapon->getAmmoItem())
						{
							matchedWeapon = *groundItem;
						}

						continue;
					}

					// move matched item from ground to the appropriate inv slot
					(*groundItem)->setOwner(unit);
					(*groundItem)->setInventorySection(_game->getRuleset()->getInventory((*templateIt)->getInventorySection()));
					(*groundItem)->setSlotX((*templateIt)->getSlotX());
					(*groundItem)->setSlotY((*templateIt)->getSlotY());
					(*groundItem)->setFuse((*templateIt)->getFuse());

					unitInv->push_back(*groundItem);
					groundInv->erase(groundItem);

					found = true;
					break;
				}
			}

			// if we failed to find an exact match, but found unloaded ammo and
			// the right weapon, unload the target weapon, load the right ammo, and use it
			if (!found
				&& matchedWeapon
				&& (!usesAmmo
					|| matchedAmmo))
			{
				// unload the existing ammo (if any) from the weapon
				BattleItem* loadedAmmo = matchedWeapon->getAmmoItem();
				if (loadedAmmo)
				{
					groundTile->addItem(loadedAmmo, groundRuleInv);
					matchedWeapon->setAmmoItem();
				}

				// load the correct ammo into the weapon
				if (matchedAmmo)
				{
					matchedWeapon->setAmmoItem(matchedAmmo);
					groundTile->removeItem(matchedAmmo);
				}

				// rescan and pick up the newly-loaded/unloaded weapon
				rescan = true;
			}
		}

		if (!found)
			itemMissing = true;
	}

	if (itemMissing)
		_inventoryPanel->showWarning(tr("STR_NOT_ENOUGH_ITEMS_FOR_TEMPLATE"));


	_inventoryPanel->arrangeGround(); // refresh ui
	updateStats();
	refreshMouse();

	_game->getResourcePack()->getSoundByDepth(
											_battleSave->getDepth(),
											ResourcePack::ITEM_DROP)->play();
} */
