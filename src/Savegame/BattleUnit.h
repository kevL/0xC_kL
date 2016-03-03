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

#ifndef OPENXCOM_BATTLEUNIT_H
#define OPENXCOM_BATTLEUNIT_H

//#include <string>
//#include <vector>

//#include <yaml-cpp/yaml.h>

#include "SavedGame.h" // GameDifficulty enum
#include "Soldier.h"

#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/Position.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

class AlienBAIState;
class BattleAIState;
class BattleItem;
class BattlescapeGame;
class CivilianBAIState;
class Language;
class Node;
class RuleArmor;
class RuleInventory;
class RuleUnit;
class SavedBattleGame;
class SavedGame;
class Soldier;
class Surface;
class Tile;

struct BattleUnitStatistics;


enum UnitStatus
{
	STATUS_STANDING,	//  0
	STATUS_WALKING,		//  1
	STATUS_FLYING,		//  2
	STATUS_TURNING,		//  3
	STATUS_AIMING,		//  4
	STATUS_COLLAPSING,	//  5
	STATUS_DEAD,		//  6
	STATUS_UNCONSCIOUS,	//  7
	STATUS_PANICKING,	//  8
	STATUS_BERSERK,		//  9
	STATUS_LIMBO,		// 10 won't participate in a 'next stage' battle.
	STATUS_DISABLED		// 11 dead or unconscious but doesn't know it yet.
};

enum UnitFaction
{
	FACTION_NONE = -1,	//-1
	FACTION_PLAYER,		// 0
	FACTION_HOSTILE,	// 1
	FACTION_NEUTRAL		// 2
};

enum UnitSide
{
	SIDE_FRONT,			// 0
	SIDE_LEFT,			// 1
	SIDE_RIGHT,			// 2
	SIDE_REAR,			// 3
	SIDE_UNDER			// 4
};

enum UnitBodyPart
{
	BODYPART_HEAD,		// 0
	BODYPART_TORSO,		// 1
	BODYPART_RIGHTARM,	// 2
	BODYPART_LEFTARM,	// 3
	BODYPART_RIGHTLEG,	// 4
	BODYPART_LEFTLEG,	// 5
	BODYPART_NONE		// 6
};

enum OutCheck
{
	OUT_ALL,		// 0
	OUT_STAT,		// 1
	OUT_HEALTH,		// 2
	OUT_STUNNED,	// 3
	OUT_HLTH_STUN	// 4
};

enum ActiveHand
{
	AH_NONE,	// 0
	AH_RIGHT,	// 1
	AH_LEFT		// 2
};


/**
 * This holds info about a mobile object in the Battlescape whether controlled
 * by player or AI.
 */
class BattleUnit // no copy cTor.
{

	public:
		static const size_t PARTS_BODY = 6;

private:
	static const size_t PARTS_ARMOR = 5; // doubles as both armorValues and sprites' cache
	static const int DOSE_LETHAL = 3;
//	static const int SPEC_WEAPON_MAX = 3;

	bool
		_aboutToFall,
		_cacheInvalid,
		_dashing,
		_diedByFire,
		_dontReselect,
		_floating,
		_hasBeenStunned,
		_hasCried,
		_hidingForTurn,
		_kneeled,
		_psiBlock,
		_revived,
		_stopShot, // to stop a unit from firing/throwing if it spots a new opponent during turning
		_takenExpl, // used to stop large units from taking damage for each part.
		_takenFire,
		_visible,
		_walkBackwards;
	int
		_aimPhase,
		_coverReserve,
		_armorHp[PARTS_ARMOR],
		_dir,
		_dirTurret,
		_dirTo,
		_dirToTurret,
		_dirFace, // used only during strafing moves
		_dirTurn, // used for determining 180 degree turn direction
		_dirVertical,
		_energy,
		_expBravery,
		_expFiring,
		_expMelee,
		_expPsiSkill,
		_expPsiStrength,
		_expReactions,
		_expThrowing,
		_fallPhase,
		_fatalWounds[PARTS_BODY],
		_fire,
		_health,
		_id,
		_kills,
		_morale,
		_motionPoints,
		_drugDose,
		_spinPhase,
		_stunLevel,
		_tu,
		_turnsExposed,
		_walkPhase,
		_walkPhaseHalf,
		_walkPhaseFull,
		_mcStrength,
		_mcSkill;
	size_t _battleOrder;

	BattleAIState* _unitAIState;
//	BattleItem* _specWeapon[SPEC_WEAPON_MAX];
	BattleItem* _fist;
	BattlescapeGame* _battleGame;
	BattleUnit* _charging;
	Surface* _cache[5];
	Tile* _tile;

	Position
		_pos,
		_posStart,
		_posStop;
	UnitFaction
		_faction,
		_originalFaction,
		_killedBy;
	UnitStatus _status;
	ActiveHand _activeHand;

	std::list<BattleUnit*> _rfSpotters;

	std::vector<int> _spotted; // for saving/loading '_hostileUnitsThisTurn'

	std::vector<BattleItem*> _inventory;
	std::vector<BattleUnit*>
		_hostileUnits,
		_hostileUnitsThisTurn;
//	std::vector<Tile*> _visibleTiles;

	std::string _spawnType;

	// static data
	UnitStats _stats;

	bool _isZombie;

	int
		_aggression,
		_aggroSound,
		_deathSound,
		_floatHeight,
		_kneelHeight,
		_intelligence,
		_moveSound,
		_rankInt,
		_standHeight,
		_turretType,
		_value;

	std::string
		_race,
		_rank,
		_type;

	std::wstring _name;

	std::vector<size_t> _loftSet;
	std::vector<std::pair<Uint8, Uint8>> _recolor;

	const RuleArmor* _armor;
	Soldier* _geoscapeSoldier;
	const RuleUnit* _unitRule;

	MovementType _moveType;
	SoldierGender _gender;
	SpecialAbility _specab;

	BattleUnitStatistics* _statistics;
	int _murdererId; // used to credit the murderer with the kills that this unit got by blowing up on death


	/// Converts an amount of experience to a stat increase.
	int improveStat(int xp);

	/// Helper function initing recolor vector.
	void setRecolor(
			int basicLook,
			int utileLook,
			int rankLook);

	/// Gets if a grenade is suitable for an AI or panic situation.
	bool isGrenadeSuitable(const BattleItem* const grenade) const;


	public:
		static const int MAX_SOLDIER_ID = 1000000;

		// scratch value for AI's left hand to tell its right hand what's up...
		// don't zone out and start patrolling again
		Position _lastCover;


		/// Creates a BattleUnit from a geoscape Soldier.
		BattleUnit( // xCom operatives
				Soldier* const sol,
				const GameDifficulty diff); // for VictoryPts value per death.
		/// Creates a BattleUnit from Unit rule.
		BattleUnit( // aLiens, civies, & Tanks
				RuleUnit* const unitRule,
				const UnitFaction faction,
				const int id,
				RuleArmor* const armor,
				const GameDifficulty diff = DIFF_BEGINNER,
				const int month = 0, // for upping aLien stats as time progresses.
				BattlescapeGame* const battleGame = nullptr);
		/// Cleans up the BattleUnit.
		~BattleUnit();

		/// Loads this unit from YAML.
		void load(const YAML::Node& node);
		/// Loads the vector of units spotted this turn during SavedBattleGame load.
		void loadSpotted(SavedBattleGame* const battleSave);
		/// Saves this unit to YAML.
		YAML::Node save() const;

		/// Gets this BattleUnit's ID.
		int getId() const;
		/// Gets this unit's type as a string.
		std::string getType() const;
		/// Gets this unit's rank string.
		std::string getRankString() const;
		/// Gets this unit's race string.
		std::string getRaceString() const;

		/// Gets this unit's geoscape Soldier object.
		Soldier* getGeoscapeSoldier() const;

		/// Sets this unit's position.
		void setPosition(
				const Position& pos,
				bool updateLast = true);
		/// Gets this unit's position.
		const Position& getPosition() const;
		/// Gets this unit's position.
		const Position& getStartPosition() const;
		/// Gets this unit's destination when walking.
		const Position& getStopPosition() const;

		/// Sets this unit's direction 0-7.
		void setUnitDirection(
				int dir,
				bool turret = true);
		/// Gets this unit's direction.
		int getUnitDirection() const;
		/// Looks at a certain point.
		void setDirectionTo(
				const Position& pos,
				bool turret = false);
		/// Looks in a certain direction.
		void setDirectionTo(
				int dir,
				bool force = false);
		/// Sets this unit's face direction - only used by strafing moves.
		void setFaceDirection(int dir);
		/// Gets this unit's face direction - only used by strafing moves.
		int getFaceDirection() const;
		/// Sets this unit's turret direction.
		void setTurretDirection(int dir);
		/// Gets this unit's turret direction.
		int getTurretDirection() const;
		/// Gets this unit's turret To direction.
		int getTurretToDirection() const;
		/// Gets this unit's vertical direction.
		int getVerticalDirection() const;

		/// Turns to the destination direction.
		void turn(bool turret = false);

		/// Gets the walk-phase for calculating Map offset.
		int getWalkPhaseTrue() const;
		/// Gets the walk-phase for sprite determination and various triggers.
		int getWalkPhase() const;
		/// Starts the walkingPhase.
		void startWalking(
				int dir,
				const Position& posStop,
				const Tile* const tileBelow);
		/// Advances the walkingPhase.
		void keepWalking(
				const Tile* const tileBelow,
				bool recache);
		/// Calculates the mid- and end-phases for unit-movement.
		void cacheWalkPhases();
		/// Flags the BattleUnit as doing a backwards-ish strafe move.
		void flagStrafeBackwards();
		/// Checks if the BattleUnit is strafing in a backwards-ish direction.
		bool isStrafeBackwards() const;
		/// Gets the BattleUnit's current walking-halfphase setting.
		int getWalkPhaseHalf() const;
		/// Gets the BattleUnit's current walking-fullphase setting.
		int getWalkPhaseFull() const;

		/// Sets this unit's status.
		void setUnitStatus(const UnitStatus status);
		/// Gets this unit's status.
		UnitStatus getUnitStatus() const;

		/// Gets the unit's gender.
		SoldierGender getGender() const;
		/// Gets the unit's faction.
		UnitFaction getFaction() const;

		/// Gets the unit's cache for the battlescape.
		Surface* getCache(int quadrant = 0) const;
		/// Sets the unit's cache and cached flag.
		void setCache(
				Surface* const cache,
				int quadrant = 0);
		/// Clears this BattleUnit's sprite-cache flag.
		void clearCache();
		/// Gets if this BattleUnit's sprite-cache is invalid.
		bool getCacheInvalid() const;

		/// Gets unit sprite recolor values.
		const std::vector<std::pair<Uint8, Uint8>>& getRecolor() const;

		/// Kneels or stands this unit.
		void kneel(bool kneel);
		/// Gets if this unit is kneeled.
		bool isKneeled() const;

		/// Gets if this unit is floating.
		bool isFloating() const;

		/// Aims this unit's weapon.
		void aim(bool aim = true);

		/// Gets this unit's time units.
		int getTimeUnits() const;
		/// Gets this unit's stamina.
		int getEnergy() const;
		/// Gets this unit's health.
		int getHealth() const;
		/// Gets this unit's bravery.
		int getMorale() const;
		/// Gets this unit's effective strength.
		int getStrength() const;

		/// Do damage to this unit.
		int takeDamage(
				const Position& relVoxel,
				int power,
				DamageType dType,
				const bool ignoreArmor = false);

		/// Plays the unit's death sound.
		void playDeathSound(bool fleshWound = false) const;

		/// Sets this unit as having cried out from a shotgun blast to the face.
		void hasCried(bool cried);
		/// Gets if this unit has cried already.
		bool hasCried() const;

		/// Sets this unit's health level.
		void setHealth(int health);

		/// Heals stun level of this unit.
		bool healStun(int power);
		/// Gets this unit's stun level.
		int getStun() const;
		/// Sets this unit's stun level.
		void setStun(int stun);

		/// Knocks this unit out instantly.
		void knockOut();

		/// Starts the collapsing sequence.
		void startCollapsing();
		/// Advances the collapsing sequence.
		void keepCollapsing();
		/// Gets the collapsing sequence phase.
		int getCollapsingPhase() const;

		/// Starts the aiming sequence. This is only for celatids.
		void startAiming();
		/// Advances the aiming sequence.
		void keepAiming();
		/// Gets aiming sequence phase.
		int getAimingPhase() const;
		/// Sets aiming sequence phase.
		void setAimingPhase(int phase);

		/// Gets if this unit is out - either dead or unconscious.
		bool isOut_t(OutCheck test = OUT_ALL) const;

		/// Gets the number of time units a certain action takes.
		int getActionTu(
				const BattleActionType bat,
				const BattleItem* item) const;
		int getActionTu(
				const BattleActionType bat,
				const RuleItem* itRule = nullptr) const;

		/// Spends time units if possible.
		bool spendTimeUnits(int tu);
		/// Spends energy if possible.
		bool spendEnergy(int energy);
		/// Sets time units.
		void setTimeUnits(int tu);
		/// Sets the unit's energy level.
		void setEnergy(int energy);

		/// Sets whether this unit is visible.
		void setUnitVisible(bool flag = true);
		/// Gets whether this unit is visible.
		bool getUnitVisible() const;
		/// Adds a unit to the BattleUnit's visible and/or recently spotted hostile units.
		void addToHostileUnits(BattleUnit* const unit);
		/// Gets the BattleUnit's list of visible hostile units.
		std::vector<BattleUnit*>& getHostileUnits();
		/// Clears visible hostile units.
		void clearHostileUnits();
		/// Gets the BattleUnit's list of hostile units that have been spotted during the current turn.
		std::vector<BattleUnit*>& getHostileUnitsThisTurn();
		/// Clears hostile units spotted during the current turn.
//		void clearHostileUnitsThisTurn();

		/// Adds tile to visible tiles.
//		bool addToVisibleTiles(Tile* const tile);
		/// Gets this unit's list of visible tiles.
//		std::vector<Tile*>* getVisibleTiles();
		/// Clears this unit's visible tiles.
//		void clearVisibleTiles();

		/// Calculates firing or throwing accuracy.
		double getAccuracy(
				const BattleAction& action,
				const BattleActionType bat = BA_NONE) const;
		/// Calculates this unit's accuracy modifier.
		double getAccuracyModifier(const BattleItem* const item = nullptr) const;

		/// Sets this unit's armor value.
		void setArmor(
				int armor,
				UnitSide side);
		/// Gets this unit's Armor.
		const RuleArmor* getArmor() const;
		/// Gets this unit's armor value on a particular side.
		int getArmor(UnitSide side) const;
		/// Checks if this unit is wearing a PowerSuit.
//		bool hasPowerSuit() const;
		/// Checks if this unit is wearing a FlightSuit.
//		bool hasFlightSuit() const;

		/// Gets this unit's current reaction score.
		int getInitiative(const int tuSpent = 0) const;

		/// Prepares this unit for a new turn.
		void prepUnit(bool full = true);
		/// Calculates and resets this BattleUnit's time units and energy.
		void prepTu(
				bool preBattle = false,
				bool hasPanicked = false,
				bool reverted = false);

		/// Changes this unit's morale.
		void moraleChange(int change);

		/// Don't reselect this unit.
		void dontReselect();
		/// Reselect this unit.
		void allowReselect();
		/// Checks whether reselecting this unit is allowed.
		bool reselectAllowed() const;

		/// Sets this unit's fire value.
		void setFireUnit(int fire);
		/// Gets this unit's fire value.
		int getFireUnit() const;
		/// Gives this BattleUnit damage from personal fire.
		void takeFire();

		/// Gets the list of items in this unit's inventory.
		std::vector<BattleItem*>* getInventory();

		/// Lets AI do its thing.
		void think(BattleAction* const action);
		/// Gets current AI state.
		BattleAIState* getAIState() const;
		/// Sets next AI State.
		void setAIState(BattleAIState* const aiState = nullptr);

		/// Sets the Tile that the BattleUnit occupies.
		void setTile(
				Tile* const tile = nullptr,
				const Tile* const tileBelow = nullptr);
		/// Gets this unit's Tile.
		Tile* getTile() const;

		/// Gets the item in the specified slot of this unit's inventory.
		BattleItem* getItem(
				const RuleInventory* const inRule,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot of this unit's inventory.
		BattleItem* getItem(
				const std::string& type,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot of this unit's inventory.
		BattleItem* getItem(
				InventorySection section,
				int x = 0,
				int y = 0) const;

		/// Sets the hand this unit has active.
		void setActiveHand(ActiveHand hand);
		/// Gets this unit's active hand.
		ActiveHand getActiveHand();

		/// Gets the item in this unit's main hand.
		BattleItem* getMainHandWeapon(
				bool quickest = false,
				bool inclMelee = true,
				bool checkFist = false);
		/// Gets a grenade if possible.
		BattleItem* getGrenade() const;
		/// Gets this unit's melee weapon if any.
		BattleItem* getMeleeWeapon() const;
		/// Gets this unit's ranged weapon if any.
		BattleItem* getRangedWeapon(bool quickest) const;

		/// Reloads weapon if needed.
		bool checkReload();

		/// Checks if this unit is in the exit area.
		bool isInExitArea(SpecialTileType tileType = START_POINT) const;

		/// Gets this unit's height taking into account kneeling/standing.
		int getHeight(bool floating = false) const;
		/// Gets this unit's floating elevation.
		int getFloatHeight() const;

		/// Gets a soldier's Firing experience.
		int getExpFiring() const;
		/// Gets a soldier's Throwing experience.
		int getExpThrowing() const;
		/// Gets a soldier's Melee experience.
		int getExpMelee() const;
		/// Gets a soldier's Reactions experience.
		int getExpReactions() const;
		/// Gets a soldier's Bravery experience.
		int getExpBravery() const;
		/// Gets a soldier's PsiSkill experience.
		int getExpPsiSkill() const;
		/// Gets a soldier's PsiStrength experience.
		int getExpPsiStrength() const;

		/// Adds one to the reaction exp counter.
		void addReactionExp();
		/// Adds one to the firing exp counter.
		void addFiringExp();
		/// Adds one to the throwing exp counter.
		void addThrowingExp();
		/// Adds qty to the psiSkill exp counter.
		void addPsiSkillExp(int qty = 1);
		/// Adds qty to the psiStrength exp counter.
		void addPsiStrengthExp(int qty = 1);
		/// Adds qty to the melee exp counter.
		void addMeleeExp(int qty = 1);

		/// Calculates experience and days wounded.
		std::vector<int> postMissionProcedures(const bool dead = false);

		/// Gets the sprite index of this unit for the MiniMap.
		int getMiniMapSpriteIndex() const;

		/// Sets the turret type of this unit (-1 is no turret).
		void setTurretType(int turretType);
		/// Gets the turret type of this unit (-1 is no turret).
		int getTurretType() const;

		/// Gets this unit's total number of fatal wounds.
		int getFatalWounds() const;
		/// Gets fatal wound amount of a body part.
		int getFatalWound(UnitBodyPart part) const;
		/// Heals fatal wounds.
		void heal(
				UnitBodyPart part,
				int wounds,
				int health);
		/// Gives pain killers to this unit.
		void morphine();
		/// Gives stimulants to this unit.
		bool amphetamine(
				int energy,
				int stun);

		/// Gets if the unit has overdosed on morphine.
		bool getOverDose() const;

		/// Gets motion points of this unit for the motion scanner.
		int getMotionPoints() const;

		/// Gets this unit's name.
		std::wstring getName(
				const Language* const lang = nullptr,
				bool debugId = false) const;

		/// Gets this unit's stats.
		const UnitStats* getBattleStats() const;

		/// Gets this unit's stand height.
		int getStandHeight() const;
		/// Gets this unit's kneel height.
		int getKneelHeight() const;

		/// Gets this unit's loft ID.
		size_t getLoft(size_t layer = 0) const;

		/// Gets this unit's victory point value.
		int getValue() const;

		/// Gets this unit's death sound.
		int getDeathSound() const;
		/// Gets this unit's move sound.
		int getMoveSound() const;
		/// Gets this unit's aggro sound.
		int getAggroSound() const;

		/// Gets whether this unit can be affected by fatal wounds.
		bool isWoundable() const;
		/// Gets whether this unit can be affected by fear.
		bool isMoralable() const;
		/// Gets whether this unit can be accessed with the Medikit.
		bool isHealable() const;
		/// Gets whether the BattleUnit can be revived.
		bool isRevivable() const;

		/// Gets this unit's intelligence.
		int getIntelligence() const;
		/// Gets this unit's aggression.
		int getAggression() const;

		/// Gets this unit's special ability.
		SpecialAbility getSpecialAbility() const;
		/// Sets this unit's special ability.
		void setSpecialAbility(const SpecialAbility specab);

		/// Adds a kill to this unit's kill-counter.
		void addKillCount();
		/// Gets if this is a Rookie and has made his/her first kill.
		bool hasFirstKill() const;

		/// Gets this unit's original faction
		UnitFaction getOriginalFaction() const;
		/// Converts this unit to a faction.
		void setFaction(UnitFaction faction);

		/// Gets if this unit is about to die.
		bool getAboutToCollapse() const;

		/// Sets this unit's health to 0 and status to dead.
		void instaKill();
		/// Sets this unit's parameters as down (collapsed/ unconscious/ dead).
		void putDown();

		/// Gets this unit's spawn unit.
		std::string getSpawnType() const;
		/// Sets this unit's spawn unit.
		void setSpawnUnit(const std::string& spawnType);

		/// Gets the faction that killed this unit.
		UnitFaction killedBy() const;
		/// Sets the faction that killed this unit.
		void killedBy(UnitFaction faction);

		/// Sets the BattleUnits that this unit is charging towards.
		void setChargeTarget(BattleUnit* const chargeTarget = nullptr);
		/// Gets the BattleUnits that this unit is charging towards.
		BattleUnit* getChargeTarget() const;

		/// Gets the carried weight in strength units.
		int getCarriedWeight(const BattleItem* const dragItem = nullptr) const;

		/// Sets how many turns this unit will be exposed for.
		void setExposed(int turns = 0);
		/// Sets how many turns this unit will be exposed for.
		int getExposed() const;

		/// This call this after the default copy constructor deletes this unit's sprite-cache.
		void invalidateCache();

		/// Gets this BattleUnit's rules if non-Soldier else nullptr.
		const RuleUnit* getUnitRules() const
		{ return _unitRule; }

		/// Sets this unit's rank integer.
		void setRankInt(int rankInt);
		/// Gets this unit's rank integer.
		int getRankInt() const;
		/// Derives a rank integer based on rank string (for xcom soldiers ONLY)
		void deriveRank();

		/// This function checks if a tile is visible using maths.
		bool checkViewSector(const Position& pos) const;

		/// Adjusts this unit's stats according to difficulty.
		void adjustStats(
				const GameDifficulty diff,
				const int month);

		/// Sets this unit's cover-reserve TU.
		void setCoverReserve(int tuReserve);
		/// Gets this unit's cover-reserve TU.
		int getCoverReserve() const;

		/// Initializes a death spin.
		void initDeathSpin();
		/// Continues a death spin.
		void contDeathSpin();
		/// Regulates inititialization, direction & duration of the death spin-cycle.
		int getSpinPhase() const;
		/// Sets the spinPhase of this unit.
		void setSpinPhase(int spinphase);

		/// To stop a unit from firing/throwing if it spots a new opponent during turning.
		void setStopShot(const bool stop = true);
		/// To stop a unit from firing/throwing if it spots a new opponent during turning.
		bool getStopShot() const;

		/// Sets this unit as dashing.
		void setDashing(bool dash = true);
		/// Gets if this unit is dashing.
		bool isDashing() const;

		/// Sets this unit as having been damaged in a single explosion.
		void setTakenExpl(bool beenhit = true);
		/// Gets if this unit has aleady been damaged in a single explosion.
		bool getTakenExpl() const;

		/// Sets this unit as having been damaged in a single fire.
		void setTakenFire(bool beenhit = true);
		/// Gets if this unit has aleady been damaged in a single fire.
		bool getTakenFire() const;

		/// Returns true if this unit is selectable.
		bool isSelectable(
				UnitFaction faction,
				bool checkReselect = false,
				bool checkInventory = false) const;

		/// Returns true if this unit has an inventory.
		bool hasInventory() const;

		/// Gets this unit's movement type.
		MovementType getMoveTypeUnit() const;

		/// Gets if this unit is hiding or not.
		bool isHiding() const
		{ return _hidingForTurn; };
		/// Sets this unit hiding or not.
		void setHiding(bool hiding = true)
		{ _hidingForTurn = hiding; };

		/// Creates special weapon for the unit.
//		void setSpecialWeapon(SavedBattleGame* save, const Ruleset* rule);
		/// Get special weapon.
//		BattleItem* getSpecialWeapon(BattleType type) const;

		/// Gets this unit's mission statistics.
		BattleUnitStatistics* getStatistics() const;
		/// Sets this unit murderer's id.
		void setMurdererId(int id);
		/// Gets this unit murderer's id.
		int getMurdererId() const;

		/// Sets this unit's order in battle.
		void setBattleOrder(size_t order);
		/// Gets this unit's order in battle.
		size_t getBattleOrder() const;

		/// Sets the BattleGame for this unit.
		void setBattleForUnit(BattlescapeGame* const battleGame);

		/// Sets this BattleUnit's turn direction when spinning 180 degrees.
		void setTurnDirection(int dir);
		/// Clears turn direction.
		void clearTurnDirection();

		/// Sets this BattleUnit as having just revived during a Turnover.
		void setRevived(bool revived = true);

		/// Gets all units in the battlescape that are valid RF-spotters of this BattleUnit.
		std::list<BattleUnit*>* getRfSpotters();

		/// Sets the parameters of a successful mind-control psi attack.
		void hostileMcValues(
				int& strength,
				int& skill);
		/// Gets if the BattleUnit is mind-controlled.
		bool isMindControlled() const;

		/// Gets if this unit is a Zombie.
		bool isZombie() const;

		/// Gets if this unit avoids fire-tiles.
		bool avoidsFire() const;

		/// Gets if this RuleUnit is immune to psionic attacks.
		bool psiBlock() const;

		/// Gets if this BattleUnit has been stunned before.
		bool beenStunned() const;
};

}

#endif
