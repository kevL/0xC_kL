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
		_targetsHostile(0)
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
		stopNodeId	= node["stopNode"]	.as<int>(-1);

	if (startNodeId != -1)
		_startNode = _battleSave->getNodes()->at(static_cast<size_t>(startNodeId));

	if (stopNodeId != -1)
		_stopNode = _battleSave->getNodes()->at(static_cast<size_t>(stopNodeId));
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node CivilianBAIState::save() const
{
	int
		startNodeId = -1,
		stopNodeId = -1;

	if (_startNode != nullptr)	startNodeId	= _startNode->getId();
	if (_stopNode != nullptr)	stopNodeId	= _stopNode->getId();

	YAML::Node node;

	node["startNode"]	= startNodeId;
	node["stopNode"]	= stopNodeId;
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

	_targetsHostile = tallyAggro();
	_spottersOrigin = tallySpotters(_unit->getPosition());

//	if (_traceAI)
//	{
//		Log(LOG_INFO) << "Civilian Unit has " << _targetsHostile << " enemies visible, " << _spottersOrigin << " of whom are spotting him. ";
//		std::string AIMode;
//		switch (_AIMode)
//		{
//			case 0: AIMode = "Patrol"; break;
//			case 3: AIMode = "Escape";
//		}
//		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
//	}

	if (_spottersOrigin != 0 && _tuEscape == 0)
		setupEscape();

	setupPatrol();

	bool evaluate = false;
	switch (_AIMode)
	{
		case AI_ESCAPE:
			if (_spottersOrigin == 0) evaluate = true;
			break;
		case AI_PATROL:
			if (_spottersOrigin != 0 || _targetsHostile != 0
				|| RNG::percent(10) == true)
			{
				evaluate = true;
			}
	}

	if (_spottersOrigin > 2
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
 * one as the '_unitAggro'.
 * @note If none of the hostiles can target the civilian this returns 0.
 * @return, quantity of potential perps
 */
int CivilianBAIState::tallyAggro() // private.
{
	int
		tally = 0,
		dist = 1000,
		distTest;

	_unitAggro = nullptr;

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
					_unitAggro = *i;
				}
			}
		}
	}

	if (_unitAggro != nullptr)
		return tally;

	return 0;
}

/**
 * Counts how many aLiens spot this unit.
 * @param pos - reference the position of unit getting spotted
 * @return, qty of spotters
 */
int CivilianBAIState::tallySpotters(const Position& pos) const // private.
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
		if ((*i)->getFaction() == FACTION_HOSTILE
			&& (*i)->isOut_t(OUT_STAT) == false
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
	tallyAggro(); // sets _unitAggro

	int
		distAggroOrigin,
		distAggroTarget;
	if (_unitAggro != nullptr)
		distAggroOrigin = TileEngine::distance(
										_unit->getPosition(),
										_unitAggro->getPosition());
	else
		distAggroOrigin = 0;

	const int spottersOrigin = tallySpotters(_unit->getPosition());

	const Tile* tile;

	int
		score (ESCAPE_FAIL),
		scoreTest;

	std::vector<Position> tileSearch (_battleSave->getTileSearch());
	RNG::shuffle(tileSearch.begin(), tileSearch.end());

	Pathfinding* const pf (_battleSave->getPathfinding());
	pf->setPathingUnit(_unit);

	const int tu = _unit->getTimeUnits() / 2;
	const std::vector<size_t> reachable (pf->findReachable(_unit, tu));

	bool coverFound (false);
	size_t i = 0;
	while (coverFound == false && i <= _battleSave->SEARCH_SIZE)
	{
		_escapeAction->target = _unit->getPosition();

		if (i < _battleSave->SEARCH_SIZE)
		{
			scoreTest = BASE_SUCCESS_SYSTEMATIC;

			_escapeAction->target.x += tileSearch[i].x;
			_escapeAction->target.y += tileSearch[i].y;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (spottersOrigin != 0)
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
			//if (_traceAI && t == 121) Log(LOG_INFO) << "best score after systematic search was: " << score;
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
		i += 10;


		if (_unitAggro != nullptr)
			distAggroTarget = TileEngine::distance(
										_escapeAction->target,
										_unitAggro->getPosition());
		else
			distAggroTarget = 0;

		scoreTest += (distAggroTarget - distAggroOrigin) * EXPOSURE_PENALTY;

		if ((tile = _battleSave->getTile(_escapeAction->target)) != nullptr
			&& std::find(
					reachable.begin(),
					reachable.end(),
					_battleSave->getTileIndex(tile->getPosition())) != reachable.end())
		{
			scoreTest += (_spottersOrigin - tallySpotters(_escapeAction->target)) * EXPOSURE_PENALTY;

			if (tile->getFire() != 0)
				scoreTest -= FIRE_PENALTY;
			else
				scoreTest += tile->getSmoke() * 3;
//			if (_traceAI) {
//				tile->setPreviewColor(scoreTest < 0 ? 3 : (scoreTest < FAST_PASS_THRESHOLD / 2 ? 8 : (scoreTest < FAST_PASS_THRESHOLD ? 9 : 5)));
//				tile->setPreviewDir(10);
//				tile->setPreviewTu(scoreTest); }

			if (scoreTest > score)
			{
				pf->calculate(
							_unit,
							_escapeAction->target,
							nullptr,
							tu);

				if (pf->getStartDirection() != -1
					|| _escapeAction->target == _unit->getPosition())
				{
					score = scoreTest;

					if (_escapeAction->target != _unit->getPosition())
						_tuEscape = pf->getTuCostTotalPf();
					else
						_tuEscape = 1;
//					if (_traceAI) {
//						tile->setPreviewColor(scoreTest < 0 ? 7: (scoreTest < FAST_PASS_THRESHOLD / 2 ? 10: (scoreTest < FAST_PASS_THRESHOLD ? 4: 5)));
//						tile->setPreviewDir(10);
//						tile->setPreviewTu(scoreTest); }
				}
				pf->abortPath();

				if (score > FAST_PASS_THRESHOLD)
					coverFound = true;
			}
		}
	}

	if (score != ESCAPE_FAIL)
	{
		//if (_traceAI) _battleSave->getTile(_escapeAction->target)->setPreviewColor(13);
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << i << " tries, "
		//<< TileEngine::distance(_unit->getPosition(), _escapeAction->target) << " tiles.";
		_escapeAction->type = BA_MOVE;
	}
	else
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";
		_escapeAction->type = BA_RETHINK;
//		_escapeAction->target = Position(0,0,0);
	}
}

/**
 * Sets up a patrol objective.
 */
void CivilianBAIState::setupPatrol() // private.
{
	if (_stopNode != nullptr
		&& _unit->getPosition() == _stopNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";
		// destination reached
		// head off to next patrol node
		_startNode = _stopNode;
		_stopNode = nullptr;
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
	while (_stopNode == nullptr && triesLeft != 0)
	{
		--triesLeft;

		_stopNode = _battleSave->getPatrolNode(true, _unit, _startNode);
		if (_stopNode == nullptr)
			_stopNode = _battleSave->getPatrolNode(false, _unit, _startNode);

		if (_stopNode != nullptr)
		{
			pf->calculate(_unit, _stopNode->getPosition());
			if (pf->getStartDirection() == -1)
				_stopNode = nullptr;

			pf->abortPath();
		}
	}

	if (_stopNode != nullptr)
	{
		_patrolAction->actor = _unit;
		_patrolAction->type = BA_MOVE;
		_patrolAction->target = _stopNode->getPosition();
	}
	else
		_patrolAction->type = BA_RETHINK;
}

/**
 * Re-evaluates the situation and makes a decision from available options.
 */
void CivilianBAIState::evaluateAIMode() // private.
{
	if (_stopNode != nullptr)
	{
		float
			escape = 0.f,
			patrol = 30.f;

		if (_targetsHostile != 0)
		{
			escape = 15.f;
			patrol = 15.f;
		}

		if (_spottersOrigin != 0)
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

		if (_spottersOrigin != 0)
			escape = 10.f * escape * static_cast<float>(_spottersOrigin + 10) / 100.f;
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
