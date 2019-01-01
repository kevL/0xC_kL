/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "SoldierInfoDeadState.h"

//#include <algorithm>
//#include <sstream>

#include "SoldierDiaryOverviewState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldierInfoDeadState screen.
 * @param solId - ID of the current Soldier
 */
SoldierInfoDeadState::SoldierInfoDeadState(size_t solId)
	:
		_solId(solId),
		_sol(nullptr)
{
	_listDead = _game->getSavedGame()->getDeadSoldiers();

	_bg				= new Surface();

	_rank			= new Surface(26, 23,   4, 4);
	_gender			= new Surface( 7,  7, 240, 8);

	_txtSoldier		= new Text(179, 16, 40, 9);
	_btnDiary		= new TextButton(60, 16, 248, 8);

	_btnPrev		= new TextButton(29, 16,  0, 32);
	_btnOk			= new TextButton(49, 16, 30, 32);
	_btnNext		= new TextButton(29, 16, 80, 32);

	_txtDeath		= new Text(60, 9, 130, 36);
	_txtDate		= new Text(80, 9, 196, 36);

	_txtRank		= new Text(130, 9,   0, 49);
	_txtMissions	= new Text(100, 9, 130, 49);
	_txtKills		= new Text(100, 9, 230, 49);

	static const int STEP_y (11);
	int yPos (80);

	_txtTimeUnits		= new Text(120, 9,   6, yPos);
	_txtTimeUnits_i		= new Text( 18, 9, 131, yPos);
	_barTimeUnits		= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtStamina			= new Text(120, 9,   5, yPos);
	_txtEnergy_i		= new Text( 18, 9, 131, yPos);
	_barStamina			= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtHealth			= new Text(120, 9,   6, yPos);
	_txtHealth_i		= new Text( 18, 9, 131, yPos);
	_barHealth			= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtBravery			= new Text(120, 9,   6, yPos);
	_txtBravery_i		= new Text( 18, 9, 131, yPos);
	_barBravery			= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtReactions		= new Text(120, 9,   6, yPos);
	_txtReactions_i		= new Text( 18, 9, 131, yPos);
	_barReactions		= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtFiring			= new Text(120, 9,   6, yPos);
	_txtFiring_i		= new Text( 18, 9, 131, yPos);
	_barFiring			= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtThrowing		= new Text(120, 9,   6, yPos);
	_txtThrowing_i		= new Text( 18, 9, 131, yPos);
	_barThrowing		= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtMelee			= new Text(120, 9,   6, yPos);
	_txtMelee_i			= new Text( 18, 9, 131, yPos);
	_barMelee			= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtStrength		= new Text(120, 9,   6, yPos);
	_txtStrength_i		= new Text( 18, 9, 131, yPos);
	_barStrength		= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtPsiStrength		= new Text(120, 9,   6, yPos);
	_txtPsiStrength_i	= new Text( 18, 9, 131, yPos);
	_barPsiStrength		= new Bar( 234, 7, 150, yPos + 1);

	yPos += STEP_y;
	_txtPsiSkill		= new Text(120, 9,   6, yPos);
	_txtPsiSkill_i		= new Text( 18, 9, 131, yPos);
	_barPsiSkill		= new Bar( 234, 7, 150, yPos + 1);

	setPalette(PAL_BASESCAPE);

	add(_bg);
	add(_rank);
	add(_gender);
	add(_btnOk,				"button",			"soldierInfo");
	add(_btnPrev,			"button",			"soldierInfo");
	add(_btnNext,			"button",			"soldierInfo");

	add(_txtSoldier,		"text1",			"soldierInfo");
	add(_txtRank,			"text1",			"soldierInfo");
	add(_txtMissions,		"text1",			"soldierInfo");
	add(_txtKills,			"text1",			"soldierInfo");
	add(_txtDeath,			"text1",			"soldierInfo");
	add(_txtDate); // text1->color2

	add(_btnDiary,			"button",			"soldierInfo");

	add(_txtTimeUnits,		"text2",			"soldierInfo");
	add(_txtTimeUnits_i,	"numbers",			"soldierInfo");
	add(_barTimeUnits,		"barTUs",			"soldierInfo");

	add(_txtStamina,		"text2",			"soldierInfo");
	add(_txtEnergy_i,		"numbers",			"soldierInfo");
	add(_barStamina,		"barEnergy",		"soldierInfo");

	add(_txtHealth,			"text2",			"soldierInfo");
	add(_txtHealth_i,		"numbers",			"soldierInfo");
	add(_barHealth,			"barHealth",		"soldierInfo");

	add(_txtBravery,		"text2",			"soldierInfo");
	add(_txtBravery_i,		"numbers",			"soldierInfo");
	add(_barBravery,		"barBravery",		"soldierInfo");

	add(_txtReactions,		"text2",			"soldierInfo");
	add(_txtReactions_i,	"numbers",			"soldierInfo");
	add(_barReactions,		"barReactions",		"soldierInfo");

	add(_txtFiring,			"text2",			"soldierInfo");
	add(_txtFiring_i,		"numbers",			"soldierInfo");
	add(_barFiring,			"barFiring",		"soldierInfo");

	add(_txtThrowing,		"text2",			"soldierInfo");
	add(_txtThrowing_i,		"numbers",			"soldierInfo");
	add(_barThrowing,		"barThrowing",		"soldierInfo");

	add(_txtMelee,			"text2",			"soldierInfo");
	add(_txtMelee_i,		"numbers",			"soldierInfo");
	add(_barMelee,			"barMelee",			"soldierInfo");

	add(_txtStrength,		"text2",			"soldierInfo");
	add(_txtStrength_i,		"numbers",			"soldierInfo");
	add(_barStrength,		"barStrength",		"soldierInfo");

	add(_txtPsiStrength,	"text2",			"soldierInfo");
	add(_txtPsiStrength_i,	"numbers",			"soldierInfo");
	add(_barPsiStrength,	"barPsiStrength",	"soldierInfo");

	add(_txtPsiSkill,		"text2",			"soldierInfo");
	add(_txtPsiSkill_i,		"numbers",			"soldierInfo");
	add(_barPsiSkill,		"barPsiSkill",		"soldierInfo");

	centerSurfaces();


	_game->getResourcePack()->getSurface("BACK06.SCR")->blit(_bg);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldierInfoDeadState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierInfoDeadState::btnOkClick),
							Options::keyCancel);

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick(		static_cast<ActionHandler>(&SoldierInfoDeadState::btnPrevClick));
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&SoldierInfoDeadState::btnPrevClick),
								Options::keyBattlePrevUnit);

	_btnNext->setText(L">");
	_btnNext->onMouseClick(		static_cast<ActionHandler>(&SoldierInfoDeadState::btnNextClick));
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&SoldierInfoDeadState::btnNextClick),
								Options::keyBattleNextUnit);


	_txtSoldier->setBig();

	_txtDeath->setText(tr("STR_DATE_DEATH"));
	_txtDate->setColor(208u); // <- text1->color2

	_btnDiary->setText(tr("STR_DIARY"));
	_btnDiary->onMouseClick(static_cast<ActionHandler>(&SoldierInfoDeadState::btnDiaryClick));


	_txtTimeUnits->		setText(tr("STR_TIME_UNITS"));
	_txtStamina->		setText(tr("STR_STAMINA"));
	_txtHealth->		setText(tr("STR_HEALTH"));
	_txtBravery->		setText(tr("STR_BRAVERY"));
	_txtReactions->		setText(tr("STR_REACTIONS"));
	_txtFiring->		setText(tr("STR_FIRING_ACCURACY"));
	_txtThrowing->		setText(tr("STR_THROWING_ACCURACY"));
	_txtMelee->			setText(tr("STR_MELEE_ACCURACY"));
	_txtStrength->		setText(tr("STR_STRENGTH"));
	_txtPsiStrength->	setText(tr("STR_PSIONIC_STRENGTH"));
	_txtPsiSkill->		setText(tr("STR_PSIONIC_SKILL"));
}

/**
 * dTor.
 */
SoldierInfoDeadState::~SoldierInfoDeadState()
{}

/**
 * Updates the displayed stats when the current dead Soldier changes.
 */
void SoldierInfoDeadState::init()
{
	State::init();

	if (_listDead->empty() == true)
	{
		_game->popState();
		return;
	}

	if (_solId >= _listDead->size())
		_solId = 0;

	_sol = _listDead->at(_solId);
	_txtSoldier->setText(_sol->getLabel());

	SurfaceSet* const baseBits (_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	baseBits->getFrame(_sol->getRankSprite())->setX(0);
	baseBits->getFrame(_sol->getRankSprite())->setY(0);
	baseBits->getFrame(_sol->getRankSprite())->blit(_rank);

	_gender->clear();
	Surface* gender;
	switch (_sol->getGender())
	{
		default:
		case GENDER_MALE:
			gender = _game->getResourcePack()->getSurface("GENDER_M");
			break;
		case GENDER_FEMALE:
			gender = _game->getResourcePack()->getSurface("GENDER_F");
	}
	if (gender != nullptr) gender->blit(_gender);

	const SoldierDeath* const death (_sol->getDeath());
	std::wostringstream date;
	date << death->getTime()->getDayString(_game->getLanguage());
	date << L" ";
	date << tr(death->getTime()->getMonthString());
	date << L" ";
	date << death->getTime()->getYear();
	_txtDate->setText(date.str());


	const UnitStats
		* const initial (_sol->getInitStats()),
		* const current (_sol->getCurrentStats());

	std::wostringstream woststr;

	woststr << current->tu;
	_txtTimeUnits_i->setText(woststr.str());
	if (current->tu > initial->tu)
	{
		_barTimeUnits->setMaxValue(current->tu);
		_barTimeUnits->setValue2(initial->tu);
	}
	else
	{
		_barTimeUnits->setMaxValue(initial->tu);
		_barTimeUnits->setValue2(current->tu);
	}
	_barTimeUnits->setValue(current->tu);

	woststr.str(L"");
	woststr << current->stamina;
	_txtEnergy_i->setText(woststr.str());
	if (current->stamina > initial->stamina)
	{
		_barStamina->setMaxValue(current->stamina);
		_barStamina->setValue2(initial->stamina);
	}
	else
	{
		_barStamina->setMaxValue(initial->stamina);
		_barStamina->setValue2(current->stamina);
	}
	_barStamina->setValue(current->stamina);

	woststr.str(L"");
	woststr << current->health;
	_txtHealth_i->setText(woststr.str());
	if (current->health > initial->health)
	{
		_barHealth->setMaxValue(current->health);
		_barHealth->setValue2(initial->health);
	}
	else
	{
		_barHealth->setMaxValue(initial->health);
		_barHealth->setValue2(current->health);
	}
	_barHealth->setValue(current->health);

	woststr.str(L"");
	woststr << current->bravery;
	_txtBravery_i->setText(woststr.str());
	_barBravery->setMaxValue(current->bravery);
	_barBravery->setValue(current->bravery);
	_barBravery->setValue2(initial->bravery);

	woststr.str(L"");
	woststr << current->reactions;
	_txtReactions_i->setText(woststr.str());
	_barReactions->setMaxValue(current->reactions);
	_barReactions->setValue(current->reactions);
	_barReactions->setValue2(initial->reactions);

	woststr.str(L"");
	woststr << current->firing;
	_txtFiring_i->setText(woststr.str());
	_barFiring->setMaxValue(current->firing);
	_barFiring->setValue(current->firing);
	_barFiring->setValue2(initial->firing);

	woststr.str(L"");
	woststr << current->throwing;
	_txtThrowing_i->setText(woststr.str());
	_barThrowing->setMaxValue(current->throwing);
	_barThrowing->setValue(current->throwing);
	_barThrowing->setValue2(initial->throwing);

	woststr.str(L"");
	woststr << current->melee;
	_txtMelee_i->setText(woststr.str());
	_barMelee->setMaxValue(current->melee);
	_barMelee->setValue(current->melee);
	_barMelee->setValue2(initial->melee);

	woststr.str(L"");
	woststr << current->strength;
	_txtStrength_i->setText(woststr.str());
	if (current->strength > initial->strength)
	{
		_barStrength->setMaxValue(current->strength);
		_barStrength->setValue2(initial->strength);
	}
	else
	{
		_barStrength->setMaxValue(initial->strength);
		_barStrength->setValue2(current->strength);
	}
	_barStrength->setValue(current->strength);


	_txtRank->setText(tr("STR_RANK_").arg(tr(_sol->getRankString())));
	_txtMissions->setText(tr("STR_MISSIONS_").arg(_sol->getMissions()));
	_txtKills->setText(tr("STR_KILLS_").arg(_sol->getKills()));

	if (current->psiSkill != 0)
	{
		woststr.str(L"");
		woststr << current->psiStrength;
		_txtPsiStrength_i->setText(woststr.str());
		_barPsiStrength->setMaxValue(current->psiStrength);
		_barPsiStrength->setValue(current->psiStrength);
		_barPsiStrength->setValue2(initial->psiStrength);

		_txtPsiStrength_i->setVisible();
		_barPsiStrength->setVisible();

		woststr.str(L"");
		woststr << current->psiSkill;
		_txtPsiSkill_i->setText(woststr.str());
		_barPsiSkill->setMaxValue(current->psiSkill);
		_barPsiSkill->setValue(current->psiSkill);
		_barPsiSkill->setValue2(initial->psiSkill);

		_txtPsiSkill_i->setVisible();
		_barPsiSkill->setVisible();
	}
	else
	{
		_txtPsiStrength_i->setVisible(false);
		_barPsiStrength->setVisible(false);

		_txtPsiSkill_i->setVisible(false);
		_barPsiSkill->setVisible(false);
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the previous Soldier.
 * @note Reversed these because SoldierMemorialState uses a reversed vector.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnNextClick(Action*)
{
	if (_solId == 0)
		_solId = _listDead->size() - 1;
	else
		--_solId;

	init();
}

/**
 * Goes to the next Soldier.
 * @note Reversed these because SoldierMemorialState uses a reversed vector.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnPrevClick(Action*)
{
	if (++_solId >= _listDead->size())
		_solId = 0;

	init();
}

/**
 * Sets the soldier-ID.
 * @param solId - the ID for the current dead Soldier
 */
void SoldierInfoDeadState::setSoldierId(size_t solId)
{
	_solId = solId;
}

/**
 * Shows the SoldierDiaryOverview screen.
 * @param action - pointer to an Action
 */
void SoldierInfoDeadState::btnDiaryClick(Action*)
{
	_game->pushState(new SoldierDiaryOverviewState(
												nullptr,
												_solId,
												nullptr,
												this));
}

}
