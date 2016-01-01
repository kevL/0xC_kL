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

#include "CivilianBAIState.h"

//#define _USE_MATH_DEFINES
//#include <cmath>


namespace OpenXcom
{

/**
 * Sets up a CivilianBAIState.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param unit			- pointer to the BattleUnit
 * @param node			- pointer to the Node the unit originates from (default nullptr)
 */
CivilianBAIState::CivilianBAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const node)
	:
		BattleAIState(
			battleSave,
			unit),
		_tuEscape(0),
		_targetsHostile(0),
		_spottersHostile(0)
{
	_startNode = node;

	_escapeAction = new BattleAction();
	_patrolAction = new BattleAction();
}

/**
 * Deletes the CivilianBAIState.
 */
CivilianBAIState::~CivilianBAIState()
{
	delete _escapeAction;
	delete _patrolAction;
}

/**
 * Loads the AI state from a YAML file.
 * @param node - reference a YAML node
 */
void CivilianBAIState::load(const YAML::Node& node)
{
	_AIMode = static_cast<AIMode>(node["AIMode"].as<int>(0));

	const int
		startNodeId	= node["startNode"]	.as<int>(-1),
		toNodeId	= node["toNode"]	.as<int>(-1);

	if (startNodeId != -1)
		_startNode = _battleSave->getNodes()->at(static_cast<size_t>(startNodeId));

	if (toNodeId != -1)
		_toNode = _battleSave->getNodes()->at(static_cast<size_t>(toNodeId));
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node CivilianBAIState::save() const
{
	int
		startNodeId = -1,
		toNodeId = -1;

	if (_startNode != nullptr)	startNodeId	= _startNode->getId();
	if (_toNode != nullptr)		toNodeId	= _toNode->getId();

	YAML::Node node;

	node["startNode"]	= startNodeId;
	node["toNode"]		= toNodeId;
	node["AIMode"]		= static_cast<int>(_AIMode);

	return node;
}

/**
 * Enters the current AI state.
 */
//void CivilianBAIState::enter(){}

/**
 * Exits the current AI state.
 */
//void CivilianBAIState::exit(){}

/**
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action (possible) AI action to execute after thinking is done.
 */
void CivilianBAIState::think(BattleAction* const action)
{
	//Log(LOG_INFO) << "CivilianBAIState::think()";
 	action->type = BA_RETHINK;
	action->actor = _unit;

	_escapeAction->AIcount = action->AIcount;

	_targetsHostile = countHostiles();
	_spottersHostile = countSpotters(_unit->getPosition());

//	if (_traceAI)
//	{
//		Log(LOG_INFO) << "Civilian Unit has " << _targetsHostile << " enemies visible, " << _spottersHostile << " of whom are spotting him. ";
//		std::string AIMode;
//		switch (_AIMode)
//		{
//			case 0: AIMode = "Patrol"; break;
//			case 3: AIMode = "Escape";
//		}
//		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
//	}

	if (_spottersHostile != 0 && _tuEscape == 0)
		setupEscape();

	setupPatrol();

	bool evaluate = false;
	switch (_AIMode)
	{
		case AI_ESCAPE:
			if (_spottersHostile == 0) evaluate = true;
			break;
		case AI_PATROL:
			if (_spottersHostile != 0 || _targetsHostile != 0
				|| RNG::percent(10) == true)
			{
				evaluate = true;
			}
	}

	if (_spottersHostile > 2
		|| _unit->getHealth() < 2 * _unit->getBattleStats()->health / 3)
	{
		evaluate = true;
	}

	if (evaluate == true)
	{
		evaluateAIMode();
//		if (_traceAI)
//		{
//			std::string AIMode;
//			switch (_AIMode)
//			{
//				case 0: AIMode = "Patrol"; break;
//				case 3: AIMode = "Escape";
//			}
//			Log(LOG_INFO) << "Re-Evaluated, now using " << AIMode << " behaviour";
//		}
	}

	switch (_AIMode)
	{
		case AI_ESCAPE:
			action->type = _escapeAction->type;
			action->target = _escapeAction->target;
			action->AIcount = 3;
			action->desperate = true;

			_unit->dontReselect();
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false);
			break;
		case AI_PATROL:
			action->type = _patrolAction->type;
			action->target = _patrolAction->target;
	}

	if (action->type == BA_MOVE
		&& action->target != _unit->getPosition())
	{
		_tuEscape = 0;
	}
}

/**
 * Counts the quantity of Hostiles that the civilian sees and sets the closest
 * one as the '_aggroTarget'.
 * @note If none of the hostiles can target the civilian this returns 0.
 * @return, quantity of potential perps
 */
int CivilianBAIState::countHostiles() // private.
{
	int
		tally = 0,
		dist = 1000,
		distTest;

	_aggroTarget = nullptr;

	const Position originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(_unit);

	Position targetVoxel;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false
			&& (*i)->getFaction() == FACTION_HOSTILE)
		{
			if (_battleSave->getTileEngine()->visible(_unit, (*i)->getTile()) == true)
			{
				++tally;
				distTest = TileEngine::distance(
											_unit->getPosition(),
											(*i)->getPosition());
				if (distTest < dist
					&& _battleSave->getTileEngine()->canTargetUnit(
																&originVoxel,
																(*i)->getTile(),
																&targetVoxel,
																_unit) == true)
				{
					dist = distTest;
					_aggroTarget = *i;
				}
			}
		}
	}

	if (_aggroTarget != nullptr)
		return tally;

	return 0;
}

/**
 * Counts how many aLiens spot this unit.
 * @param pos - reference the position of unit getting spotted
 * @return, qty of spotters
 */
int CivilianBAIState::countSpotters(const Position& pos) const // private.
{
	bool checking = (pos != _unit->getPosition());
	int tally = 0;

	Position
		originVoxel,
		targetVoxel;

	const BattleUnit* unit;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->isOut_t(OUT_STAT) == false
			&& (*i)->getFaction() == FACTION_HOSTILE
			&& TileEngine::distance(pos, (*i)->getPosition()) < 25)
		{
			if (checking == true)
				unit = _unit;
			else
				unit = nullptr;

			originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(*i);
			if (_battleSave->getTileEngine()->canTargetUnit(
														&originVoxel,
														_battleSave->getTile(pos),
														&targetVoxel,
														*i,
														unit) == true)
			{
				++tally;
			}
		}
	}

	return tally;
}

/**
 * Sets up an escape objective.
 */
void CivilianBAIState::setupEscape() // private.
{
	int
		bestTileScore = -100000,
		tileScore,
		tu = _unit->getTimeUnits() / 2,
		unitsSpotting = countSpotters(_unit->getPosition()),
		dist = 0;

	countHostiles(); // sets an _aggroTarget
	if (_aggroTarget != nullptr)
		dist = TileEngine::distance(
								_unit->getPosition(),
								_aggroTarget->getPosition());

	const Tile* tile;
	Position posBest (0,0,0);

	Pathfinding* const pf (_battleSave->getPathfinding());
	pf->setPathingUnit(_unit);
	std::vector<int> reachable (pf->findReachable(_unit, tu));

	std::vector<Position> tileSearch (_battleSave->getTileSearch());
	RNG::shuffle(tileSearch.begin(), tileSearch.end());

	bool coverFound = false;
	size_t tries = 0;
	while (tries < 150 && coverFound == false)
	{
		_escapeAction->target = _unit->getPosition();

		tileScore = 0;

		if (tries < _battleSave->SEARCH_SIZE) //121 // looking for cover
		{
			// looking for cover
			_escapeAction->target.x += tileSearch[tries].x;
			_escapeAction->target.y += tileSearch[tries].y;

			tileScore = BASE_SUCCESS_SYSTEMATIC;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (unitsSpotting != 0)
				{
					// maybe don't stay in the same spot? move or something if there's any point to it?
					_escapeAction->target.x += RNG::generate(-20,20);
					_escapeAction->target.y += RNG::generate(-20,20);
				}
				else
					tileScore += CUR_TILE_PREF;
			}
		}
		else
		{
			//if (_traceAI && tries == 121) Log(LOG_INFO) << "best score after systematic search was: " << bestTileScore;

			tileScore = BASE_SUCCESS_DESPERATE; // ruuuuuuun

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10,10);
			_escapeAction->target.y += RNG::generate(-10,10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (_escapeAction->target.z < 0)
				_escapeAction->target.z = 0;
			else if (_escapeAction->target.z >= _battleSave->getMapSizeZ())
				_escapeAction->target.z = _unit->getPosition().z;
		}

		// civilians shouldn't have any tactical sense anyway so save some CPU cycles here
		tries += 10;

		// THINK, DAMN YOU1
		tile = _battleSave->getTile(_escapeAction->target);

		int distTarget = 0;
		if (_aggroTarget != nullptr)
			distTarget = TileEngine::distance(
										_aggroTarget->getPosition(),
										_escapeAction->target);

		if (dist >= distTarget)
			tileScore -= (distTarget - dist) * 10;
		else
			tileScore += (distTarget - dist) * 10;

		if (tile == nullptr) // no you can't quit the battlefield by running off the map.
			tileScore = -100001;
		else
		{
			const int spotters = countSpotters(_escapeAction->target);

			// just ignore unreachable tiles
			if (std::find(
						reachable.begin(),
						reachable.end(),
						_battleSave->getTileIndex(tile->getPosition())) == reachable.end())
			{
				continue;
			}

			if (_spottersHostile != 0 || spotters != 0)
			{
				if (_spottersHostile <= spotters) // that's for giving away our position
					tileScore -= (1 + spotters - _spottersHostile) * EXPOSURE_PENALTY;
				else
					tileScore += (_spottersHostile - spotters) * EXPOSURE_PENALTY;
			}

			if (tile->getFire() != 0)
				tileScore -= FIRE_PENALTY;

//			if (_traceAI)
//			{
//				tile->setPreviewColor(tileScore < 0 ? 3 : (tileScore < FAST_PASS_THRESHOLD / 2 ? 8 : (tileScore < FAST_PASS_THRESHOLD ? 9 : 5)));
//				tile->setPreviewDir(10);
//				tile->setPreviewTu(tileScore);
//			}
		}

		if (tile != nullptr && tileScore > bestTileScore)
		{
			// calculate TUs to tile;
			// could be getting this from findReachable() somehow but that would break something for sure...
			pf->calculate(
						_unit,
						_escapeAction->target,
						nullptr,
						tu);

			if (_escapeAction->target == _unit->getPosition()
				|| pf->getStartDirection() != -1)
			{
				bestTileScore = tileScore;
				posBest = _escapeAction->target;
				_tuEscape = pf->getTotalTUCost();

				if (_escapeAction->target == _unit->getPosition())
					_tuEscape = 1;

//				if (_traceAI)
//				{
//					tile->setPreviewColor(tileScore < 0? 7: (tileScore < FAST_PASS_THRESHOLD / 2? 10: (tileScore < FAST_PASS_THRESHOLD ?4: 5)));
//					tile->setPreviewDir(10);
//					tile->setPreviewTu(tileScore);
//				}
			}

			pf->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogogo!!
		}
	}

	_escapeAction->target = posBest;

	//if (_traceAI) _battleSave->getTile(_escapeAction->target)->setPreviewColor(13);

	if (bestTileScore < -99999)
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";
		// do something, just don't look dumbstruck :P
		_escapeAction->type = BA_RETHINK;
		return;
	}

	//if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << tries << " tries, " << TileEngine::distance(_unit->getPosition(), posBest) << " squares or so away.";
	_escapeAction->type = BA_MOVE;
}

/**
 * Sets up a patrol objective.
 */
void CivilianBAIState::setupPatrol() // private.
{
	if (_toNode != nullptr
		&& _unit->getPosition() == _toNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";
		// destination reached
		// head off to next patrol node
		_startNode = _toNode;
		_toNode = nullptr;
	}

	if (_startNode == nullptr)
		_startNode = _battleSave->getNearestNode(_unit);
/*{
		// assume closest node as "from node"
		// on same level to avoid strange things, and the node has to match unit size or it will freeze
		int dist = 1000000;
		for (std::vector<Node*>::const_iterator
				i = _battleSave->getNodes()->begin();
				i != _battleSave->getNodes()->end();
				++i)
		{
			Node* node = *i;
			const int distTest = _battleSave->getTileEngine()->distanceSqr(_unit->getPosition(), node->getPosition());
			if (_unit->getPosition().z == node->getPosition().z
				&& distTest < dist
				&& (node->getNodeType() & Node::TYPE_SMALL))
			{
				_startNode = node;
				dist = distTest;
			}
		}
	} */

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	int triesLeft = 5; // look for a new node to walk towards
	while (_toNode == nullptr && triesLeft != 0)
	{
		--triesLeft;

		_toNode = _battleSave->getPatrolNode(true, _unit, _startNode);
		if (_toNode == nullptr)
			_toNode = _battleSave->getPatrolNode(false, _unit, _startNode);

		if (_toNode != nullptr)
		{
			pf->calculate(_unit, _toNode->getPosition());
			if (pf->getStartDirection() == -1)
				_toNode = nullptr;

			pf->abortPath();
		}
	}

	if (_toNode != nullptr)
	{
		_patrolAction->actor = _unit;
		_patrolAction->type = BA_MOVE;
		_patrolAction->target = _toNode->getPosition();
	}
	else
		_patrolAction->type = BA_RETHINK;
}

/**
 * Re-evaluates the situation and makes a decision from available options.
 */
void CivilianBAIState::evaluateAIMode() // private.
{
	if (_toNode != nullptr)
	{
		float
			escape = 0.f,
			patrol = 30.f;

		if (_targetsHostile != 0)
		{
			escape = 15.f;
			patrol = 15.f;
		}

		if (_spottersHostile != 0)
		{
			patrol = 0.f;
			if (_tuEscape == 0) setupEscape();
		}

		switch (_AIMode)
		{
			case AI_PATROL: patrol *= 1.1f; break;
			case AI_ESCAPE: escape *= 1.1f;
		}

		const float healthRatio = static_cast<float>(_unit->getHealth())
								/ static_cast<float>(_unit->getBattleStats()->health);
		if (healthRatio < 0.33f)
			escape *= 1.7f;
		else if (healthRatio < 0.67f)
			escape *= 1.4f;
		else if (healthRatio < 0.999f)
			escape *= 1.1f;

		switch (_unit->getAggression())
		{
			case 0: escape *= 1.4f; break;
			case 2: escape *= 0.7f;
		}

		if (_spottersHostile != 0)
			escape = 10.f * escape * static_cast<float>(_spottersHostile + 10) / 100.f;
		else
			escape /= 2.f;

		if (RNG::generate(1, static_cast<int>(patrol) + static_cast<int>(escape)) <= static_cast<int>(patrol))
		{
			_AIMode = AI_PATROL;
			return;
		}
	}

	_AIMode = AI_ESCAPE;
}

}
