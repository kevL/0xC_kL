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
 * @param parent	- pointer to BattlescapeGame
 * @param unit		- pointer to a dying BattleUnit
 * @param dType		- type of damage that caused the death (RuleItem.h)
 * @param noScream	- true to disable the death sound for pre-battle powersource explosions as well as stun (default false)
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* const parent,
		BattleUnit* const unit,
		const DamageType dType,
		const bool noScream)
	:
		BattleState(parent),
		_unit(unit),
		_dType(dType),
		_noScream(noScream),
		_battleSave(parent->getBattleSave()),
		_doneScream(false),
		_extraTicks(0),
		_init(true)
{
//	_unit->clearVisibleTiles();
//	_unit->clearHostileUnits();

	if (_noScream == false)			// NOT a pre-battle hidden power-source explosion death; needed here to stop Camera CTD.
	{
		_unit->setUnitVisible();	// Has side-effect of keeping stunned (noScream) victims non-revealed if not already visible. See think() also.

		if (_unit->getFaction() == FACTION_HOSTILE)
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
		}
	}

	if (_unit->getUnitVisible() == true)
	{
		centerOnUnitDeath();

		if (_unit->getFaction() == FACTION_PLAYER)
			_parent->getMap()->setUnitDying();

		if (_unit->getSpawnType().empty() == false)
			_unit->setDirectionTo(3); // -> STATUS_TURNING if not facing correctly. Else STATUS_STANDING
		else
			_unit->initDeathSpin(); // -> STATUS_TURNING
	}
	else // pre-battle hidden power-source explosion death or a stunned non-visible unit
	{
		if (_unit->isOut_t(OUT_HEALTH) == true)
			_unit->instaKill();
		else
			_unit->knockOut(); // convert if has a "spawnType" set. Else sets health0 / stun=health
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
 * mission is over, etc.
 */
void UnitDieBState::think()
{
// #0 TODO: Separate _noSound (ie. unconscious) from _hiddenExplosions.
	if (_noScream == false)	// Has side-effect of not Centering on stunned victims. See cTor also.
	{
		if (_init == true)
		{
			_init = false;
			centerOnUnitDeath();
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

// #1
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
// #3
	else if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		_unit->keepCollapsing(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ie. isOut_t()
	}
// #2
	else if (_unit->isOut_t(OUT_STAT) == false) // this ought be Status_Standing/Disabled also.
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

// #6 - finish.
	if (_extraTicks == 1)
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
			if (_unit->getUnitStatus() == STATUS_DEAD)
			{
				if (_dType == DT_NONE
					&& _unit->getSpawnType().empty() == true)
				{
					stInfo = "STR_HAS_DIED_FROM_A_FATAL_WOUND"; // ... or a Morphine overdose.
				}
				else if (Options::battleNotifyDeath == true)
					stInfo = "STR_HAS_BEEN_KILLED";
			}
			else
				stInfo = "STR_HAS_BECOME_UNCONSCIOUS";

			if (stInfo.empty() == false)
			{
				Game* const game (_battleSave->getBattleState()->getGame());
				const Language* const lang (game->getLanguage());
				game->pushState(new InfoboxDialogState(lang->getString(stInfo, _unit->getGender())
															.arg(_unit->getName(lang))));
			}
		}
/*		// if all units from either faction are killed - auto-end the mission.
		if (Options::battleAutoEnd == true && _battleSave->getSide() == FACTION_PLAYER)
		{
			int liveHostile, livePlayer;
			_parent->tallyUnits(liveHostile, livePlayer);

			if (liveHostile == 0 || livePlayer == 0)
			{
				_battleSave->setSelectedUnit(nullptr);
				_parent->cancelTacticalAction(true);
				_parent->requestEndTurn();
			}
		} */
	}
// #5
//	else if (_extraTicks > 0)
//		++_extraTicks;
// #4
	else if (_unit->isOut_t(OUT_STAT) == true) // and this ought be Status_Dead OR _Unconscious.
	{
		_extraTicks = 1;

		if (_unit->getUnitStatus() == STATUS_UNCONSCIOUS
			&& _unit->getSpecialAbility() == SPECAB_EXPLODE)
		{
			_unit->instaKill();
		}
		else
			_unit->putDown();

		if (_unit->getSpawnType().empty() == true)
			convertToBody();
		else
			_parent->convertUnit(_unit);
	}

	_unit->clearCache();
	_parent->getMap()->cacheUnit(_unit);
}

/**
 * Converts a BattleUnit to a body-item.
 * @note Dead or Unconscious units get a nullptr-Tile but keep track of the
 * Position of their death.
 */
void UnitDieBState::convertToBody() // private.
{
	_battleSave->getBattleState()->showPsiButton(false);

	const Position pos (_unit->getPosition());
	const bool carried (pos == Position(-1,-1,-1));
	if (carried == false)
		_battleSave->deleteBody(_unit);

	const int armorSize (_unit->getArmor()->getSize() - 1);

	if (armorSize == 0 // move inventory from unit to the ground for non-large units
		&& carried == false
		&& (Options::battleWeaponSelfDestruction == false
			  || _unit->getOriginalFaction() != FACTION_HOSTILE
			  || _unit->getUnitStatus() == STATUS_UNCONSCIOUS))
	{
		std::vector<BattleItem*> itemsToKeep;

		for (std::vector<BattleItem*>::const_iterator
				i = _unit->getInventory()->begin();
				i != _unit->getInventory()->end();
				++i)
		{
			_parent->dropItem(pos, *i);

			if ((*i)->getRules()->isFixed() == false)
				(*i)->setOwner();
			else
				itemsToKeep.push_back(*i);
		}

		_unit->getInventory()->clear();

		for (std::vector<BattleItem*>::const_iterator
				i = itemsToKeep.begin();
				i != itemsToKeep.end();
				++i)
		{
			_unit->getInventory()->push_back(*i);
		}
	}

	_unit->setTile(nullptr); // remove unit-tile link


	if (carried == true) // unconscious unit is being carried when it dies
	{
		for (std::vector<BattleItem*>::const_iterator // change the unconscious body-item to a corpse in the carrying unit's inventory
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			if ((*i)->getUnit() == _unit) // unit is in an inventory so unit must be a 1x1 unit
			{
				(*i)->changeRule(_parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseBattlescape()[0]));
				break;
			}
		}
	}
	else
	{
		Tile
			* tile,
			* tileExpl,
			* tileExplBelow;
		bool soundPlayed (false);
		size_t quadrants (static_cast<size_t>((armorSize + 1) * (armorSize + 1)));

		for (int // count downward to original position so that dropItem() correctly positions large units @ their NW quadrant.
				y = armorSize;
				y != -1;
				--y)
		{
			for (int
					x = armorSize;
					x != -1;
					--x)
			{
				tile = _battleSave->getTile(pos + Position(x,y,0));

				// This block is lifted from TileEngine::explode(), switch(DT_IN).
				if (_unit->getUnitRules() != nullptr
					&& _unit->getUnitRules()->isMechanical() == true)
				{
					if (RNG::percent(6) == true)
					{
						tileExpl = tile;
						tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));

						while (tileExpl != nullptr // safety.
							&& tileExpl->getPosition().z > 0
							&& tileExpl->getMapData(O_OBJECT) == nullptr // only floors & content can catch fire.
							&& tileExpl->getMapData(O_FLOOR) == nullptr
							&& tileExpl->hasNoFloor(tileExplBelow) == true)
						{
							tileExpl = tileExplBelow;
							tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));
						}

						if (tileExpl != nullptr // safety.
							&& tileExpl->getFire() == 0)
						{
							tileExpl->addFire(tileExpl->getFuel() + RNG::generate(1,2)); // Could use a ruleset-factor in here.
							tileExpl->addSmoke(std::max(1,
														std::min(6,
																 tileExpl->getFlammability() / 10)));

							if (soundPlayed == false)
							{
								soundPlayed = true;
								_parent->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::SMALL_EXPLOSION)
															->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
							}
						}
					}
					tile->addSmoke(RNG::generate(0,2)); // more smoke ...
				}

				BattleItem* const body (new BattleItem(
												_parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseBattlescape()[--quadrants]),
												_battleSave->getCanonicalBattleId()));
				body->setUnit(_unit);

				if (tile != nullptr						// kL, safety. (had a CTD when ethereal dies on water).
					&& tile->getTileUnit() == _unit)	// <- check in case unit was displaced by another unit ... that sounds pretty darn shakey.
				{										// TODO: iterate over all mapTiles searching for the unit and nullptring any tile's association to it.
					tile->setUnit(nullptr);
				}

				_parent->dropItem(
								pos + Position(x,y,0),
								body, true);
			}
		}
		_parent->getTileEngine()->calculateFOV(pos, true); // expose any units that were hiding behind dead unit
	}
}

/**
 * Centers Camera on the collapsing BattleUnit.
 */
void UnitDieBState::centerOnUnitDeath() // private.
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
