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
 * @param parent		- pointer to BattlescapeGame
 * @param unit			- pointer to the hurt BattleUnit
 * @param dType			- type of damage that caused the unit to collapse (RuleItem.h)
 * @param isSilent		- true to disable focusing (and peripherally the death-sound)
 *						  of stunned units (default false)
 * @param isPreTactical	- true to speed things along through pre-battle powersource
 *						  explosions (default false)
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* const parent,
		BattleUnit* const unit,
		const DamageType dType,
		const bool isSilent,
		const bool isPreTactical)
	:
		BattleState(parent),
		_unit(unit),
		_dType(dType),
		_isSilent(isSilent),
		_isPreTactical(isPreTactical),
		_battleSave(parent->getBattleSave()),
		_post(0)
{
	// TODO: '_isSilent' and '_isPreTactical' even '_extraTicks' can be combined into
	// an int-var or an enum: DeathPhase or DeathMode or similar. Do straighten
	// this hodge-podge out ...!
//	_unit->clearVisibleTiles();
//	_unit->clearHostileUnits();

	if (_isPreTactical == true) // power-source explosion
	{
		switch (_unit->getHealth())
		{
			case 0:  _unit->instaKill(); break;
			default: _unit->knockOut(); // convert if has a "spawnType" set. Else sets health=0 / stun=health.
		}
	}
	else
	{
		switch (_unit->getFaction())
		{
			case FACTION_HOSTILE:
				for (std::vector<Node*>::const_iterator // flag nearby Nodes as Type_Dangerous.
						i = _battleSave->getNodes()->begin();
						i != _battleSave->getNodes()->end();
						++i)
				{
					if (TileEngine::distance(
										(*i)->getPosition(),
										_unit->getPosition()) < 6)
						(*i)->setNodeType((*i)->getNodeType() | Node::TYPE_DANGEROUS);
				}
				// no break;

			case FACTION_NEUTRAL:
				_unit->setUnitVisible();	// reveal Map if '_unit' is currently selected by the AI
				break;						// NOTE: player-units are always visible.

			case FACTION_PLAYER:
				_parent->getMap()->setUnitDying(); // reveal Map for the duration ....
		}

		if (_unit->getSpawnType().empty() == false)
			_unit->setDirectionTo(3); // -> STATUS_TURNING if not facing correctly. Else STATUS_STANDING
		else
			_unit->initDeathSpin(); // -> STATUS_TURNING
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
	if (_action.actor != nullptr) oststr << " id-" << _action.actor->getId();
	else oststr << " - Actor INVALID";

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
//	#0
	if (_isSilent == false)
	{
		_isSilent = true; // done.

		_parent->getMap()->getCamera()->focusPosition(_unit->getPosition());	// NOTE: Can't be done in cTor or init() because
																				// ... BattleState machine gets confused.
		if (   _unit->getHealth() == 0
			&& _unit->hasCried() == false
			&& _unit->getOverDose() == false)
		{
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
			switch (_unit->getSpinPhase())
			{
				case -1: // unit-convert here.
					_unit->turn();
					break;

				default:
					//Log(LOG_INFO) << "unitDieB: think() set interval " << BattlescapeState::STATE_INTERVAL_DEATHSPIN;
					_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_DEATHSPIN);
					_unit->contDeathSpin();
					break;
			}
			break; // -> STATUS_STANDING

		case STATUS_STANDING:
			//Log(LOG_INFO) << "unitDieB: think() set interval " << BattlescapeState::STATE_INTERVAL_STANDARD;
			_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
			_unit->startCollapsing();

			if (_unit->getSpawnType().empty() == false) // collapse immediately.
			{
				while (_unit->getUnitStatus() == STATUS_COLLAPSING)
					_unit->keepCollapsing();
			}
			break; // -> STATUS_COLLAPSING -> STATUS_DEAD or STATUS_UNCONSCIOUS

		case STATUS_COLLAPSING:
			_unit->keepCollapsing(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS
			break;

		case STATUS_DEAD:
		case STATUS_UNCONSCIOUS:
			switch (_post)
			{
				case 0:
					_post = 1;

					switch (_unit->getUnitStatus())
					{
						case STATUS_DEAD:
							_unit->putDown(); // TODO: Straighten these out vis-a-vis the cTor, knockOut().
							break;

						case STATUS_UNCONSCIOUS:
							if (_unit->getSpecialAbility() == SPECAB_EXPLODE)
								_unit->instaKill();
					}

					if (_unit->getSpawnType().empty() == false)
						_parent->convertUnit(_unit);
					else
						convertToBody();

					if (_isPreTactical == true)
						_parent->popBattleState(); // NOTE: If unit was selected it will be de-selected in popBattleState().
					break;

				case 1:
				{
					bool persistReveal (false);
					for (std::vector<BattleUnit*>::const_iterator
							i = _battleSave->getUnits()->begin();
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
						_parent->getMap()->setUnitDying(false);

					_parent->getTileEngine()->calculateUnitLighting();
					_parent->popBattleState(); // NOTE: If unit was selected it will be de-selected in popBattleState().

					_battleSave->getBattleState()->updateSoldierInfo();	// update visUnit-indicators in case other units
																		// were hiding behind the one who just fell, etc.
					if (_unit->getGeoscapeSoldier() != nullptr)
					{
						std::string stInfo;
						switch (_unit->getUnitStatus())
						{
							case STATUS_DEAD:
								if (_dType == DT_NONE && _unit->getSpawnType().empty() == true)
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
				}
			}
	}

	if (_isPreTactical == false)
		_parent->getMap()->cacheUnitSprite(_unit);
}

/**
 * Converts the BattleUnit to a body-item.
 * @note Dead or Unconscious units get a nullptr-Tile but keep track of the
 * Position of their body. Also, the updated UnitStatus is valid here.
 */
void UnitDieBState::convertToBody() // private.
{
	_unit->setUnitTile(); // This should have never been done. Only the Tile's link to unit should be broken.

	if (_isPreTactical == false)
		_battleSave->getBattleState()->showPsiButton(false);	// ... why is this here ...
																// any reason it's not in, say, the cTor or init() or popBattleState()
	if (_unit->canInventory() == true
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
		playSound  (true),
		calcLights (false);

	int unitSize (_unit->getArmor()->getSize());
	size_t quadrant (static_cast<size_t>(unitSize * unitSize));
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

			if (   _unit->getUnitRules() != nullptr
				&& _unit->getUnitRules()->isMechanical() == true)
			{
				if (RNG::percent(6) == true)
				{
					tileExpl = tile;
					tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));

					while (tileExpl != nullptr // safety.
						&& tileExpl->isFloored(tileExplBelow) == false)
					{
						tileExpl = tileExplBelow;
						tileExplBelow = _battleSave->getTile(tileExpl->getPosition() + Position(0,0,-1));
					}

					if (   tileExpl != nullptr // safety.
						&& tileExpl->getFire() == 0)
					{
						if ((calcLights = tileExpl->addFire(tileExpl->getFuel() + RNG::generate(1,2))) == false)
							tileExpl->addSmoke(std::max(tileExpl->getFuel() + 1,
													  ((tileExpl->getFlammability() + 9) / 10) + 1));

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
				tile->setTileUnit();								// cf. SavedBattleGame::deleteBody().

			BattleItem* const body (new BattleItem(
											_parent->getRuleset()->getItemRule(_unit->getArmor()->getCorpseBattlescape()[--quadrant]),
											_battleSave->getCanonicalBattleId()));
			if (quadrant == 0) body->setItemUnit(_unit); // only quadrant #0 denotes the unit's corpse/body.
			_parent->dropItem(
							body,
							pos + Position(x,y,0),
							DROP_CREATE);
		}
	}

	if (calcLights == true)
		_parent->getTileEngine()->calculateTerrainLighting();

	_parent->getTileEngine()->calcFovUnits_pos(pos, true);	// expose any units that were hiding behind dead unit
}															// and account for possible obscuring effects too.

/**
 * Dying cannot be cancelled.
 */
//void UnitDieBState::cancel(){}

}
