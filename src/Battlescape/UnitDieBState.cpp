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
 * sent through this state; handle it more simply with instaKill() or whatever.
 * @param parent	- pointer to BattlescapeGame
 * @param unit		- pointer to a dying BattleUnit
 * @param dType		- type of damage that caused the death (RuleItem.h)
 * @param noScream	- true to disable death-sound of stunned units (default false)
 * @param hidden	- true to speed things along through pre-battle powersource
 *					  explosions (default false)
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* const parent,
		BattleUnit* const unit,
		const DamageType dType,
		const bool noScream,
		const bool hidden)
	:
		BattleState(parent),
		_unit(unit),
		_dType(dType),
		_noScream(noScream),
		_hidden(hidden),
		_battleSave(parent->getBattleSave()),
		_doneScream(false),
		_extraTicks(-1),
		_init(true)
{
	// TODO: '_noScream' and '_hidden' even '_extraTicks' can be combined into
	// an int-var or an enum: DeathPhase or DeathMode or similar. Do straighten
	// this hodge-podge out ...!
//	_unit->clearVisibleTiles();
//	_unit->clearHostileUnits();

	if (_hidden == false)
	{
		_unit->setUnitVisible();
		centerOnDeath();

		switch (_unit->getFaction())
		{
			case FACTION_HOSTILE:
			{
				const std::vector<Node*>* const nodeList (_battleSave->getNodes());
				if (nodeList != nullptr) // this had better happen.
				{
					for (std::vector<Node*>::const_iterator
							i = nodeList->begin();
							i != nodeList->end();
							++i)
					{
						if (TileEngine::distance(
											(*i)->getPosition(),
											_unit->getPosition()) < 3)
						{
							(*i)->setNodeType((*i)->getNodeType() | Node::TYPE_DANGEROUS);
						}
					}
				}
				break;
			}

			case FACTION_PLAYER:
				_parent->getMap()->setUnitDying();
		}

		if (_unit->getSpawnType().empty() == false)
			_unit->setDirectionTo(3); // -> STATUS_TURNING if not facing correctly. Else STATUS_STANDING
		else
			_unit->initDeathSpin(); // -> STATUS_TURNING
	}
	else // pre-battle hidden power-source explosion death
	{
		if (_unit->getHealth() == 0)
			_unit->instaKill();
		else
			_unit->knockOut(); // convert if has a "spawnType" set. Else sets health=0 / stun=health.
	}
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{}

/**
 * Initializes this state.
 */
//void UnitDieBState::init(){}

/**
 * Runs state functionality every cycle.
 * @note Progresses the death sequence, displays any messages, checks if the
 * mission is over, etc. This routine gets an award for the klunkiest ever
 * written. TODO: Fix that.
 */
void UnitDieBState::think()
{
//	#0
	if (_noScream == false)
	{
		if (_init == true)
		{
			_init = false;
			centerOnDeath();
		}

		if (_doneScream == false
			&& _unit->isOut_t(OUT_STUNNED) == false
			&& _unit->hasCried() == false
			&& _unit->getOverDose() == false)
		{
			_doneScream = true;
			_unit->playDeathSound();
		}
	}

//	#1
	if (_unit->getUnitStatus() == STATUS_TURNING)
	{
		if (_unit->getSpinPhase() != -1)
		{
			//Log(LOG_INFO) << "unitDieB: think() set interval = " << BattlescapeState::STATE_INTERVAL_DEATHSPIN;
			_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_DEATHSPIN);
			_unit->contDeathSpin(); // -> STATUS_STANDING
		}
		else // spawn conversion is going to happen
			_unit->turn(); // -> STATUS_STANDING
	}
//	#3
	else if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		_unit->keepCollapsing(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ie. isOut_t()
	}
//	#2
	else if (_unit->isOut_t(OUT_STAT) == false)
	{
		//Log(LOG_INFO) << "unitDieB: think() set interval = " << BattlescapeState::STATE_INTERVAL_STANDARD;
		_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
		_unit->startCollapsing(); // -> STATUS_COLLAPSING

		if (_unit->getSpawnType().empty() == false)
		{
			while (_unit->getUnitStatus() == STATUS_COLLAPSING)
				_unit->keepCollapsing(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() ) -> goto #4
		}
	}

//	#5 - finish.
	if (--_extraTicks == 0)
	{
		bool moreDead (false);
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getAboutToCollapse() == true)
			{
				moreDead = true;
				break;
			}
		}
		if (moreDead == false)
			_parent->getMap()->setUnitDying(false);

		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		// need to freshen visUnit-indicators in case other units were hiding behind the one who just fell
		_battleSave->getBattleState()->updateSoldierInfo(false);

		if (_unit->getGeoscapeSoldier() != nullptr)
		{
			std::string stInfo;
			switch (_unit->getUnitStatus())
			{
				case STATUS_DEAD:
					if (_dType == DT_NONE
						&& _unit->getSpawnType().empty() == true)
					{
						stInfo = "STR_HAS_DIED_FROM_A_FATAL_WOUND"; // ... or a Morphine overdose.
					}
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
															.arg(_unit->getName(lang))));
			}
		}

		// if all units from either faction are killed - auto-end the mission.
//		if (Options::battleAutoEnd == true && _battleSave->getSide() == FACTION_PLAYER)
//		{
//			int liveHostile, livePlayer;
//			_parent->tallyUnits(liveHostile, livePlayer);
//
//			if (liveHostile == 0 || livePlayer == 0)
//			{
//				_battleSave->setSelectedUnit();
//				_parent->cancelTacticalAction(true);
//				_parent->requestEndTurn();
//			}
//		}
	}
//	#4
	else if (_unit->isOut_t(OUT_STAT) == true) // and this ought be Status_Dead OR _Unconscious.
	{
		_extraTicks = 1;

		if (_unit->getSpecialAbility() == SPECAB_EXPLODE
			&& _unit->getUnitStatus() == STATUS_UNCONSCIOUS)
		{
			_unit->instaKill();
		}
		else
			_unit->putDown(); // TODO: Straighten these out vis-a-vis the cTor, knockOut().

		if (_unit->getSpawnType().empty() == false)
			_parent->convertUnit(_unit);
		else
			convertToBody();

		if (_hidden == true)
			_parent->popState(); // NOTE: If unit was selected it will be de-selected in popState().
	}

	if (_hidden == false)
	{
//		_unit->flagCache(); // <- set in startCollapsing() and keepCollapsing()
		_parent->getMap()->cacheUnit(_unit);
	}
}

/**
 * Converts the BattleUnit to a body-item.
 * @note Dead or Unconscious units get a nullptr-Tile but keep track of the
 * Position of their body. Also, the updated UnitStatus is valid here.
 */
void UnitDieBState::convertToBody() // private.
{
	_unit->setTile();

	if (_hidden == false)
		_battleSave->getBattleState()->showPsiButton(false);	// ... why is this here ...
																// any reason it's not in, say, the cTor or init()
	if (_unit->hasInventory() == true
		&& (Options::battleWeaponSelfDestruction == false
			|| _unit->getOriginalFaction() != FACTION_HOSTILE
			|| _unit->getUnitStatus() == STATUS_UNCONSCIOUS))
	{
		_parent->dropUnitInventory(_unit);
	}

	Tile
		* tile,
		* tileExpl,
		* tileExplBelow;
	bool
		playSound (true),
		calcLights (false);

	int unitSize (_unit->getArmor()->getSize());
	size_t quadrants (static_cast<size_t>(unitSize * unitSize));
	--unitSize;

	const Position pos (_unit->getPosition());
	for (int
			y = unitSize;
			y != -1;
			--y)
	{
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			tile = _battleSave->getTile(pos + Position(x,y,0));

			if (_unit->getUnitRules() != nullptr
				&& _unit->getUnitRules()->isMechanical() == true)
			{
				if (RNG::percent(6) == true)
				{
					tileExpl = tile;
					tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));

					while (tileExpl != nullptr				// safety.
						&& tileExpl->getPosition().z > 0	// safety.
//						&& tileExpl->getMapData(O_OBJECT) == nullptr
//						&& tileExpl->getMapData(O_FLOOR) == nullptr
						&& tileExpl->hasNoFloor(tileExplBelow) == true)
					{
						tileExpl = tileExplBelow;
						tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));
					}

					if (tileExpl != nullptr // safety.
						&& tileExpl->getFire() == 0)
					{
						calcLights = tileExpl->addFire(tileExpl->getFuel() + RNG::generate(1,2));
						tileExpl->addSmoke(std::max(1,
													std::min(6,
															 tileExpl->getFlammability() / 10)));

						if (playSound == true)
						{
							playSound = false;
							_parent->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::SMALL_EXPLOSION)
														->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
						}
					}
				}
				tile->addSmoke(RNG::generate(0,2));
			}

			if (tile != nullptr && tile->getTileUnit() == _unit)	// safety. had a CTD when ethereal dies on water.
				tile->setUnit();									// TODO: iterate over all mapTiles searching for the unit-item and
																	// null-ing all tile-links to it. cf. SavedBattleGame::deleteBody().

			BattleItem* const body (new BattleItem(
											_parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseBattlescape()[--quadrants]),
											_battleSave->getCanonicalBattleId()));
			body->setUnit(_unit);
			_parent->dropItem(
							body,
							pos + Position(x,y,0),
							DROP_CREATE);
		}
	}

	if (calcLights == true)
		_parent->getTileEngine()->calculateTerrainLighting();

	_parent->getTileEngine()->calcFovPos(pos, true);	// expose any units that were hiding behind dead unit
}														// and account for possible obscuring effects too.

/**
 * Centers the Camera on the collapsing unit.
 */
void UnitDieBState::centerOnDeath() // private.
{
	Camera* const deathCam (_parent->getMap()->getCamera());
	if (deathCam->isOnScreen(_unit->getPosition()) == false)
		deathCam->centerOnPosition(_unit->getPosition());
	else if (_unit->getPosition().z != deathCam->getViewLevel())
		deathCam->setViewLevel(_unit->getPosition().z);
}

/**
 * Dying cannot be cancelled.
 */
//void UnitDieBState::cancel(){}

}
