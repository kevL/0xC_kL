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


namespace OpenXcom
{

/**
 * Sets up a CivilianBAIState.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param unit			- pointer to the BattleUnit
 * @param startNode		- pointer to the Node the unit originates at (default nullptr)
 */
CivilianBAIState::CivilianBAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const startNode)
	:
		BattleAIState(
			battleSave,
			unit,
			startNode),
		_targetsHostile(0)
{
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
	BattleAIState::load(node);
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node CivilianBAIState::save() const
{
	return BattleAIState::save();
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
 * @param action - AI BattleAction to execute after thinking is done (BattlescapeGame.h)
 */
void CivilianBAIState::think(BattleAction* const action)
{
	//Log(LOG_INFO) << "CivilianBAIState::think()";

	_pf = _battleSave->getPathfinding();
	_te = _battleSave->getTileEngine();

	action->type = BA_THINK;

	_pf->setPathingUnit(_unit);

	_spottersOrigin = tallySpotters(_unit->getPosition());
	_targetsHostile = selectHostile(); // sets _unitAggro.

	setupPatrol();

	if (_tuEscape == -1 && _spottersOrigin != 0)
		setupEscape();

	bool evaluate (false);
	switch (_AIMode)
	{
		case AI_PATROL:
			if (_spottersOrigin != 0 || _targetsHostile != 0
				|| RNG::percent(10) == true)
			{
				evaluate = true;
			}
			break;

		case AI_ESCAPE:
			if (_spottersOrigin == 0)
				evaluate = true;
	}

	if (evaluate == false && _spottersOrigin != 0) // (_spottersOrigin != 0 || _unit->getHealth() < _unit->getBattleStats()->health * 2 / 3)
		evaluate = true;

	if (evaluate == true)
		evaluateAiMode();

	switch (_AIMode)
	{
		case AI_PATROL:
			action->type = _patrolAction->type;
			action->posTarget = _patrolAction->posTarget;
			break;

		case AI_ESCAPE:
			action->type = _escapeAction->type;
			action->posTarget = _escapeAction->posTarget;
//			action->finalAction = true; // <- AlienBAI uses this instead of AICount= 3 <--
			action->AIcount = 3;
			action->desperate = true;

			_unit->dontReselect();
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false);
	}

	if (action->type == BA_MOVE
		&& action->posTarget != _unit->getPosition())
	{
		_tuEscape = -1;
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
		_startNode = _stopNode;
		_stopNode = nullptr;
	}

	if (_startNode == nullptr)
		_startNode = _battleSave->getNearestNode(_unit);

	_stopNode = _battleSave->getPatrolNode(true, _unit, _startNode);
	if (_stopNode == nullptr)
		_stopNode = _battleSave->getPatrolNode(false, _unit, _startNode);

	if (_stopNode != nullptr)
	{
		_pf->calculatePath(_unit, _stopNode->getPosition());
		if (_pf->getStartDirection() == -1)
			_stopNode = nullptr;

		_pf->abortPath();
	}

	if (_stopNode != nullptr)
	{
		_patrolAction->type = BA_MOVE;
		_patrolAction->posTarget = _stopNode->getPosition();
	}
	else
		_patrolAction->type = BA_THINK;
}

/**
 * Sets up an escape objective.
 */
void CivilianBAIState::setupEscape() // private.
{
//	if (_unitAggro == nullptr)
//	selectHostile(); // sets _unitAggro
//	const int spottersOrigin (tallySpotters(_unit->getPosition()));
	_spottersOrigin = tallySpotters(_unit->getPosition());

	int
		distAggroOrigin,
		distAggroTarget;
	if (_unitAggro != nullptr)
		distAggroOrigin = TileEngine::distance(
										_unit->getPosition(),
										_unitAggro->getPosition());
	else
		distAggroOrigin = 0;

	Tile* tile;
	int
		score (ESCAPE_FAIL),
		scoreTest;

	std::vector<Position> tileSearch (_battleSave->getTileSearch());
	RNG::shuffle(tileSearch.begin(), tileSearch.end());

	const int tuHalf (_unit->getTu() >> 1u);
	_reachable = _pf->findReachable(_unit, tuHalf);

//	bool coverFound (false);
	size_t i (0u);
	while (/*coverFound == false &&*/ i <= SavedBattleGame::SEARCH_SIZE)
	{
		_escapeAction->posTarget = _unit->getPosition();

		if (i < SavedBattleGame::SEARCH_SIZE)
		{
			scoreTest = BASE_SUCCESS_SYSTEMATIC;

			_escapeAction->posTarget.x += tileSearch[i].x;
			_escapeAction->posTarget.y += tileSearch[i].y;

			if (_escapeAction->posTarget == _unit->getPosition())
			{
//				if (spottersOrigin != 0)
				if (_spottersOrigin != 0)
				{
					_escapeAction->posTarget.x += RNG::generate(-20,20);
					_escapeAction->posTarget.y += RNG::generate(-20,20);
				}
				else
					scoreTest += CUR_TILE_PREF;
			}
		}
		else
		{
			scoreTest = BASE_SUCCESS_DESPERATE;

			_escapeAction->posTarget = _unit->getPosition();
			_escapeAction->posTarget.x += RNG::generate(-10,10);
			_escapeAction->posTarget.y += RNG::generate(-10,10);
			_escapeAction->posTarget.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (_escapeAction->posTarget.z < 0)
				_escapeAction->posTarget.z = 0;
			else if (_escapeAction->posTarget.z >= _battleSave->getMapSizeZ())
				_escapeAction->posTarget.z = _battleSave->getMapSizeZ();
		}
		i += static_cast<size_t>(RNG::generate(1,10));


		if (_unitAggro != nullptr)
			distAggroTarget = TileEngine::distance(
												_escapeAction->posTarget,
												_unitAggro->getPosition());
		else
			distAggroTarget = 0;

		scoreTest += (distAggroTarget - distAggroOrigin) * EXPOSURE_PENALTY;

		if ((tile = _battleSave->getTile(_escapeAction->posTarget)) != nullptr
			&& std::find(
					_reachable.begin(),
					_reachable.end(),
					_battleSave->getTileIndex(tile->getPosition())) != _reachable.end())
		{
//			scoreTest += (spottersOrigin - tallySpotters(_escapeAction->target)) * EXPOSURE_PENALTY;
			scoreTest += (_spottersOrigin - tallySpotters(_escapeAction->posTarget)) * EXPOSURE_PENALTY;

			if (tile->getFire() != 0)
				scoreTest -= FIRE_PENALTY;
			else
				scoreTest += tile->getSmoke() * SMOKE_BONUS_MULT;

			if (_traceAI) {
				tile->setPreviewColor(debugTraceColor(false, scoreTest));
				tile->setPreviewDir(TRACE_DIR);
				tile->setPreviewTu(scoreTest); }

			if (scoreTest > score)
			{
				_pf->calculatePath(
							_unit,
							_escapeAction->posTarget,
							tuHalf);

				if (_pf->getStartDirection() != -1
					|| _escapeAction->posTarget == _unit->getPosition())
				{
					score = scoreTest;
					_tuEscape = _pf->getTuCostTotalPf();

					if (_traceAI) {
						tile->setPreviewColor(debugTraceColor(true, scoreTest));
						tile->setPreviewDir(TRACE_DIR);
						tile->setPreviewTu(scoreTest); }
				}
				_pf->abortPath();

//				if (score > FAST_PASS_THRESHOLD)
//					coverFound = true;
			}
		}
	}

	if (score != ESCAPE_FAIL)
	{
		if (_traceAI) _battleSave->getTile(_escapeAction->posTarget)->setPreviewColor(TRACE_PURPLE);
		_escapeAction->type = BA_MOVE;
	}
	else
	{
		_escapeAction->type = BA_THINK;
		_tuEscape = -1;
	}
}

/**
 * Evaluates the situation and makes a decision from available options.
 */
void CivilianBAIState::evaluateAiMode() // private.
{
	if (_stopNode != nullptr)
	{
		float
			patrolOdds,
			escapeOdds;

		if (_targetsHostile != 0)
		{
			patrolOdds = 10.f;
			escapeOdds = 20.f;
		}
		else
		{
			patrolOdds = 30.f;
			escapeOdds = 0.f;
		}

		if (_spottersOrigin != 0)
		{
			patrolOdds = 0.f;
			if (_tuEscape == -1)
				setupEscape();
		}

		switch (_AIMode)
		{
			case AI_PATROL: patrolOdds *= 1.1f; break;
			case AI_ESCAPE: escapeOdds *= 1.1f;
		}

		const float healthRatio (static_cast<float>(_unit->getHealth())
							   / static_cast<float>(_unit->getBattleStats()->health));
		if (healthRatio < 0.33f)
			escapeOdds *= 1.7f;
		else if (healthRatio < 0.67f)
			escapeOdds *= 1.4f;
		else if (healthRatio < 0.999f)
			escapeOdds *= 1.1f;

		switch (_unit->getAggression())
		{
			case 0:
				escapeOdds *= 1.4f;
				break;
			case 1:
				break;
			default:
			case 2:
				escapeOdds *= 0.7f;
		}

		if (_spottersOrigin != 0)
			escapeOdds = escapeOdds * 10.f * static_cast<float>(_spottersOrigin + EXPOSURE_PENALTY) / 100.f;
		else
			escapeOdds /= 2.f;

		if (RNG::generate(0.f, patrolOdds + escapeOdds) <= patrolOdds)
		{
			_AIMode = AI_PATROL;
			return;
		}
	}

	_AIMode = AI_ESCAPE;
}

/**
 * Counts the quantity of Hostiles that the civilian sees and sets the closest
 * aLien as the '_unitAggro'.
 * @return, qty of potential perps
 */
int CivilianBAIState::selectHostile() // private.
{
	int
		tally (0),
		dist (1000000),
		distTest;

	_unitAggro = nullptr;

	const Position originVoxel (_te->getSightOriginVoxel(_unit));
	Position targetVoxel;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_HOSTILE
			&& (*i)->isOut_t(OUT_STAT) == false
			&& _te->visible(_unit, (*i)->getUnitTile()) == true)
		{
			++tally;
			distTest = TileEngine::distSqr(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < dist
				&& _te->canTargetUnit(
								&originVoxel,
								(*i)->getUnitTile(),
								&targetVoxel,
								_unit) == true)
			{
				dist = distTest;
				_unitAggro = *i;
			}
		}
	}

	if (_unitAggro != nullptr)
		return tally;

	return 0;
}

/**
 * Counts how many aLiens spot the civilian.
 * @param pos - reference to a Position to check
 * @return, qty of spotters
 */
int CivilianBAIState::tallySpotters(const Position& pos) const // private.
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
		if ((*i)->getFaction() == FACTION_HOSTILE // Could exclude MC'd xCom ...
			&& (*i)->isOut_t(OUT_STAT) == false
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

}
