/*
 * Copyright 2010-2018 OpenXcom Developers.
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
		_hasRifle(false), // TODO: enum AIWeaponType ...
		_hasMelee(false),
		_hasBlaster(false),
		_doGrenade(false),
		_distClosest(CAP_DIST),
		_reserve(BA_NONE)
{
	//if (_unit->getId() != 1000006) _traceAI = 0;
	//Log(LOG_INFO) << "Create AlienBAIState traceAI= " << _traceAI;

	switch (_unit->getOriginalFaction())
	{
		case FACTION_HOSTILE:
			_aggression = _unit->getAggression();
			break;
		case FACTION_PLAYER:
		case FACTION_NEUTRAL:
			_aggression = 10; // ie. the penalty brigade.
	}

	_escapeAction	= new BattleAction();
	_patrolAction	= new BattleAction();
	_ambushAction	= new BattleAction();
	_attackAction	= new BattleAction();
	_psiAction		= new BattleAction();

	_escapeAction->actor =
	_patrolAction->actor =
	_ambushAction->actor =
	_attackAction->actor =
	_psiAction   ->actor = _unit; // NOTE: Not really used.
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
 * Loads the AI-state from a YAML file.
 * @param node - reference a YAML node
 */
void AlienBAIState::load(const YAML::Node& node)
{
	BattleAIState::load(node);
//	_wasHitBy = node["wasHitBy"].as<std::vector<int>>(_wasHitBy);
}

/**
 * Saves the AI-state to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBAIState::save() const
{
	return BattleAIState::save();
//	node["wasHitBy"] = _wasHitBy;
}

/**
 * Runs any code the state needs to keep updating every AI-cycle.
 * @param aiAction - pointer to a BattleAction to fill w/ data (BattlescapeGame.h)
 */
void AlienBAIState::thinkAi(BattleAction* const aiAction)
{
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "AlienBAIState::think(), id-" << _unit->getId() << " pos" << _unit->getPosition();
		Log(LOG_INFO) << ". agression= " << _aggression;
	}

	if (_unit->getChargeTarget() != nullptr
		&& _unit->getChargeTarget()->isOut_t(OUT_STAT) == true)
	{
		_unit->setChargeTarget();
	}
//	_wasHitBy.clear();

	_hasBlaster =
	_hasRifle   =
	_doGrenade  = false;

	_psiAction->type = BA_NONE;

	_hasMelee = (_unit->getMeleeWeapon() != nullptr);

	aiAction->weapon = _unit->getMainHandWeapon();	// will get Rifle OR Melee
//	if (aiAction->weapon == nullptr)
//		aiAction->weapon = _unit->getMeleeWeapon();	// will get Melee OR Fist

	_attackAction->weapon = aiAction->weapon;
	_attackAction->diff = _battleSave->getSavedGame()->getDifficultyInt(); // for grenade-efficacy and blaster-waypoints.

	_spottersOrigin = tallySpotters(_unit->getPosition());
	_targetsExposed = tallyTargets();
	_targetsVisible = selectNearestTarget(); // sets _unitAggro.

	if (_traceAI) {
		Log(LOG_INFO) << "_spottersOrigin = " << _spottersOrigin;
		Log(LOG_INFO) << "_targetsExposed = " << _targetsExposed;
		Log(LOG_INFO) << "_targetsVisible = " << _targetsVisible;
		Log(LOG_INFO) << "_AIMode = " << BattleAIState::debugAiMode(_AIMode);
	}

//	_pf->setPathingUnit(_unit);
//	_reachable = _pf->findReachable(_unit, _unit->getTu()); // done in BattlescapeGame::handleUnitAi().
//	if (_traceAI) {
//		Log(LOG_INFO) << ". reachable IDs";
//		int x,y,z;
//		for (std::vector<size_t>::const_iterator
//				i = _reachable.begin();
//				i != _reachable.end();
//				++i)
//		{
//			_battleSave->tileCoords(*i, &x,&y,&z);
//			Log(LOG_INFO) << ". . " << (*i)
//						  << "\t(" << x << "," << y << "," << z << ")";
//		}
//	}

	int tuReserve (-1);
	if (aiAction->weapon != nullptr)
	{
		const RuleItem* const itRule (aiAction->weapon->getRules());
		if (_traceAI) Log(LOG_INFO) << ". weapon " << itRule->getType();

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
				if (_traceAI) Log(LOG_INFO) << ". . weapon is Firearm";
				if (itRule->isWaypoints() != 0
					&& _targetsExposed > _targetsVisible) // else let BL fallback to aimed-shot
				{
					if (_traceAI) Log(LOG_INFO) << ". . . blaster TRUE";
					_hasBlaster = true;
					tuReserve = _unit->getTu()
							  - _unit->getActionTu(BA_LAUNCH, aiAction->weapon);
				}
				else
				{
					if (_traceAI) Log(LOG_INFO) << ". . . rifle TRUE";
					_hasRifle = true;
					tuReserve = _unit->getTu()
							  - _unit->getActionTu(
												itRule->getDefaultAction(), // note: this needs chooseFireMethod() ...
												aiAction->weapon);
				}
				break;

			case BT_MELEE:
				if (_traceAI) Log(LOG_INFO) << ". . melee TRUE";
				_hasMelee = true;
				tuReserve = _unit->getTu()
						  - _unit->getActionTu(BA_MELEE, aiAction->weapon);
				break;

//			case BT_GRENADE:
//				Log(LOG_INFO) << ". . grenade TRUE";
//				_doGrenade = true; // <- getMainHandWeapon() does not return grenades.
		}
	}
	else if (_traceAI) Log(LOG_INFO) << ". weapon is NULL";
//	else if () // kL_add -> Give the invisible 'meleeWeapon' param a try ....
//	{}

	if (tuReserve > -1)
		_reachableAttack = _pf->findReachable(_unit, tuReserve); // TODO: Let aLiens turn-to-shoot w/out extra Tu.
	else
		_reachableAttack.clear();


	// NOTE: These setups could have an order: Escape, Ambush, Attack, Patrol.

	if (_traceAI) Log(LOG_INFO) << ". setupAttack()";
	setupAttack();
	if (_traceAI) Log(LOG_INFO) << "";

	switch (_psiAction->type) // if a psi-action was determined by setupAttack() just do it ->
	{
		case BA_PSIPANIC:
		case BA_PSICONTROL:
			_unit->psiTried(_unit->psiTried() + 1);

			_battleSave->getBattleGame()->decAiActionCount();

			aiAction->type      = _psiAction->type;
			aiAction->posTarget = _psiAction->posTarget;

			if (_traceAI) Log(LOG_INFO) << "AlienBAIState::think() EXIT, Psi";
			return;

//		case BA_NONE: break;
	}


	if (_traceAI) Log(LOG_INFO) << ". setupPatrol()";
	setupPatrol();
	if (_traceAI) Log(LOG_INFO) << "";

	if (_targetsExposed != 0 && _tuAmbush == -1 && _hasMelee == false)
	{
		if (_traceAI) Log(LOG_INFO) << ". setupAmbush()";
		setupAmbush();
		if (_traceAI) Log(LOG_INFO) << "";
	}

	if (_spottersOrigin != 0 && _tuEscape == -1)
	{
		if (_traceAI) Log(LOG_INFO) << ". setupEscape()";
		setupEscape();
		if (_traceAI) Log(LOG_INFO) << "";
	}


	if (_traceAI) Log(LOG_INFO) << ". evaluate [1] " << BattleAIState::debugAiMode(_AIMode);
	bool evaluate;
	switch (_AIMode)
	{
		default: // avoid g++ compiler warning.
		case AI_PATROL:
			evaluate = _spottersOrigin != 0
					|| _targetsVisible != 0
					|| _targetsExposed != 0
					|| RNG::percent(10);
			break;

		case AI_COMBAT:
			evaluate = _attackAction->type == BA_THINK;
			break;

		case AI_AMBUSH:
			evaluate = _hasRifle == false
					|| _tuAmbush == -1
					|| _targetsVisible != 0;
			break;

		case AI_ESCAPE:
			evaluate = _spottersOrigin == 0
					|| _targetsExposed == 0;
	}

	if (_traceAI) Log(LOG_INFO) << ". do Evaluate = " << evaluate;
	if (evaluate == true
		|| (_spottersOrigin > 1
			|| _unit->getHealth() < (_unit->getBattleStats()->health << 1u) / 3
			|| (_unitAggro != nullptr
				&& (_unitAggro->getExposed() == -1
					|| _unitAggro->getExposed() > _unit->getIntelligence()))
			|| (_battleSave->isCheating() == true
				&& _AIMode != AI_COMBAT)))
	{
		if (_traceAI) Log(LOG_INFO) << ". . AIMode pre-Evaluate = " << BattleAIState::debugAiMode(_AIMode);
		evaluateAiMode();
		if (_traceAI) Log(LOG_INFO) << ". . AIMode post-Evaluate = " << BattleAIState::debugAiMode(_AIMode);
	}
	if (_traceAI) Log(LOG_INFO) << ". evaluate [2] " << BattleAIState::debugAiMode(_AIMode);

	switch (_AIMode)
	{
		case AI_PATROL:
			_unit->setChargeTarget();

			if (aiAction->weapon != nullptr
				&& aiAction->weapon->getRules()->getBattleType() == BT_FIREARM)
			{
				switch (_aggression)
				{
					case 0:  _reserve = BA_AIMEDSHOT; break;
					case 1:  _reserve = BA_AUTOSHOT;  break;
					case 2:
					default: _reserve = BA_SNAPSHOT;
				}
			}
			else
				_reserve = BA_NONE;

			aiAction->type		= _patrolAction->type;
			aiAction->posTarget	= _patrolAction->posTarget;
//			aiAction->firstTU	= _patrolAction->firstTU;
			if (_traceAI) Log(LOG_INFO) << ". . ActionType = " << BattleAction::debugBat(aiAction->type);
			break;

		case AI_COMBAT:
			_reserve = BA_NONE;

			aiAction->type			= _attackAction->type;
			aiAction->posTarget		= _attackAction->posTarget;
//			aiAction->firstTU		= _attackAction->firstTU;
			aiAction->weapon		= _attackAction->weapon;
			aiAction->finalFacing	= _attackAction->finalFacing;
			aiAction->TU			= _unit->getActionTu(
													aiAction->type,
													aiAction->weapon);

			if (_traceAI) Log(LOG_INFO) << ". . ActionType = " << BattleAction::debugBat(aiAction->type);
			switch (aiAction->type)
			{
				case BA_THROW:
					if (aiAction->weapon != nullptr // TODO: Ensure this was done already ....
						&& aiAction->weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (_traceAI) Log(LOG_INFO) << ". . Throw grenade - spend Tu for COMBAT";
						int costTu (aiAction->weapon->getInventorySection()
										->getCost(_battleSave->getBattleGame()->getRuleset()->getInventoryRule(ST_RIGHTHAND)));

						if (aiAction->weapon->getFuse() == -1)
							costTu += _unit->getActionTu(BA_PRIME, aiAction->weapon);

						_unit->expendTu(costTu); // cf. grenadeAction() - priming the fuse is done in ProjectileFlyBState.
					}
					break;

				case BA_MOVE:
					if (_hasRifle == true
						&& _unit->getTu() > _unit->getActionTu(				// Should this include TU for specific move ....
															BA_SNAPSHOT,	// TODO: Hook this into _reserve/ selectFireMethod().
															aiAction->weapon))
					{
						if (_traceAI) Log(LOG_INFO) << ". . Move w/ rifle + tu for COMBAT";
						_battleSave->getBattleGame()->decAiActionCount();
					}
					break;

				case BA_LAUNCH:
					if (_traceAI) Log(LOG_INFO) << ". . Launch - copy waypoints for COMBAT";
					aiAction->waypoints = _attackAction->waypoints;
			}

//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false); // don't worry about reserving TUs, factored that in already.
			break;

		case AI_AMBUSH:
			_reserve = BA_NONE;
			_unit->setChargeTarget();

			aiAction->type			= _ambushAction->type;
			aiAction->posTarget		= _ambushAction->posTarget;
//			aiAction->firstTU		= _ambushAction->firstTU;
			aiAction->finalFacing	= _ambushAction->finalFacing;
			aiAction->finalAction	= true;

			if (_traceAI) Log(LOG_INFO) << ". . ActionType = " << BattleAction::debugBat(aiAction->type);
			break;

		case AI_ESCAPE:
			_reserve = BA_NONE;
			_unit->setChargeTarget();

			aiAction->type			= _escapeAction->type;
			aiAction->posTarget		= _escapeAction->posTarget;
//			aiAction->firstTU		= _escapeAction->firstTU;
			aiAction->finalAction	=
			aiAction->desperate		= true;

			if (_traceAI) Log(LOG_INFO) << ". . ActionType = " << BattleAction::debugBat(aiAction->type);
			_unit->setHiding(); // used by UnitWalkBState::postPathProcedures()
	}

	if (aiAction->type == BA_MOVE)
	{
		if (_traceAI) Log(LOG_INFO) << ". BA_MOVE";
//		if (aiAction->posTarget == _unit->getPosition())	// leave this in because unit could conceivably
//		{													// do a finalFacing pivot.
//			if (_traceAI) Log(LOG_INFO) << ". . Stay put";
//			aiAction->type = BA_NONE;
//		}
//		else
//		{
//			if (_traceAI) Log(LOG_INFO) << ". . Move";
		_tuAmbush =
		_tuEscape = -1; // if moving re-evaluate Ambush/Escape target.
//		}
	}
	//Log(LOG_INFO) << "AlienBAIState::think() EXIT";
}

/**
 * Sets up a BattleAction for AI_PATROL Mode.
 * @note This is mainly going from node to node & moving about the map -
 * handles Node selection.
 * @note Fills out the '_patrolAction' with useful data.
 */
void AlienBAIState::setupPatrol() // private.
{
	if (_traceAI) Log(LOG_INFO) << "AlienBAIState::setupPatrol() id-" << _unit->getId();
	_patrolAction->TU = 0;

	if (_stopNode != nullptr
		&& _unit->getPosition() == _stopNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";
		_startNode = _stopNode;
		_stopNode->allocate(false);
		_stopNode = nullptr;

		const int dir (_battleSave->getTileEngine()->faceWindow(_unit->getPosition()));
		_unit->setDirectionTo(dir);
		while (_unit->getUnitStatus() == STATUS_TURNING)
			_unit->turn();
	}

	if (_startNode == nullptr)
		_startNode = _battleSave->getStartNode(_unit);

	_pf->setPathingUnit(_unit);

	bool scout (true);
	if (_battleSave->getTacType() == TCT_BASEDEFENSE	// aLiens attacking XCOM Base are always on scout.
		&& _unit->getArmor()->getSize() == 1)			// In base defense missions the non-large aliens walk towards target nodes - or
	{													// once there shoot objects thereabouts so scan the room for objects to destroy.
		if (_startNode->isAlienTarget() == true
			&& _attackAction->weapon != nullptr
			&& _attackAction->weapon->getClip() != nullptr
			&& _attackAction->weapon->getClip()->getRules()->getDamageType() != DT_HE
			&& (   _attackAction->weapon->getRules()->getAccuracySnap() != 0 // TODO: this ought be expanded to include melee.
				|| _attackAction->weapon->getRules()->getAccuracyAuto() != 0
				|| _attackAction->weapon->getRules()->getAccuracyAimed() != 0)
			&& _battleSave->baseDestruct()[static_cast<size_t>(_startNode->getPosition().x / 10)]
										  [static_cast<size_t>(_startNode->getPosition().y / 10)].second > 0)
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
						&& data->isBaseObject() == true)
//						&& data->getDieMCD() && data->getArmor() < 60) // TODO: Create function canDestroy(int power);
					{
						_patrolAction->type = BA_SNAPSHOT;
						_patrolAction->weapon = _attackAction->weapon;
						_patrolAction->posTarget = Position(i,j,1);
						_patrolAction->TU = _unit->getActionTu(BA_SNAPSHOT, _attackAction->weapon);

						return;
					}
				}
			}
		}
		else
		{
			int // find closest objective-node which is not already allocated
				dist (CAP_DIST_SQR),
				distTest;

			for (std::vector<Node*>::const_iterator
					i = _battleSave->getNodes()->begin();
					i != _battleSave->getNodes()->end();
					++i)
			{
				if ((*i)->isAlienTarget() == true && (*i)->isAllocated() == false)
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
	else if (_startNode != nullptr
		&& _startNode->getNodeRank() != NR_SCOUT
		&& (_battleSave->getTile(_unit->getPosition()) == nullptr // <- shouldn't be necessary.
			|| _battleSave->getTile(_unit->getPosition())->getFire() == 0)
		&& (_battleSave->isCheating() == false
			|| RNG::percent(_aggression * 25) == false))
	{
		scout = false;
	}

	if (_stopNode == nullptr)
	{
		_stopNode = _battleSave->getPatrolNode(scout, _unit, _startNode);
		if (_stopNode == nullptr)
			_stopNode = _battleSave->getPatrolNode(!scout, _unit, _startNode);
	}

	if (_stopNode != nullptr)
	{
		_pf->calculatePath(_unit, _stopNode->getPosition());
		if (_pf->getStartDirection() != -1)
		{
			_stopNode->allocate();
			_patrolAction->type = BA_MOVE;
			_patrolAction->posTarget = _stopNode->getPosition();
//			_patrolAction->firstTU = _pf->getTuFirst();
		}
		else
			_stopNode = nullptr;
	}

	if (_stopNode == nullptr)
	{
		_patrolAction->type = BA_THINK;
//		_patrolAction->firstTU = -1;
	}

	if (_traceAI) {
		Log(LOG_INFO) << BattleAction::debugBAction(*_patrolAction);
		Log(LOG_INFO) << "AlienBAIState::setupPatrol() EXIT";
		Log(LOG_INFO) << "";
	}
}

/**
 * Sets up a BattleAction for AI_COMBAT Mode. Checks psi or waypoint actions
 * first, then checks grenade, shoot, or melee actions.
 * @note This will be a weapon, grenade, psionic, or waypoint attack -- or
 * perhaps moving to get a line of sight to a target. Fills out the
 * '_attackAction' with useful data.
 */
void AlienBAIState::setupAttack() // private.
{
	if (_traceAI) Log(LOG_INFO) << "AlienBAIState::setupAttack() id-" << _unit->getId();
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "AlienBAIState::setupAttack() id-" << _unit->getId();
	//Log(LOG_INFO) << ". _targetsExposed= " << _targetsExposed;

	_attackAction->type = BA_THINK;

	if (_traceAI) Log(LOG_INFO) << ". _targetsExposed = " << _targetsExposed;
	if (_targetsExposed != 0 && RNG::percent(PSI_OR_BLASTER_PCT) == true)
	{
		if (_traceAI) Log(LOG_INFO) << ". . Run psiAction() OR wayPointAction()";
		//Log(LOG_INFO) << ". . psiTried= " << _unit->psiTried();
		if ((_unit->psiTried() < PSI_TRIED_LIMIT && psiAction() == true)
			|| (_hasBlaster == true && wayPointAction() == true))
		{
			if (_traceAI) {
				if (_psiAction->type != BA_NONE) Log(LOG_INFO) << ". . . psi action";
				else Log(LOG_INFO) << ". . . blaster action";
			}
			return;
		}
		else if (_traceAI) Log(LOG_INFO) << ". . . no psi OR wayPoint action";
	}
	else
	{
		if (_traceAI) Log(LOG_INFO) << ". . no psi OR wayPoint action";
		_hasBlaster = false;
	}

	//if (_traceAI) Log(LOG_INFO) << ". selectNearestTarget()";
//	if (selectNearestTarget() != 0)
	if (_traceAI) Log(LOG_INFO) << ". _targetsVisible = " << _targetsVisible;
	if (_targetsVisible != 0)
	{
		if (_traceAI) Log(LOG_INFO) << ". . Call grenadeAction()";
		if (grenadeAction() == false)
		{
			if (_traceAI) Log(LOG_INFO) << ". . . try rifle Or melee";
			if (_hasRifle == true && _hasMelee == true)
			{
				if (_traceAI) Log(LOG_INFO) << ". . Melee & Rifle are TRUE, Call chooseMeleeOrRanged()";
				chooseMeleeOrRanged();
			}

			if (_hasRifle == true)
			{
				if (_traceAI) Log(LOG_INFO) << ". . Call rifleAction()";
				rifleAction();
				if (_traceAI) Log(LOG_INFO) << "";
			}
			else if (_hasMelee == true)
			{
				if (_traceAI) Log(LOG_INFO) << ". . Call meleeAction()";
				meleeAction();
				if (_traceAI) Log(LOG_INFO) << "";
			}
		}
		else if (_traceAI) Log(LOG_INFO) << ". . grenadeAction() TRUE";
	}
	//if (_traceAI) Log(LOG_INFO) << ". selectNearestTarget() DONE";

	if (_traceAI) {
		Log(LOG_INFO) << ". Attack bat = " << BattleAction::debugBat(_attackAction->type);
		if		(_attackAction->type == BA_MOVE)	Log(LOG_INFO) << ". . walk to " << _attackAction->posTarget;
		else if	(_attackAction->type != BA_THINK)	Log(LOG_INFO) << ". . shoot/throw at " << _attackAction->posTarget;
	}

	if (_attackAction->type == BA_THINK
		&& (_spottersOrigin != 0
			|| RNG::generate(0, _aggression) != 0))
	{
		bool debugFound = findFirePosition();
		if (_traceAI) {
			if (debugFound) Log(LOG_INFO) << ". . findFirePosition TRUE " << _attackAction->posTarget;
			else Log(LOG_INFO) << ". . findFirePosition FAILED";
		}
	}

	if (_traceAI) {
		Log(LOG_INFO) << BattleAction::debugBAction(*_attackAction);
		Log(LOG_INFO) << "AlienBAIState::setupAttack() EXIT";
		Log(LOG_INFO) << "";
	}
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
	if (_traceAI) Log(LOG_INFO) << "AlienBAIState::setupAmbush() id-" << _unit->getId();
	_ambushAction->type = BA_THINK;
//	_ambushAction->firstTU = -1;

//	if (selectPlayerTarget() == true) // sets _unitAggro.
	if (_unitAggro != nullptr)
	{
		//Log(LOG_INFO) << ". _unitAggro id-" << _unitAggro->getId();
		int
			score (0),
			scoreTest,
			tu;
//			tuFirst;

		std::vector<int> targetPath;

//		for (std::vector<Node*>::const_iterator			// use node positions for this since it gives map makers a good
//				i = _battleSave->getNodes()->begin();	// degree of control over how the units will use the environment.
//				i != _battleSave->getNodes()->end();	// Is that why ambushes work so crappy.
//				++i)
//		{
//			pos = (*i)->getPosition();
//			if ((tile = _battleSave->getTile(pos)) != nullptr
//				&& tile->getDangerous() == false
//				&& pos.z == _unit->getPosition().z
//				&& TileEngine::distSqr(pos, _unit->getPosition()) < static_cast<int>(SavedBattleGame::SEARCH_SIZE)
//				&& std::find(
//						_reachableAttack.begin(),
//						_reachableAttack.end(),
//						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
//			{
		Tile* tile;

		Position
			originVoxel (_te->getSightOriginVoxel(_unitAggro)),
			scanVoxel, // placeholder.
			pos;

		std::vector<Position> tileSearch (_battleSave->getTileSearch());
		RNG::shuffle(tileSearch.begin(), tileSearch.end());

		for (std::vector<Position>::const_iterator
				i = tileSearch.begin();
				i != tileSearch.end();
				++i)
		{
			pos = _unit->getPosition() + *i;
			//Log(LOG_INFO) << ". . tileSearch " << pos;

			if ((tile = _battleSave->getTile(pos)) != nullptr
				&& std::find(
						_reachableAttack.begin(),
						_reachableAttack.end(),
						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
			{
				//Log(LOG_INFO) << ". . . reachable w/ Attack " << pos;
				if (_traceAI > 1) {
					tile->setPreviewColor(TRACE_YELLOW);
					tile->setPreviewDir(TRACE_DIR);
					tile->setPreviewTu(485); // "4m8u5h"
				}

				const int spotters (tallySpotters(pos));
				//Log(LOG_INFO) << ". . . spotters = " << spotters;
				if (spotters /*tallySpotters(pos)*/ == 0
					&& _te->doTargetUnit(
									&originVoxel,
									tile,
									&scanVoxel,
									_unitAggro,
									_unit) == false)
				{
					//Log(LOG_INFO) << ". . . . " << _unitAggro->getId() << " cannot target " << pos;
					//Log(LOG_INFO) << ". . . . calc Path for ACTOR to pos";
					_pf->setPathingUnit(_unit);
					_pf->calculatePath(_unit, pos);

					if (_pf->getStartDirection() != -1)
					{
						tu = _pf->getTuCostTotalPf();
//						tuFirst = _pf->getTuFirst();

						//Log(LOG_INFO) << ". . . . . calc Path for TARGET to pos";
						_pf->setPathingUnit(_unitAggro);
						_pf->calculatePath(_unitAggro, pos);

						if (_pf->getStartDirection() != -1)
						{
							//Log(LOG_INFO) << ". . . . . . cross-pathing Done";
							scoreTest = BASE_SUCCESS_SYSTEMATIC;
							scoreTest -= tu;

							if (_te->faceWindow(pos) != -1)
								scoreTest += COVER_BONUS;

							//Log(LOG_INFO) << ". . . . . . scoreTest = " << scoreTest;
							if (scoreTest > score)
							{
								score = scoreTest;
								//Log(LOG_INFO) << ". . . . . . . high Score = " << score;

								targetPath = _pf->copyPath();

								_ambushAction->posTarget = pos;
//								_ambushAction->firstTU = tuFirst;
								_tuAmbush = tu;
								//Log(LOG_INFO) << ". . . . . . . pos " << pos;
								//Log(LOG_INFO) << ". . . . . . . tu = " << tu;

//								if (score > FAST_PASS_THRESHOLD - 20)
//								{
									//Log(LOG_INFO) << ". . . . . . . . Beats (FAST_PASS_THRESHOLD - 20) break";
//									break;
//								}
							}
						}
					}
				}
			}
		}

		if (_tuAmbush != -1) // This is the most cockamamie thing I've ever seen. REWRITE REQUIRED.
		{
			//Log(LOG_INFO) << ". . _tuAmbush = " << _tuAmbush;

			_ambushAction->type = BA_MOVE;
			originVoxel = _te->getSightOriginVoxel(_unit, &_ambushAction->posTarget);

			_pf->setPathingUnit(_unitAggro);
			pos = _unitAggro->getPosition();

			Position posNext;

			size_t t (targetPath.size());
			while (t-- != 0u)
			{
				//Log(LOG_INFO) << ". . . walk hypoTarget";

				_pf->getTuCostPf(pos, targetPath.back(), &posNext);
				targetPath.pop_back();

				tile = _battleSave->getTile(pos = posNext);
				if (_te->doTargetUnit(
									&originVoxel,
									tile,
									&scanVoxel,
									_unit,
									_unitAggro) == true)
				{
					if (_traceAI > 1) {
						tile->setPreviewColor(TRACE_RED);
						tile->setPreviewDir(TRACE_DIR);
						tile->setPreviewTu(485); // "4m8u5h"
					}
					_ambushAction->finalFacing = TileEngine::getDirectionTo(_ambushAction->posTarget, pos);
					//Log(LOG_INFO) << ". . . . finalFacing = " << _ambushAction->finalFacing;
					break;
				}
			}
		}

		if (_traceAI > 1) {
			tile = _battleSave->getTile(_ambushAction->posTarget);
			tile->setPreviewColor(TRACE_PURPLE);
			tile->setPreviewDir(TRACE_DIR);
			tile->setPreviewTu(485); // "4m8u5h"
		}
	}

	if (_traceAI) {
		Log(LOG_INFO) << BattleAction::debugBAction(*_ambushAction);
		Log(LOG_INFO) << "AlienBAIState::setupAmbush() EXIT";
		Log(LOG_INFO) << "";
	}
}

/**
 * Sets up a BattleAction for AI_ESCAPE Mode.
 * @note The idea is to check within a 11x11 tile square for a tile that is not
 * seen by '_unitAggro'. If there is no such tile run away from the target.
 * @note Fills out the '_escapeAction' with useful data.
 */
void AlienBAIState::setupEscape() // private.
{
	if (_traceAI) Log(LOG_INFO) << "AlienBAIState::setupEscape() id-" << _unit->getId();
//	selectNearestTarget(); // sets _unitAggro
//	const int spottersOrigin (tallySpotters(_unit->getPosition()));

	int
		distAggroOrigin,
		distAggroTarget;
	if (_unitAggro != nullptr)
	{
		distAggroOrigin = TileEngine::distance(
										_unit->getPosition(),
										_unitAggro->getPosition());
		if (_traceAI) Log(LOG_INFO) << ". aggroUnit VALID dist= " << distAggroOrigin;
	}
	else
	{
		distAggroOrigin = 0;
		if (_traceAI) Log(LOG_INFO) << ". aggroUnit NOT Valid dist= " << distAggroOrigin;
	}

	Tile* tile;
	int
		score (ESCAPE_FAIL),
		scoreTest;
	Position pos;

	std::vector<Position> tileSearch (_battleSave->getTileSearch());
	RNG::shuffle(tileSearch.begin(), tileSearch.end());

	_pf->setPathingUnit(_unit);

	bool
//		coverFound (false),
		first (true);
	size_t t (SavedBattleGame::SEARCH_SIZE);
	while (/*coverFound == false &&*/ t <= SavedBattleGame::SEARCH_SIZE)
	{
		//if (_traceAI) Log(LOG_INFO) << ". t= " << t;
		if (first == true)
		{
			//if (_traceAI) Log(LOG_INFO) << ". . first";
			first = false;

			t = 0u;
			scoreTest = 0;

			if (_battleSave->getTile(_unit->getLastCover()) != nullptr)
				pos = _unit->getLastCover();
			else
				pos = _unit->getPosition();
		}
		else if (t < SavedBattleGame::SEARCH_SIZE)
		{
			//if (_traceAI) Log(LOG_INFO) << ". . in Search_Size";
			scoreTest = BASE_SUCCESS_SYSTEMATIC;

			pos = _unit->getPosition();
			pos.x += tileSearch[t].x;
			pos.y += tileSearch[t].y;

			if (pos == _unit->getPosition())
			{
//				if (spottersOrigin != 0)
				if (_spottersOrigin != 0)
				{
					pos.x += RNG::generate(-20,20);
					pos.y += RNG::generate(-20,20);
				}
				else
					scoreTest += CUR_TILE_PREF;
			}
			++t;
		}
		else // last ditch chance.
		{
			++t;
			//if (_traceAI) Log(LOG_INFO) << ". . out Search_Size";
			scoreTest = BASE_SUCCESS_DESPERATE;

			pos = _unit->getPosition();
			pos.x += RNG::generate(-10,10);
			pos.y += RNG::generate(-10,10);
			pos.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (pos.z < 0)
				pos.z = 0;
			else if (pos.z >= _battleSave->getMapSizeZ())
				pos.z = _battleSave->getMapSizeZ();
		}


		//if (_traceAI) Log(LOG_INFO) << ". posTarget " << pos;

		if ((tile = _battleSave->getTile(pos)) != nullptr
			&& std::find(
					_reachable.begin(),
					_reachable.end(),
					_battleSave->getTileIndex(pos)) != _reachable.end())
		{
			//if (_traceAI) Log(LOG_INFO) << ". . is Reachable";

			if (_unitAggro != nullptr)
				distAggroTarget = TileEngine::distance(
													pos,
													_unitAggro->getPosition());
			else
				distAggroTarget = 0;

			scoreTest += (distAggroTarget - distAggroOrigin)    * EXPOSURE_PENALTY;
//			scoreTest += (spottersOrigin - tallySpotters(pos))  * EXPOSURE_PENALTY;
			scoreTest += (_spottersOrigin - tallySpotters(pos)) * EXPOSURE_PENALTY;

			if (tile->getFire() != 0)
				scoreTest -= FIRE_PENALTY;
			else
				scoreTest += tile->getSmoke() * SMOKE_BONUS;

			if (tile->getDangerous() == true)
				scoreTest -= BASE_SUCCESS_SYSTEMATIC;

			//if (_traceAI) Log(LOG_INFO) << ". . scoreTest= " << scoreTest;

			if (_traceAI > 1) {
				tile->setPreviewColor(BattleAIState::debugTraceColor(false, scoreTest));
				tile->setPreviewDir(TRACE_DIR);
				tile->setPreviewTu(scoreTest);
			}

			if (scoreTest > score)
			{
				_pf->calculatePath(_unit, pos);
				//if (_traceAI) {
				//	Log(LOG_INFO) << ". . . calculatePath() dir= " << _pf->getStartDirection();
				//	Log(LOG_INFO) << ". . . on pos= " << (pos == _unit->getPosition());
				//}

				if (_pf->getStartDirection() != -1
					|| pos == _unit->getPosition())
				{
					score = scoreTest;
					//if (_traceAI) Log(LOG_INFO) << ". . . . set score= " << score;

					_escapeAction->posTarget = pos;
//					_escapeAction->firstTU = _pf->getTuFirst();
					_tuEscape = _pf->getTuCostTotalPf();

					//if (_traceAI) {
//						Log(LOG_INFO) << ". . . . firstTU= " << _escapeAction->firstTU;
					//	Log(LOG_INFO) << ". . . . tuTotal= " << _tuEscape;
					//}

					if (_traceAI > 1) {
						tile->setPreviewColor(BattleAIState::debugTraceColor(true, scoreTest));
						tile->setPreviewDir(TRACE_DIR);
						tile->setPreviewTu(score);
					}
				}
//				if (score > FAST_PASS_THRESHOLD)
//					coverFound = true;
			}
		}
	}

	if (score != ESCAPE_FAIL)
	{
//		if (_traceAI) _battleSave->getTile(_escapeAction->posTarget)->setPreviewColor(TRACE_PURPLE);
		if (_traceAI) Log(LOG_INFO) << ". set bat BA_MOVE";
		_escapeAction->type = BA_MOVE;
	}
	else
	{
		if (_traceAI) Log(LOG_INFO) << ". set bat BA_THINK";
		_escapeAction->type = BA_THINK;
//		_escapeAction->firstTU = -1;
		_tuEscape = -1;
	}

	if (_traceAI) {
		Log(LOG_INFO) << BattleAction::debugBAction(*_escapeAction);
		Log(LOG_INFO) << "AlienBAIState::setupEscape() EXIT";
		Log(LOG_INFO) << "";
	}
}

/**
 * Selects the AI Mode for BattlescapeGame::handleUnitAI().
 */
void AlienBAIState::evaluateAiMode() // private.
{
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "AlienBAIState::evaluateAiMode() id-" << _unit->getId();
	}
	if (_unit->getChargeTarget() != nullptr
		&& _attackAction->type != BA_THINK)
	{
		if (_traceAI) Log(LOG_INFO) << ". chargeTarget VALID - Not Think - return COMBAT";
		_AIMode = AI_COMBAT;
		return;
	}

	// if the aliens are cheating or the unit is charging enforce combat as a priority
	if (_battleSave->isCheating() == true // <- do i want this - kL_note
		|| _unit->getChargeTarget() != nullptr
		|| _hasBlaster == true)	// The two (_hasBlaster== true) checks in this function ought obviate the entire re-evaluate thing!
								// Note there is a valid targetPosition but targetUnit is NOT at that Pos if blaster=TRUE ....
	{
		if (_traceAI) Log(LOG_INFO) << ". chargeTarget Or waypoints Or blaster - set COMBAT";
		_AIMode = AI_COMBAT;
	}
	else
	{
		if (_traceAI) Log(LOG_INFO) << ". Evaluate ...";
		float
			patrolOdds (28.f),
			combatOdds (23.f),
			ambushOdds (13.f),
			escapeOdds (13.f);

		if (_unit->getChargeTarget() != nullptr
			|| _unit->getTu() > (_unit->getBattleStats()->tu >> 1u)) // has over half TU.
		{
			escapeOdds = 4.3f;
		}
		else if (_hasMelee == true)
			escapeOdds = 10.5f;

		if (_targetsVisible != 0)
			patrolOdds = 8.2f;

		if (_spottersOrigin != 0)
		{
			patrolOdds = 0.f;
			if (_tuEscape == -1) setupEscape();
		}

		if (_hasRifle == false || _tuAmbush == -1)
		{
			ambushOdds = 0.f;
			if (_hasMelee == true)
				combatOdds *= 1.2f;
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
			case AI_PATROL: patrolOdds *= 1.2f; break;
			case AI_AMBUSH: ambushOdds *= 1.2f; break;
			case AI_COMBAT: combatOdds *= 1.2f; break;
			case AI_ESCAPE: escapeOdds *= 1.2f;
		}

		if (_unit->getHealth() < _unit->getBattleStats()->health / 3)
		{
			escapeOdds *= 1.8f;
			combatOdds *= 0.6f;
			ambushOdds *= 0.7f;
		}
		else if (_unit->getHealth() < (_unit->getBattleStats()->health << 1u) / 3)
		{
			escapeOdds *= 1.5f;
			combatOdds *= 0.8f;
			ambushOdds *= 0.9f;
		}
		else if (_unit->getHealth() < _unit->getBattleStats()->health)
			escapeOdds *= 1.2f;

		switch (_aggression)
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
				combatOdds *= Vicegrip(1.2f + (static_cast<float>(_aggression) / 10.f), 0.1f, 2.f);
				escapeOdds *= Vicegrip(0.9f - (static_cast<float>(_aggression) / 10.f), 0.1f, 2.f);
		}

		if (_AIMode == AI_COMBAT)
			ambushOdds *= 1.3f;

		if (_spottersOrigin != 0)
		{
			escapeOdds *= (10.f * static_cast<float>(_spottersOrigin + 10) / 100.f);
			combatOdds *= ( 5.f * static_cast<float>(_spottersOrigin + 20) / 100.f);
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

		if (   _hasMelee   == false
			&& _hasRifle   == false
			&& _hasBlaster == false
			&& _doGrenade  == false)
		{
			combatOdds =
			ambushOdds = 0.f;
		}

		if (_traceAI) {
			Log(LOG_INFO) << "patrolOdds = " << patrolOdds;
			Log(LOG_INFO) << "combatOdds = " << combatOdds;
			Log(LOG_INFO) << "ambushOdds = " << ambushOdds;
			Log(LOG_INFO) << "escapeOdds = " << escapeOdds;
		}

		const float decision (RNG::generate(0.f, patrolOdds + combatOdds + ambushOdds + escapeOdds));

		if (_traceAI) Log(LOG_INFO) << "decision Pre = " << decision;
		if (decision <= patrolOdds)
			_AIMode = AI_PATROL;
		else if (decision <= patrolOdds + combatOdds)
			_AIMode = AI_COMBAT;
		else if (decision <= patrolOdds + combatOdds + ambushOdds)
			_AIMode = AI_AMBUSH;
		else
			_AIMode = AI_ESCAPE;
		if (_traceAI) Log(LOG_INFO) << "decision Post = " << BattleAIState::debugAiMode(_AIMode);
	}

	// TODO: These fallbacks should go in accord with the Odds above^
	// Check validity of the decision and if that fails try a fallback behaviour according to priority:
	// 1) Patrol
	// 2) Combat
	// 3) Ambush
	// 4) Escape
	if (_AIMode == AI_PATROL && _stopNode == nullptr)
	{
		if (_traceAI) Log(LOG_INFO) << ". fallback COMBAT";
		_AIMode = AI_COMBAT;
	}

	if (_AIMode == AI_COMBAT)
	{
		if (_traceAI) Log(LOG_INFO) << ". AI_COMBAT _hasBlaster = " << _hasBlaster;
//		if (_unitAggro)
		if (_attackAction->type == BA_LAUNCH
//		if (_hasBlaster == true // note: Blaster-wielding units should go for an AimedShot ... costs less TU.
//			|| _unitAggro != nullptr)
			|| (_battleSave->getTile(_attackAction->posTarget) != nullptr
				&& _battleSave->getTile(_attackAction->posTarget)->getTileUnit() != nullptr))
		{
			if (_traceAI) Log(LOG_INFO) << ". . try rifle Or do blaster Action";
			if (_attackAction->type != BA_THINK || findFirePosition() == true)
			{
				if (_traceAI) Log(LOG_INFO) << ". . . NOT Think Or findFirePosition() TRUE - ret COMBAT";
				return;
			}
		}
		else if (selectTarget() == true && findFirePosition() == true)
		{
			if (_traceAI) Log(LOG_INFO) << ". . selectTarget() TRUE And findFirePosition() TRUE - ret COMBAT";
			return;
		}

		if (_traceAI) Log(LOG_INFO) << ". fallback AMBUSH";
		_AIMode = AI_AMBUSH;
	}

	if (_AIMode == AI_AMBUSH && _tuAmbush == -1)
	{
		if (_traceAI) Log(LOG_INFO) << ". fallback ESCAPE";
		_AIMode = AI_ESCAPE;
	}

	if (_AIMode == AI_ESCAPE && _tuEscape == -1)
	{
		if (_traceAI) Log(LOG_INFO) << ". fallback PATROL -> Error: Nothing To Do.";
		_AIMode = AI_PATROL;
	}
	if (_traceAI) Log(LOG_INFO) << " Final decision = " << BattleAIState::debugAiMode(_AIMode);
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

	const BattleUnit* hypoUnit;
	if (pos != _unit->getPosition())
		hypoUnit = _unit;
	else
		hypoUnit = nullptr;

	Position
		originVoxel,
		targetVoxel;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i) == true
			&& TileEngine::distSqr(pos, (*i)->getPosition()) <= TileEngine::SIGHTDIST_TSp_Sqr) // Could use checkViewSector() and/or visible()
		{
			originVoxel = _te->getSightOriginVoxel(*i);
			if (_te->doTargetUnit(				// check if xCom agent can target on position
								&originVoxel,	// WARNING: Does not include visible() check.
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
	_distClosest = CAP_DIST;

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
			&& _te->visible(_unit, (*i)->getUnitTile()) == true)
		{
			++ret;
			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < _distClosest)
			{
				canTarget = false;
				if (_hasMelee == true && _hasRifle == false)
					canTarget = findMeleePosition(*i, _unit->getTu());
				else
				{
					origin = _te->getSightOriginVoxel(_unit);
					canTarget = _te->doTargetUnit(
											&origin,
											(*i)->getUnitTile(),
											&target,
											_unit);
				}
//				if (_hasRifle == true || _hasMelee == false) // -> is ambiguity like that required.
//				{
//					origin = _te->getOriginVoxel(action);
//					canTarget = _te->doTargetUnit(
//											&origin,
//											(*i)->getTile(),
//											&target,
//											_unit);
//				}
//				else if (findMeleePosition(*i, _unit->getTu()) == true)
//				{
//					dir = TileEngine::getDirectionTo(
//												_attackAction->target,
//												(*i)->getPosition());
//					canTarget = _te->validMeleeRange( // this appears to be done already in findMeleePosition() ...
//												_attackAction->target,
//												dir,
//												_unit,
//												*i);
//				}

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
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "selectPlayerTarget()";
	}

	_unitAggro = nullptr;
	int
		dist (CAP_DIST_SQR),
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
	if (_traceAI && _unitAggro != nullptr) {
		Log(LOG_INFO) << ". dist = " << std::sqrt(dist);
		Log(LOG_INFO) << ". unitAggro id-" << _unitAggro->getId();
	}

	if (_traceAI) Log(LOG_INFO) << "";
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
		dist (-CAP_DIST_SQR),
		distTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true)
		{
			distTest = RNG::generate(0, TileEngine::SIGHTDIST_TSp_Sqr);
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
 * Selects a position where a target can be targeted and initiates BA_MOVE.
 * @note Checks an 11x11 grid for a position nearby from which Actor can target.
 * @return, true if found
 */
bool AlienBAIState::findFirePosition() // private.
{
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "findFirePosition()";
	}

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
				if (_battleSave->getTileEngine()->doTargetUnit(
															&originVoxel,
															_unitAggro->getUnitTile(),
															&targetVoxel,
															_unit) == true)
				{
					_pf->calculatePath(_unit, pos);
					if (_pf->getStartDirection() != -1) // && _pf->getTuCostTotalPf() <= _unit->getTu()
					{
						scoreTest = BASE_SUCCESS_SYSTEMATIC - tallySpotters(pos) * EXPOSURE_PENALTY;
						scoreTest += _unit->getTu() - _pf->getTuCostTotalPf();

						if (_unitAggro->checkViewSector(pos) == false)
							scoreTest += 15;

						if (scoreTest > score)
						{
							score = scoreTest;
							_attackAction->posTarget = pos;
//							_attackAction->firstTU = _pf->getTuFirst();
							_attackAction->finalFacing = TileEngine::getDirectionTo(
																				pos,
																				_unitAggro->getPosition());
//							if (score > FAST_PASS_THRESHOLD + 25)
//								break;
						}
					}
				}
			}
		}

		if (score > BASE_SUCCESS)
		{
//			if (_unit->getPosition() != _attackAction->posTarget)
			_attackAction->type = BA_MOVE;
//			else chooseFireMethod() & setup the action for handleUnitAI()

			if (_traceAI) {
				Log(LOG_INFO) << ". success " << _attackAction->posTarget << " score = " << score;
				Log(LOG_INFO) << "";
			}
			return true;
		}
	}

	if (_traceAI) {
		Log(LOG_INFO) << "Firepoint failed";//: best estimation was " << _attackAction->posTarget << " with a score of " << score;
		Log(LOG_INFO) << "";
	}

//	_attackAction->firstTU = -1;
	return false;
}

/**
 * Selects a point near enough to a BattleUnit to perform a melee attack.
 * @param targetUnit	- pointer to a target BattleUnit
 * @param tuCap			- maximum time units that the path can cost
 * @return, true if a point was found
 */
bool AlienBAIState::findMeleePosition( // private.
		const BattleUnit* const targetUnit,
		int tuCap) const
{
	bool ret (false);

	const int
		actorSize (_unit->getArmor()->getSize()),
		targetSize (targetUnit->getArmor()->getSize());
	size_t dist (static_cast<size_t>(CAP_DIST));

	_pf->setPathingUnit(_unit);

	Position pos;
	const Position& posTarget (targetUnit->getPosition());
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
					pos = posTarget + Position(x,y,z);
					if (_battleSave->getTile(pos) != nullptr
						&& std::find(
								_reachable.begin(),
								_reachable.end(),
								_battleSave->getTileIndex(pos)) != _reachable.end())
					{
						if (_te->validMeleeRange(
											pos,
											TileEngine::getDirectionTo(pos, posTarget),
											_unit,
											targetUnit) == true
							&& _battleSave->setUnitPosition(_unit, pos, true) == true
							&& (_battleSave->getTile(pos)->getDangerous() == false
								|| RNG::generate(0, _aggression) != 0))
						{
							_pf->calculatePath(_unit, pos, tuCap);
							if (_pf->getStartDirection() != -1 && _pf->getPath().size() < dist)
							{
								dist = _pf->getPath().size();
								ret = true;
								_attackAction->posTarget = pos;
//								_attackAction->firstTU = _pf->getTuFirst();
							}
						}
					}
				}
			}
		}
	}

//	if (ret == false) _attackAction->firstTU = -1;
	return ret;
}

/**
 * Tries to setup a melee attack/charge.
 */
void AlienBAIState::meleeAction() // private.
{
//	if (_unit->getTu() < _unit->getActionTu(BA_MELEE, _unit->getMeleeWeapon()))
//		return;

	int dir;

	if (_unitAggro != nullptr
		&& _unitAggro->getUnitStatus() == STATUS_STANDING)
//		&& _unitAggro->isOut_t(OUT_STAT) == false)
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

	const int tuReserve (_unit->getTu()
					   - _unit->getActionTu(BA_MELEE, _attackAction->weapon));
	int
		dist ((tuReserve >> 2u) + 1),
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
		//if (_traceAI) Log(LOG_INFO) << "AlienBAIState::meleeAction: [target]: " << (_unitAggro->getId()) << " at: "  << _attackAction->target;
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
//	const int offset (_unit->getArmor()->getSize() - 1);
//	_unit->setDirectionTo(_unitAggro->getPosition() + Position(offset, offset, 0));
	_unit->setDirectionTo(_unitAggro->getPosition());
	while (_unit->getUnitStatus() == STATUS_TURNING)
		_unit->turn();

//	_attackAction->finalFacing	= _unit->getUnitDirection();
	_attackAction->posTarget	= _unitAggro->getPosition();
	_attackAction->type			= BA_MELEE;
}

/**
 * Tries to trace a waypoint-projectile.
 * @return, true if blaster
 */
bool AlienBAIState::wayPointAction() // private.
{
	_attackAction->TU = _unit->getActionTu(
										BA_LAUNCH,
										_attackAction->weapon);
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "AlienBAIState::wayPointAction() id-" << _unit->getId() << " w/ " << _attackAction->weapon->getRules()->getType();
		Log(LOG_INFO) << ". actionTU = " << _attackAction->TU;
		Log(LOG_INFO) << ". unitTU = " << _unit->getTu();
	}

	if (_attackAction->TU <= _unit->getTu())
	{
		_pf->setPathingUnit(_unit); // jic.

		std::vector<BattleUnit*> targets;
		const int explRadius (_attackAction->weapon->getClip()->getRules()->getExplosionRadius());

		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if (_traceAI) Log(LOG_INFO) << ". . test Vs unit id-" << (*i)->getId() << " pos " << (*i)->getPosition();
			if (validTarget(*i, true, true) == true)
			{
				if (_traceAI) Log(LOG_INFO) << ". . . unit VALID";
				if (explosiveEfficacy(
								(*i)->getPosition(),
								_unit,
								explRadius,
								_attackAction->diff) == true)
				{
					if (_traceAI) Log(LOG_INFO) << ". . . . explEff VALID";
					if (pathWaypoints(*i) == true)
						targets.push_back(*i);
				}
				else if (_traceAI) Log(LOG_INFO) << ". . . . explEff invalid";
			}
		}

		if (targets.empty() == false)
		{
			//Log(LOG_INFO) << ". targets available";
			BattleUnit* const saladhead (targets.at(RNG::pick(targets.size())));
			//Log(LOG_INFO) << ". . total = " << targets.size() << " Target id-" << saladhead->getId();
			if (pathWaypoints(saladhead) == true) // safety.
			{
				//Log(LOG_INFO) << ". . . Return, do LAUNCH";
				//Log(LOG_INFO) << "";
				_unitAggro = saladhead;
				_attackAction->type = BA_LAUNCH;
				_attackAction->posTarget = _attackAction->waypoints.front();
				return true;
			}
		}
	}

	_hasBlaster = false;
	_attackAction->type = BA_THINK;
	_attackAction->waypoints.clear(); // tidy.
	//Log(LOG_INFO) << ". waypoint action FAILED - Think !";
	//Log(LOG_INFO) << "";
	return false;
}

/**
 * Constructs a waypoint-path for a guided projectile.
 * @note Helper for wayPointAction().
 * @param unit - pointer to a BattleUnit to path at
 * @return, true if waypoints get positioned
 */
bool AlienBAIState::pathWaypoints(const BattleUnit* const unit) // private.
{
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "AlienBAIState::pathWaypoints() vs id-" << unit->getId() << " pos " << unit->getPosition();
		Log(LOG_INFO) << ". actor id-" << _unit->getId() << " pos " << _unit->getPosition();
	}

	_pf->setPathingUnit(_unit); // jic.
	_pf->calculatePath(
				_unit,
				unit->getPosition(),
				Pathfinding::TU_INFINITE,
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
			if (_traceAI) Log(LOG_INFO) << ". . place WP " << pos;
		}

		// pathing done & wp's have been positioned:
		if (_traceAI) Log(LOG_INFO) << ". . qty WP's = " << _attackAction->waypoints.size() << " / max WP's = " << _attackAction->weapon->getRules()->isWaypoints();
		if (_attackAction->waypoints.size() != 0u)
		{
			int wp (_attackAction->weapon->getRules()->isWaypoints()
														+ _attackAction->diff
														- static_cast<int>(DIFF_SUPERHUMAN));
			if (wp < 1) wp = 1;
			if (static_cast<int>(_attackAction->waypoints.size()) <= wp)
			{
				if (_traceAI) Log(LOG_INFO) << ". . . path valid, ret TRUE";
				return true;
			}
			if (_traceAI) Log(LOG_INFO) << ". . too many WP's !!";
		}
	}

	if (_traceAI) Log(LOG_INFO) << ". path or WP's invalid, ret FALSE";
	return false;
}

/**
 * Tries to setup a shot.
 */
void AlienBAIState::rifleAction() // private.
{
	_attackAction->posTarget = _unitAggro->getPosition();

	if (_attackAction->weapon->getClip()->getRules()->getExplosionRadius() < 1
		|| explosiveEfficacy(
						_unitAggro->getPosition(),
						_unit,
						_attackAction->weapon->getClip()->getRules()->getExplosionRadius(),
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

	const int dist (TileEngine::distance(
									_unit->getPosition(),
									_attackAction->posTarget));
	const RuleItem* const itRule (_attackAction->weapon->getRules());
	if (   dist > itRule->getMaxRange()
		|| dist < itRule->getMinRange())
	{
		return;
	}

	int tuReserve (_unit->getTu());
	if (_tuEscape != -1
		&& RNG::generate(0, _aggression) == 0)
	{
		tuReserve -= _tuEscape;
	}

	if (dist <= itRule->getAutoRange())
	{
		if (itRule->getAutoTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
		else if (itRule->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (itRule->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
	}
	else if (dist <= itRule->getSnapRange())
	{
		if (itRule->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (itRule->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (itRule->getAutoTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
	else if (dist <= itRule->getAimRange())
	{
		if (itRule->getAimedTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (itRule->getSnapTu() != 0
			&& tuReserve >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (itRule->getAutoTu() != 0
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

			if (tuCost <= _unit->getTu())
			{
				BattleAction action;
				action.actor = _unit;
				action.posTarget = _unitAggro->getPosition();
				action.weapon = grenade;
				action.type = BA_THROW;

				const Position
					originVoxel (_battleSave->getTileEngine()->getSightOriginVoxel(_unit)),
					targetVoxel (Position::toVoxelSpaceCentered(
															action.posTarget,
															FLOOR_TLEVEL - _battleSave->getTile(action.posTarget)->getTerrainLevel()));

				if (_battleSave->getTileEngine()->validateThrow(action, originVoxel, targetVoxel) == true)
				{
					_attackAction->posTarget = action.posTarget;
					_attackAction->weapon = grenade;
					_attackAction->type = BA_THROW;

					_hasRifle =
					_hasMelee = false;
					_doGrenade = true;
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
 * @param pos		- reference to the target's position
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

	const Tile* const tile (_battleSave->getTile(pos));
	if (tile->isFloored(tile->getTileBelow(_battleSave)) == true)
	{
		const int firstGrenade (_battleSave->getBattleState()->getGame()->getRuleset()->getFirstGrenade());
		if (firstGrenade == -1
			|| (firstGrenade == 0 && _battleSave->getTurn() > 4 - diff)
			|| (firstGrenade  > 0 && _battleSave->getTurn() > firstGrenade - 1))
		{
			pct = (100 - attacker->getMorale()) / 3;
			pct += (10 - static_cast<int>(
						 static_cast<float>(attacker->getHealth()) / static_cast<float>(attacker->getBattleStats()->health)
						 * 10.f)) << 1u;
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
				case TCT_TERRORSITE:
					pct += 57;
			}

			pct += diff << 1u;

			const BattleUnit* const targetUnit (tile->getTileUnit());

			VoxelType voxelTest;

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
				if ((*i)->isOut_t(OUT_HEALTH) == false
					&& *i != attacker
					&& *i != targetUnit
					&& std::abs((*i)->getPosition().z - pos.z) < Options::battleExplosionHeight + 1
					&& TileEngine::distance(
										pos,
										(*i)->getPosition()) < radius + 1)
				{
					//Log(LOG_INFO) << ". . dangerousFALSE = " << ((*i)->getTile() != nullptr && (*i)->getTile()->getDangerous() == false);
					//Log(LOG_INFO) << ". . exposed = " << ((*i)->getFaction() == FACTION_HOSTILE || (*i)->getExposed() < _unit->getIntelligence() + 1);
					if (   (*i)->getUnitTile() != nullptr
						&& (*i)->getUnitTile()->getDangerous() == false
						&& ((*i)->getFaction() == FACTION_HOSTILE
							|| (   (*i)->getExposed() != -1
								&& (*i)->getExposed() <= _unit->getIntelligence())))
					{
						const Position
							voxelPosA (Position::toVoxelSpaceCentered(pos, 12)),
							voxelPosB (Position::toVoxelSpaceCentered((*i)->getPosition(), 12));

						std::vector<Position> trj;
						voxelTest = _battleSave->getTileEngine()->plotLine(
																		voxelPosA,
																		voxelPosB,
																		false,
																		&trj,
																		targetUnit,
																		true,
																		false,
																		*i);
						//Log(LOG_INFO) << "trajSize = " << (int)trj.size() << "; impact = " << impact;
						if (voxelTest == VOXEL_UNIT
							&& (*i)->getPosition() == Position::toTileSpace(trj.front()))
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
	if (_traceAI) {
		Log(LOG_INFO) << "";
		Log(LOG_INFO) << "AlienBAIState::psiAction() id-" << _unit->getId();
	}

	if (_unit->getOriginalFaction() == FACTION_HOSTILE
		&& _unit->getBattleStats()->psiSkill != 0)
	{
		//Log(LOG_INFO) << "AlienBAIState::psiAction() id-" << _unit->getId();

		const RuleItem* const itRule (_battleSave->getBattleGame()->getAlienPsi()->getRules());

		int tuCost (_unit->getActionTu(BA_PSIPANIC, itRule));
		if (_tuEscape != -1)
			tuCost += _tuEscape;
		if (_traceAI) Log(LOG_INFO) << ". tuCost = " << tuCost;
		if (_unit->getTu() >= tuCost)
		{
			const int attack (static_cast<int>(static_cast<float>(
							 _unit->getBattleStats()->psiStrength * _unit->getBattleStats()->psiSkill) / 50.f));
			if (_traceAI) Log(LOG_INFO) << ". . attack = " << attack;

			BattleUnit* unitTarget (nullptr);
			int
				defense,
				dist,
				los,
				losTest,
				weight (0),
				weightTest;
			Position
				originVoxel,
				targetVoxel; // placeholder.

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getGeoscapeSoldier() != nullptr	// what about doggies .... Should use isMoralable() for doggies ....
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
										_unit->getPosition()) << 1u;
					originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(_unit);
					losTest = static_cast<int>(_battleSave->getTileEngine()->doTargetUnit(
																						&originVoxel,
																						(*i)->getUnitTile(),
																						&targetVoxel,
																						_unit)) * PSI_LOS_WEIGHT;
					//Log(LOG_INFO) << ". . . ";
					//Log(LOG_INFO) << ". . . targetID = " << (*i)->getId();
					//Log(LOG_INFO) << ". . . defense = " << defense;
					//Log(LOG_INFO) << ". . . dist = " << dist;
					//Log(LOG_INFO) << ". . . currentMood = " << currentMood;
					//Log(LOG_INFO) << ". . . hasSight = " << hasSight;

					weightTest = attack // NOTE: This is NOT a true calculation of Success.
							   - defense
							   - dist
							   + losTest
							   + RNG::generate(0, PSI_SWITCH_TARGET);
					//Log(LOG_INFO) << ". . . weightTest = " << weightTest;

					if (weightTest > weight || unitTarget == nullptr)
					{
						weight = weightTest;
						los = losTest;
						unitTarget = *i;
					}
				}
			}

			if (unitTarget != nullptr && weight - los > PSI_CUTOFF)
			{
				if (_traceAI) Log(LOG_INFO) << ". . . target Valid - acceptable Prob.";
//				if (_targetsVisible
//					&& _attackAction->weapon
//					&& _attackAction->weapon->getAmmoItem())
//				{
//					if (_attackAction->weapon->getAmmoItem()->getRules()->getPower() > chance)
//						return false;
//				}
//				else if (RNG::generate(35, 155) > chance)
//					return false;

				//Log(LOG_INFO) << ". . . target Valid - acceptable Prob. id-" << unitTarget->getId() << " exposed= " << unitTarget->getExposed();

				_unitAggro = unitTarget;
				_psiAction->posTarget = unitTarget->getPosition();

				int moraleCheck (unitTarget->getMorale());
				if (moraleCheck > 0) //&& weight < 30 // panicAtk is valid since target has morale to chew away esp. if aLien atkStr is low
				{
					//Log(LOG_INFO) << ". . test if MC or Panic - attack= " << attack;
					const int bravery (unitTarget->getBattleStats()->bravery);
					int panicChance (110 - bravery);
					moraleCheck -= panicChance;
					//Log(LOG_INFO) << ". . panicOdds_1 = " << panicChance;

					if		(moraleCheck <  0)	panicChance -= bravery >> 1u;
					else if	(moraleCheck < 50)	panicChance -= bravery;
					else						panicChance -= bravery << 1u;

					//Log(LOG_INFO) << ". . panicOdds_2 = " << panicChance;
					panicChance += (RNG::generate(51,100) - (attack >> 1u));
					//Log(LOG_INFO) << ". . panicOdds_3 = " << panicChance;
					if (RNG::percent(panicChance) == true)
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
			else if (_traceAI) Log(LOG_INFO) << ". . . target NOT Valid or unacceptable Prob.";
		}
	}

	if (_traceAI) {
		Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False";
		Log(LOG_INFO) << "";
	}
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
		&& (dangerTile == false
			|| unit->getUnitTile()->getDangerous() == false
			|| RNG::generate(0, _aggression) != 0))			// target has not been grenaded
	{
		//Log(LOG_INFO) << "AlienBAIState::validTarget() targetId-" << unit->getId() << " exposed= " << unit->getExposed();
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
		_hasRifle = false;
		return;
	}

	if (_unit->getHealth() > (_unit->getBattleStats()->health << 1u) / 3) // is over 2/3 health
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
				meleeOdds += (power - 50) >> 1u;

			if (_targetsVisible > 1)
				meleeOdds -= 15 * (_targetsVisible - 1);

			if (meleeOdds > 0)
			{
				switch (_aggression)
				{
					case 0:
						meleeOdds -= 20;
						break;
					default:
						meleeOdds += 10 * _aggression;
				}

				if (RNG::percent(meleeOdds) == true)
				{
					_hasRifle = false;
					const int tuReserve (_unit->getTu()
									   - _unit->getActionTu(BA_MELEE, itRule));
					_pf->setPathingUnit(_unit);
					_reachableAttack = _pf->findReachable(_unit, tuReserve);
					return;
				}
			}
		}
	}

	_hasMelee = false;
}

/**
 * Gets the TU-reservation setting.
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
			&& _battleSave->getTileEngine()->doTargetTilepart(
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
					if (_battleSave->getTileEngine()->doTargetTilepart(
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
