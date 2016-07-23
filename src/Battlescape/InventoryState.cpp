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
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/BattlescapeButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
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
 * @param tuMode - true if in battle when inventory usage costs TU (default false)
 * @param parent - pointer to parent BattlescapeState (default nullptr)
 */
InventoryState::InventoryState(
		bool tuMode,
		BattlescapeState* const parent)
	:
		_tuMode(tuMode),
		_parent(parent)
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

	_srfBg		= new Surface(320, 200);
	_srfRagdoll	= new Surface(320, 200);

	_txtName	= new Text(200, 17, 36, 6);
	_srfGender	= new Surface(7, 7, 28, 1);

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

	_numHead		= new NumberText(7, 5,  79,  31);
	_numTorso		= new NumberText(7, 5,  79, 144);
	_numRightArm	= new NumberText(7, 5,  40,  80);
	_numLeftArm		= new NumberText(7, 5, 117,  80);
	_numRightLeg	= new NumberText(7, 5,  40, 120);
	_numLeftLeg		= new NumberText(7, 5, 117, 120);
	_numFire		= new NumberText(7, 5, 154,  43);

	_txtItem	= new Text(160, 9, 128, 140);

	_btnOk		= new BattlescapeButton(35, 23, 237, 0);
	_btnPrev	= new BattlescapeButton(23, 23, 273, 0);
	_btnNext	= new BattlescapeButton(23, 23, 297, 0);

	_btnRank	= new BattlescapeButton(26, 23,   0,   0);
	_btnUnload	= new BattlescapeButton(32, 25, 288,  32);
	_btnGroundL	= new BattlescapeButton(32, 15,   0, 137);
	_btnGroundR	= new BattlescapeButton(32, 15, 288, 137);

//	_btnCreateTemplate	= new BattlescapeButton(32,22, _templateBtnX, _createTemplateBtnY);
//	_btnApplyTemplate	= new BattlescapeButton(32,22, _templateBtnX, _applyTemplateBtnY);
//	_btnClearInventory	= new BattlescapeButton(32,22, _templateBtnX, _clearInventoryBtnY);

	_txtAmmo = new Text(40, 24, 288, 64);
	_srfLoad = new Surface(
						RuleInventory::HAND_W * RuleInventory::SLOT_W,
						RuleInventory::HAND_H * RuleInventory::SLOT_H,
						288,88);

	_inventoryPanel	= new Inventory(
								_game,
//								320,200,
								0,0,
								_parent == nullptr);

	setPalette(PAL_BATTLESCAPE);

	add(_srfBg);
	_game->getResourcePack()->getSurface("Inventory")->blit(_srfBg);

	add(_srfGender);
	add(_srfRagdoll);
	add(_txtName,		"textName",			"inventory", _srfBg);

	add(_txtWeight,		"textWeight",		"inventory", _srfBg);
	add(_txtTUs,		"textTUs",			"inventory", _srfBg);
	add(_txtFAcc,		"textFiring",		"inventory", _srfBg);
	add(_txtReact,		"textReaction",		"inventory", _srfBg);
	add(_txtThrow,		"textThrowing",		"inventory", _srfBg);
	add(_txtMelee,		"textMelee",		"inventory", _srfBg);
	add(_txtPStr,		"textPsiStrength",	"inventory", _srfBg);
	add(_txtPSkill,		"textPsiSkill",		"inventory", _srfBg);

	add(_numOrder);
	add(_numTuCost);

	add(_txtItem,		"textItem",			"inventory", _srfBg);
	add(_txtAmmo,		"textAmmo",			"inventory", _srfBg);
	add(_btnOk,			"buttonOK",			"inventory", _srfBg);
	add(_btnPrev,		"buttonPrev",		"inventory", _srfBg);
	add(_btnNext,		"buttonNext",		"inventory", _srfBg);
	add(_btnUnload,		"buttonUnload",		"inventory", _srfBg);
	add(_btnGroundL,	"buttonGround",		"inventory", _srfBg);
	add(_btnGroundR,	"buttonGround",		"inventory", _srfBg);
	add(_btnRank,		"rank",				"inventory", _srfBg);

//	add(_btnCreateTemplate,	"buttonCreate",	"inventory", _srfBg);
//	add(_btnApplyTemplate,	"buttonApply",	"inventory", _srfBg);
//	add(_btnClearInventory);

	add(_txtUseTU,		"textTUs",			"inventory", _srfBg);
	add(_txtThrowTU,	"textTUs",			"inventory", _srfBg);
	add(_txtPsiTU,		"textTUs",			"inventory", _srfBg);

	add(_numHead);
	add(_numTorso);
	add(_numRightArm);
	add(_numLeftArm);
	add(_numRightLeg);
	add(_numLeftLeg);
	add(_numFire);

	add(_srfLoad);
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

	_numHead->setColor(RED);
	_numHead->setVisible(false);
	_numTorso->setColor(RED);
	_numTorso->setVisible(false);
	_numRightArm->setColor(RED);
	_numRightArm->setVisible(false);
	_numLeftArm->setColor(RED);
	_numLeftArm->setVisible(false);
	_numRightLeg->setColor(RED);
	_numRightLeg->setVisible(false);
	_numLeftLeg->setColor(RED);
	_numLeftLeg->setVisible(false);
	_numFire->setColor(RED);
	_numFire->setVisible(false);

	_txtItem->setHighContrast();

	_txtAmmo->setAlign(ALIGN_LEFT);
	_txtAmmo->setHighContrast();

	_btnOk->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnOkClick),
							Options::keyBattleInventory);
//	_btnOk->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnOk->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnOk->setTooltip("STR_OK");

	_btnPrev->onMouseClick(		static_cast<ActionHandler>(&InventoryState::btnPrevClick));
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnPrevClick),
								Options::keyBattlePrevUnit);
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnPrevClick),
								SDLK_LEFT);
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnPrevClick),
								SDLK_KP4);
//	_btnPrev->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnPrev->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnPrev->setTooltip("STR_PREVIOUS_UNIT");

	_btnNext->onMouseClick(		static_cast<ActionHandler>(&InventoryState::btnNextClick));
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnNextClick),
								Options::keyBattleNextUnit);
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnNextClick),
								SDLK_RIGHT);
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnNextClick),
								SDLK_KP6);
//	_btnNext->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnNext->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnNext->setTooltip("STR_NEXT_UNIT");

	_btnUnload->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnLoadIconClick));
	_btnUnload->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnSaveLayouts),
								SDL_BUTTON_RIGHT);
//	_btnUnload->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnLoadIconClick));
//	_btnUnload->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnUnload->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnUnload->setTooltip("STR_UNLOAD_WEAPON");

	_inventoryPanel->onMousePress(static_cast<ActionHandler>(&InventoryState::btnGroundPress));

	_btnGroundL->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnGroundClick));
	_btnGroundL->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnClearGroundClick),
								SDL_BUTTON_RIGHT);

	_btnGroundR->onMouseClick(		static_cast<ActionHandler>(&InventoryState::btnGroundClick));
	_btnGroundR->onMouseClick(		static_cast<ActionHandler>(&InventoryState::btnClearUnitClick),
									SDL_BUTTON_RIGHT);
	_btnGroundR->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnClearUnitClick),
									Options::keyInvClear);
//	_btnGroundR->onMouseIn(			static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnGroundR->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnGroundR->setTooltip("STR_SCROLL_RIGHT");

	_btnRank->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnRankClick));
//	_btnRank->onMouseIn(	static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnRank->onMouseOut(	static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnRank->setTooltip("STR_UNIT_STATS");


/*	_btnCreateTemplate->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnCreateTemplateClick));
	_btnCreateTemplate->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnCreateTemplateClick),
										Options::keyInvCreateTemplate); */
//	_btnCreateTemplate->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnCreateTemplate->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnCreateTemplate->setTooltip("STR_CREATE_INVENTORY_TEMPLATE");

/*	_btnApplyTemplate->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnApplyTemplateClick));
	_btnApplyTemplate->onKeyboardPress(	static_cast<ActionHandler>(&InventoryState::btnApplyTemplateClick),
										Options::keyInvApplyTemplate); */
//	_btnApplyTemplate->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnApplyTemplate->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnApplyTemplate->setTooltip("STR_APPLY_INVENTORY_TEMPLATE");

/*	_btnClearInventory->onMouseClick(	static_cast<ActionHandler>(&InventoryState::btnClearUnitClick));
	_btnClearInventory->onKeyboardPress(static_cast<ActionHandler>(&InventoryState::btnClearUnitClick),
										Options::keyInvClear); */
//	_btnClearInventory->onMouseIn(		static_cast<ActionHandler>(&InventoryState::txtTooltipIn));
//	_btnClearInventory->onMouseOut(		static_cast<ActionHandler>(&InventoryState::txtTooltipOut));
//	_btnClearInventory->setTooltip("STR_CLEAR_INVENTORY");


	// only use copy/paste layout-template buttons in setup (i.e. non-tu) mode
/*	if (_tuMode)
	{
		_btnCreateTemplate->setVisible(false);
		_btnApplyTemplate->setVisible(false);
		_btnClearInventory->setVisible(false);
	}
	else
		_updateTemplateButtons(true); */

//	_inventoryPanel->setSelectedUnitInventory(_battleSave->getSelectedUnit());
	_inventoryPanel->drawGrids();
	_inventoryPanel->setTuMode(_tuMode);
	_inventoryPanel->onMouseClick(	static_cast<ActionHandler>(&InventoryState::inClick),
									0u);
	_inventoryPanel->onMouseOver(	static_cast<ActionHandler>(&InventoryState::inMouseOver));
	_inventoryPanel->onMouseOut(	static_cast<ActionHandler>(&InventoryState::inMouseOut));

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
		TileEngine* const te (_battleSave->getTileEngine());

		Tile* const tile (_battleSave->getSelectedUnit()->getUnitTile());
		te->applyGravity(tile);

		te->calculateTerrainLighting();
		te->calcFovUnits_all(true);

		_battleSave->getBattleGame()->getMap()->setNoDraw(false);
	}

	_game->getScreen()->fadeScreen();
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
 * Hits the think Timer.
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
 * Updates displayed stats when the current-unit changes.
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

	_srfRagdoll->clear();
	_btnRank->clear();
	_srfGender->clear();

	_txtName->setText(unit->getName(_game->getLanguage()));

	_inventoryPanel->setSelectedUnitInventory(unit);

	const Soldier* const sol (unit->getGeoscapeSoldier());
	if (sol != nullptr)
	{
		SurfaceSet* const srtRank (_game->getResourcePack()->getSurfaceSet("SMOKE.PCK"));
		srtRank->getFrame(20 + sol->getRank())->blit(_btnRank);

		std::string look (sol->getArmor()->getSpriteInventory()); // See also: ArticleStateArmor cTor.
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
		srfGender->blit(_srfGender);

		switch (sol->getLook())
		{
			default:
			case LOOK_BLONDE:		look += "0"; break;
			case LOOK_BROWNHAIR:	look += "1"; break;
			case LOOK_ORIENTAL:		look += "2"; break;
			case LOOK_AFRICAN:		look += "3";
		}
		look += ".SPK";

		if (_game->getResourcePack()->getSurface(look) == nullptr)
//			&& CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false)
		{
			look = sol->getArmor()->getSpriteInventory() + ".SPK";

			if (_game->getResourcePack()->getSurface(look) == nullptr)
				look = sol->getArmor()->getSpriteInventory();
		}

		if (_game->getResourcePack()->getSurface(look) != nullptr)
			_game->getResourcePack()->getSurface(look)->blit(_srfRagdoll);
	}
	else
	{
		Surface* const dolphins (_game->getResourcePack()->getSurface("Dolphins"));
		dolphins->blit(_btnRank);

		Surface* const ragdoll (_game->getResourcePack()->getSurface(unit->getArmor()->getSpriteInventory()));
		if (ragdoll != nullptr)
			ragdoll->blit(_srfRagdoll);
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
	const RuleInterface* const uiRule (_game->getRuleset()->getInterface("inventory"));
	if (weight > strength)
		_txtWeight->setSecondaryColor(static_cast<Uint8>(uiRule->getElement("weight")->color2));
	else
		_txtWeight->setSecondaryColor(static_cast<Uint8>(uiRule->getElement("weight")->color));

	const int psiSkill (selUnit->getBattleStats()->psiSkill);

	if (_tuMode == true)
	{
		switch (selUnit->getBattleStats()->throwing)
		{
			case 0:
				_txtThrowTU->setVisible(false);
				break;

			default:
				_txtThrowTU->setVisible();
				_txtThrowTU->setText(tr("STR_THROW_").arg(selUnit->getActionTu(BA_THROW)));
		}

		switch (psiSkill)
		{
			case 0:
				_txtPsiTU->setVisible(false);
				break;

			default:
				if (selUnit->getOriginalFaction() == FACTION_HOSTILE)
				{
					_txtPsiTU->setVisible();
					_txtPsiTU->setText(tr("STR_PSI_")
								.arg(selUnit->getActionTu(
													BA_PSIPANIC,
													_parent->getBattleGame()->getAlienPsi())));
				}
		}
	}
	else
	{
		_txtFAcc->setText(tr("STR_ACCURACY_SHORT").arg(selUnit->getBattleStats()->firing));
		_txtReact->setText(tr("STR_REACTIONS_SHORT").arg(selUnit->getBattleStats()->reactions));
		_txtThrow->setText(tr("STR_THROWACC_SHORT").arg(selUnit->getBattleStats()->throwing));
		_txtMelee->setText(tr("STR_MELEEACC_SHORT").arg(selUnit->getBattleStats()->melee));

		switch (psiSkill)
		{
			case 0:
				_txtPStr->setText(L"");
				_txtPSkill->setText(L"");
				break;

			default:
				_txtPStr->setText(tr("STR_PSIONIC_STRENGTH_SHORT").arg(selUnit->getBattleStats()->psiStrength));
				_txtPSkill->setText(tr("STR_PSIONIC_SKILL_SHORT").arg(psiSkill));
		}
	}
}

/**
 * Shows woundage values.
 */
void InventoryState::updateWounds() // private.
{
	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());

	unsigned wound (static_cast<unsigned>(selUnit->getFireUnit()));
	_numFire->setValue(wound);
	_numFire->setVisible(wound != 0u);

	UnitBodyPart bodyPart;
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
				_numHead->setValue(wound);
				_numHead->setVisible(wound != 0u);
				break;

			case BODYPART_TORSO:
				_numTorso->setValue(wound);
				_numTorso->setVisible(wound != 0u);
				break;

			case BODYPART_RIGHTARM:
				_numRightArm->setValue(wound);
				_numRightArm->setVisible(wound != 0u);
				break;

			case BODYPART_LEFTARM:
				_numLeftArm->setValue(wound);
				_numLeftArm->setVisible(wound != 0u);
				break;

			case BODYPART_RIGHTLEG:
				_numRightLeg->setValue(wound);
				_numRightLeg->setVisible(wound != 0u);
				break;

			case BODYPART_LEFTLEG:
				_numLeftLeg->setValue(wound);
				_numLeftLeg->setVisible(wound != 0u);
		}
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnOkClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
	{
		_game->popState();

		if (_parent != nullptr && _tuMode == false) // pre-battle but going into tactical!
		{
			_battleSave->resetUnitsOnTiles();

			Tile* const inTile (_battleSave->getBattleInventory());
			_battleSave->distributeEquipt(inTile);

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
					(*i)->prepUnit(true);
			}
		}
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
			_parent->selectNextPlayerUnit(false, false, true);
		else
			_battleSave->selectNextFactionUnit(false, false, true);

		init();
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
			_parent->selectPreviousPlayerUnit(false, false, true);
		else
			_battleSave->selectPreviousFactionUnit(false, false, true);

		init();
	}
}

/**
 * Unloads the selected weapon or saves a Soldier's equipment-layout.
 * @param action - pointer to an Action
 */
void InventoryState::btnLoadIconClick(Action*)
{
	if (_inventoryPanel->unload() == true)
	{
		_txtItem->setText(L"");
		_txtAmmo->setText(L"");
		_txtUseTU->setText(L"");

		_srfLoad->clear();

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
 * Saves a Soldier's equipment-layout.
 * @note Called from btnLoadIconClick() if in pre-battle.
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

		// NOTE: When using getInventory() the loaded ammos are skipped because
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
 * Shifts ground-items.
 * @param action - pointer to an Action
 */
void InventoryState::btnGroundPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_WHEELDOWN:
			_inventoryPanel->arrangeGround(+1);
			break;
		case SDL_BUTTON_WHEELUP:
			_inventoryPanel->arrangeGround(-1);
	}
	_inventoryPanel->drawItems();
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

	_inventoryPanel->drawItems();
}

/**
 * Clears the current unit's inventory and places all items on the ground.
 * @param action - pointer to an Action
 */
void InventoryState::btnClearUnitClick(Action*)
{
	if (_tuMode == false									// don't accept clicks in battlescape because this doesn't cost TU.
		&& _inventoryPanel->getSelectedItem() == nullptr)	// or if mouse is holding an item
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		std::vector<BattleItem*>* const equiptList (unit->getInventory());
		if (equiptList->empty() == false)
		{
			_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();

			const RuleInventory* const grdRule (_game->getRuleset()->getInventoryRule(ST_GROUND));
			Tile* const tile (unit->getUnitTile());
			for (std::vector<BattleItem*>::const_iterator
					i = equiptList->begin();
					i != equiptList->end();
					++i)
			{
				(*i)->setFuse(-1);
				(*i)->setOwner();
				(*i)->setInventorySection(grdRule);
				tile->addItem(*i);
			}
			equiptList->clear();

			_inventoryPanel->arrangeGround();
			_inventoryPanel->drawItems();

			updateStats();
			_battleSave->getBattleState()->refreshMousePosition();
		}
	}
}

/**
 * Clears the ground-tile inventory and returns the items to base-stores.
 * @note This works for Craft-equip only, for Base-equip it would be irrelevant.
 * @param action - pointer to an Action
 */
void InventoryState::btnClearGroundClick(Action*)
{
	if (_parent == nullptr									// don't accept clicks during tactical or pre-tactical
		&& _inventoryPanel->getSelectedItem() == nullptr)	// or if mouse-grab has an item
	{
		Craft* craft (nullptr);	// find a Craft and later its Base_

		const std::vector<Base*>* const baseList (_battleSave->getSavedGame()->getBases());
		for (std::vector<Base*>::const_iterator
				i = baseList->begin();
				i != baseList->end() && craft == nullptr;
				++i)
		{
			if ((*i)->getTactical() == true) return; // if a Base is in tactical than any Craft isn't.

			for (std::vector<Craft*>::const_iterator
					j = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end() && craft == nullptr;
					++j)
			{
				if ((*j)->getTactical() == true)
					craft = *j;
			}
		}

		if (craft != nullptr) // safety, not base-equip screen.
		{
			Tile* const tile (_battleSave->getSelectedUnit()->getUnitTile());
			std::vector<BattleItem*>* const grdList (tile->getInventory());
			if (grdList->empty() == false)
			{
				_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();

				Base* const base (craft->getBase());
				const bool isQuickBattle (_game->getSavedGame()->getMonthsPassed() == -1);

				BattleItem* load;

				std::string type;
				for (std::vector<BattleItem*>::const_iterator
						i = grdList->begin();
						i != grdList->end();
						++i)
				{
					if ((*i)->selfPowered() == false
						&& (load = (*i)->getAmmoItem()) != nullptr)
					{
						type = load->getRules()->getType();
						craft->getCraftItems()->removeItem(type);

						if (isQuickBattle == false)
							base->getStorageItems()->addItem(type);
					}

					type = (*i)->getRules()->getType();
					craft->getCraftItems()->removeItem(type);

					if (isQuickBattle == false)
						base->getStorageItems()->addItem(type);
				}
				grdList->clear();
				// NOTE: There's no need to adjust variables on those BattleItems.
				// The SavedBattleGame itself is about to go ~poof~

				_inventoryPanel->arrangeGround();
				_inventoryPanel->drawItems();
			}
		}
	}
}

/**
 * Shows the unit-info screen.
 * @param action - pointer to an Action
 */
void InventoryState::btnRankClick(Action*)
{
	if (_inventoryPanel->getSelectedItem() == nullptr)
	{
		_game->pushState(new UnitInfoState(
										_battleSave->getSelectedUnit(),
										_parent,
										true, false,
										_tuMode == false));
		_game->getScreen()->fadeScreen();
	}
}

/**
 * Updates item-info.
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
						_srfLoad->drawRect(
										&rect,
										static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")
											->getElement("grid")->color));

						++rect.x;
						++rect.y;
						rect.w = static_cast<Uint16>(rect.w - 2u);
						rect.h = static_cast<Uint16>(rect.h - 2u);
						_srfLoad->drawRect(&rect, 15u);

						aItem->getRules()->drawHandSprite(
													_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
													_srfLoad);
					}
				}
		}
		_txtAmmo->setText(wst);
	}

	updateStats();
}

/**
 * Shows item-info.
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
		_srfLoad->clear();

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
							_srfLoad->drawRect(
											&rect,
											static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")
												->getElement("grid")->color));

							++rect.x;
							++rect.y;
							rect.w = static_cast<Uint16>(rect.w - 2u);
							rect.h = static_cast<Uint16>(rect.h - 2u);
							_srfLoad->drawRect(&rect, 15u);

							aItem->getRules()->drawHandSprite(
														_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
														_srfLoad);
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
 * Hides item-info.
 * @param action - pointer to an Action
 */
void InventoryState::inMouseOut(Action*)
{
	_numTuCost->setVisible(false);

	_txtItem->setText(L"");
	_txtAmmo->setText(L"");
	_txtUseTU->setText(L"");

	_srfLoad->clear();
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
	bool isArt;
	std::wostringstream label;

	const RuleItem* const itRule (selOver->getRules());
	if (itRule->getBattleType() == BT_CORPSE)
	{
		isArt = false;

		const BattleUnit* const corpseUnit (selOver->getItemUnit());
		if (corpseUnit != nullptr)
		{
			if (corpseUnit->getType().compare(0u,11u, "STR_FLOATER") == 0 // special handling for Floaters.
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
		isArt = true;
		label << tr("STR_ALIEN_ARTIFACT");
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

		_btnCreateTemplate->altSurface();
		_btnApplyTemplate->altSurface();
		_btnClearInventory->altSurface();
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


	_inventoryPanel->arrangeGround(); // update ui
	updateStats();
	refreshMouse();
} */
