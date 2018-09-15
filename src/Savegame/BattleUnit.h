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

#ifndef OPENXCOM_BATTLEUNIT_H
#define OPENXCOM_BATTLEUNIT_H

//#include <string>
//#include <vector>

//#include <yaml-cpp/yaml.h>

#include "SavedGame.h" // DifficultyLevel
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
	STATUS_LATENT,		// 10 - can't participate in 2nd-stage's battle.
	STATUS_LATENT_START	// 11 - LATENT but Standing or Unconscious on 1st-stage's start-tile(s).
};

enum UnitFaction
{
	FACTION_NONE = -1,	//-1
	FACTION_PLAYER,		// 0 // NOTE: Don't muck with these; they're used by
	FACTION_HOSTILE,	// 1 // BattleUnitKill::hostileTurn() determination
	FACTION_NEUTRAL		// 2 // deliberately as ints.
};

enum UnitSide
{
	SIDE_FRONT,	// 0
	SIDE_LEFT,	// 1
	SIDE_RIGHT,	// 2
	SIDE_REAR,	// 3
	SIDE_UNDER	// 4
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
	OUT_HEALTH_STUN	// 4
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
class BattleUnit
{

	public:
//		static bool _debug;
		static const size_t PARTS_BODY = 6u;

private:
	static const size_t PARTS_ARMOR = 5u;
	static const int DOSE_LETHAL = 3;
//	static const int SPEC_WEAPON_MAX = 3;

	bool
		_aboutToCollapse,
		_cacheInvalid,
		_dashing,
		_dontReselect,
		_floating,
		_hasBeenStunned,
		_hasCried,
		_hidingForTurn,
		_isFireDeath,
		_isMechanical,
		_kneeled,
		_psiBlock,
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
		_fatalWounds[PARTS_BODY],
		_fire,
		_health,
		_id,
		_takedowns,
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
		_collapsePhase,
		_mcStrength,
		_mcSkill,
		_psiTriedQty;
	size_t _battleOrder;

	BattleAIState* _unitAiState;
//	BattleItem* _specWeapon[SPEC_WEAPON_MAX];
	BattleItem* _fist;
	BattlescapeGame* _battleGame;
	BattleUnit* _chargeTarget;
	SavedBattleGame* _battleSave;
	Surface* _spriteCache[4u]; // holds quadrants
	Tile* _tile;

	Position
		_lastCover,
		_pos,
		_posStart,
		_posStop;
	UnitFaction
		_faction,
		_originalFaction,
		_killerFaction;
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

	bool
		_isZombie,
		_capturable;

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
		_value;

	std::string
		_race,
		_rank,
		_type;

	std::wstring _label;

	std::vector<size_t> _loftSet;
	std::vector<std::pair<Uint8, Uint8>> _recolor;

	const RuleArmor* _arRule;
	Soldier* _geoscapeSoldier;
	const RuleUnit* _unitRule;

	MoveType _mType;
	SoldierGender _gender;
	SpecialAbility _specab;
	TurretType _turretType;

	BattleUnitStatistics* _tacstats;
	int _murdererId; // used to credit another unit with any kills that this BattleUnit got by blowing up on death


	/// Converts an amount of experience to a stat increase.
	int improveStat(int xp) const;

	/// Helper function initing recolor vector.
	void setRecolor(
			int basicLook,
			int utileLook,
			int rankLook);

	/// Calculates the mid- and end-phases for unit-movement.
	void cacheWalkPhases();

	/// Gets if a grenade-type is suitable for an AI or panic situation.
	bool isGrenadeSuitable(const BattleItem* const grenade) const;


	public:
		static const int
			MAX_SOLDIER_ID = 1000000,
			DIR_FACEPLAYER = 3;

		/// Creates a BattleUnit from a geoscape Soldier.
		BattleUnit( // xCom operatives
				Soldier* const sol,
				SavedBattleGame* const battleSave);
		/// Creates a BattleUnit from a RuleUnit.
		BattleUnit( // aLiens, civies, & Tanks
				RuleUnit* const unitRule,
				const UnitFaction faction,
				const int id,
				RuleArmor* const armor,
				SavedBattleGame* const battleSave,
				BattlescapeGame* const battleGame = nullptr);
		/// Cleans up the BattleUnit.
		~BattleUnit();

		/// Loads the BattleUnit from YAML.
		void load(const YAML::Node& node);
		/// Loads the vector of units-spotted this turn during SavedBattleGame load.
		void loadSpotted();
		/// Saves the BattleUnit to YAML.
		YAML::Node save() const;

		/// Gets the BattleUnit's ID.
		int getId() const;
		/// Gets the BattleUnit's type as a string.
		std::string getType() const;
		/// Gets the BattleUnit's rank string.
		std::string getRankString() const;
		/// Gets the BattleUnit's race string.
		std::string getRaceString() const;

		/// Gets the BattleUnit's geoscape Soldier.
		Soldier* getGeoscapeSoldier() const;

		/// Sets the BattleUnit's position.
		void setPosition(const Position& pos);
		/// Gets the BattleUnit's position.
		const Position& getPosition() const;
		/// Gets the BattleUnit's position.
		const Position& getStartPosition() const;
		/// Gets the BattleUnit's destination when walking.
		const Position& getStopPosition() const;

		/// Sets the BattleUnit's direction 0-7.
		void setUnitDirection(
				int dir,
				bool turret = true);
		/// Gets the BattleUnit's direction.
		int getUnitDirection() const;
		/// Looks at a certain point.
		void setDirectionTo(
				const Position& pos,
				bool turret = false);
		/// Looks in a certain direction.
		void setDirectionTo(
				int dir,
				bool turret = false);
		/// Sets the BattleUnit's face direction - only used by strafing moves.
		void setFaceDirection(int dir);
		/// Gets the BattleUnit's face direction - only used by strafing moves.
		int getFaceDirection() const;
		/// Sets the BattleUnit's turret direction.
		void setTurretDirection(int dir);
		/// Gets the BattleUnit's turret direction.
		int getTurretDirection() const;
		/// Gets the BattleUnit's turret To direction.
//		int getTurretToDirection() const;
		/// Gets the BattleUnit's vertical direction.
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
				const Tile* const tileBelow = nullptr);
		/// Advances the walkingPhase.
		void keepWalking(
				const Tile* const tileBelow,
				bool recache);
		/// Flags the BattleUnit as doing a backwards-ish strafe move.
		void flagStrafeBackwards();
		/// Checks if the BattleUnit is strafing in a backwards-ish direction.
		bool isStrafeBackwards() const;
		/// Gets the BattleUnit's current walking-halfphase setting.
		int getWalkPhaseHalf() const;
		/// Gets the BattleUnit's current walking-fullphase setting.
		int getWalkPhaseFull() const;

		/// Sets the BattleUnit's status.
		void setUnitStatus(const UnitStatus status);
		/// Gets the BattleUnit's status.
		UnitStatus getUnitStatus() const;

		/// Gets the BattleUnit's gender.
		SoldierGender getGender() const;

		/// Gets the BattleUnit's faction.
		UnitFaction getFaction() const;
		/// Converts the BattleUnit to a faction.
		void setFaction(const UnitFaction faction);
		/// Gets the BattleUnit's original faction
		UnitFaction getOriginalFaction() const;

		/// Sets this BattleUnit's cache and sprite-cached flag.
		void setCache(
				Surface* const cache,
				size_t quadrant);
		/// Gets the BattleUnit's sprite-cache for the battlescape.
		Surface* getCache(size_t quadrant) const;
		/// Clears the BattleUnit's sprite-cache flag.
		void setCacheInvalid();
		/// Checks if the BattleUnit's sprite-cache is invalid.
		bool isCacheInvalid() const;

		/// Gets unit-sprite recolor values.
		const std::vector<std::pair<Uint8, Uint8>>& getRecolor() const;

		/// Kneels or stands the BattleUnit.
		void kneelUnit(bool kneel);
		/// Gets if the BattleUnit is kneeled.
		bool isKneeled() const;

		/// Sets the BattleUnit floating.
		void setFloating(bool isAirborne = true);
		/// Checks if the BattleUnit is floating.
		bool isFloating() const;

		/// Aims or lowers the BattleUnit's weapon.
		void toggleShoot();

		/// Gets the BattleUnit's turn-units.
		int getTu() const;
		/// Gets the BattleUnit's stamina.
		int getEnergy() const;
		/// Gets the BattleUnit's health.
		int getHealth() const;
		/// Gets the BattleUnit's bravery.
		int getMorale() const;
		/// Gets the BattleUnit's effective strength.
		int getStrength() const;

		/// Do damage to the BattleUnit.
		int takeDamage(
				const Position& relVoxel,
				int power,
				DamageType dType,
				const bool ignoreArmor = false);

		/// Plays the BattleUnit's death sound.
		void playDeathSound(bool fleshwound = false) const;

		/// Sets the BattleUnit as having cried out from a shotgun blast to the face.
		void hasCriedShotgun(bool cried);
		/// Gets if the BattleUnit has cried already.
		bool hasCriedShotgun() const;

		/// Sets the BattleUnit's health level.
		void setHealth(int health);

		/// Heals stun-level of the BattleUnit.
		bool reduceStun(int stun);
		/// Gets the BattleUnit's stun-level.
		int getStun() const;
		/// Sets the BattleUnit's stun-level.
		void setStun(int stun);
		/// Checks if the BattleUnit is currently stunned.
		bool isStunned() const;

		/// Starts a death-spin.
		void startSpinning();
		/// Continues a death-spin.
		void keepSpinning();
		/// Gets the spin-phase of the BattleUnit.
//		int getSpinPhase() const;
		/// Sets the spin-phase of the BattleUnit.
//		void setSpinPhase(int phase);

		/// Starts a collapsing sequence.
		void startCollapsing();
		/// Advances a collapsing sequence.
		bool keepCollapsing();
		/// Gets the collapsing sequence phase.
		int getCollapsePhase() const;

		/// Starts an aiming sequence. This is only for celatids. And grenade launchers.
		bool startAiming();
		/// Advances an aiming sequence.
		void keepAiming();
		/// Gets the aiming sequence phase.
		int getAimingPhase() const;
		/// Sets the aiming sequence phase.
		void setAimingPhase(int phase);

		/// Gets if the BattleUnit is out - either dead or unconscious.
		bool isOut_t(const OutCheck test = OUT_ALL) const;

		/// Gets the number of turn-units a certain action takes.
		int getActionTu(
				const BattleActionType bat,
				const BattleItem* item) const;
		int getActionTu(
				const BattleActionType bat,
				const RuleItem* itRule = nullptr) const;

		/// Expends turn-units if possible.
		bool expendTu(int tu);
		/// Expends energy if possible.
		bool expendEnergy(int energy);
		/// Expends TU and Energy.
		void expendTuEnergy(
				int tu,
				int energy);
		/// Sets the BattleUnit's turn-units.
		void setTu(int tu = 0);
		/// Sets the BattleUnit's energy-level.
		void setEnergy(int energy = 0);

		/// Sets whether the BattleUnit is visible.
		void setUnitVisible(bool flag = true);
		/// Gets whether the BattleUnit is visible.
		bool getUnitVisible() const;

		/// Adds a unit to the BattleUnit's vectors of spotted hostile units.
		bool addToHostileUnits(BattleUnit* const unit);
		/// Gets the BattleUnit's vector of currently spotted hostile units.
		std::vector<BattleUnit*>& getHostileUnits();
		/// Clears currently spotted hostile units.
		void clearHostileUnits();
		/// Gets the BattleUnit's vector of recently spotted hostile units.
		std::vector<BattleUnit*>& getHostileUnitsThisTurn();
		/// Clears recently spotted hostile units.
//		void clearHostileUnitsThisTurn();

		/// Adds tile to visible tiles.
//		bool addToVisibleTiles(Tile* const tile);
		/// Gets the BattleUnit's list of visible tiles.
//		std::vector<Tile*>* getVisibleTiles();
		/// Clears the BattleUnit's visible tiles.
//		void clearVisibleTiles();

		/// Calculates firing or throwing accuracy.
		double getAccuracy(
				const BattleAction& action,
				const BattleActionType bat = BA_NONE) const;
		/// Calculates the BattleUnit's accuracy modifier.
		double getAccuracyModifier(const BattleItem* const item = nullptr) const;

		/// Sets the BattleUnit's armor value.
		void setArmor(
				int armor,
				const UnitSide side);
		/// Gets the BattleUnit's Armor.
		const RuleArmor* getArmor() const;
		/// Gets the BattleUnit's armor value on a particular side.
		int getArmor(const UnitSide side) const;
		/// Checks if the BattleUnit is wearing a PowerSuit.
//		bool hasPowerSuit() const;
		/// Checks if the BattleUnit is wearing a FlightSuit.
//		bool hasFlightSuit() const;

		/// Gets the BattleUnit's current reaction score.
		int getInitiative(const int actionTu = 0) const;

		/// Prepares the BattleUnit for a new turn.
		void prepareUnit(bool preBattle = false);
		/// Calculates and resets the BattleUnit's turn-units and energy.
		void prepTuEnergy(
				bool preBattle = false,
				bool isPanicked = false,
				bool reverted = false);

		/// Changes the BattleUnit's morale.
		void moraleChange(int change);

		/// Sets the BattleUnit's dontReselect flag.
		void setReselect(bool reselect = true);
		/// Gets the BattleUnit's dontReselect flag.
		bool getReselect() const;
		/// Checks if the BattleUnit is selectable.
		bool isSelectable(
				UnitFaction faction,
				bool checkReselect = false,
				bool checkInventory = false) const;

		/// Sets the BattleUnit's fire value.
		void setUnitFire(int fire);
		/// Gets the BattleUnit's fire value.
		int getUnitFire() const;
		/// Gives the BattleUnit damage from personal fire.
		void hitUnitFire();

		/// Gets the list of items in the BattleUnit's inventory.
		std::vector<BattleItem*>* getInventory();

		/// Lets AI do its thing.
		void thinkAi(BattleAction* const action);
		/// Sets next AI State.
		void setAIState(BattleAIState* const aiState = nullptr);
		/// Gets current AI state.
		BattleAIState* getAIState() const;

		/// Clears the BattleUnit from its current Tiles.
		void clearTileQuadrants() const;

		/// Sets the Tile that the BattleUnit occupies.
		void setUnitTile(
				Tile* const tile = nullptr,
				const Tile* const tileBelow = nullptr);
		/// Gets the BattleUnit's Tile.
		Tile* getUnitTile() const;
		/// Gets the Tile below the BattleUnit.
		Tile* getUnitTileBelow() const;

		/// Gets the item in the specified slot of the BattleUnit's inventory.
		BattleItem* getItem(
				const RuleInventory* const inRule,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot of the BattleUnit's inventory.
		BattleItem* getItem(
				const std::string& type,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot of the BattleUnit's inventory.
		BattleItem* getItem(
				InventorySection section,
				int x = 0,
				int y = 0) const;

		/// Sets the hand the BattleUnit has active.
		void setActiveHand(const ActiveHand hand);
		/// Gets the hand the BattleUnit has active.
		ActiveHand getActiveHand();
		/// Determines the BattleUnit's active-hand.
		ActiveHand deterActiveHand();

		/// Gets the item in the BattleUnit's main-hand.
		BattleItem* getMainHandWeapon(
				bool quickest = false,
				bool inclMelee = true,
				bool checkFist = false);
		/// Gets a grenade if possible.
		BattleItem* getGrenade() const;
		/// Gets the BattleUnit's melee weapon if any.
		BattleItem* getMeleeWeapon() const;
		/// Gets the BattleUnit's ranged weapon if any.
		BattleItem* getRangedWeapon(bool quickest) const;

		/// Reloads weapon if needed.
		bool checkReload();

		/// Checks if the BattleUnit is standing on a specified tile-type.
		bool isOnTiletype(const TileType tileType) const;

		/// Gets the BattleUnit's height taking into account kneeling/standing.
		int getHeight(bool floating = false) const;
		/// Gets the BattleUnit's floating elevation.
		int getFloatHeight() const;

		/// Gets a Soldier's firing-experience.
		int getExpFiring() const;
		/// Gets a Soldier's throwing-experience.
		int getExpThrowing() const;
		/// Gets a Soldier's melee-experience.
		int getExpMelee() const;
		/// Gets a Soldier's reactions-experience.
		int getExpReactions() const;
		/// Gets a Soldier's bravery-experience.
		int getExpBravery() const;
		/// Gets a Soldier's psiSkill-experience.
		int getExpPsiSkill() const;
		/// Gets a Soldier's psiStrength-experience.
		int getExpPsiStrength() const;

		/// Adds one to the firing-exp counter.
		void addFiringExp();
		/// Adds one to the throwing-exp counter.
		void addThrowingExp();
		/// Adds qty to the melee-exp counter.
		void addMeleeExp(int qty = 1);
		/// Adds one to the reaction-exp counter.
		void addReactionExp();
		/// Adds qty to the psiSkill-exp counter.
		void addPsiSkillExp(int qty = 1);
		/// Adds qty to the psiStrength-exp counter.
		void addPsiStrengthExp(int qty = 1);

		/// Calculates experience and days wounded.
		std::vector<int> postMissionProcedures(const bool dead = false);

		/// Gets the sprite index of the BattleUnit for the MiniMap.
		int getMiniMapSpriteIndex() const;

		/// Sets the turret-type of the BattleUnit.
		void setTurretType(const TurretType turretType);
		/// Gets the turret-type of the BattleUnit.
		TurretType getTurretType() const;

		/// Gets the BattleUnit's total quantity of fatal wounds.
		int getFatalsTotal() const;
		/// Gets the fatals on a specified body-part.
		int getFatals(const UnitBodyPart part) const;
		/// Heals fatal wounds and replenishes health/morale.
		void heal(
				const UnitBodyPart part,
				int wounds,
				int health);
		/// Gives pain killers to the BattleUnit.
		void morphine();
		/// Gives stimulants to the BattleUnit.
		bool amphetamine(
				int energy,
				int stun);

		/// Gets if the BattleUnit has overdosed on morphine.
		bool getOverDose() const;

		/// Gets motion-points of the BattleUnit for the motion-scanner.
		int getMotionPoints() const;
		/// Calculates arbitrary pre-battle TU and motion-points.
		void preBattleMotion();

		/// Gets the BattleUnit's label.
		std::wstring getLabel(
				const Language* const lang = nullptr,
				bool debugId = false) const;

		/// Gets the BattleUnit's stats.
		const UnitStats* getBattleStats() const;

		/// Gets the BattleUnit's stand height.
		int getStandHeight() const;
		/// Gets the BattleUnit's kneel height.
		int getKneelHeight() const;

		/// Gets the BattleUnit's loft ID.
		size_t getLoft(size_t quad = 0u) const;

		/// Gets the BattleUnit's victory point value.
		int getValue() const;

		/// Gets the BattleUnit's death sound.
		int getDeathSound() const;
		/// Gets the BattleUnit's move sound.
		int getMoveSound() const;
		/// Gets the BattleUnit's aggro sound.
		int getAggroSound() const;

		/// Gets whether the BattleUnit can be affected by fatal wounds.
		bool isWoundable() const;
		/// Gets whether the BattleUnit can be affected by fear.
		bool isMoralable() const;
		/// Gets whether the BattleUnit can be accessed with the Medikit.
		bool isHealable() const;
		/// Gets whether the BattleUnit can be revived.
		bool isRevivable() const;

		/// Gets the BattleUnit's intelligence.
		int getIntelligence() const;
		/// Gets the BattleUnit's aggression.
		int getAggression() const;

		/// Gets the BattleUnit's SpecialAbility.
		SpecialAbility getSpecialAbility() const;
		/// Sets the BattleUnit's SpecialAbility.
		void setSpecialAbility(const SpecialAbility specab);

		/// Checks whether the BattleUnit can be captured alive.
		bool isCapturable() const;

		/// Gets the BattleUnit's spawn unit.
		std::string getSpawnType() const;
		/// Sets the BattleUnit's spawn unit.
		void setSpawnType(const std::string& spawnType);

		/// Adds a takedown.
		void addTakedown();
		/// Gets the quantity of kills/stuns the BattleUnit currently has.
		int getTakedowns() const;
		/// Gets if the BattleUnit is a Rookie and has made his/her first takedown.
		bool hasFirstTakedown() const;

		/// Checks if the BattleUnit is about to collapse.
		bool isAboutToCollapse() const;

		/// Sets the BattleUnit's variables as unconscious/dead.
		void putdown(bool autokill);

		/// Gets the faction that killed the BattleUnit.
		UnitFaction killerFaction() const;
		/// Sets the faction that killed the BattleUnit.
		void killerFaction(const UnitFaction faction);

		/// Sets the BattleUnits that the BattleUnit is charging towards.
		void setChargeTarget(BattleUnit* const chargeTarget = nullptr);
		/// Gets the BattleUnits that the BattleUnit is charging towards.
		BattleUnit* getChargeTarget() const;

		/// Gets the carried weight in strength units.
		int getCarriedWeight(const BattleItem* const dragItem = nullptr) const;

		/// Sets how many turns the BattleUnit will be exposed for.
		void setExposed(int turns = 0);
		/// Sets how many turns the BattleUnit will be exposed for.
		int getExposed() const;

		/// Gets the BattleUnit's rules if non-Soldier else nullptr.
		const RuleUnit* getUnitRules() const
		{ return _unitRule; }

		/// Sets the BattleUnit's rank integer.
		void setRankInt(int rankInt);
		/// Gets the BattleUnit's rank integer.
		int getRankInt() const;
		/// Derives a rank integer based on rank string (for xcom soldiers ONLY)
		void deriveRank();

		/// Checks if a Position is within the BattleUnit's FoV-cone.
		bool checkViewSector(const Position& pos) const;

		/// Adjusts the BattleUnit's stats according to difficulty.
		void adjustStats();

		/// Sets the BattleUnit's reserved-TU for finding cover.
		void setCoverReserve(int tuReserved);
		/// Gets the BattleUnit's reserved-TU for finding cover.
		int getCoverReserve() const;

		/// Sets whether to stop a unit from firing/throwing.
		void setStopShot(bool stop = true);
		/// Gets whether to stop a unit from firing/throwing.
		bool getStopShot() const;

		/// Sets the BattleUnit as dashing.
		void setDashing(bool dash = true);
		/// Gets if the BattleUnit is dashing.
		bool getDashing() const;

		/// Sets the BattleUnit as having been damaged in a single explosion.
		void setTakenExplosive(bool beenhit = true);
		/// Gets if the BattleUnit has aleady been damaged in a single explosion.
		bool getTakenExplosive() const;

		/// Sets the BattleUnit as having been damaged in a single fire.
		void setTakenFire(bool beenhit = true);
		/// Gets if the BattleUnit has aleady been damaged in a single fire.
		bool getTakenFire() const;

		/// Checks if the BattleUnit has a player-accessible Inventory.
		bool canInventory() const;

		/// Gets the BattleUnit's movement type.
		MoveType getMoveTypeUnit() const;

		/// Sets the BattleUnit hiding or not.
		void setHiding(bool hiding = true)
		{ _hidingForTurn = hiding; };
		/// Checks if the BattleUnit is hiding or not.
		bool isHiding() const
		{ return _hidingForTurn; };

		/// Creates special weapon for the BattleUnit.
//		void setSpecialWeapon(SavedBattleGame* save, const Ruleset* rule);
		/// Get special weapon.
//		BattleItem* getSpecialWeapon(BattleType type) const;

		/// Gets the BattleUnit's mission statistics.
		BattleUnitStatistics* getStatistics() const;
		/// Sets the BattleUnit's murderer-ID.
		void setMurdererId(int id);
		/// Gets the BattleUnit's murderer-ID.
		int getMurdererId() const;

		/// Sets the BattleUnit's order in battle.
		void setBattleOrder(size_t order);
		/// Gets the BattleUnit's order in battle.
		size_t getBattleOrder() const;

		/// Sets the BattleGame for the BattleUnit.
		void setBattleForUnit(BattlescapeGame* const battleGame);

		/// Sets the BattleUnit's turn direction when spinning 180 degrees.
		void setTurnDirection(int dir);
		/// Clears turn direction.
		void clearTurnDirection();

		/// Gets all units in the battlescape that are valid RF-spotters of the BattleUnit.
		std::list<BattleUnit*>* getRfSpotters();

		/// Sets the parameters of a successful mind-control psi attack.
		void hostileMcValues(
				int& strength,
				int& skill);
		/// Gets if the BattleUnit is mind-controlled.
		bool isMindControlled() const;

		/// Checks if the BattleUnit is a Zombie.
		bool isZombie() const;
		/// Checks if the BattleUnit is a mechanical apparatus.
		bool isMechanical() const;

		/// Checks if the BattleUnit avoids fire-tiles.
		bool avoidsFire() const;

		/// Checks if the BattleUnit is immune to psionic attacks.
		bool psiBlock() const;
		/// Sets how many times an aLien has tried to psi-attack during its turn.
		void psiTried(int tried);
		/// Gets how many times an aLien has tried to psi-attack during its turn.
		int psiTried() const;

		/// Checks if the BattleUnit has been stunned before.
		bool beenStunned() const;

		/// Gets the BattleUnit's last-cover Position.
		Position getLastCover() const;

		/// Tries to burn a Tile if this BattleUnit is capable of doing so.
		void burnTile(Tile* const tile);

		/// Converts UnitStatus to a string for the Logfile.
		static std::string debugStatus(const UnitStatus status);
};

}

#endif
