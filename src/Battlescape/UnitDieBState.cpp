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

#include "UnitDieBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "InfoboxDialogState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Creates a UnitDieBState.
 * @note If a unit is already unconscious when it dies then it should never get
 * sent through this state; handle it more simply with BattleUnit::putdown().
 * @param battle		- pointer to the BattlescapeGame
 * @param unit			- pointer to the hurt BattleUnit
 * @param isPreTactical	- true to speed things along through pre-battle powersource
 *						  explosions; implicitly isSilent=TRUE and (unused) isInjury=FALSE
 * @param isSilent		- true to disable focusing (and peripherally the death-sound)
 *						  of stunned units (default true)
 * @param isInjury		- true if invoked by dType=DT_NONE (default false)
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* const battle,
		BattleUnit* const unit,
		const bool isPreTactical,
		const bool isSilent,
		const bool isInjury)
	:
		BattleState(battle),
		_unit(unit),
		_isPreTactical(isPreTactical),
		_isSilent(isSilent),
		_isInjury(isInjury),
		_battleSave(battle->getBattleSave()),
		_post(0),
		_isInfected(unit->getSpawnType().empty() == false)
{
	//Log(LOG_INFO) << "UnitDieBState::cTor " << _unit->getId(); // _action.actor->getId()

//	_unit->clearVisibleTiles();
//	_unit->clearHostileUnits();

	if (_isPreTactical == true) // power-source explosion: skip all animations.
	{
		switch (_unit->getHealth())
		{
			case 0:
				_unit->setUnitStatus(STATUS_DEAD);
				break;
			default:
				_unit->setUnitStatus(STATUS_UNCONSCIOUS);
		}
	}
	else
	{
		switch (_unit->getFaction())
		{
			case FACTION_HOSTILE:
				for (std::vector<Node*>::const_iterator // flag nearby Nodes as Type_Dangerous.
						i  = _battleSave->getNodes()->begin();
						i != _battleSave->getNodes()->end();
						++i)
				{
					if (TileEngine::distance(
										(*i)->getPosition(),
										_unit->getPosition()) < 5)
						(*i)->setDangerous();
				}
				// no break;

			case FACTION_NEUTRAL:
				if (_isInfected == false)
					_unit->setUnitVisible();	// reveal Map if '_unit' is currently selected by the AI
				break;							// NOTE: player-units are always visible.

			case FACTION_PLAYER:
				_battle->getMap()->setUnitDying(); // reveal Map for the duration ....
		}

		if (_isInfected == true)
			_unit->setDirectionTo(BattleUnit::DIR_FACEPLAYER);	// -> STATUS_TURNING if not already facing Player -> STATUS_STANDING
		else
			_unit->startSpinning();								// -> STATUS_TURNING
	}
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{}

/**
 * Gets the label of this BattleState.
 * @return, label of the substate
 */
std::string UnitDieBState::getBattleStateLabel() const
{
	std::ostringstream oststr;
	oststr << "UnitDieBState";
	if (_action.actor != nullptr) oststr << " ActorId-" << _action.actor->getId();
	if (_unit         != nullptr) oststr << " UnitId-"  << _unit        ->getId();
	return oststr.str();
}

/**
 * Initializes this state.
 */
//void UnitDieBState::init(){}

/**
 * Runs BattleState functionality every cycle.
 * @note Progresses the death sequence, displays any messages, checks if the
 * mission is over, etc. This routine gets an award for the klunkiest ever
 * written. TODO: Fix that.
 */
void UnitDieBState::think()
{
	//Log(LOG_INFO) << "UnitDieBState::think() " << _unit->getId() << " status= " << BattleUnit::debugStatus(_unit->getUnitStatus());

//	#0
	if (_isSilent == false) // skip focus and sound.
	{
		_isSilent = true; // done.

		//Log(LOG_INFO) << ". focusPosition()";
		_battle->getMap()->getCamera()->focusPosition(_unit->getPosition());	// NOTE: Can't be done in cTor or init() because
																				// ... BattleState machine gets confused.
		if (   _unit->getHealth() == 0
			&& _unit->hasCriedShotgun() == false
			&& _unit->getOverDose() == false)
		{
			//Log(LOG_INFO) << "";
			//Log(LOG_INFO) << "UnitDieBState::think() id-" << _unit->getId();
			//Log(LOG_INFO) << ". call playDeathSound()";

			_unit->playDeathSound();
		}
	}

//	STATUS_STANDING,	//  0
//	STATUS_WALKING,		//  1
//	STATUS_FLYING,		//  2
//	STATUS_TURNING,		//  3
//	STATUS_AIMING,		//  4
//	STATUS_COLLAPSING,	//  5
//	STATUS_DEAD,		//  6
//	STATUS_UNCONSCIOUS,	//  7
//	STATUS_PANICKING,	//  8
//	STATUS_BERSERK,		//  9
//	STATUS_LATENT,		// 10
//	STATUS_LATENT_START	// 11

	switch (_unit->getUnitStatus())
	{
		case STATUS_TURNING:
			//Log(LOG_INFO) << ". STATUS_TURNING";
			if (_isInfected == false)
			{
				//Log(LOG_INFO) << "unitDieB: think() set interval " << BattlescapeState::STATE_INTERVAL_DEATHSPIN;
				_battle->setStateInterval(BattlescapeState::STATE_INTERVAL_DEATHSPIN);
				_unit->keepSpinning();
			}
			else // face Player.
				_unit->turn();
			break; // -> STATUS_STANDING

		case STATUS_STANDING:
			//Log(LOG_INFO) << ". STATUS_STANDING";
			if (_isInfected == false || _unit->isZombie() == true)
			{
				//Log(LOG_INFO) << "unitDieB: think() set interval " << BattlescapeState::STATE_INTERVAL_STANDARD;
				_battle->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
				_unit->startCollapsing();
			}
			else // NOTE: UnitSprite might try to bork on this. Set cache-invalid might do it <- need a test-case.
			{
				_unit->setUnitStatus(STATUS_DEAD);
				_unit->setHealth(0);
			}
			break; // -> STATUS_COLLAPSING -> STATUS_DEAD or STATUS_UNCONSCIOUS

		case STATUS_COLLAPSING:
			//Log(LOG_INFO) << ". STATUS_COLLAPSING";
			if (_unit->keepCollapsing() == false) break; // -> STATUS_DEAD or STATUS_UNCONSCIOUS
			// no break;

		case STATUS_DEAD:
		case STATUS_UNCONSCIOUS:
			//Log(LOG_INFO) << ". STATUS_DEAD or _UNCONSCIOUS";
			switch (_post)
			{
				case 0:
					//Log(LOG_INFO) << ". . post 0";

					if (_isInfected == true)
						_battle->convertUnit(_unit);

					drop();
					_unit->putdown(_isInfected == true
								|| _unit->isMechanical() == true
								|| _unit->isCapturable() == false
								|| _unit->getSpecialAbility() == SPECAB_EXPLODE);

					if (_isPreTactical == true)
						_battle->popBattleState(); // NOTE: If unit was selected it will be de-selected in popBattleState().
					else
						++_post;
					return; // don't bother w/ sprite-cache.

				case 1:
				{
					//Log(LOG_INFO) << ". . post 1";

					bool persistReveal (false);
					for (std::vector<BattleUnit*>::const_iterator
							i  = _battleSave->getUnits()->begin();
							i != _battleSave->getUnits()->end();
							++i)
					{
						if ((*i)->isAboutToCollapse() == true)
						{
							persistReveal = true;
							break;
						}
					}
					if (persistReveal == false)
						_battle->getMap()->setUnitDying(false);

					_battle->popBattleState(); // NOTE: If unit was selected it will be de-selected in popBattleState().

					_battleSave->getBattleState()->updateSoldierInfo();	// update visUnit-indicators in case other units
																		// were hiding behind the one who just fell, etc.
																		// NOTE: See also drop() below_
																		// This could be unnecessary.
					if (_unit->getGeoscapeSoldier() != nullptr)
					{
						std::string stInfo;
						switch (_unit->getUnitStatus())
						{
							case STATUS_DEAD:
								if (_isInjury == true && _isInfected == false)
									stInfo = "STR_HAS_DIED_FROM_A_FATAL_WOUND"; // ... or a Morphine overdose.
								else if (Options::battleNotifyDeath == true)
									stInfo = "STR_HAS_BEEN_KILLED";
								break;

							case STATUS_UNCONSCIOUS:
								stInfo = "STR_HAS_BECOME_UNCONSCIOUS";
						}

						if (stInfo.empty() == false)
						{
							Game* const game (_battleSave->getBattleState()->getGame());
							const Language* const lang (game->getLanguage());
							game->pushState(new InfoboxDialogState(lang->getString(stInfo, _unit->getGender())
																			.arg(_unit->getLabel(lang))));
						}
					}
					return; // don't bother w/ sprite-cache.
				}
			}
	}

	if (_isPreTactical == false)
		_battle->getMap()->cacheUnitSprite(_unit);
}

/**
 * Converts the BattleUnit to a body-item.
 * @note Dead or Unconscious units get a nullptr-Tile but keep track of the
 * Position of their body. Also, the updated UnitStatus is valid here.
 */
void UnitDieBState::drop() // private.
{
	//Log(LOG_INFO) << "UnitDieBState::drop() " << _unit->getId();

	if (_isPreTactical == false)
		_battleSave->getBattleState()->showPsiButton(false);	// ... why is this here ...
																// any reason it's not in, say, the cTor or init() or popBattleState()
	if (_unit->canInventory() == true
		&& (Options::battleWeaponSelfDestruction == false
			|| _unit->getOriginalFaction() != FACTION_HOSTILE
			|| _unit->getUnitStatus() == STATUS_UNCONSCIOUS))
	{
		_battle->dropUnitInventory(_unit);
	}

	Tile
		* tile,
		* tileExpl,
		* tileExplBelow;
	bool playSound (true);

	int quad (_unit->getArmor()->getSize());
	size_t quads (static_cast<size_t>(quad * quad));
	--quad;

	const Position& pos (_unit->getPosition());
	for (int
			y = quad;
			y != -1;
			--y)
	{
		for (int
				x = quad;
				x != -1;
				--x)
		{
			if (_unit->isMechanical() == true)
			{
				tile = _battleSave->getTile(pos + Position(x,y,0));

				if (RNG::percent(6) == true)
				{
					tileExpl = tile;
					tileExplBelow = tileExpl->getTileBelow(_battleSave);

					while (tileExpl != nullptr // safety.
						&& tileExpl->isFloored(tileExplBelow) == false)
					{
						tileExpl = tileExplBelow;
						tileExplBelow = tileExpl->getTileBelow(_battleSave);
					}

					if (   tileExpl != nullptr // safety.
						&& tileExpl->getFire() == 0)
					{
						if (tileExpl->addFire(tileExpl->getFuel() + RNG::generate(1,2)) == false)
							tileExpl->addSmoke(std::max(tileExpl->getFuel() + 1,
													  ((tileExpl->getBurnable() + 9) / 10) + 1));

						if (playSound == true)
						{
							playSound = false;
							_battle->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::SMALL_EXPLOSION)
															->play(-1, _battle->getMap()->getSoundAngle(_unit->getPosition()));
						}
					}
				}
				tile->addSmoke(RNG::generate(0,2));
			}

			BattleItem* const body (new BattleItem(
												_battle->getRuleset()->getItemRule(_unit->getArmor()->getCorpseBattlescape()[--quads]),
												_battleSave->getCanonicalBattleId()));
			if (quads == 0) body->setBodyUnit(_unit); // only quadrant #0 denotes the unit's corpse/body.
			_battle->dropItem(
							body,
							pos + Position(x,y,0),
							true);
		}
	}

	_battle->getTileEngine()->calculateTerrainLighting();
	_battle->getTileEngine()->calculateUnitLighting();

	_battle->getTileEngine()->calcFovUnits_pos(pos, true);	// expose any units that were hiding behind dead unit
}																// and account for possible obscuring effects too.

/**
 * Dying cannot be cancelled.
 */
//void UnitDieBState::cancel(){}

}
