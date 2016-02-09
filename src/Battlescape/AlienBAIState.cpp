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

#include "AlienBAIState.h"

#include "BattlescapeState.h"
#include "Map.h"

#include "../fmath.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

//bool const kL_bDebug = true;

/**
 * Sets up an AlienBAIState w/ BattleAIState.
 * @param battleSave	- pointer to SavedBattleGame
 * @param unit			- pointer to the BattleUnit
 * @param startNode		- pointer to the Node the unit originates at (default nullptr)
 */
AlienBAIState::AlienBAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const startNode)
	:
		BattleAIState(
			battleSave,
			unit,
			startNode),
		_targetsExposed(0),
		_targetsVisible(0),
		_tuAmbush(-1),
//		_reserveTUs(0),
		_rifle(false),
		_melee(false),
		_blaster(false),
//		_grenade(false),
		_hasPsiBeenSet(false),
		_distClosest(1000),
		_reserve(BA_NONE)
{
	//Log(LOG_INFO) << "Create AlienBAIState";
	_escapeAction	= new BattleAction();
	_patrolAction	= new BattleAction();
	_ambushAction	= new BattleAction();
	_attackAction	= new BattleAction();
	_psiAction		= new BattleAction();
	//Log(LOG_INFO) << "Create AlienBAIState EXIT";
}

/**
 * Deletes the AlienBAIState.
 */
AlienBAIState::~AlienBAIState()
{
	//Log(LOG_INFO) << "Delete AlienBAIState";
	delete _escapeAction;
	delete _patrolAction;
	delete _ambushAction;
	delete _attackAction;
	delete _psiAction;
}

/**
 * Loads the AI state from a YAML file.
 * @param node - reference a YAML node
 */
void AlienBAIState::load(const YAML::Node& node)
{
	BattleAIState::load(node);
//	_wasHitBy = node["wasHitBy"].as<std::vector<int>>(_wasHitBy);
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBAIState::save() const
{
	return BattleAIState::save();
//	node["wasHitBy"] = _wasHitBy;
}

/**
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action - pointer to AI BattleAction to execute
 */
void AlienBAIState::think(BattleAction* const action)
{
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "AlienBAIState::think(), id-" << _unit->getId() << " pos " << _unit->getPosition();

	_pf = _battleSave->getPathfinding();
	_te = _battleSave->getTileEngine();

 	action->type = BA_THINK;

	if (_unit->getChargeTarget() != nullptr
		&& _unit->getChargeTarget()->isOut_t(OUT_STAT) == true)
	{
		_unit->setChargeTarget();
	}
//	_wasHitBy.clear();

	_blaster =
	_rifle = false;
	_melee = (_unit->getMeleeWeapon() != nullptr);
//	_grenade = false;

	action->weapon = _unit->getMainHandWeapon(); // will get Rifle OR Melee
//	if (action->weapon == nullptr)
//		action->weapon = _unit->getMeleeWeapon(); // will get Melee OR Fist

	_attackAction->weapon = action->weapon;
	_attackAction->diff = static_cast<int>(_battleSave->getBattleState()->getGame()->getSavedGame()->getDifficulty());

//	_attackAction->AIcount = action->AIcount;
//	_escapeAction->AIcount = action->AIcount;

	_spottersOrigin = tallySpotters(_unit->getPosition());
	_targetsExposed = tallyTargets();
	_targetsVisible = selectNearestTarget(); // sets _unitAggro.

	Log(LOG_INFO) << "_spottersOrigin = " << _spottersOrigin;
	Log(LOG_INFO) << "_targetsExposed = " << _targetsExposed;
	Log(LOG_INFO) << "_targetsVisible = " << _targetsVisible;
	Log(LOG_INFO) << "_AIMode = " << BattleAIState::debugAiMode(_AIMode);

	_pf->setPathingUnit(_unit);

	_reachable = _pf->findReachable(_unit, _unit->getTimeUnits());

	int tuReserve (-1);
	if (action->weapon != nullptr)
	{
		const RuleItem* const itRule (action->weapon->getRules());
		Log(LOG_INFO) << ". weapon " << itRule->getType();

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
			{
				//Log(LOG_INFO) << ". . weapon is Firearm";
				if (itRule->isWaypoints() != 0
					&& _targetsExposed > _targetsVisible) // else let BL fallback to aimed shot
				{
					Log(LOG_INFO) << ". . blaster TRUE";
					_blaster = true;
					tuReserve = _unit->getTimeUnits()
							  - _unit->getActionTu(BA_LAUNCH, action->weapon);
				}
				else
				{
					Log(LOG_INFO) << ". . rifle TRUE";
					_rifle = true;
					tuReserve = _unit->getTimeUnits()
							  - _unit->getActionTu(
												itRule->getDefaultAction(), // note: this needs chooseFireMethod() ...
												action->weapon);
				}
				break;
			}
			case BT_MELEE:
			{
				Log(LOG_INFO) << ". . melee TRUE";
				_melee = true;
				tuReserve = _unit->getTimeUnits()
						  - _unit->getActionTu(BA_MELEE, action->weapon);
				break;
			}
//			case BT_GRENADE:
//			{
//				Log(LOG_INFO) << ". . grenade TRUE";
//				_grenade = true; // <- this is no longer useful since getMainHandWeapon() does not return grenades.
//			}
		}

	}
	else Log(LOG_INFO) << ". . weapon is NULL";
//	else if () // kL_add -> Give the invisible 'meleeWeapon' param a try ....
//	{}

	if (tuReserve > -1)
		_reachableAttack = _pf->findReachable(_unit, tuReserve); // TODO: Let aLiens turn-to-shoot w/out extra Tu.
	else
		_reachableAttack.clear();


	// NOTE: These setups probly have an order: Escape, Ambush, Attack, Patrol.
	Log(LOG_INFO) << ". . . setupPatrol()";
	setupPatrol();
	Log(LOG_INFO) << "";

	Log(LOG_INFO) << ". . . setupAttack()";
	setupAttack();
	Log(LOG_INFO) << "";

	if (_targetsExposed != 0 && _tuAmbush == -1 && _melee == false)
	{
		Log(LOG_INFO) << ". . . setupAmbush()";
		setupAmbush();
		Log(LOG_INFO) << "";
	}

	if (_spottersOrigin != 0 && _tuEscape == -1)
	{
		Log(LOG_INFO) << ". . . setupEscape()";
		setupEscape();
		Log(LOG_INFO) << "";
	}

	if (_hasPsiBeenSet == false && _psiAction->type != BA_NONE)
	{
		Log(LOG_INFO) << ". . psi TRUE";
		_hasPsiBeenSet = true;

		action->type = _psiAction->type;
		action->target = _psiAction->target;
		action->AIcount -= 1;

		Log(LOG_INFO) << "AlienBAIState::think() EXIT, Psi";
		return;
	}
	_hasPsiBeenSet = false;

	Log(LOG_INFO) << ". evaluate [1] " << BattleAIState::debugAiMode(_AIMode);
	bool evaluate (false);
	switch (_AIMode)
	{
		default:
		case AI_PATROL:
			if (_spottersOrigin != 0
				|| _targetsVisible != 0
				|| _targetsExposed != 0
				|| RNG::percent(9) == true)
			{
				evaluate = true;
			}
			break;

		case AI_COMBAT:
			if (_attackAction->type == BA_THINK)
				evaluate = true;
			break;

		case AI_AMBUSH:
			if (_rifle == false
				|| _tuAmbush == -1
				|| _targetsVisible != 0)
			{
				evaluate = true;
			}
			break;

		case AI_ESCAPE:
			if (_spottersOrigin == 0
				|| _targetsExposed == 0)
			{
				evaluate = true;
			}
	}
	Log(LOG_INFO) << ". do Evaluate = " << evaluate;

	if (evaluate == false
		&& (_spottersOrigin > 1
			|| _unit->getHealth() < _unit->getBattleStats()->health * 2 / 3
			|| (_unitAggro != nullptr
				&& _unitAggro->getExposed() > _unit->getIntelligence())
			|| (_battleSave->isCheating() == true
				&& _AIMode != AI_COMBAT)))
	{
		evaluate = true;
	}
	Log(LOG_INFO) << ". do Evaluate (other) = " << evaluate;

	if (evaluate == true)
	{
		Log(LOG_INFO) << ". AIMode pre-Evaluate = " << BattleAIState::debugAiMode(_AIMode);
		evaluateAiMode();
		Log(LOG_INFO) << ". AIMode post-Evaluate = " << BattleAIState::debugAiMode(_AIMode);
	}

	Log(LOG_INFO) << ". evaluate [2] " << BattleAIState::debugAiMode(_AIMode);
	_reserve = BA_NONE;
	switch (_AIMode)
	{
		case AI_PATROL:
			_unit->setChargeTarget();

			if (action->weapon != nullptr
				&& action->weapon->getRules()->getBattleType() == BT_FIREARM)
			{
				switch (_unit->getAggression())
				{
					case 0: _reserve = BA_AIMEDSHOT;	break;
					case 1: _reserve = BA_AUTOSHOT;		break;
					default:
					case 2: _reserve = BA_SNAPSHOT;
				}
			}

			action->type = _patrolAction->type;
			action->target = _patrolAction->target;
			break;

		case AI_COMBAT:
			action->type = _attackAction->type;
			action->target = _attackAction->target;
			action->weapon = _attackAction->weapon;

			Log(LOG_INFO) << ". . ActionType = " << BattleAction::debugActionType(action->type);
			switch (action->type)
			{
				case BA_THROW:
					if (action->weapon != nullptr // TODO: Ensure this was done already ....
						&& action->weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						Log(LOG_INFO) << ". . Throw grenade - spend Tu for COMBAT";
						int costTu (action->weapon->getInventorySection()
										->getCost(_battleSave->getBattleGame()->getRuleset()->getInventoryRule(ST_RIGHTHAND)));

						if (action->weapon->getFuse() == -1)
							costTu += _unit->getActionTu(BA_PRIME, action->weapon);

						_unit->spendTimeUnits(costTu); // cf. grenadeAction() - priming the fuse is done in ProjectileFlyBState.
					}
					break;

				case BA_MOVE:
					if (_rifle == true
						&& _unit->getTimeUnits() > _unit->getActionTu(
																BA_SNAPSHOT, // TODO: Hook this into _reserve.
																action->weapon))
					{
						Log(LOG_INFO) << ". . Move w/ rifle + tu for COMBAT";
						action->AIcount -= 1;
					}
					break;

				case BA_LAUNCH:
					Log(LOG_INFO) << ". . Launch - copy waypoints for COMBAT";
					action->waypoints = _attackAction->waypoints;
			}

//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false); // don't worry about reserving TUs, factored that in already.
			action->finalFacing = _attackAction->finalFacing;
			action->TU = _unit->getActionTu(
										action->type,
										action->weapon);
			break;

		case AI_AMBUSH:
			_unit->setChargeTarget();

			action->type = _ambushAction->type;
			action->target = _ambushAction->target;
			action->finalFacing = _ambushAction->finalFacing;
			action->finalAction = true;

			break;

		case AI_ESCAPE:
			_unit->setChargeTarget();

			action->type = _escapeAction->type;
			action->target = _escapeAction->target;
			action->finalAction = true;
//			action->AIcount = 3; // <- CivilianBAI uses this instead of finalAction= true <--
			action->desperate = true;

			_unit->setHiding(); // used by UnitWalkBState::postPathProcedures()
	}

	if (action->type == BA_MOVE)
	{
		Log(LOG_INFO) << ". do BA_MOVE";
		if (action->target == _unit->getPosition())
		{
			Log(LOG_INFO) << ". . Stay put";
			action->type = BA_NONE;
		}
		else
		{
			Log(LOG_INFO) << ". . Move";
			_tuAmbush =
			_tuEscape = -1; // if moving re-evaluate Ambush/Escape target.
		}
	}
	Log(LOG_INFO) << "AlienBAIState::think() EXIT";
}

/**
 * Sets up a BattleAction for AI_PATROL Mode.
 * @note This is mainly going from node to node & moving about the map -
 * handles Node selection.
 * @note Fills out the '_patrolAction' with useful data.
 */
void AlienBAIState::setupPatrol() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol()";
	_patrolAction->TU = 0;

	if (_stopNode != nullptr
		&& _unit->getPosition() == _stopNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";
		_startNode = _stopNode;
		_stopNode->freeNode();
		_stopNode = nullptr;

		const int dir (_battleSave->getTileEngine()->faceWindow(_unit->getPosition()));
		_unit->setDirectionTo(dir);
		while (_unit->getUnitStatus() == STATUS_TURNING)
			_unit->turn();
	}

	if (_startNode == nullptr)
		_startNode = _battleSave->getNearestNode(_unit);

	_pf->setPathingUnit(_unit);

	bool scout (true);
	if (_battleSave->getTacType() == TCT_BASEDEFENSE	// aLiens attacking XCOM Base are always on scout.
		&& _unit->getArmor()->getSize() == 1)			// In base defense missions the non-large aliens walk towards target nodes - or
	{													// once there shoot objects thereabouts so scan the room for objects to destroy.
		if (_startNode->isTarget() == true
			&& _attackAction->weapon != nullptr
			&& _attackAction->weapon->getAmmoItem() != nullptr
			&& _attackAction->weapon->getAmmoItem()->getRules()->getDamageType() != DT_HE
			&& (   _attackAction->weapon->getRules()->getAccuracySnap() != 0 // TODO: this ought be expanded to include melee.
				|| _attackAction->weapon->getRules()->getAccuracyAuto() != 0
				|| _attackAction->weapon->getRules()->getAccuracyAimed() != 0)
			&& _battleSave->getModuleMap()[_startNode->getPosition().x / 10]
										  [_startNode->getPosition().y / 10].second > 0)
		{
			const MapData* data;
			const int
				x ((_unit->getPosition().x / 10) * 10),
				y ((_unit->getPosition().y / 10) * 10);

			for (int
					i = x;
					i != x + 9;
					++i)
			{
				for (int
						j = y;
						j != y + 9;
						++j)
				{
					if ((data = _battleSave->getTile(Position(i,j,1))->getMapData(O_OBJECT)) != nullptr
						&& data->isBaseModule() == true)
//						&& data->getDieMCD() && data->getArmor() < 60) // TODO: Create function canDestroy(int power);
					{
						_patrolAction->type = BA_SNAPSHOT;
						_patrolAction->weapon = _attackAction->weapon;
						_patrolAction->target = Position(i,j,1);
						_patrolAction->TU = _unit->getActionTu(BA_SNAPSHOT, _attackAction->weapon);

						return;
					}
				}
			}
		}
		else
		{
			int // find closest objective-node which is not already allocated
				dist (1000000),
				distTest;

			for (std::vector<Node*>::const_iterator
					i = _battleSave->getNodes()->begin();
					i != _battleSave->getNodes()->end();
					++i)
			{
				if ((*i)->isTarget() == true && (*i)->isAllocated() == false)
				{
					distTest = TileEngine::distSqr(
												_unit->getPosition(),
												(*i)->getPosition());
					if (_stopNode == nullptr
						|| (distTest < dist && *i != _startNode))
					{
						dist = distTest;
						_stopNode = *i;
					}
				}
			}
		}
	}
	else
	{
		// After turn 20 or if the morale is low aLiens move out of the UFO to scout.
		// kL_note: That, above is wrong. Orig behavior depends on "aggression" setting;
		// determines whether aliens come out of UFO to scout/search (attack, actually).
		// Also anyone standing in fire should also probably move ....
		if (_startNode != nullptr
			&& _startNode->getNodeRank() != NR_SCOUT
			&& (_battleSave->getTile(_unit->getPosition()) == nullptr // <- shouldn't be necessary.
				|| _battleSave->getTile(_unit->getPosition())->getFire() == 0)
			&& (_battleSave->isCheating() == false
				|| RNG::percent(_unit->getAggression() * 25) == false))
		{
			scout = false;
		}

	}

	if (_stopNode == nullptr)
	{
		_stopNode = _battleSave->getPatrolNode(scout, _unit, _startNode);
		if (_stopNode == nullptr)
			_stopNode = _battleSave->getPatrolNode(!scout, _unit, _startNode);
	}

	_patrolAction->type = BA_THINK;
	if (_stopNode != nullptr)
	{
		_pf->calculate(_unit, _stopNode->getPosition());
		if (_pf->getStartDirection() != -1)
		{
			_stopNode->allocateNode();
			_patrolAction->type = BA_MOVE;
			_patrolAction->target = _stopNode->getPosition();
		}
		else
			_stopNode = nullptr;

		_pf->abortPath();
	}
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol() EXIT";
}

/**
 * Sets up a BattleAction for AI_COMBAT Mode.
 * @note This will be a weapon, grenade, psionic, or waypoint attack -- or
 * perhaps moving to get a line of sight to a target. Fills out the
 * '_attackAction' with useful data.
 */
void AlienBAIState::setupAttack() // private.
{
	Log(LOG_INFO) << "AlienBAIState::setupAttack() id-" << _unit->getId();
	_attackAction->type = BA_THINK;
	_psiAction->type = BA_NONE;

	if (_targetsExposed != 0 && RNG::percent(PSI_OR_BLASTER_PCT) == true)
	{
		Log(LOG_INFO) << ". _targetsExposed = " << _targetsExposed;
		if (psiAction() == true
			|| (_blaster == true && wayPointAction() == true))
		{
			return;
		}
	}
	else
		_blaster = false;

	//Log(LOG_INFO) << ". selectNearestTarget()";
//	if (selectNearestTarget() != 0)
	if (_targetsVisible != 0)
	{
		Log(LOG_INFO) << ". _targetsVisible = " << _targetsVisible;
		Log(LOG_INFO) << ". . try grenadeAction()";
		if (grenadeAction() == false)
		{
			Log(LOG_INFO) << ". . . try rifle Or melee";
			if (_rifle == true && _melee == true)
			{
				Log(LOG_INFO) << ". . Melee & Rifle are TRUE, do chooseMeleeOrRanged()";
				chooseMeleeOrRanged();
			}

			if (_rifle == true)
			{
				Log(LOG_INFO) << ". . rifleAction()";
				rifleAction();
				Log(LOG_INFO) << "";
			}
			else if (_melee == true)
			{
				Log(LOG_INFO) << ". . meleeAction()";
				meleeAction();
				Log(LOG_INFO) << "";
			}
		}
		else Log(LOG_INFO) << ". . grenadeAction() TRUE";
	}
	//Log(LOG_INFO) << ". selectNearestTarget() DONE";

	Log(LOG_INFO) << ". Attack bat = " << BattleAction::debugActionType(_attackAction->type);

	if		(_attackAction->type == BA_MOVE)	Log(LOG_INFO) << ". . walk to " << _attackAction->target;
	else if	(_attackAction->type != BA_THINK)	Log(LOG_INFO) << ". . shoot at " << _attackAction->target;

	if (_attackAction->type == BA_THINK
		|| _spottersOrigin != 0
		|| RNG::generate(0, _unit->getAggression()) < _unit->getAggression())
	{
		if (findFirePosition() == true)
			Log(LOG_INFO) << ". . findFirePosition TRUE " << _attackAction->target;
		else Log(LOG_INFO) << ". . findFirePosition FAILED";
	}
	Log(LOG_INFO) << "AlienBAIState::setupAttack() EXIT";
}

/**
 * Sets up a BattleAction for AI_AMBUSH Mode.
 * @note The idea is to check within a 11x11 tile square for a tile which is not
 * seen by the aggroTarget but that can be reached by him/her. Then intuit where
 * AI will see that target first from a covered position and set that as the
 * final facing.
 * @note Fills out the '_ambushAction' with useful data.
 */
void AlienBAIState::setupAmbush() // private.
{
	_ambushAction->type = BA_THINK;
	_tuAmbush = -1;

//	if (selectPlayerTarget() == true) // sets _unitAggro.
	if (_unitAggro != nullptr)
	{
		Position
			originVoxel (_te->getSightOriginVoxel(_unitAggro)),
			scanVoxel, // placeholder.
			pos;
		const Tile* tile;
		int
			score (0),
			scoreTest,
			tu;

		std::vector<int> targetPath;

/*		for (std::vector<Node*>::const_iterator			// use node positions for this since it gives map makers a good
				i = _battleSave->getNodes()->begin();	// degree of control over how the units will use the environment.
				i != _battleSave->getNodes()->end();	// Is that why ambushes work so crappy.
				++i)
		{
			pos = (*i)->getPosition();
			if ((tile = _battleSave->getTile(pos)) != nullptr
				&& tile->getDangerous() == false
				&& pos.z == _unit->getPosition().z
				&& TileEngine::distSqr(pos, _unit->getPosition()) < static_cast<int>(SavedBattleGame::SEARCH_SIZE)
				&& std::find(
						_reachableAttack.begin(),
						_reachableAttack.end(),
						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
			{ */
		std::vector<Position> tileSearch (_battleSave->getTileSearch());
		RNG::shuffle(tileSearch.begin(), tileSearch.end());

		for (std::vector<Position>::const_iterator
				i = tileSearch.begin();
				i != tileSearch.end();
				++i)
		{
			pos = _unit->getPosition() + *i;
			if ((tile = _battleSave->getTile(pos)) != nullptr
				&& std::find(
						_reachableAttack.begin(),
						_reachableAttack.end(),
						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
			{
/*				if (_traceAI) // color all the nodes in range purple.
				{
					tile->setPreviewDir(10);
					tile->setPreviewColor(13);
				} */

				if (tallySpotters(pos) == 0
					&& _te->canTargetUnit(
									&originVoxel,
									tile,
									&scanVoxel,
									_unitAggro,
									_unit) == false)
				{
					_pf->setPathingUnit(_unit);
					_pf->calculate(_unit, pos);

					if (_pf->getStartDirection() != -1)
					{
						tu = _pf->getTuCostTotalPf();

						scoreTest = BASE_SUCCESS_SYSTEMATIC;
						scoreTest -= tu;

						_pf->setPathingUnit(_unitAggro);
						_pf->calculate(_unitAggro, pos);

						if (_pf->getStartDirection() != -1)
						{
							if (_te->faceWindow(pos) != -1)
								scoreTest += COVER_BONUS;

							if (scoreTest > score)
							{
								score = scoreTest;
								targetPath = _pf->copyPath();

								_ambushAction->target = pos;
//								if (pos == _unit->getPosition())
//									_tuAmbush = 1;
//								else
								_tuAmbush = tu;

								if (score > FAST_PASS_THRESHOLD - 20)
									break;
							}
						}
					}
				}
			}
		}

		if (score != 0)
		{
			_ambushAction->type = BA_MOVE;
			originVoxel = _te->getSightOriginVoxel(_unit, &_ambushAction->target);
			Position posNext;

			_pf->setPathingUnit(_unitAggro);
			pos = _unitAggro->getPosition();

			size_t t (targetPath.size());
			while (t != 0)
			{
				--t;

				_pf->getTuCostPf(pos, targetPath.back(), &posNext);
				targetPath.pop_back();
				pos = posNext;

				tile = _battleSave->getTile(pos);
				if (_te->canTargetUnit(
									&originVoxel,
									tile,
									&scanVoxel,
									_unit,
									_unitAggro) == true)
				{
					_ambushAction->finalFacing = TileEngine::getDirectionTo(_ambushAction->target, pos);
					break;
				}
			}

			//if (_traceAI) Log(LOG_INFO) << "Ambush estimation will move to " << _ambushAction->target;
			return;
		}
	}
	//if (_traceAI) Log(LOG_INFO) << "Ambush estimation failed";
}

/**
 * Sets up a BattleAction for AI_ESCAPE Mode.
 * @note The idea is to check within a 11x11 tile square for a tile that is not
 * seen by '_unitAggro'. If there is no such tile run away from the target.
 * @note Fills out the '_escapeAction' with useful data.
 */
void AlienBAIState::setupEscape() // private.
{
//	selectNearestTarget(); // sets _unitAggro
//	const int spottersOrigin (tallySpotters(_unit->getPosition()));

	int
		distAggroOrigin,
		distAggroTarget;
	if (_unitAggro != nullptr)
		distAggroOrigin = TileEngine::distance(
										_unit->getPosition(),
										_unitAggro->getPosition());
	else
		distAggroOrigin = 0;

	const Tile* tile;
	int
		score (ESCAPE_FAIL),
		scoreTest;

	std::vector<Position> tileSearch (_battleSave->getTileSearch());
	RNG::shuffle(tileSearch.begin(), tileSearch.end());

	_pf->setPathingUnit(_unit);

	bool
		coverFound (false),
		first (true);
	size_t i (SavedBattleGame::SEARCH_SIZE);
	while (coverFound == false && i <= SavedBattleGame::SEARCH_SIZE)
	{
		if (first == true)
		{
			first = false;
			i = 0;

			scoreTest = 0;

			if (_battleSave->getTile(_unit->_lastCover) != nullptr)
				_escapeAction->target = _unit->_lastCover;
			else
				_escapeAction->target = _unit->getPosition();
		}
		else if (i++ < SavedBattleGame::SEARCH_SIZE)
		{
			scoreTest = BASE_SUCCESS_SYSTEMATIC;

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += tileSearch[i].x;
			_escapeAction->target.y += tileSearch[i].y;

			if (_escapeAction->target == _unit->getPosition())
			{
//				if (spottersOrigin != 0)
				if (_spottersOrigin != 0)
				{
					_escapeAction->target.x += RNG::generate(-20,20);
					_escapeAction->target.y += RNG::generate(-20,20);
				}
				else
					scoreTest += CUR_TILE_PREF;
			}
		}
		else
		{
			scoreTest = BASE_SUCCESS_DESPERATE;

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10,10);
			_escapeAction->target.y += RNG::generate(-10,10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (_escapeAction->target.z < 0)
				_escapeAction->target.z = 0;
			else if (_escapeAction->target.z >= _battleSave->getMapSizeZ())
				_escapeAction->target.z = _battleSave->getMapSizeZ();
		}


		if (_unitAggro != nullptr)
			distAggroTarget = TileEngine::distance(
												_escapeAction->target,
												_unitAggro->getPosition());
		else
			distAggroTarget = 0;

		scoreTest += (distAggroTarget - distAggroOrigin) * EXPOSURE_PENALTY;

		if ((tile = _battleSave->getTile(_escapeAction->target)) != nullptr
			&& std::find(
					_reachable.begin(),
					_reachable.end(),
					_battleSave->getTileIndex(_escapeAction->target)) != _reachable.end())
		{
			scoreTest += (_spottersOrigin - tallySpotters(_escapeAction->target)) * EXPOSURE_PENALTY;

			if (tile->getFire() != 0)
				scoreTest -= FIRE_PENALTY;
			else
				scoreTest += tile->getSmoke() * 5;

			if (tile->getDangerous() == true)
				scoreTest -= BASE_SUCCESS_SYSTEMATIC;
//			if (_traceAI) {
//				tile->setPreviewColor(scoreTest < 0 ? 3: (scoreTest < FAST_PASS_THRESHOLD / 2 ? 8: (scoreTest < FAST_PASS_THRESHOLD ? 9: 5)));
//				tile->setPreviewDir(10);
//				tile->setPreviewTu(scoreTest); }

			if (scoreTest > score)
			{
				_pf->calculate(_unit, _escapeAction->target);

				if (_pf->getStartDirection() != -1
					|| _escapeAction->target == _unit->getPosition())
				{
					score = scoreTest;
					_tuEscape = _pf->getTuCostTotalPf();
//					if (_traceAI) {
//						tile->setPreviewColor(scoreTest < 0? 7:(scoreTest < FAST_PASS_THRESHOLD / 2? 10:(scoreTest < FAST_PASS_THRESHOLD? 4:5)));
//						tile->setPreviewDir(10);
//						tile->setPreviewTu(scoreTest); }
				}
				_pf->abortPath();

				if (score > FAST_PASS_THRESHOLD)
					coverFound = true;
			}
		}
	}

	if (score != ESCAPE_FAIL)
	{
		//if (_traceAI) _battleSave->getTile(_escapeAction->target)->setPreviewColor(13);
		_escapeAction->type = BA_MOVE;
	}
	else
	{
		_escapeAction->type = BA_THINK;
		_tuEscape = -1;
	}
}

/**
 * Selects the AI Mode for BattlescapeGame::handleUnitAI().
 */
void AlienBAIState::evaluateAiMode() // private.
{
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "AlienBAIState::evaluateAiMode() id-" << _unit->getId();
	if (_unit->getChargeTarget() != nullptr
		&& _attackAction->type != BA_THINK)
	{
		Log(LOG_INFO) << ". chargeTarget NOT Think - return COMBAT";
		_AIMode = AI_COMBAT;
		return;
	}

	// if the aliens are cheating or the unit is charging enforce combat as a priority
	if (_battleSave->isCheating() == true // <- hmm, do i want this - kL_note
		|| _unit->getChargeTarget() != nullptr
		|| _blaster == true)	// The two (_blaster== true) checks in this function ought obviate the entire re-evaluate thing!
								// Note there is a valid targetPosition but targetUnit is NOT at that Pos if blaster=TRUE ....
	{
		Log(LOG_INFO) << ". chargeTarget Or waypoints Or blaster - set COMBAT";
		_AIMode = AI_COMBAT;
	}
	else
	{
		Log(LOG_INFO) << ". Evaluate ...";
		float
			patrolOdds (28.f),
			combatOdds (23.f),
			ambushOdds (13.f),
			escapeOdds (13.f);

		if (_unit->getTimeUnits() > _unit->getBattleStats()->tu / 2
			|| _unit->getChargeTarget() != nullptr)
		{
			escapeOdds = 5.f;
		}
		else if (_melee == true)
			escapeOdds = 10.5f;

		if (_targetsVisible != 0)
			patrolOdds = 8.f;

		if (_spottersOrigin != 0)
		{
			patrolOdds = 0.f;
			if (_tuEscape == -1) setupEscape();
		}

		if (_rifle == false || _tuAmbush == -1)
		{
			ambushOdds = 0.f;
			if (_melee == true)
			{
				combatOdds *= 1.2f;
			}
		}

		if (_targetsExposed != 0)
		{
			if (_targetsExposed == 1)
				combatOdds *= 1.9f;

			if (_tuEscape == -1)
			{
				if (selectPlayerTarget() == true) // sets _unitAggro.
					setupEscape();
				else
					escapeOdds = 0.f;
			}
		}
		else
		{
			combatOdds =
			escapeOdds = 0.f;
		}

		switch (_AIMode)
		{
			case AI_PATROL:
				patrolOdds *= 1.2f;
				break;
			case AI_AMBUSH:
				ambushOdds *= 1.2f;
				break;
			case AI_COMBAT:
				combatOdds *= 1.2f;
				break;
			case AI_ESCAPE:
				escapeOdds *= 1.2f;
		}

		if (_unit->getHealth() < _unit->getBattleStats()->health / 3)
		{
			escapeOdds *= 1.8f;
			combatOdds *= 0.6f;
			ambushOdds *= 0.7f;
		}
		else if (_unit->getHealth() < _unit->getBattleStats()->health * 2 / 3)
		{
			escapeOdds *= 1.5f;
			combatOdds *= 0.8f;
			ambushOdds *= 0.9f;
		}
		else if (_unit->getHealth() < _unit->getBattleStats()->health)
			escapeOdds *= 1.2f;

		switch (_unit->getAggression())
		{
			case 0:
				escapeOdds *= 1.5f;
				combatOdds *= 0.55f;
				break;
			case 1:
				ambushOdds *= 1.2f;
				break;
			case 2:
				combatOdds *= 1.65f;
				escapeOdds *= 0.5f;
				break;

			default:
				combatOdds *= std::max(
									0.1f,
									std::min(
											2.f,
											1.2f + (static_cast<float>(_unit->getAggression()) / 10.f)));
				escapeOdds *= std::min(
									2.f,
									std::max(
											0.1f,
											0.9f - (static_cast<float>(_unit->getAggression()) / 10.f)));
		}

		if (_AIMode == AI_COMBAT)
			ambushOdds *= 1.5f;

		if (_spottersOrigin != 0)
		{
			escapeOdds *= (10.f * static_cast<float>(_spottersOrigin + 10) / 100.f);
			combatOdds *= (5.f * static_cast<float>(_spottersOrigin + 20) / 100.f);
		}
		else
			escapeOdds /= 2.f;

		if (_targetsVisible != 0)
		{
			combatOdds *= (10.f * static_cast<float>(_targetsVisible + 10) / 100.f);

			if (_distClosest < 5)
				ambushOdds = 0.f;
		}

		if (_tuAmbush != -1)
			ambushOdds *= 2.f;
		else
			ambushOdds = 0.f;

		if (_battleSave->getTacType() == TCT_BASEDEFENSE)
		{
			escapeOdds *= 0.8f;
			ambushOdds *= 0.5f;
		}

		Log(LOG_INFO) << "patrolOdds = " << patrolOdds;
		Log(LOG_INFO) << "combatOdds = " << combatOdds;
		Log(LOG_INFO) << "ambushOdds = " << ambushOdds;
		Log(LOG_INFO) << "escapeOdds = " << escapeOdds;

		const float decision (RNG::generate(0.f, patrolOdds + combatOdds + ambushOdds + escapeOdds));

		Log(LOG_INFO) << "decision Pre = " << decision;
		if (decision <= patrolOdds)
			_AIMode = AI_PATROL;
		else if (decision <= patrolOdds + combatOdds)
			_AIMode = AI_COMBAT;
		else if (decision <= patrolOdds + combatOdds + ambushOdds)
			_AIMode = AI_AMBUSH;
		else
			_AIMode = AI_ESCAPE;
		Log(LOG_INFO) << "decision Post = " << BattleAIState::debugAiMode(_AIMode);
	}

	// TODO: These fallbacks should go in accord with the Odds above^
	// Check validity of the decision and if that fails try a fallback behaviour according to priority:
	// 1) Patrol
	// 2) Combat
	// 3) Ambush
	// 4) Escape
	if (_AIMode == AI_PATROL && _stopNode == nullptr)
	{
		Log(LOG_INFO) << ". fallback COMBAT";
		_AIMode = AI_COMBAT;
	}

	if (_AIMode == AI_COMBAT)
	{
		Log(LOG_INFO) << ". AI_COMBAT _blaster = " << (int)_blaster;
//		if (_unitAggro)
		if (_attackAction->type == BA_LAUNCH
//		if (_blaster == true // note: Blaster-wielding units should go for an AimedShot ... costs less TU.
//			|| _unitAggro != nullptr)
			|| (_battleSave->getTile(_attackAction->target) != nullptr
				&& _battleSave->getTile(_attackAction->target)->getTileUnit() != nullptr))
		{
			Log(LOG_INFO) << ". . try rifle Or do blaster Action";
			if (_attackAction->type != BA_THINK || findFirePosition() == true)
			{
				Log(LOG_INFO) << ". . . NOT Think Or findFirePosition() TRUE - ret COMBAT";
				return;
			}
		}
		else if (selectTarget() == true && findFirePosition() == true)
		{
			Log(LOG_INFO) << ". . selectTarget() TRUE And findFirePosition() TRUE - ret COMBAT";
			return;
		}

		Log(LOG_INFO) << ". fallback AMBUSH";
		_AIMode = AI_AMBUSH;
	}

	if (_AIMode == AI_AMBUSH && _tuAmbush == -1)
	{
		Log(LOG_INFO) << ". fallback ESCAPE";
		_AIMode = AI_ESCAPE;
	}

	if (_AIMode == AI_ESCAPE && _tuEscape == -1)
	{
		Log(LOG_INFO) << ". fallback PATROL -> Error: Nothing To Do.";
		_AIMode = AI_PATROL;
	}
	Log(LOG_INFO) << " Final decision = " << BattleAIState::debugAiMode(_AIMode);
}

/**
 * Counts exposed targets Player and neutral.
 * @return, quantity of targets
 */
int AlienBAIState::tallyTargets() const // private.
{
	int ret (0);
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true)
			++ret;
	}
	return ret;
}

/**
 * Counts Player units that spot a position.
 * @param pos - reference to a Position to check
 * @return, qty of spotters
 */
int AlienBAIState::tallySpotters(const Position& pos) const // private.
{
	int ret (0);
	Position
		originVoxel,
		targetVoxel;

	const BattleUnit* hypoUnit;
	if (pos != _unit->getPosition())
		hypoUnit = _unit;
	else
		hypoUnit = nullptr;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i) == true
			&& TileEngine::distSqr(pos, (*i)->getPosition()) <= TileEngine::SIGHTDIST_TSp_Sqr) // Could use checkViewSector() and/or visible()
		{
			originVoxel = _te->getSightOriginVoxel(*i);
			if (_te->canTargetUnit(
								&originVoxel,
								_battleSave->getTile(pos),
								&targetVoxel,
								*i,
								hypoUnit) == true)
			{
				++ret;
			}
		}
	}
	return ret;
}

/**
 * Selects the nearest exposed AND visible conscious BattleUnit (Player or
 * neutral) that can be shot at or eaten and returns the total.
 * @note This function includes civilians as viable targets.
 * @return, viable targets
 */
int AlienBAIState::selectNearestTarget() // private.
{
	_unitAggro = nullptr;
	_distClosest = 1000;

	Position
		origin,
		target;
	int
		ret (0),
		distTest;
//		dir;

//	BattleAction action;
	bool canTarget;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true
			&& _te->visible(_unit, (*i)->getTile()) == true)
		{
			++ret;
			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < _distClosest)
			{
				canTarget = false;
				if (_melee == true && _rifle == false)
					canTarget = findMeleePosition(*i, _unit->getTimeUnits());
				else
				{
					origin = _te->getSightOriginVoxel(_unit);
					canTarget = _te->canTargetUnit(
											&origin,
											(*i)->getTile(),
											&target,
											_unit);
				}
/*				if (_rifle == true || _melee == false) // -> is ambiguity like that required.
				{
					origin = _te->getOriginVoxel(action);
					canTarget = _te->canTargetUnit(
											&origin,
											(*i)->getTile(),
											&target,
											_unit);
				}
				else if (findMeleePosition(*i, _unit->getTimeUnits()) == true)
				{
					dir = TileEngine::getDirectionTo(
												_attackAction->target,
												(*i)->getPosition());
					canTarget = _te->validMeleeRange( // this appears to be done already in findMeleePosition() ...
												_attackAction->target,
												dir,
												_unit,
												*i);
				} */

				if (canTarget == true)
				{
					_distClosest = distTest;
					_unitAggro = *i;
				}
			}
		}
	}

	if (_unitAggro != nullptr)
		return ret;

	return 0;
}

/**
 * Selects the closest exposed Player unit.
 * @return, true if found
 */
bool AlienBAIState::selectPlayerTarget() // private.
{
	_unitAggro = nullptr;
	int
		dist (1000000),
		distTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true) == true)
		{
			distTest = TileEngine::distSqr(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < dist)
			{
				dist = distTest;
				_unitAggro = *i;
			}
		}
	}

	return (_unitAggro != nullptr);
}

/**
 * Selects an exposed Player or neutral unit.
 * @return, true if found
 */
bool AlienBAIState::selectTarget() // private.
{
	_unitAggro = nullptr;
	int
		dist (-1000000),
		distTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true)
		{
			distTest = RNG::generate(0,400);
			distTest -= TileEngine::distSqr(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest > dist)
			{
				dist = distTest;
				_unitAggro = *i;
			}
		}
	}

	return (_unitAggro != nullptr);
}

/**
 * Selects a position where a target can be targetted and initiates BA_MOVE.
 * @note Checks an 11x11 grid for a position nearby from which Actor can target.
 * @return, true if found
 */
bool AlienBAIState::findFirePosition() // private.
{
	if (selectPlayerTarget() == true) // sets _unitAggro.
	{
		_attackAction->type = BA_THINK;

		Position
			originVoxel,
			targetVoxel,
			pos;
		const Tile* tile;

		_pf->setPathingUnit(_unit);

		int
			score (0),
			scoreTest;

		std::vector<Position> tileSearch (_battleSave->getTileSearch());
		RNG::shuffle(tileSearch.begin(), tileSearch.end());

		for (std::vector<Position>::const_iterator
				i = tileSearch.begin();
				i != tileSearch.end();
				++i)
		{
			pos = _unit->getPosition() + *i;
			if ((tile = _battleSave->getTile(pos)) != nullptr
				&& std::find(
						_reachableAttack.begin(),
						_reachableAttack.end(),
						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
			{
				originVoxel = _te->getSightOriginVoxel(_unit, &pos);
				if (_battleSave->getTileEngine()->canTargetUnit(
															&originVoxel,
															_unitAggro->getTile(),
															&targetVoxel,
															_unit) == true)
				{
					_pf->calculate(_unit, pos);
					if (_pf->getStartDirection() != -1) // && _pf->getTuCostTotalPf() <= _unit->getTimeUnits()
					{
						scoreTest = BASE_SUCCESS_SYSTEMATIC - tallySpotters(pos) * EXPOSURE_PENALTY;
						scoreTest += _unit->getTimeUnits() - _pf->getTuCostTotalPf();

						if (_unitAggro->checkViewSector(pos) == false)
							scoreTest += 10;

						if (scoreTest > score)
						{
							score = scoreTest;
							_attackAction->target = pos;
							_attackAction->finalFacing = TileEngine::getDirectionTo(
																				pos,
																				_unitAggro->getPosition());

							if (score > FAST_PASS_THRESHOLD + 25)
								break;
						}
					}
				}
			}
		}

		if (score > BASE_SUCCESS)
		{
			//if (_traceAI) Log(LOG_INFO) << "Firepoint found at " << _attackAction->target << " with a score of " << score;
//			if (_unit->getPosition() != _attackAction->target)
			_attackAction->type = BA_MOVE;
//			else chooseFireMethod() & setup the action for handleUnitAI()

			return true;
		}
	}

	//if (_traceAI) Log(LOG_INFO) << "Firepoint failed: best estimation was " << _attackAction->target << " with a score of " << score;
	return false;
}

/**
 * Selects a point near enough to a BattleUnit to perform a melee attack.
 * @param targetUnit	- pointer to a target BattleUnit
 * @param maxTuCost		- maximum time units that the path can cost
 * @return, true if a point was found
 */
bool AlienBAIState::findMeleePosition( // private.
		const BattleUnit* const targetUnit,
		int maxTuCost) const
{
	bool ret (false);

	const int
		actorSize (_unit->getArmor()->getSize()),
		targetSize (targetUnit->getArmor()->getSize());
	size_t dist (1000);

	_pf->setPathingUnit(_unit);

	Position pos;
	int dir;

	for (int
			z = -1;
			z != 2;
			++z)
	{
		for (int
				x = -actorSize;
				x <= targetSize;
				++x)
		{
			for (int
					y = -actorSize;
					y <= targetSize;
					++y)
			{
				if (x != 0 || y != 0) // skip the unit itself
				{
					pos = targetUnit->getPosition() + Position(x,y,z);
					if (_battleSave->getTile(pos) != nullptr
						&& std::find(
								_reachable.begin(),
								_reachable.end(),
								_battleSave->getTileIndex(pos)) != _reachable.end())
					{
						dir = TileEngine::getDirectionTo(pos, targetUnit->getPosition());
						if (_te->validMeleeRange(pos, dir, _unit, targetUnit) == true
							&& _battleSave->setUnitPosition(_unit, pos, true) == true
							&& _battleSave->getTile(pos)->getDangerous() == false)
						{
							_pf->calculate(_unit, pos, maxTuCost);
							if (_pf->getStartDirection() != -1 && _pf->getPath().size() < dist)
							{
								ret = true;
								dist = _pf->getPath().size();
								_attackAction->target = pos;
							}
							_pf->abortPath();
						}
					}
				}
			}
		}
	}
	return ret;
}

/**
 * Tries to setup a melee attack/charge.
 */
void AlienBAIState::meleeAction() // private.
{
//	if (_unit->getTimeUnits() < _unit->getActionTu(BA_MELEE, _unit->getMeleeWeapon()))
//		return;

	int dir;

	if (_unitAggro != nullptr
		&& _unitAggro->isOut_t(OUT_STAT) == false)
	{
		dir = TileEngine::getDirectionTo(
									_unit->getPosition(),
									_unitAggro->getPosition());
		if (_te->validMeleeRange(_unit, dir, _unitAggro) == true)
		{
			faceMelee();
			return;
		}
	}

	const int tuReserve (_unit->getTimeUnits()
					   - _unit->getActionTu(
										BA_MELEE,
										_attackAction->weapon));
	int
		dist (tuReserve / 4 + 1),
		distTest;

	_unitAggro = nullptr;

	// TODO: set up a vector of BattleUnits to pick from
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true)
		{
			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest == 1
				|| (distTest < dist && findMeleePosition(*i, tuReserve) == true))
			{
				dist = distTest;

				_unitAggro = *i;
				_attackAction->type = BA_MOVE;
				_unit->setChargeTarget(_unitAggro);
			}
		}
	}

	if (_unitAggro != nullptr)
	{
		//if (_traceAI) { Log(LOG_INFO) << "AlienBAIState::meleeAction: [target]: " << (_unitAggro->getId()) << " at: "  << _attackAction->target; }
		dir = TileEngine::getDirectionTo(
									_unit->getPosition(),
									_unitAggro->getPosition());
		if (_te->validMeleeRange(_unit, dir, _unitAggro) == true)
			faceMelee();
	}
}

/**
 * Finishes setting up a melee attack/charge.
 */
void AlienBAIState::faceMelee() // private.
{
	_unit->setDirectionTo(
			_unitAggro->getPosition() + Position(
											_unit->getArmor()->getSize() - 1,
											_unit->getArmor()->getSize() - 1,
											0));

	while (_unit->getUnitStatus() == STATUS_TURNING)
		_unit->turn();

	//if (_traceAI) Log(LOG_INFO) << "Attack unit: " << _unitAggro->getId();
	_attackAction->target = _unitAggro->getPosition();
	_attackAction->type = BA_MELEE;
}

/**
 * Tries to trace a waypoint projectile.
 * @return, true if blaster
 */
bool AlienBAIState::wayPointAction() // private.
{
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "AlienBAIState::wayPointAction() id-" << _unit->getId() << " w/ " << _attackAction->weapon->getRules()->getType();
	_attackAction->TU = _unit->getActionTu(
										BA_LAUNCH,
										_attackAction->weapon);
	Log(LOG_INFO) << ". actionTU = " << _attackAction->TU;
	Log(LOG_INFO) << ". unitTU = " << _unit->getTimeUnits();

	if (_attackAction->TU <= _unit->getTimeUnits())
	{
		_pf->setPathingUnit(_unit); // jic.

		std::vector<BattleUnit*> targets;
		const int explRadius (_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius());

		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			Log(LOG_INFO) << ". . test Vs unit id-" << (*i)->getId() << " pos " << (*i)->getPosition();
			if (validTarget(*i, true, true) == true)
			{
				Log(LOG_INFO) << ". . . unit VALID";
				if (explosiveEfficacy(
								(*i)->getPosition(),
								_unit,
								explRadius,
								_attackAction->diff) == true)
				{
					Log(LOG_INFO) << ". . . . explEff VALID";
					if (pathWaypoints(*i) == true)
						targets.push_back(*i);
				}
				else Log(LOG_INFO) << ". . . . explEff invalid";

				_pf->abortPath();
			}
		}

		if (targets.empty() == false)
		{
			Log(LOG_INFO) << ". targets available";
			BattleUnit* const saladhead (targets.at(RNG::pick(targets.size())));
			Log(LOG_INFO) << ". . total = " << targets.size() << " Target id-" << saladhead->getId();
			if (pathWaypoints(saladhead) == true) // safety.
			{
				Log(LOG_INFO) << ". . . Return, do LAUNCH";
				Log(LOG_INFO) << "";
				_unitAggro = saladhead;
				_attackAction->type = BA_LAUNCH;
				_attackAction->target = _attackAction->waypoints.front();
				return true;
			}
		}
	}

	_blaster = false;
	_attackAction->type = BA_THINK;
	_attackAction->waypoints.clear(); // tidy.
	Log(LOG_INFO) << ". waypoint action FAILED - Think !";
	Log(LOG_INFO) << "";
	return false;
}

/**
 * Constructs a waypoint path for a guided projectile.
 * @note Helper for wayPointAction().
 * @param unit - pointer to a BattleUnit to path at
 * @return, true if waypoints get positioned
 */
bool AlienBAIState::pathWaypoints(const BattleUnit* const unit) // private.
{
	Log(LOG_INFO) << "";
	Log(LOG_INFO) << "AlienBAIState::pathWaypoints() vs id-" << unit->getId() << " pos " << unit->getPosition();
	Log(LOG_INFO) << ". actor id-" << _unit->getId() << " pos " << _unit->getPosition();

	_pf->setPathingUnit(_unit);
	_pf->calculate(
				_unit,
				unit->getPosition(),
				-1,
				unit);
	int dir (_pf->dequeuePath());

	if (dir != -1)
	{
		_attackAction->waypoints.clear();

		Position
			pos (_unit->getPosition()),
			vect;
		int dir2;

		while (dir != -1)
		{
			dir2 = dir;
			while (dir != -1 && dir == dir2)
			{
				Pathfinding::directionToVector(dir, &vect);
				pos += vect; // step along path one tile
				dir = _pf->dequeuePath();
			}
			// dir changed:
			_attackAction->waypoints.push_back(pos); // place wp. Auto-explodes at last wp. Or when it hits anything, lulz.
			Log(LOG_INFO) << ". . place WP " << pos;
		}

		// pathing done & wp's have been positioned:
		Log(LOG_INFO) << ". . qty WP's = " << _attackAction->waypoints.size() << " / max WP's = " << _attackAction->weapon->getRules()->isWaypoints();
		if (_attackAction->waypoints.size() != 0
			&& _attackAction->waypoints.size() <= _attackAction->weapon->getRules()->isWaypoints())
		{
			Log(LOG_INFO) << ". path valid, ret TRUE";
			return true;
		}
		Log(LOG_INFO) << ". . too many WP's !!";
	}

	Log(LOG_INFO) << ". path or WP's invalid, ret FALSE";
	Log(LOG_INFO) << "";
	return false;
}

/**
 * Tries to setup a shot.
 */
void AlienBAIState::rifleAction() // private.
{
	_attackAction->target = _unitAggro->getPosition();

	if (_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius() < 1
		|| explosiveEfficacy(
						_unitAggro->getPosition(),
						_unit,
						_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius(),
						_attackAction->diff) == true)
	{
		chooseFireMethod();
	}
}

/**
 * Selects a fire method based on range, time units, and time units reserved for
 * cover.
 */
void AlienBAIState::chooseFireMethod() // private.
{
	_attackAction->type = BA_THINK;

	int tuReserve (_unit->getTimeUnits());
	if (_tuEscape != -1
		&& RNG::generate(0,_unit->getAggression()) == 0)
	{
		tuReserve -= _tuEscape;
	}

	const int dist (TileEngine::distance(
									_unit->getPosition(),
									_attackAction->target));
	if (dist <= _attackAction->weapon->getRules()->getAutoRange())
	{
		if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
		else if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
	}
	else if (dist <= _attackAction->weapon->getRules()->getSnapRange())
	{
		if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
	else if (dist <= _attackAction->weapon->getRules()->getAimRange())
	{
		if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
}

/**
 * Tries to setup a grenade throw.
 * @return, true to grenade
 */
bool AlienBAIState::grenadeAction() // private.
{
	BattleItem* const grenade (_unit->getGrenade());
	if (grenade != nullptr)
	{
		if (explosiveEfficacy(
						_unitAggro->getPosition(),
						_unit,
						grenade->getRules()->getExplosionRadius(),
						_attackAction->diff) == true)
		{
			int tuCost (grenade->getInventorySection()
							->getCost(_battleSave->getBattleGame()->getRuleset()->getInventoryRule(ST_RIGHTHAND)));

			if (grenade->getFuse() == -1)
				tuCost += _unit->getActionTu(BA_PRIME, grenade);
			tuCost += _unit->getActionTu(BA_THROW, grenade); // the Prime itself is done 'auto' in ProjectileFlyBState.

			if (tuCost <= _unit->getTimeUnits())
			{
				BattleAction action;
				action.actor = _unit;
				action.target = _unitAggro->getPosition();
				action.weapon = grenade;
				action.type = BA_THROW;

				const Position
					originVoxel (_battleSave->getTileEngine()->getSightOriginVoxel(_unit)),
					targetVoxel (Position::toVoxelSpaceCentered(
															action.target,
															2 - _battleSave->getTile(action.target)->getTerrainLevel())); // LoFT of floor is typically 2 voxels thick.

				if (_battleSave->getTileEngine()->validateThrow(action, originVoxel, targetVoxel) == true)
				{
					_attackAction->target = action.target;
					_attackAction->weapon = grenade;
					_attackAction->type = BA_THROW;

					_rifle =
					_melee = false;
					return true;
				}
			}
		}
	}
	return false;
}

/**
 * Decides if it's worthwhile to create an explosion.
 * @note Also called from TileEngine::reactionShot().
 * @param pos		- reference the target's position
 * @param attacker	- pointer to the attacking unit
 * @param radius	- radius of explosion in tile space
 * @param diff		- game difficulty
// * @param grenade	- true if explosion will be from a grenade
 * @return, true if it's worthwile creating an explosion at the target position
 */
bool AlienBAIState::explosiveEfficacy(
		const Position& pos,
		const BattleUnit* const attacker,
		const int radius,
		const int diff) const
//		bool grenade) const
{
	//Log(LOG_INFO) << "\n";
	//Log(LOG_INFO) << "explosiveEfficacy() rad = " << radius;
	int pct (0);

	const int firstGrenade (_battleSave->getBattleState()->getGame()->getRuleset()->getFirstGrenade());
	if (firstGrenade == -1
		|| (firstGrenade == 0
			&& _battleSave->getTurn() > 4 - diff)
		|| (firstGrenade > 0
			&& _battleSave->getTurn() > firstGrenade - 1))
	{
		pct = (100 - attacker->getMorale()) / 3;
		pct += 2 * (10 - static_cast<int>(
						 static_cast<float>(attacker->getHealth()) / static_cast<float>(attacker->getBattleStats()->health)
						 * 10.f));
		pct += attacker->getAggression() * 10;

		const int dist (TileEngine::distance(
										attacker->getPosition(),
										pos));
		if (dist <= radius)
		{
			pct -= (radius - dist + 1) * 5;
			if (std::abs(attacker->getPosition().z - pos.z) < Options::battleExplosionHeight + 1)
				pct -= 15;
		}

		switch (_battleSave->getTacType())
		{
			case TCT_BASEASSAULT:
				pct -= 23;
				break;
			case TCT_BASEDEFENSE:
			case TCT_MISSIONSITE:
				pct += 56;
		}

		pct += diff * 2;

		const BattleUnit* const targetUnit (_battleSave->getTile(pos)->getTileUnit());

		//Log(LOG_INFO) << "attacker = " << attacker->getId();
		//Log(LOG_INFO) << "pos = " << pos;
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			//Log(LOG_INFO) << "\n";
			//Log(LOG_INFO) << ". id = " << (*i)->getId();
			//Log(LOG_INFO) << ". isNOTOut = " << ((*i)->isOut(true) == false);
			//Log(LOG_INFO) << ". isNOTAttacker = " << (*i != attacker);
			//Log(LOG_INFO) << ". isNOTtargetUnit = " << (*i != targetUnit);
			//Log(LOG_INFO) << ". vertCheck = " << (std::abs((*i)->getPosition().z - pos.z) < Options::battleExplosionHeight + 1);
			//Log(LOG_INFO) << ". inDist = " << (_battleSave->getTileEngine()->distance(pos, (*i)->getPosition()) < radius + 1);
			if ((*i)->isOut_t(OUT_HLTH) == false
				&& *i != attacker
				&& *i != targetUnit
				&& std::abs((*i)->getPosition().z - pos.z) < Options::battleExplosionHeight + 1
				&& TileEngine::distance(
									pos,
									(*i)->getPosition()) < radius + 1)
			{
				//Log(LOG_INFO) << ". . dangerousFALSE = " << ((*i)->getTile() != nullptr && (*i)->getTile()->getDangerous() == false);
				//Log(LOG_INFO) << ". . exposed = " << ((*i)->getFaction() == FACTION_HOSTILE || (*i)->getExposed() < _unit->getIntelligence() + 1);
				if ((*i)->getTile() != nullptr
					&& (*i)->getTile()->getDangerous() == false
					&& ((*i)->getFaction() == FACTION_HOSTILE
						|| ((*i)->getExposed() != -1
							&& (*i)->getExposed() <= _unit->getIntelligence())))
				{
					const Position
						voxelPosA (Position::toVoxelSpaceCentered(pos, 12)),
						voxelPosB (Position::toVoxelSpaceCentered((*i)->getPosition(), 12));

					std::vector<Position> trajectory;
					const VoxelType impact (_battleSave->getTileEngine()->plotLine(
																				voxelPosA,
																				voxelPosB,
																				false,
																				&trajectory,
																				targetUnit,
																				true,
																				false,
																				*i));
					//Log(LOG_INFO) << "trajSize = " << (int)trajectory.size() << "; impact = " << impact;
					if (impact == VOXEL_UNIT
						&& (*i)->getPosition() == Position::toTileSpace(trajectory.front()))
					{
						//Log(LOG_INFO) << "trajFront " << (trajectory.front() / Position(16,16,24));
						if ((*i)->getFaction() != FACTION_HOSTILE)
							pct += 12;

						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							pct -= 6;
							if ((*i)->getFaction() == FACTION_HOSTILE)
								pct -= 12;
						}
					}
				}
			}
		}
	}

	return (RNG::percent(pct) == true);
}

/**
 * Tries to setup a psionic attack.
 * @note Psionic targetting: pick from any of the exposed units. Exposed means
 * they have been previously spotted and are therefore known to the AI
 * regardless of whether they can be seen or not because they're psycho.
 * @return, true if a psionic attack should be performed
 */
bool AlienBAIState::psiAction() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::psiAction() ID = " << _unit->getId();
	if (_unit->getBattleStats()->psiSkill != 0
		&& _hasPsiBeenSet == false
		&& _unit->isMindControlled() == false)
	{
		const RuleItem* const itRule (_battleSave->getBattleGame()->getAlienPsi()->getRules()); //getRuleset()->getItem("ALIEN_PSI_WEAPON"));

		int tuCost (_unit->getActionTu(BA_PSIPANIC, itRule));
		if (_tuEscape != -1)
			tuCost += _tuEscape; // check if aLien has the required TUs and can still make it to cover
		//Log(LOG_INFO) << "AlienBAIState::psiAction() tuCost = " << tuCost;
		if (_unit->getTimeUnits() >= tuCost)
		{
			const int attack (static_cast<int>(static_cast<float>(
							 _unit->getBattleStats()->psiStrength * _unit->getBattleStats()->psiSkill) / 50.f));
			//Log(LOG_INFO) << ". . attack = " << attack;

			BattleUnit* unitTarget (nullptr);
			int
				defense,
				dist,
				los,
				losTest,
				choice (0),
				choiceTest;
			Position
				origin,
				target;

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getGeoscapeSoldier() != nullptr	// what about doggies .... Should use isFearable() for doggies ....
					&& validTarget(*i, true) == true		// will check for Mc, Exposed, etc.
					&& (itRule->isLosRequired() == false
						|| std::find(
								_unit->getHostileUnits().begin(),
								_unit->getHostileUnits().end(),
								*i) != _unit->getHostileUnits().end()))
				{
					// is this gonna crash..................
					defense = (*i)->getBattleStats()->psiStrength; // stupid aLiens don't know soldier's psiSkill tho
					dist = TileEngine::distance(
										(*i)->getPosition(),
										_unit->getPosition()) * 2;
					origin = _battleSave->getTileEngine()->getSightOriginVoxel(_unit);
					losTest = static_cast<int>(_battleSave->getTileEngine()->canTargetUnit(
																						&origin,
																						(*i)->getTile(),
																						&target,
																						_unit)) * PSI_LOS_WEIGHT;
					//Log(LOG_INFO) << ". . . ";
					//Log(LOG_INFO) << ". . . targetID = " << (*i)->getId();
					//Log(LOG_INFO) << ". . . defense = " << defense;
					//Log(LOG_INFO) << ". . . dist = " << dist;
					//Log(LOG_INFO) << ". . . currentMood = " << currentMood;
					//Log(LOG_INFO) << ". . . hasSight = " << hasSight;

					choiceTest = attack // Note that this is NOT a true calculation of Success.
							   - defense
							   - dist
							   + losTest
							   + RNG::generate(0, PSI_SWITCH_TARGET);
					//Log(LOG_INFO) << ". . . choiceTest = " << choiceTest;

					if (choiceTest > choice || unitTarget == nullptr)
					{
						choice = choiceTest;
						los = losTest;
						unitTarget = *i;
					}
				}
			}

			if (unitTarget != nullptr && choice - los > PSI_CUTOFF)
			{
/*				if (_targetsVisible
					&& _attackAction->weapon
					&& _attackAction->weapon->getAmmoItem())
				{
					if (_attackAction->weapon->getAmmoItem()->getRules()->getPower() > chance)
					{
						//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 2, False";
						return false;
					}
				}
				else if (RNG::generate(35, 155) > chance)
				{
					//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 3, False";
					return false;
				} */

				//if (_traceAI) Log(LOG_INFO) << "making a psionic attack this turn";

				_unitAggro = unitTarget;
				_psiAction->target = unitTarget->getPosition();

				const int morale (unitTarget->getMorale());
				if (morale > 0)		// panicAtk is valid since target has morale to chew away
//					&& choice < 30)	// esp. if aLien atkStr is low
				{
					//Log(LOG_INFO) << ". . test if MC or Panic";
					const int bravery (unitTarget->getBattleStats()->bravery);
					int panicOdds (110 - bravery); // ie, moraleHit
					const int moraleResult (morale - panicOdds);
					//Log(LOG_INFO) << ". . panicOdds_1 = " << panicOdds;

					if (moraleResult < 0)
						panicOdds -= bravery / 2;
					else if (moraleResult < 50)
						panicOdds -= bravery;
					else
						panicOdds -= bravery * 2;

					//Log(LOG_INFO) << ". . panicOdds_2 = " << panicOdds;
					panicOdds += (RNG::generate(51,100) - (attack / 5));
					//Log(LOG_INFO) << ". . panicOdds_3 = " << panicOdds;
					if (RNG::percent(panicOdds) == true)
					{
						//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do Panic vs " << unitTarget->getId();
						_psiAction->type = BA_PSIPANIC;
						return true;
					}
				}

				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do MindControl vs " << unitTarget->getId();
				_psiAction->type = BA_PSICONTROL;
				return true;
			}
		}
	}

	//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False";
	return false;
}

/**
 * Validates a target.
 * @param unit			- pointer to a target to validate
 * @param dangerTile	- true to care if target has already been grenaded (default false)
 * @param includeCivs	- true to include civilians in the threat assessment (default false)
 * @return, true if the target is something to be killed
 */
bool AlienBAIState::validTarget( // private.
		const BattleUnit* const unit,
		bool dangerTile,
		bool includeCivs) const
{
	if ((unit->getFaction() == FACTION_PLAYER				// target must not be on aLien side
			|| (unit->getFaction() == FACTION_NEUTRAL
				&& includeCivs == true))
		&& unit->isOut_t(OUT_STAT) == false					// ignore targets that are dead/unconscious
		&& unit->getExposed() != -1
		&& unit->getExposed() <= _unit->getIntelligence()	// target must be a unit that this aLien 'knows about'
		&& (unit->getTile()->getDangerous() == false
			|| dangerTile == false))						// target has not been grenaded
	{
		return true;
	}
	return false;
}

/**
 * aLien has a dichotomy on its hands: has a ranged weapon as well as melee
 * ability ... so make a determination on which to use this round.
 */
void AlienBAIState::chooseMeleeOrRanged() // private.
{
	const BattleItem* const weapon (_unit->getRangedWeapon(false));
	if (weapon == nullptr) // safety.
	{
		_rifle = false;
		return;
	}

	if (_unit->getHealth() > _unit->getBattleStats()->health * 2 / 3) // is over 2/3 health
	{
		const RuleItem* const itRule (_unit->getMeleeWeapon()->getRules());
		if (itRule != nullptr)
		{
			int meleeOdds (10);

			int power (itRule->getPower());
			if (itRule->isStrengthApplied() == true)
				power += _unit->getStrength();	// note that power of an actual hit is +str halved - ExplosionBState::init().
												// TODO: Do a comparison of rifle-damaqe vs. melee-damage (* acu / 100).
			power = static_cast<int>(Round(
					static_cast<float>(power) * _unitAggro->getArmor()->getDamageModifier(itRule->getDamageType())));

			if (power > 50)
				meleeOdds += (power - 50) / 2;

			if (_targetsVisible > 1)
				meleeOdds -= 15 * (_targetsVisible - 1);

			if (meleeOdds > 0)
			{
				if (_unit->getAggression() == 0)
					meleeOdds -= 20;
				else if (_unit->getAggression() > 1)
					meleeOdds += 10 * _unit->getAggression();

				if (RNG::percent(meleeOdds) == true)
				{
					_rifle = false;
					const int tuReserve (_unit->getTimeUnits()
									   - _unit->getActionTu(BA_MELEE, itRule));
					_pf->setPathingUnit(_unit);
					_reachableAttack = _pf->findReachable(_unit, tuReserve);
					return;
				}
			}
		}
	}

	_melee = false;
}

/**
 * Gets the TU reservation setting.
 * @return, the reserved BattleActionType (BattlescapeGame.h)
 */
BattleActionType AlienBAIState::getReservedAiAction() const
{
	return _reserve;
}

}

/**
 * Enters the current AI state.
 *
void AlienBAIState::enter()
{
	Log(LOG_INFO) << "AlienBAIState::enter() ROOOAARR !";
} */

/**
 * Exits the current AI state.
 *
void AlienBAIState::exit()
{
	Log(LOG_INFO) << "AlienBAIState::exit()";
} */

/**
 * Gets the currently targeted unit.
 * @return, pointer to a BattleUnit
 *
BattleUnit* AlienBAIState::getTarget()
{
	return _unitAggro;
}*/

/**
 * Checks nearby nodes to see if they'd make good grenade targets.
 * @param action - pointer to BattleAction struct;
 * contents details one weapon and user and sets the target
 * @return, true if a viable node was found
 *
bool AlienBAIState::getNodeOfBestEfficacy(BattleAction* action)
{
	if (_battleSave->getTurn() < 3) // <- note.
		return false;

	int bestScore = 2;

	Position
		originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(_unit),
		targetVoxel;

	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		int dist = _battleSave->getTileEngine()->distance(
													(*i)->getPosition(),
													_unit->getPosition());
		if (dist < 21
			&& dist > action->weapon->getRules()->getExplosionRadius()
			&& _battleSave->getTileEngine()->canTargetTilepart(
															&originVoxel,
															_battleSave->getTile((*i)->getPosition()),
															O_FLOOR,
															&targetVoxel,
															_unit))
		{
			int nodePoints = 0;

			for (std::vector<BattleUnit*>::const_iterator
					j = _battleSave->getUnits()->begin();
					j != _battleSave->getUnits()->end();
					++j)
			{
				dist = _battleSave->getTileEngine()->distance(
														(*i)->getPosition(),
														(*j)->getPosition());
				if ((*j)->isOut() == false
					&& dist < action->weapon->getRules()->getExplosionRadius())
				{
					Position targetOriginVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(*j);
					if (_battleSave->getTileEngine()->canTargetTilepart(
																	&targetOriginVoxel,
																	_battleSave->getTile((*i)->getPosition()),
																	O_FLOOR,
																	&targetVoxel,
																	*j))
					{
						if ((*j)->getFaction() != FACTION_HOSTILE)
						{
							if ((*j)->getExposed() <= _unit->getIntelligence())
								++nodePoints;
						}
						else
							nodePoints -= 2;
					}
				}
			}

			if (nodePoints > bestScore)
			{
				bestScore = nodePoints;
				action->target = (*i)->getPosition();
			}
		}
	}

	return (bestScore > 2);
} */

/**
 * Sets the "was hit" flag to true.
 * @param attacker - pointer to a BattleUnit
 *
void AlienBAIState::setWasHitBy(BattleUnit* attacker)
{
	if (attacker->getFaction() != _unit->getFaction() && !getWasHitBy(attacker->getId()))
		_wasHitBy.push_back(attacker->getId());
} */

/**
 * Gets whether the unit was hit.
 * @return, true if unit was hit
 *
bool AlienBAIState::getWasHitBy(int attacker) const
{
	return std::find(_wasHitBy.begin(), _wasHitBy.end(), attacker) != _wasHitBy.end();
} */
