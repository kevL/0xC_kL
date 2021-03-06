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

#include "MedikitState.h"

#include <cmath>
//#include <sstream>

#include "BattlescapeState.h"	// control Civies
#include "Camera.h"				// control Civies
#include "Map.h"				// control Civies
#include "MedikitView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/SavedBattleGame.h"
//#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Helper function that returns a string representation of a type.
 * @note Used mostly for numbers.
 * @param val - the value to stringify
 * @return, a string representation of the value
 */
template<typename T>
std::wstring toString(T val)
{
	std::wostringstream woststr;
	woststr << val;
	return woststr.str();
}


/**
 * Helper struct for the Medikit.
 */
struct MedikitTitle
	:
		public Text
{
	/// Creates a MedikitTitle.
	MedikitTitle(
			int y,
			const std::wstring& title);
};

/**
 * Initializes a MedikitTitle.
 * @param y		- the title's y-origin
 * @param title	- reference to the title
 */
MedikitTitle::MedikitTitle(
		int y,
		const std::wstring& title)
	:
		Text(73,9, 186,y)
{
	this->setText(title);
	this->setHighContrast();
	this->setAlign(ALIGN_CENTER);
}

/**
 * Helper struct for the Medikit.
 */
struct MedikitText
	:
		public Text
{
	/// Creates a MedikitText.
	explicit MedikitText(int y);
};

/**
 * Initializes a MedikitText.
 * @param y - the text's y-origin
 */
MedikitText::MedikitText(int y)
	:
		Text(33,17, 220,y)
{
	// NOTE: can't set setBig() here. The needed font is only set when added to State
	this->setColor(16); // orange
	this->setHighContrast();
	this->setAlign(ALIGN_CENTER);
}

/**
 * Helper struct for the Medikit.
 */
struct MedikitButton
	:
		public InteractiveSurface
{
	/// Creates a MedikitButton.
	explicit MedikitButton(int y);
};

/**
 * Initializes a MedikitButton.
 * @param y - the button's y-origin
 */
MedikitButton::MedikitButton(int y)
	:
		InteractiveSurface(25,21, 192,y)
{}


/**
 * Initializes the Medikit State.
 * @param action - pointer to BattleAction (BattlescapeGame.h)
 */
MedikitState::MedikitState(BattleAction* const action)
	:
		_action(action)
{
/*	if (Options::maximizeInfoScreens)
	{
		Options::baseXResolution = Screen::ORIGINAL_WIDTH;
		Options::baseYResolution = Screen::ORIGINAL_HEIGHT;
		_game->getScreen()->resetDisplay(false);
	} */

	_bg = new Surface(
					Options::baseXResolution,
					Options::baseYResolution);

	setPalette(PAL_BATTLESCAPE);

	if (_game->getScreen()->getDY() > 50)
	{
		_fullScreen = false;
		_bg->drawRect(67, 44, 190, 100, LIME);
	}

	_txtPart	= new Text(16, 9,  89, 120);
	_txtWound	= new Text(16, 9, 145, 120);
	_mediView	= new MedikitView(
								52, 58, 95, 60,
								_game,
								_action->targetUnit,
								_txtPart,
								_txtWound);

	_btnPain	= new MedikitButton(48);
	_btnStim	= new MedikitButton(84);
	_btnHeal	= new MedikitButton(120);
	_btnClose	= new InteractiveSurface(7, 7, 222, 148);

	_txtPain	= new MedikitText(51);
	_txtStim	= new MedikitText(87);
	_txtHeal	= new MedikitText(123);

	_numHealth		= new NumberText(15, 5,  90, 8);
	_numTotalHP		= new NumberText(15, 5, 225, 8);
	_numStun		= new NumberText(15, 5, 105, 8);
	_barHealth		= new Bar(300, 5, 120, 8);

	_numEnergy		= new NumberText(15, 5, 90, 15);
	_barEnergy		= new Bar(102, 3, 120, 16);

	_numMorale		= new NumberText(15, 5, 90, 22);
	_barMorale		= new Bar(102, 3, 120, 23);

	_numTimeUnits	= new NumberText(15, 5, 90, 164);
	_barTimeUnits	= new Bar(102, 3, 120, 165);

	_txtUnit		= new Text(160, 9, 100, 171);

	add(_numHealth);
	add(_numStun,		"numStun",		"battlescape");
	add(_numEnergy,		"numEnergy",	"battlescape");
	add(_numMorale,		"numMorale",	"battlescape");
	add(_numTimeUnits,	"numTUs",		"battlescape");
	add(_barHealth,		"barHealth",	"battlescape");
	add(_barEnergy,		"barEnergy",	"battlescape");
	add(_barMorale,		"barMorale",	"battlescape");
	add(_barTimeUnits,	"barTUs",		"battlescape");
	add(_numTotalHP); // goes on top of Health (stun) bar.

	_numHealth->setColor(RED);
	_numTotalHP->setColor(RED);
	_numTotalHP->setValue(static_cast<unsigned>(_action->targetUnit->getBattleStats()->health));

//	_barHealth->setScale();
//	_barEnergy->setScale();
//	_barMorale->setScale();
//	_barTimeUnits->setScale();
//	_barHealth->setMaxValue();
//	_barEnergy->setMaxValue();
//	_barMorale->setMaxValue();
//	_barTimeUnits->setMaxValue();

	_barHealth->offsetSecond(-2);


	add(_bg);

	std::wstring wst (tr("STR_PAIN_KILLER"));
	add(new MedikitTitle( 36, wst), "textPK",	"medikit", _bg); // not in Interfaces.rul

	wst = tr("STR_STIMULANT");
	add(new MedikitTitle( 72, wst), "textStim",	"medikit", _bg); // not in Interfaces.rul

	wst = tr("STR_HEAL");
	add(new MedikitTitle(108, wst), "textHeal",	"medikit", _bg); // not in Interfaces.rul

	add(_mediView,	"body",			"medikit", _bg);
	add(_btnHeal,	"buttonHeal",	"medikit", _bg); // not in Interfaces.rul
	add(_btnStim,	"buttonStim",	"medikit", _bg); // not in Interfaces.rul
	add(_btnPain,	"buttonPK",		"medikit", _bg); // not in Interfaces.rul
	add(_txtPain,	"numPain",		"medikit", _bg);
	add(_txtStim,	"numStim",		"medikit", _bg);
	add(_txtHeal,	"numHeal",		"medikit", _bg);
	add(_txtPart,	"textPart",		"medikit", _bg);
	add(_txtWound,	"numWounds",	"medikit", _bg);
	add(_txtUnit);
	add(_btnClose);

	centerSurfaces();


	_game->getResourcePack()->getSurface("MEDIBORD.PCK")->blit(_bg);

	_txtPain->setBig();
	_txtStim->setBig();
	_txtHeal->setBig();

	_txtPart->setHighContrast();
	_txtWound->setHighContrast();

	_txtUnit->setText(_action->targetUnit->getLabel(_game->getLanguage()));
	_txtUnit->setColor(WHITE);
	_txtUnit->setHighContrast();
	_txtUnit->setAlign(ALIGN_RIGHT);

	_btnClose->onMouseClick(	static_cast<ActionHandler>(&MedikitState::closeClick));
	_btnClose->onKeyboardPress(	static_cast<ActionHandler>(&MedikitState::closeClick),
								Options::keyCancel);
	_btnClose->onKeyboardPress(	static_cast<ActionHandler>(&MedikitState::closeClick),
								Options::keyOk);
	_btnClose->onKeyboardPress(	static_cast<ActionHandler>(&MedikitState::closeClick),
								Options::keyOkKeypad);

	_btnHeal->onMouseClick(static_cast<ActionHandler>(&MedikitState::healClick));
	_btnStim->onMouseClick(static_cast<ActionHandler>(&MedikitState::stimClick));
	_btnPain->onMouseClick(static_cast<ActionHandler>(&MedikitState::painClick));

	update();
}

/**
 * Closes the window on right-click.
 * @param action - pointer to an Action
 */
void MedikitState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		closeClick();
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action (default nullptr)
 */
void MedikitState::closeClick(Action*)
{
/*	if (Options::maximizeInfoScreens)
	{
		Screen::updateScale(
						Options::battlescapeScale,
						Options::battlescapeScale,
						Options::baseXBattlescape,
						Options::baseYBattlescape,
						true);
		_game->getScreen()->resetDisplay(false);
	} */

	_game->getSavedGame()->getBattleSave()->getBattleState()->updateMedicIcons();
	_game->popState();
}

/**
 * Handler for clicking the heal button.
 * @param action - pointer to an Action
 */
void MedikitState::healClick(Action*)
{
	const int healQty (_action->weapon->getHealQuantity());
	if (healQty != 0)
	{
		if (_action->targetUnit->getOriginalFaction() != FACTION_HOSTILE)
		{
			if (_action->actor->expendTu(_action->TU) == true)
			{
				++_action->actor->getStatistics()->medikitApplications;

				_action->weapon->setHealQuantity(healQty - 1);
				const RuleItem* const itRule (_action->weapon->getRules());
				_action->targetUnit->heal(
										_mediView->getSelectedPart(),
										itRule->getWoundRecovery(),
										itRule->getHealthRecovery());
				_mediView->autoSelectPart();
				_mediView->invalidate();

				if (_action->targetUnit->getUnitStatus() == STATUS_UNCONSCIOUS
					&& _action->targetUnit->isStunned() == false)
				{
					_action->actor->getStatistics()->revivedSoldier += 2;
					closeClick(); // if the unit has revived quit this screen automatically
				}
				else
					update();
			}
			else
			{
				_action->result = BattlescapeGame::PLAYER_ERROR[0u];
//				closeClick();
			}
		}
		else
		{
			_action->result = BattlescapeGame::PLAYER_ERROR[4u];
//			closeClick();
		}
	}
}

/**
 * Handler for clicking the stimulant button.
 * @note Amphetamine.
 * @param action - pointer to an Action
 */
void MedikitState::stimClick(Action*)
{
	const int stimQty (_action->weapon->getStimulantQuantity());
	if (stimQty != 0)
	{
		if (_action->targetUnit->getOriginalFaction() != FACTION_HOSTILE)
		{
			if (_action->actor->expendTu(_action->TU) == true)
			{
				++_action->actor->getStatistics()->medikitApplications;

				_action->weapon->setStimulantQuantity(stimQty - 1);
				const RuleItem* const itRule (_action->weapon->getRules());
				if (_action->targetUnit->amphetamine(
												itRule->getEnergyRecovery(),
												itRule->getStunRecovery()) == true)
				{
					if (_action->targetUnit->getFatalsTotal() != 0)
						++_action->actor->getStatistics()->revivedSoldier;

					closeClick(); // if the unit has revived quit this screen automatically
				}
				else
					update();
			}
			else
			{
				_action->result = BattlescapeGame::PLAYER_ERROR[0u];
//				closeClick();
			}
		}
		else
		{
			_action->result = BattlescapeGame::PLAYER_ERROR[4u];
//			closeClick();
		}
	}
}

/**
 * Handler for clicking the painkiller button.
 * @note Morphine.
 * @param action - pointer to an Action
 */
void MedikitState::painClick(Action*)
{
	const int painQty (_action->weapon->getPainKillerQuantity());
	if (painQty != 0)
	{
		if (_action->targetUnit->getOriginalFaction() != FACTION_HOSTILE)
		{
			if (_action->actor->expendTu(_action->TU) == true)
			{
				++_action->actor->getStatistics()->medikitApplications;

				_action->weapon->setPainKillerQuantity(painQty - 1);
				_action->targetUnit->morphine();

				if (_action->targetUnit->getHealth() == 0) // overdose.
					closeClick();
				else if (_action->targetUnit->getFaction() == FACTION_NEUTRAL) // take control of Civies.
				{
					_action->targetUnit->setFaction(FACTION_PLAYER);
					_action->targetUnit->prepTuEnergy();

					SavedBattleGame* const battleSave (_game->getSavedGame()->getBattleSave());

					battleSave->setSelectedUnit(_action->targetUnit);
					battleSave->getBattleState()->updateSoldierInfo();

					battleSave->getBattleGame()->cancelTacticalAction();
					_action->actor = _action->targetUnit;

					battleSave->getBattleGame()->setupSelector();

					battleSave->getBattleGame()->getMap()->getCamera()->centerPosition(
																					_action->targetUnit->getPosition(),
																					false);
					closeClick();
				}
				else
					update();
			}
			else
			{
				_action->result = BattlescapeGame::PLAYER_ERROR[0u];
//				closeClick();
			}
		}
		else
		{
			_action->result = BattlescapeGame::PLAYER_ERROR[4u];
//			closeClick();
		}
	}
}

/**
 * Updates this Medikit state.
 */
void MedikitState::update()
{
	_txtPain->setText(toString(_action->weapon->getPainKillerQuantity()));
	_txtStim->setText(toString(_action->weapon->getStimulantQuantity()));
	_txtHeal->setText(toString(_action->weapon->getHealQuantity()));

	// Health/ Stun/ Stamina/ Morale of the recipient
	double stat (static_cast<double>(_action->targetUnit->getBattleStats()->health));
	const int health (_action->targetUnit->getHealth());
	_numHealth->setValue(static_cast<unsigned>(health));
	if (_action->targetUnit->getStun() != 0)
	{
		_numStun->setValue(static_cast<unsigned>(_action->targetUnit->getStun()));
		_numStun->setVisible();
	}
	else
		_numStun->setVisible(false);
	_barHealth->setValue(std::ceil(
							static_cast<double>(health) / stat * 100.));
	_barHealth->setValue2(std::ceil(
							static_cast<double>(_action->targetUnit->getStun()) / stat * 100.));

	stat = static_cast<double>(_action->targetUnit->getBattleStats()->stamina);
	const int energy (_action->targetUnit->getEnergy());
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setValue(std::ceil(
							static_cast<double>(energy) / stat * 100.));

	const int morale (_action->targetUnit->getMorale());
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setValue(static_cast<double>(morale));

	// TU of the Medikit user
	stat = static_cast<double>(_action->actor->getBattleStats()->tu);
	const int tu (_action->actor->getTu());
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setValue(std::ceil(
							static_cast<double>(tu) / stat * 100.));
}

}
