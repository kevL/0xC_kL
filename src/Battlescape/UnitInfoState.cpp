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

#include "UnitInfoState.h"

//#include <sstream>

#include "../fmath.h"

#include "BattlescapeGame.h"
#include "BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the UnitInfo screen.
 * @param unit			- pointer to the selected BattleUnit
 * @param parent		- pointer to parent Battlescape
 * @param fromInventory	- true if player is here from the inventory (default false)
 * @param mindProbe		- true if player is using a Mind Probe (default false)
 * @param preBattle		- true if preEquip state; ie tuMode not tactical (default false)
 */
UnitInfoState::UnitInfoState(
		const BattleUnit* const unit,
		BattlescapeState* const parent,
		const bool fromInventory,
		const bool mindProbe,
		const bool preBattle)
	:
		_unit(unit),
		_parent(parent),
		_fromInventory(fromInventory),
		_mindProbe(mindProbe),
		_preBattle(preBattle),
		_battleSave(_game->getSavedGame()->getBattleSave())
{
//	if (Options::maximizeInfoScreens)
//	{
//		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
//		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
//		_game->getScreen()->resetDisplay(false);
//	}

	_bg		= new Surface(320, 200);
	_exit	= new InteractiveSurface(
//								320, 180, 0, 20);
								Options::baseXResolution,
								Options::baseYResolution,
								-((Options::baseXResolution - 320) >> 1u),
								-((Options::baseYResolution - 200) >> 1u));

	_txtName		= new Text(288, 17, 16, 4);
	_gender			= new Surface(7, 7, 22, 4);
	_battleOrder	= new NumberText(7, 5, 0, 5); // x-value is set in init()

	static const int STEP_y (9);
	int yPos (38);

	_txtTimeUnits		= new Text(140, 9,   8, yPos);
	_txtTimeUnits_i		= new Text( 18, 9, 151, yPos);
	_barTimeUnits		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtEnergy			= new Text(140, 9,   8, yPos);
	_txtEnergy_i		= new Text( 18, 9, 151, yPos);
	_barEnergy			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtHealth			= new Text(140, 9,   8, yPos);
	_txtHealth_i		= new Text( 18, 9, 151, yPos);
	_txtStun_i			= new Text( 18, 9, 127, yPos);
	_barHealth			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtFatals			= new Text(140, 9,   8, yPos);
	_txtFatals_i		= new Text( 18, 9, 151, yPos);
	_barFatals			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtBravery			= new Text(140, 9,   8, yPos);
	_txtBravery_i		= new Text( 18, 9, 151, yPos);
	_barBravery			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtMorale			= new Text(140, 9,   8, yPos);
	_txtMorale_i		= new Text( 18, 9, 151, yPos);
	_barMorale			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtReactions		= new Text(140, 9,   8, yPos);
	_txtReactions_i		= new Text( 18, 9, 151, yPos);
	_barReactions		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtFiring			= new Text(140, 9,   8, yPos);
	_txtFiring_i		= new Text( 18, 9, 151, yPos);
	_barFiring			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtThrowing		= new Text(140, 9,   8, yPos);
	_txtThrowing_i		= new Text( 18, 9, 151, yPos);
	_barThrowing		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtMelee			= new Text(140, 9,   8, yPos);
	_txtMelee_i			= new Text( 18, 9, 151, yPos);
	_barMelee			= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtStrength		= new Text(140, 9,   8, yPos);
	_txtStrength_i		= new Text( 18, 9, 151, yPos);
	_barStrength		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtPsiStrength		= new Text(140, 9,   8, yPos);
	_txtPsiStrength_i	= new Text( 18, 9, 151, yPos);
	_barPsiStrength		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtPsiSkill		= new Text(140, 9,   8, yPos);
	_txtPsiSkill_i		= new Text( 18, 9, 151, yPos);
	_barPsiSkill		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtFrontArmor		= new Text(140, 9,   8, yPos);
	_txtFrontArmor_i	= new Text( 18, 9, 151, yPos);
	_barFrontArmor		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtLeftArmor		= new Text(140, 9,   8, yPos);
	_txtLeftArmor_i		= new Text( 18, 9, 151, yPos);
	_barLeftArmor		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtRightArmor		= new Text(140, 9,   8, yPos);
	_txtRightArmor_i	= new Text( 18, 9, 151, yPos);
	_barRightArmor		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtRearArmor		= new Text(140, 9,   8, yPos);
	_txtRearArmor_i		= new Text( 18, 9, 151, yPos);
	_barRearArmor		= new Bar( 280, 5, 170, yPos + 1);

	yPos += STEP_y;
	_txtUnderArmor		= new Text(140, 9,   8, yPos);
	_txtUnderArmor_i	= new Text( 18, 9, 151, yPos);
	_barUnderArmor		= new Bar( 280, 5, 170, yPos + 1);

	if (_mindProbe == false)
	{
		_btnPrev = new TextButton(18, 18,   2, 2);
		_btnNext = new TextButton(18, 18, 300, 2);
	}

	setPalette(PAL_BATTLESCAPE);

	add(_bg);
	add(_exit);
	add(_txtName,			"textName",			"stats");

	add(_gender);
	add(_battleOrder);

	add(_txtTimeUnits);
	add(_txtTimeUnits_i);
	add(_barTimeUnits,		"barTUs",			"stats");

	add(_txtEnergy);
	add(_txtEnergy_i);
	add(_barEnergy,			"barEnergy",		"stats");

	add(_txtHealth);
	add(_txtHealth_i);
	add(_txtStun_i);
	add(_barHealth,			"barHealth",		"stats");

	add(_txtFatals);
	add(_txtFatals_i);
	add(_barFatals,			"barWounds",		"stats");

	add(_txtBravery);
	add(_txtBravery_i);
	add(_barBravery,		"barBravery",		"stats");

	add(_txtMorale);
	add(_txtMorale_i);
	add(_barMorale,			"barMorale",		"stats");

	add(_txtReactions);
	add(_txtReactions_i);
	add(_barReactions,		"barReactions",		"stats");

	add(_txtFiring);
	add(_txtFiring_i);
	add(_barFiring,			"barFiring",		"stats");

	add(_txtThrowing);
	add(_txtThrowing_i);
	add(_barThrowing,		"barThrowing",		"stats");

	add(_txtMelee);
	add(_txtMelee_i);
	add(_barMelee,			"barMelee",			"stats");

	add(_txtStrength);
	add(_txtStrength_i);
	add(_barStrength,		"barStrength",		"stats");

	add(_txtPsiStrength);
	add(_txtPsiStrength_i);
	add(_barPsiStrength,	"barPsiStrength",	"stats");

	add(_txtPsiSkill);
	add(_txtPsiSkill_i);
	add(_barPsiSkill,		"barPsiSkill",		"stats");

	add(_txtFrontArmor);
	add(_txtFrontArmor_i);
	add(_barFrontArmor,		"barFrontArmor",	"stats");

	add(_txtLeftArmor);
	add(_txtLeftArmor_i);
	add(_barLeftArmor,		"barLeftArmor",		"stats");

	add(_txtRightArmor);
	add(_txtRightArmor_i);
	add(_barRightArmor,		"barRightArmor",	"stats");

	add(_txtRearArmor);
	add(_txtRearArmor_i);
	add(_barRearArmor,		"barRearArmor",		"stats");

	add(_txtUnderArmor);
	add(_txtUnderArmor_i);
	add(_barUnderArmor,		"barUnderArmor",	"stats");

	if (_mindProbe == false)
	{
		add(_btnPrev, "button", "stats");
		add(_btnNext, "button", "stats");
	}

	centerSurfaces();


	_game->getResourcePack()->getSurface("UNIBORD.PCK")->blit(_bg);

	_exit->onMouseClick(	static_cast<ActionHandler>(&UnitInfoState::exitClick),
							SDL_BUTTON_RIGHT);
	_exit->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::exitClick),
							Options::keyCancel);
	_exit->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::exitClick),
							Options::keyOk);
	_exit->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::exitClick),
							Options::keyOkKeypad);
	_exit->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::exitClick),
							Options::keyBattleStats);

	const Element* const el (_game->getRuleset()->getInterface("stats")->getElement("text"));
	Uint8
		color  (static_cast<Uint8>(el->color)),
		color2 (static_cast<Uint8>(el->color2));

	_txtName->setAlign(ALIGN_CENTER);
	_txtName->setBig();
	_txtName->setHighContrast();

	_battleOrder->setColor(WHITE);
	_battleOrder->setVisible(false);

	_txtTimeUnits->setText(tr("STR_TIME_UNITS"));
	_txtTimeUnits->setColor(color);
	_txtTimeUnits->setHighContrast();
	_txtTimeUnits_i->setColor(color2);
	_txtTimeUnits_i->setHighContrast();

	_txtEnergy->setText(tr("STR_ENERGY"));
	_txtEnergy->setColor(color);
	_txtEnergy->setHighContrast();
	_txtEnergy_i->setColor(color2);
	_txtEnergy_i->setHighContrast();

	_txtHealth->setText(tr("STR_HEALTH"));
	_txtHealth->setColor(color);
	_txtHealth->setHighContrast();
	_txtHealth_i->setColor(color2);
	_txtHealth_i->setHighContrast();
	_txtStun_i->setColor(BROWN_L);
	_txtStun_i->setHighContrast();
	_txtStun_i->setAlign(ALIGN_RIGHT);

	_txtFatals->setText(tr("STR_FATAL_WOUNDS"));
	_txtFatals->setColor(color);
	_txtFatals->setHighContrast();
	_txtFatals_i->setColor(color2);
	_txtFatals_i->setHighContrast();

	_txtBravery->setText(tr("STR_BRAVERY"));
	_txtBravery->setColor(color);
	_txtBravery->setHighContrast();
	_txtBravery_i->setColor(color2);
	_txtBravery_i->setHighContrast();

	_txtMorale->setText(tr("STR_MORALE"));
	_txtMorale->setColor(color);
	_txtMorale->setHighContrast();
	_txtMorale_i->setColor(color2);
	_txtMorale_i->setHighContrast();

	_txtReactions->setText(tr("STR_REACTIONS"));
	_txtReactions->setColor(color);
	_txtReactions->setHighContrast();
	_txtReactions_i->setColor(color2);
	_txtReactions_i->setHighContrast();

	_txtFiring->setText(tr("STR_FIRING_ACCURACY"));
	_txtFiring->setColor(color);
	_txtFiring->setHighContrast();
	_txtFiring_i->setColor(color2);
	_txtFiring_i->setHighContrast();

	_txtThrowing->setText(tr("STR_THROWING_ACCURACY"));
	_txtThrowing->setColor(color);
	_txtThrowing->setHighContrast();
	_txtThrowing_i->setColor(color2);
	_txtThrowing_i->setHighContrast();

	_txtMelee->setText(tr("STR_MELEE_ACCURACY"));
	_txtMelee->setColor(color);
	_txtMelee->setHighContrast();
	_txtMelee_i->setColor(color2);
	_txtMelee_i->setHighContrast();

	_txtStrength->setText(tr("STR_STRENGTH"));
	_txtStrength->setColor(color);
	_txtStrength->setHighContrast();
	_txtStrength_i->setColor(color2);
	_txtStrength_i->setHighContrast();

	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH"));
	_txtPsiStrength->setColor(color);
	_txtPsiStrength->setHighContrast();
	_txtPsiStrength_i->setColor(color2);
	_txtPsiStrength_i->setHighContrast();

	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL"));
	_txtPsiSkill->setColor(color);
	_txtPsiSkill->setHighContrast();
	_txtPsiSkill_i->setColor(color2);
	_txtPsiSkill_i->setHighContrast();

	_txtFrontArmor->setText(tr("STR_FRONT_ARMOR_UC"));
	_txtFrontArmor->setColor(color);
	_txtFrontArmor->setHighContrast();
	_txtFrontArmor_i->setColor(color2);
	_txtFrontArmor_i->setHighContrast();

	_txtLeftArmor->setText(tr("STR_LEFT_ARMOR_UC"));
	_txtLeftArmor->setColor(color);
	_txtLeftArmor->setHighContrast();
	_txtLeftArmor_i->setColor(color2);
	_txtLeftArmor_i->setHighContrast();

	_txtRightArmor->setText(tr("STR_RIGHT_ARMOR_UC"));
	_txtRightArmor->setColor(color);
	_txtRightArmor->setHighContrast();
	_txtRightArmor_i->setColor(color2);
	_txtRightArmor_i->setHighContrast();

	_txtRearArmor->setText(tr("STR_REAR_ARMOR_UC"));
	_txtRearArmor->setColor(color);
	_txtRearArmor->setHighContrast();
	_txtRearArmor_i->setColor(color2);
	_txtRearArmor_i->setHighContrast();

	_txtUnderArmor->setText(tr("STR_UNDER_ARMOR_UC"));
	_txtUnderArmor->setColor(color);
	_txtUnderArmor->setHighContrast();
	_txtUnderArmor_i->setColor(color2);
	_txtUnderArmor_i->setHighContrast();

	if (_mindProbe == false)
	{
		_btnPrev->setText(L"<");
		_btnPrev->onMouseClick(		static_cast<ActionHandler>(&UnitInfoState::btnPrevClick));
		_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnPrevClick),
									Options::keyBattlePrevUnit);
		_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnPrevClick),
									SDLK_LEFT);
		_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnPrevClick),
									SDLK_KP4);

		_btnNext->setText(L">");
		_btnNext->onMouseClick(		static_cast<ActionHandler>(&UnitInfoState::btnNextClick));
		_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnNextClick),
									Options::keyBattleNextUnit);
		_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnNextClick),
									SDLK_RIGHT);
		_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&UnitInfoState::btnNextClick),
									SDLK_KP6);

//		_timer = new Timer(300u);
//		_timer->onTimer((StateHandler)& UnitInfoState::keyRepeat);
//		_timer->start();
	}
}

/**
 * dTor.
 */
UnitInfoState::~UnitInfoState()
{
//	delete _timer;
}

/**
 * Hits the think-timer.
 *
void UnitInfoState::think()
{
	if (_mindProbe == false)
	{
		State::think();
		_timer->think(this, nullptr);
	}
} */

/**
 * Advances to the next/previous Unit when right/left key is depressed.
 *
void UnitInfoState::keyRepeat() // private.
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
 * Updates unit-info which changes when advancing or regressing through units.
 */
void UnitInfoState::init()
{
	State::init();
	_gender->clear();

	std::wostringstream woststr;

	// Sprites & Order ->
	const Soldier* const sol (_unit->getGeoscapeSoldier());
	if (sol != nullptr)
	{
		woststr << tr(_unit->getRankString())
				<< L" ";

		Surface* gender;
		switch (sol->getGender())
		{
			default:
			case GENDER_MALE:
				gender = _game->getResourcePack()->getSurface("GENDER_M");
				break;

			case GENDER_FEMALE:
				gender = _game->getResourcePack()->getSurface("GENDER_F");
		}
		gender->blit(_gender);

		if (_mindProbe == false)
		{
			const unsigned order (_unit->getBattleOrder());
			if (order < 10u)
				_battleOrder->setX(_btnNext->getX() - 6);
			else
				_battleOrder->setX(_btnNext->getX() - 10);

			_battleOrder->setValue(order);
			_battleOrder->setVisible();
		}
		else
			_battleOrder->setVisible(false);
	}
	else
		_battleOrder->setVisible(false);

	// NAME ->
	woststr << _unit->getLabel(
						_game->getLanguage(),
						_battleSave->getDebugTac() == true); //BattlescapeGame::_debugPlay == true);
	_txtName->setText(woststr.str());
	_txtName->setBig();

	// TU, Health, & Bravery ->
	int stat (_unit->getTu());
	_txtTimeUnits_i->setText(Text::intWide(stat));
	_barTimeUnits->setMaxValue(static_cast<double>(_unit->getBattleStats()->tu));
	_barTimeUnits->setValue(static_cast<double>(stat));

	stat = _unit->getEnergy();
	_txtEnergy_i->setText(Text::intWide(stat));
	_barEnergy->setMaxValue(static_cast<double>(_unit->getBattleStats()->stamina));
	_barEnergy->setValue(static_cast<double>(stat));

	stat = _unit->getHealth();
	_txtHealth_i->setText(Text::intWide(stat));
	const int stat2 (_unit->getStun());
	if (stat2 != 0)
	{
		_txtStun_i->setText(Text::intWide(stat2));
		_txtStun_i->setVisible();
	}
	else
		_txtStun_i->setVisible(false);
	_barHealth->setMaxValue(static_cast<double>(_unit->getBattleStats()->health));
	_barHealth->setValue(static_cast<double>(stat));
	_barHealth->setValue2(static_cast<double>(stat2));

	woststr.str(L"");
	if (_unit->isWoundable() == true)
	{
		stat = _unit->getFatalsTotal();
		woststr << stat;

		_barFatals->setMaxValue(static_cast<double>(stat));
		_barFatals->setValue(static_cast<double>(stat));
		_barFatals->setVisible();
	}
	else
	{
		woststr << L"-";
		_barFatals->setVisible(false);
	}
	_txtFatals_i->setText(woststr.str());

	woststr.str(L"");
	stat = _unit->getBattleStats()->bravery;
	if (stat != 110)
	{
		woststr << stat;
		_barBravery->setMaxValue(static_cast<double>(stat));
		_barBravery->setValue(static_cast<double>(stat));
		_barBravery->setVisible();
	}
	else
	{
		woststr << L"oo";
		_barBravery->setVisible(false);
	}
	_txtBravery_i->setText(woststr.str());

	woststr.str(L"");
	if (_unit->isMoralable() == true)
	{
		stat = _unit->getMorale();
		woststr << stat;

		_barMorale->setValue(static_cast<double>(stat));
		_barMorale->setVisible();
	}
	else
	{
		woststr << L"oo";
		_barMorale->setVisible(false);
	}
	_txtMorale_i->setText(woststr.str());

	// Primary Abilities ->
	const double acuModi (_unit->getAccuracyModifier());

	double arbitraryVariable (static_cast<double>(_unit->getBattleStats()->reactions));
	_barReactions->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barReactions->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_txtReactions_i->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBattleStats()->firing);
	_barFiring->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barFiring->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_txtFiring_i->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBattleStats()->throwing);
	_barThrowing->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barThrowing->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_txtThrowing_i->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBattleStats()->melee);
	_barMelee->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi;
	_barMelee->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_txtMelee_i->setText(Text::intWide(stat));

	arbitraryVariable = static_cast<double>(_unit->getBattleStats()->strength);
	_barStrength->setMaxValue(arbitraryVariable);
	arbitraryVariable *= acuModi / 2. + 0.5;
	_barStrength->setValue(arbitraryVariable);
	stat = static_cast<int>(Round(arbitraryVariable));
	_txtStrength_i->setText(Text::intWide(stat));

	// Psionics ->
	const int psiSkill (_unit->getBattleStats()->psiSkill);
	if (psiSkill != 0 || sol == nullptr)
	{
		woststr.str(L"");
		if (_unit->isMoralable() == true)
		{
			stat = _unit->getBattleStats()->psiStrength;
			woststr << stat;
			_barPsiStrength->setMaxValue(static_cast<double>(stat));
			_barPsiStrength->setValue(static_cast<double>(stat));
			_barPsiStrength->setVisible();
		}
		else
		{
			woststr << L"oo";
			_barPsiStrength->setVisible(false);
		}
		_txtPsiStrength_i->setText(woststr.str());
		_txtPsiStrength_i->setVisible();

		if (psiSkill != 0)
		{
			_txtPsiSkill_i->setText(Text::intWide(psiSkill));
			_barPsiSkill->setMaxValue(static_cast<double>(psiSkill));
			_barPsiSkill->setValue(static_cast<double>(psiSkill));

			_txtPsiSkill_i->setVisible();
			_barPsiSkill->setVisible();
		}
		else
		{
			_txtPsiSkill_i->setVisible(false);
			_barPsiSkill->setVisible(false);
		}
	}
	else // Soldier w/out psiSkill
	{
//		_barPsiSkill->setMaxValue(0.); // TODO: revert those class defaults.
		_txtPsiSkill_i->setVisible(false);
		_barPsiSkill->setVisible(false);
		_txtPsiStrength_i->setVisible(false);
		_barPsiStrength->setVisible(false);
	}

	// Armor ->
	stat = _unit->getArmor(SIDE_FRONT);
	_txtFrontArmor_i->setText(Text::intWide(stat));
	_barFrontArmor->setMaxValue(static_cast<double>(_unit->getArmor()->getFrontArmor()));
	_barFrontArmor->setValue(static_cast<double>(stat));

	arbitraryVariable = _unit->getArmor()->getSideArmor(); // haha!!
	stat = _unit->getArmor(SIDE_LEFT);
	_txtLeftArmor_i->setText(Text::intWide(stat));
	_barLeftArmor->setMaxValue(arbitraryVariable);
	_barLeftArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_RIGHT);
	_txtRightArmor_i->setText(Text::intWide(stat));
	_barRightArmor->setMaxValue(arbitraryVariable);
	_barRightArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_REAR);
	_txtRearArmor_i->setText(Text::intWide(stat));
	_barRearArmor->setMaxValue(_unit->getArmor()->getRearArmor());
	_barRearArmor->setValue(static_cast<double>(stat));

	stat = _unit->getArmor(SIDE_UNDER);
	_txtUnderArmor_i->setText(Text::intWide(stat));
	_barUnderArmor->setMaxValue(_unit->getArmor()->getUnderArmor());
	_barUnderArmor->setValue(static_cast<double>(stat));
}

/**
 * Closes the window on right-click.
 * @note OReally.
 * @param action - pointer to an Action
 */
void UnitInfoState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN // Might not be needed, cf. ScannerState::handle().
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		exitClick();
	}
//	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
//	{
//		if (_mindProbe == false)
//		{
//			if (action->getDetails()->button.button == SDL_BUTTON_X1)
//				btnNextClick(action);
//			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
//				btnPrevClick(action);
//		}
//	}
}

/**
 * Selects the next eligible unit.
 * @param action - pointer to an Action
 */
void UnitInfoState::btnNextClick(Action*)
{
	if (_parent != nullptr)				// this is from a Battlescape Game
		_unit = _parent->selectNextPlayerUnit();
	else								// this is from the Craft Equipment screen
		_unit = _battleSave->selectNextUnit(false, false, true); // no tanks.

	if (_unit != nullptr)
		init();
	else
		exitClick();
}

/**
 * Selects the previous eligible unit.
 * @param action - pointer to an Action
 */
void UnitInfoState::btnPrevClick(Action*)
{
	if (_parent != nullptr)					// so you are here from a Battlescape Game -> no, I'm here from Picadilly.
		_unit = _parent->selectPreviousPlayerUnit();
	else									// so you are here from the Craft Equipment screen
		_unit = _battleSave->selectPrevUnit(false, false, true); // no tanks.

	if (_unit != nullptr)
		init();
	else
		exitClick();
}

/**
 * Exits the state.
 * @param action - pointer to an Action (default nullptr)
 */
void UnitInfoState::exitClick(Action*) // private.
{
	_game->getScreen()->fadeScreen();

	if (_mindProbe == true)
	{
		const BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (unit->getTu() < unit->getActionTu(
											BA_USE,
											_game->getRuleset()->getItemRule("STR_MIND_PROBE")))
		{
			_battleSave->getBattleGame()->cancelTacticalAction();
		}
	}

//	if (_fromInventory == false)
//	{
//		if (Options::maximizeInfoScreens)
//		{
//			Screen::updateScale(Options::battlescapeScale, Options::battlescapeScale, Options::baseXBattlescape, Options::baseYBattlescape, true);
//			_game->getScreen()->resetDisplay(false);
//		}
//	} else
	if (_mindProbe == true || _unit->canInventory() == true)
		_game->popState();
	else if (_preBattle == false)
	{
		if (_fromInventory == true)
			_game->popState();

		_game->popState();
	}
}

}
