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

#include "RuleArmor.h"

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of Armor.
 * @param type - reference to the type
 */
RuleArmor::RuleArmor(const std::string& type)
	:
		_type(type),
		_frontArmor(0),
		_sideArmor(0),
		_rearArmor(0),
		_underArmor(0),
		_drawRoutine(0),
		_mType(MT_WALK),
		_size(1),
		_weight(0),
		_deathFrames(3),
		_shootFrames(0),
		_firePhase(0),
		_constantAnimation(false),
		_accessInventory(true),
		_forcedTorso(TORSO_STANDARD),
		_colorGroupFace(0),
		_colorGroupHair(0),
		_colorGroupUtile(0),
		_colorGroupRank(0),
		_isBasic(false),
		_isSpacesuit(false),
		_agility(0)
{
	_stats.tu			=
	_stats.stamina		=
	_stats.health		=
	_stats.bravery		=
	_stats.reactions	=
	_stats.firing		=
	_stats.throwing		=
	_stats.strength		=
	_stats.psiSkill		=
	_stats.psiStrength	=
	_stats.melee		= 0;

	for (size_t
			i = 0u;
			i != DAMAGE_TYPES;
			++i)
	{
		_damageModifier[i] = 1.f;
	}

	_colorFace.resize((static_cast<size_t>(LOOK_AFRICAN) + 1u) << 1u);
	_colorHair.resize((static_cast<size_t>(LOOK_AFRICAN) + 1u) << 1u);
}

/**
 * dTor.
 */
RuleArmor::~RuleArmor()
{}

/**
 * Loads the Armor from a YAML file.
 * @param node - reference a YAML node
 */
void RuleArmor::load(const YAML::Node& node)
{
	_type				= node["type"]				.as<std::string>(_type);
	_spriteSheet		= node["spriteSheet"]		.as<std::string>(_spriteSheet);
	_spriteImage		= node["spriteImage"]		.as<std::string>(_spriteImage);
	_accessInventory	= node["accessInventory"]	.as<bool>(_accessInventory);

	if (node["corpseBattle"])
	{
		_corpseBattle	= node["corpseBattle"]	.as<std::vector<std::string>>();
		_corpseGeo		= _corpseBattle[0u];
	}
	_corpseGeo		= node["corpseGeo"]		.as<std::string>(_corpseGeo);

	_mType = static_cast<MoveType>(node["movementType"].as<int>(static_cast<int>(_mType)));

//	_specWeapon		= node["specialWeapon"]	.as<std::string>(_specWeapon);
	_frontArmor		= node["frontArmor"]	.as<int>(_frontArmor);
	_sideArmor		= node["sideArmor"]		.as<int>(_sideArmor);
	_rearArmor		= node["rearArmor"]		.as<int>(_rearArmor);
	_underArmor		= node["underArmor"]	.as<int>(_underArmor);
	_drawRoutine	= node["drawRoutine"]	.as<int>(_drawRoutine);
	_size			= node["size"]			.as<int>(_size);
	_weight			= node["weight"]		.as<int>(_weight);
	_agility		= node["agility"]		.as<int>(_agility);
	_isSpacesuit	= node["isSpacesuit"]	.as<bool>(_isSpacesuit);

	_storeItem		= node["storeItem"]		.as<std::string>(_storeItem);

	if (_storeItem == "STR_NONE")
		_isBasic = true;

	_stats.mergeStats(node["stats"].as<UnitStats>(_stats));

	if (const YAML::Node& vuln = node["damageModifier"])
	{
		for (size_t
				i = 0u;
				i != vuln.size() && i != DAMAGE_TYPES;
				++i)
		{
			_damageModifier[i] = vuln[i].as<float>(1.f);
		}
	}

	_loftSet			= node["loftSet"]			.as<std::vector<size_t>>(_loftSet);
	_deathFrames		= node["deathFrames"]		.as<int>(_deathFrames);
	_shootFrames		= node["shootFrames"]		.as<int>(_shootFrames);
	_firePhase			= node["firePhase"]			.as<int>(_firePhase);
	_constantAnimation	= node["constantAnimation"]	.as<bool>(_constantAnimation);

	_forcedTorso = static_cast<ForcedTorso>(node["forcedTorso"].as<int>(_forcedTorso));

	_colorGroupFace		= node["spriteFaceGroup"]	.as<int>(_colorGroupFace);
	_colorGroupHair		= node["spriteHairGroup"]	.as<int>(_colorGroupHair);
	_colorGroupRank		= node["spriteRankGroup"]	.as<int>(_colorGroupRank);
	_colorGroupUtile	= node["spriteUtileGroup"]	.as<int>(_colorGroupUtile);
	_colorFace			= node["spriteFaceColor"]	.as<std::vector<int>>(_colorFace);
	_colorHair			= node["spriteHairColor"]	.as<std::vector<int>>(_colorHair);
	_colorRank			= node["spriteRankColor"]	.as<std::vector<int>>(_colorRank);
	_colorUtile			= node["spriteUtileColor"]	.as<std::vector<int>>(_colorUtile);

	_units = node["units"].as<std::vector<std::string>>(_units);
}

/**
 * Gets the string of the Armor type.
 * @return, armor type
 */
std::string RuleArmor::getType() const
{
	return _type;
}

/**
 * Gets the Armor's sprite sheet.
 * @return, sprite sheet
 */
std::string RuleArmor::getSpriteSheet() const
{
	return _spriteSheet;
}

/**
 * Gets the Armor's inventory sprite.
 * @return, inventory sprite
 */
std::string RuleArmor::getSpriteInventory() const
{
	return _spriteImage;
}

/**
 * Gets the front armor level.
 * @return, front armor level
 */
int RuleArmor::getFrontArmor() const
{
	return _frontArmor;
}

/**
 * Gets the side armor level.
 * @return, side armor level
 */
int RuleArmor::getSideArmor() const
{
	return _sideArmor;
}

/**
 * Gets the rear armor level.
 * @return, rear armor level
 */
int RuleArmor::getRearArmor() const
{
	return _rearArmor;
}

/**
 * Gets the under armor level.
 * @return, under armor level
 */
int RuleArmor::getUnderArmor() const
{
	return _underArmor;
}

/**
 * Gets the corpse item used in the Geoscape.
 * @return, name of the corpse item
 */
std::string RuleArmor::getCorpseGeoscape() const
{
	return _corpseGeo;
}

/**
 * Gets the list of corpse items dropped by the unit in the Battlescape (one per
 * quadrant).
 * @return, reference to the list of corpse drops
 */
const std::vector<std::string>& RuleArmor::getCorpseBattlescape() const
{
	return _corpseBattle;
}

/**
 * Gets the storage item needed for a Soldier to equip the Armor.
 * @return, type of the store item ("STR_NONE" for infinite armor)
 */
std::string RuleArmor::getStoreItem() const
{
	return _storeItem;
}

/**
 * Gets the type of special weapon.
 * @return, the name of the special weapon
 *
std::string RuleArmor::getSpecialWeapon() const
{
	return _specWeapon;
} */

/**
 * Gets the drawing routine ID for the Armor.
 * @return, drawing routine ID
 */
int RuleArmor::getDrawRoutine() const
{
	return _drawRoutine;
}

/**
 * Gets the movement type of the Armor.
 * @note Useful for determining whether the Armor can fly.
 * @important: do not use this function outside the BattleUnit constructor
 * unless you are SURE you know what you are doing.
 * For more information see the BattleUnit constructor.
 * @return, MoveType (MapData.h)
 */
MoveType RuleArmor::getMoveTypeArmor() const
{
	return _mType;
}

/**
 * Gets the size of this armor. Normally 1 (small) or 2 (big).
 * @return, size
 */
int RuleArmor::getSize() const
{
	return _size;
}

/**
 * Gets the damage modifier for a certain damage type.
 * @param dType - the damageType (RuleItem.h)
 * @return, damage modifier (0.f to 1.f+)
 */
float RuleArmor::getDamageModifier(DamageType dType) const
{
	return _damageModifier[static_cast<size_t>(dType)];
}

/**
 * Gets the Line of Fire Template set.
 * @return, reference to the loftSet as a vector of templates
 */
const std::vector<size_t>& RuleArmor::getLoftSet() const
{
	return _loftSet;
}

/**
 * Gets the Armor's stats.
 * @return, pointer to UnitStats
 */
const UnitStats* RuleArmor::getStats() const
{
	return &_stats;
}

/**
 * Gets the Armor's weight.
 * @return, weight
 */
int RuleArmor::getWeight() const
{
	return _weight;
}

/**
 * Gets the number of death frames.
 * @return, frames
 */
int RuleArmor::getDeathFrames() const
{
	return _deathFrames;
}

/**
 * Gets the number of shoot frames.
 * @return, frames
 */
int RuleArmor::getShootFrames() const
{
	return _shootFrames;
}

/**
 * Gets the frame of the Armor's aiming-animation that first draws a projectile.
 * @return, frame
 */
int RuleArmor::getFirePhase() const
{
	return _firePhase;
}

/**
 * Gets if the Armor uses constant animation.
 * @return, true if constant
 */
bool RuleArmor::getConstantAnimation() const
{
	return _constantAnimation;
}

/**
 * Checks if the Armor ignores gender (power suit/flying suit).
 * @return, the torso to force on the sprite (RuleArmor.h)
 */
ForcedTorso RuleArmor::getForcedTorso() const
{
	return _forcedTorso;
}

/**
 * Gets hair base color-group for replacement (0 don't replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getColorGroupFace() const
{
	return _colorGroupFace;
}

/**
 * Gets hair base color-group for replacement (0 don't replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getColorGroupHair() const
{
	return _colorGroupHair;
}

/**
 * Gets utile base color-group for replacement (0 don't replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getColorGroupUtile() const
{
	return _colorGroupUtile;
}

/**
 * Gets rank base color-group for replacement (0 don't replace).
 * @return, colorgroup or 0
 */
int RuleArmor::getColorGroupRank() const
{
	return _colorGroupRank;
}

/**
 * Gets new face colors for replacement (0 don't replace).
 * @param id -
 * @return, colorindex or 0
 */
int RuleArmor::getColorFace(int id) const
{
	const size_t foff (static_cast<size_t>(id));
	if (foff < _colorFace.size())
		return _colorFace[foff];

	return 0;
}

/**
 * Gets new hair colors for replacement (0 don't replace).
 * @param id -
 * @return, colorindex or 0
 */
int RuleArmor::getColorHair(int id) const
{
	const size_t foff (static_cast<size_t>(id));
	if (foff < _colorHair.size())
		return _colorHair[foff];

	return 0;
}

/**
 * Gets new utile colors for replacement (0 don't replace).
 * @param id -
 * @return, colorindex or 0
 */
int RuleArmor::getColorUtile(int id) const
{
	const size_t foff (static_cast<size_t>(id));
	if (foff < _colorUtile.size())
		return _colorUtile[foff];

	return 0;
}

/**
 * Gets new rank colors for replacement (0 don't replace).
 * @param id -
 * @return, colorindex or 0
 */
int RuleArmor::getColorRank(int id) const
{
	const size_t foff (static_cast<size_t>(id));
	if (foff < _colorRank.size())
		return _colorRank[foff];

	return 0;
}

/**
 * Checks if the Armor's inventory be accessed.
 * @return, true if inventory can be opened by player
 */
bool RuleArmor::canInventory() const
{
	return _accessInventory;
}

/**
 * Gets the list of BattleUnits the Armor applies to.
 * @return, reference to the list of unit-types (empty = applies to all)
 */
const std::vector<std::string>& RuleArmor::getUnits() const
{
	return _units;
}

/**
 * Gets if the Armor is basic.
 * @note True denotes armor of the lowest basic standard issue wear. It does not
 * require space in Base Stores and is identical to Armors that have
 * 'storeItem' = "STR_NONE". Its cost is zero.
 * @return, true if basic
 */
bool RuleArmor::isBasic() const
{
	return _isBasic;
}

/**
 * Gets if the Armor is powered and suitable for Mars.
 * @return, true if life-supporting
 */
bool RuleArmor::isSpacesuit() const
{
	return _isSpacesuit;
}

/**
 * Gets the Armor's agility used to determine stamina expenditure.
 * Higher values equate to less energy cost. Typically:
 *		0 personal armor
 *		1 no armor
 *		2 powered/flight suits
 * @note Armor cannot subtract more energy than a tile requires due to coding.
 * @return, agility
 */
int RuleArmor::getAgility() const
{
	return _agility;
}

}
