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

#include "UnitSprite.h"

//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
//#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"
#include "../Engine/SurfaceSet.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
UnitSprite::UnitSprite(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_unit(nullptr),
		_itRT(nullptr),
		_itLT(nullptr),
		_unitSet(nullptr),
		_itSetRT(nullptr),
		_itSetLT(nullptr),
		_quad(0),
		_aniFrame(0),
		_drawRoutine(0),
		_color(nullptr),
		_colorSize(0)
{}

/**
 * Deletes the UnitSprite.
 */
UnitSprite::~UnitSprite()
{}

/**
 * Changes the surface sets for the UnitSprite to get resources for rendering.
 * @param unitSet - pointer to a unit's SurfaceSet
 * @param itSetRT - pointer to an item's SurfaceSet
 * @param itSetLT - pointer to an item's SurfaceSet
 */
void UnitSprite::setSurfaces(
		SurfaceSet* const unitSet,
		SurfaceSet* const itSetRT,
		SurfaceSet* const itSetLT)
{
	_unitSet = unitSet;
	_itSetRT = itSetRT;
	_itSetLT = itSetLT;

	_redraw = true;
}

/**
 * Links this sprite to a BattleUnit to get the data for rendering.
 * @param unit - pointer to a BattleUnit
 * @param quad - quadrant for large units (default 0)
 */
void UnitSprite::setBattleUnit(
		BattleUnit* const unit,
		int quad)
{
	_drawRoutine = unit->getArmor()->getDrawRoutine();

	_unit = unit;
	_quad = quad;

	_redraw = true;

	if (Options::battleHairBleach == true)
	{
		_colorSize = static_cast<int>(_unit->getRecolor().size());
		if (_colorSize != 0)
			_color = &(_unit->getRecolor()[0u]);
		else
			_color = nullptr;
	}
}

/**
 * Links this sprite's right-hand to a BattleItem to get the data for rendering.
 * @param item - pointer to a BattleItem (default nullptr)
 */
void UnitSprite::setBattleItRH(const BattleItem* const item)
{
	_itRT = item;
	_redraw = true;
}

/**
 * Links this sprite's left-hand to a BattleItem to get the data for rendering.
 * @param item - pointer to a BattleItem (default nullptr)
 */
void UnitSprite::setBattleItLH(const BattleItem* const item)
{
	_itLT = item;
	_redraw = true;
}


namespace
{

struct ColorReplace
{
	static const Uint8
		ColorGroup = 15 << 4,
		ColorShade = 15;

	///
	static inline bool loop(
			Uint8& dest,
			const Uint8& src,
			const std::pair<Uint8, Uint8>& face_color)
	{
		if ((src & ColorGroup) == face_color.first)
		{
			dest = face_color.second + (src & ColorShade);
			return true;
		}

		return false;
	}

	///
	static inline void func(
			Uint8& dest,
			const Uint8& src,
			const std::pair<Uint8, Uint8>* color,
			int colors,
			int)
	{
		if (src != 0)
		{
			for (size_t
					i = 0;
					i != static_cast<size_t>(colors);
					++i)
			{
				if (loop(
						dest,
						src,
						color[i]) == true)
				{
					return;
				}
			}
			dest = src;
		}
	}
};

}


void UnitSprite::drawRecolored(Surface* src)
{
	if (_colorSize != 0)
	{
		lock();
		ShaderDraw<ColorReplace>(
							ShaderSurface(this),
							ShaderSurface(src),
							ShaderScalar(_color),
							ShaderScalar(_colorSize));
		unlock();
	}
	else
		src->blit(this);
}

/**
 * Sets the animation frame for animated units.
 * @param frame - frame number
 */
void UnitSprite::setAnimationFrame(int frame)
{
	_aniFrame = frame;
}

/**
 * Draws a unit using the drawing rules for the unit.
 * @note This function is called by Map for each BattleUnit in the viewable area
 * of the screen.
 */
void UnitSprite::draw()
{
	//Log(LOG_INFO) << "UnitSprite::draw() Routine " << _drawRoutine;
	Surface::draw();

	void (UnitSprite::*routines[])() = // Array of drawing routines.
	{
		&UnitSprite::drawRoutine0,
		&UnitSprite::drawRoutine1,
		&UnitSprite::drawRoutine2,
		&UnitSprite::drawRoutine3,
		&UnitSprite::drawRoutine4,
		&UnitSprite::drawRoutine5,
		&UnitSprite::drawRoutine6,
		&UnitSprite::drawRoutine7,
		&UnitSprite::drawRoutine8,
		&UnitSprite::drawRoutine9,
		&UnitSprite::drawRoutine0
	};

	(this->*(routines[_drawRoutine]))(); // Call the matching routine.
}

/**
 * Drawing routine for soldiers, sectoids, and non-stock civilians (all routine 0),
 * or mutons (subroutine 10).
 */
void UnitSprite::drawRoutine0()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int // magic static numbers
		maleTorso	=  32,
		femaleTorso	= 267,
		legsStand	=  16,
		legsKneel	=  24,
		die			= 264,
		legsFloat	= 275,

		larmStand	=   0,
		rarmStand	=   8,
		rarm1H		= 232,
		larm2H		= 240,
		rarm2H		= 248,
		rarmShoot	= 256,

// direction:           0      1      2        3        4        5        6        7
// #firstFrame:        56     80    104      128      152      176      200      224
		legsWalk[8] = {56, 56+24, 56+24*2, 56+24*3, 56+24*4, 56+24*5, 56+24*6, 56+24*7},
// #firstFrame:        40     64     88      112      136      160      184      208
		larmWalk[8] = {40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7},
// #firstFrame:        48     72     96      120      144      168      192      216
		rarmWalk[8] = {48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7},

		yoffWalk[8]		= { 1,  0, -1,  0,  1,  0, -1,  0}, // bobbing up and down
		yoffWalk_mut[8]	= { 1,  1,  0,  0,  1,  1,  0,  0}, // bobbing up and down (muton)
		offX[8]			= { 8, 10,  7,  4, -9,-11, -7, -3}, // for the weapons
		offY[8]			= {-6, -3,  0,  2,  0, -4, -7, -9}, // for the weapons
		offX2[8]		= {-8,  3,  5, 12,  6, -1, -5,-13}, // for the left handed weapons
		offY2[8]		= { 1, -4, -2,  0,  3,  3,  5,  0}, // for the left handed weapons
		offX3[8]		= { 0,  0,  2,  2,  0,  0,  0,  0}, // for the weapons (muton)
		offY3[8]		= {-3, -3, -1, -1, -1, -3, -3, -2}, // for the weapons (muton)
		offX4[8]		= {-8,  2,  7, 14,  7, -2, -4, -8}, // for the left handed weapons
		offY4[8]		= {-3, -3, -1,  0,  3,  3,  0,  1}, // for the left handed weapons
		offX5[8]		= {-1,  1,  1,  2,  0, -1,  0,  0}, // for the weapons (muton)
		offY5[8]		= { 1, -1, -1, -1, -1, -1, -3,  0}, // for the weapons (muton)
		offX6[8]		= { 0,  6,  6,  12,-4, -5, -5,-13}, // for the left handed rifles
		offY6[8]		= {-4, -4, -1,  0,  5,  0,  1,  0}, // for the left handed rifles
		offX7[8]		= { 0,  6,  8, 12,  2, -5, -5,-13}, // for the left handed rifles (muton)
		offY7[8]		= {-4, -6, -1,  0,  3,  0,  1,  0}, // for the left handed rifles (muton)

		offYKneel  =  4,
		offXAiming = 16,

		expectedUnitHeight = 22;

	const int unitDir (_unit->getUnitDirection());
	int walkPhase;
	if (_unit->isStrafeBackwards() == true)
	{
		if ((walkPhase = 8 - _unit->getWalkPhase()) == 8)
			walkPhase = 0;
	}
	else
		walkPhase = _unit->getWalkPhase();

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
		drawRecolored(torso);
		return;
	}

	Surface
		* rightArm,
		* leftArm,
		* legs,
		* itRT = nullptr,
		* itLT = nullptr;

	if (_drawRoutine == 0)
	{
		if (_unit->getArmor()->getForcedTorso() == TORSO_ALWAYS_FEMALE
			|| (_unit->getGender() == GENDER_FEMALE
				&& _unit->getArmor()->getForcedTorso() != TORSO_ALWAYS_MALE))
		{
			torso = _unitSet->getFrame(unitDir + femaleTorso);
		}
		else
			torso = _unitSet->getFrame(unitDir + maleTorso);
	}
	else
	{
		if (_unit->getGender() == GENDER_FEMALE)
			torso = _unitSet->getFrame(unitDir + femaleTorso);
		else
			torso = _unitSet->getFrame(unitDir + maleTorso);
	}

	const bool
		isWalking = (_unit->getUnitStatus() == STATUS_WALKING),
		isKneeled = _unit->isKneeled();
	int torsoHandsWeaponY;

	if (isWalking == true)
	{
		if (_drawRoutine == 10)
			torsoHandsWeaponY = yoffWalk_mut[walkPhase];
		else
			torsoHandsWeaponY = yoffWalk[walkPhase];

		torso->setY(torsoHandsWeaponY);

		rightArm	= _unitSet->getFrame(walkPhase + rarmWalk[unitDir]);
		leftArm		= _unitSet->getFrame(walkPhase + larmWalk[unitDir]);
		legs		= _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
	}
	else
	{
		torsoHandsWeaponY = 0;

		if (isKneeled == true)
			legs = _unitSet->getFrame(unitDir + legsKneel);
		else if (_unit->isFloating() == true
			&& _unit->getMoveTypeUnit() == MT_FLY)
		{
			legs = _unitSet->getFrame(unitDir + legsFloat);
		}
		else
			legs = _unitSet->getFrame(unitDir + legsStand);

		rightArm	= _unitSet->getFrame(unitDir + rarmStand);
		leftArm		= _unitSet->getFrame(unitDir + larmStand);
	}

	sortHandObjects();

	if (_itRT != nullptr)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& (_itRT->getRules()->isTwoHanded() == true
				|| _itRT->getRules()->getBattleType() == BT_MELEE))
		{
			itRT = _itSetRT->getFrame((unitDir + 2) % 8 + _itRT->getRules()->getHandSprite());
			itRT->setX(offX[unitDir]);
			itRT->setY(offY[unitDir]);
		}
		else
		{
			itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());
			if (_drawRoutine == 10)
			{
				if (_itRT->getRules()->isTwoHanded() == true)
				{
					itRT->setX(offX3[unitDir]);
					itRT->setY(offY3[unitDir]);
				}
				else
				{
					itRT->setX(offX5[unitDir]);
					itRT->setY(offY5[unitDir]);
				}
			}
			else
			{
				itRT->setX(0);
				itRT->setY(0);
			}
		}

		if (_itRT->getRules()->isTwoHanded() == true
			|| _itRT->getRules()->getBattleType() == BT_MELEE)
		{
			leftArm = _unitSet->getFrame(unitDir + larm2H);
			if (_unit->getUnitStatus() == STATUS_AIMING)
				rightArm = _unitSet->getFrame(unitDir + rarmShoot);
			else
				rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}
		else
		{
			if (_drawRoutine == 10)
				rightArm = _unitSet->getFrame(unitDir + rarm2H);
			else
				rightArm = _unitSet->getFrame(unitDir + rarm1H);
		}

		if (isWalking == true)
		{
			rightArm->setY(torsoHandsWeaponY);
			itRT->setY(itRT->getY() + torsoHandsWeaponY);
			if (_itRT->getRules()->isTwoHanded() == true
				|| _itRT->getRules()->getBattleType() == BT_MELEE)
			{
				leftArm->setY(torsoHandsWeaponY);
			}
		}
	}

	if (_itLT != nullptr)
	{
		leftArm = _unitSet->getFrame(larm2H + unitDir);
		itLT = _itSetLT->getFrame(unitDir + _itLT->getRules()->getHandSprite());
		if (_itLT->getRules()->isTwoHanded() == false)
		{
			if (_drawRoutine == 10)
			{
				itLT->setX(offX4[unitDir]);
				itLT->setY(offY4[unitDir]);
			}
			else
			{
				itLT->setX(offX2[unitDir]);
				itLT->setY(offY2[unitDir]);
			}
		}
		else
		{
			itLT->setX(0);
			itLT->setY(0);

			rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}

		if (_unit->getUnitStatus() == STATUS_AIMING
			&& (_itLT->getRules()->isTwoHanded() == true
				|| _itLT->getRules()->getBattleType() == BT_MELEE))
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());
			if (_drawRoutine == 10)
			{
				itLT->setX(offX7[unitDir]);
				itLT->setY(offY7[unitDir]);
			}
			else
			{
				itLT->setX(offX6[unitDir]);
				itLT->setY(offY6[unitDir]);
			}
			rightArm = _unitSet->getFrame(unitDir + rarmShoot);
		}

		if (isWalking == true)
		{
			leftArm->setY(torsoHandsWeaponY);
			itLT->setY(itLT->getY() + torsoHandsWeaponY);
			if (_itLT->getRules()->isTwoHanded() == true
				|| _itLT->getRules()->getBattleType() == BT_MELEE)
			{
				rightArm->setY(torsoHandsWeaponY);
			}
		}
	}

	if (isKneeled == true)
	{
		torso->		setY(offYKneel);
		rightArm->	setY(offYKneel);
		leftArm->	setY(offYKneel);
		if (itRT) itRT->setY(itRT->getY() + offYKneel);
		if (itLT) itLT->setY(itLT->getY() + offYKneel);
	}
	else if (isWalking == false)
	{
		torso->		setY(0);
		rightArm->	setY(0);
		leftArm->	setY(0);
	}

	if (itRT) itRT->setY(itRT->getY() + (expectedUnitHeight - _unit->getStandHeight()));
	if (itLT) itLT->setY(itLT->getY() + (expectedUnitHeight - _unit->getStandHeight()));

	if (_unit->getUnitStatus() == STATUS_AIMING)
	{
		if (_itRT == nullptr && _itLT == nullptr) // using Universal Fist. so PUNCH!! ( this is so funny )
			rightArm = _unitSet->getFrame(rarmShoot + unitDir);

		torso->		setX(offXAiming);
		rightArm->	setX(offXAiming);
		leftArm->	setX(offXAiming);
		legs->		setX(offXAiming);
		if (itRT) itRT->setX(itRT->getX() + offXAiming);
		if (itLT) itLT->setX(itLT->getX() + offXAiming);
	}

	switch (unitDir)
	{
		case 0:
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
			drawRecolored(leftArm);
			drawRecolored(legs);
			if (itLT) itLT->blit(this);
			drawRecolored(torso);
			if (itRT) itRT->blit(this);
			drawRecolored(rightArm);
		break;
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			if (itLT) itLT->blit(this);
			if (itRT) itRT->blit(this);
			drawRecolored(rightArm);
		break;
		case 3:
			if (_unit->getUnitStatus() != STATUS_AIMING
				&& ((_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
					|| (_itLT != nullptr && _itLT->getRules()->isTwoHanded() == true)))
			{
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
				drawRecolored(rightArm);
			}
			else
			{
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				drawRecolored(rightArm);
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
			}
		break;
		case 4:
			drawRecolored(legs);
			drawRecolored(rightArm);
			drawRecolored(torso);
			drawRecolored(leftArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 5:
			if (_unit->getUnitStatus() != STATUS_AIMING
				&& ((_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
					|| (_itLT != nullptr && _itLT->getRules()->isTwoHanded() == true)))
			{
				drawRecolored(rightArm);
				drawRecolored(legs);
				drawRecolored(torso);
				drawRecolored(leftArm);
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
			}
			else
			{
				drawRecolored(rightArm);
				drawRecolored(legs);
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
				drawRecolored(torso);
				drawRecolored(leftArm);
			}
		break;
		case 6:
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			if (_unit->getUnitStatus() != STATUS_AIMING
				&& ((_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
					|| (_itLT != nullptr && _itLT->getRules()->isTwoHanded() == true)))
			{
				drawRecolored(rightArm);
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
				drawRecolored(leftArm);
				drawRecolored(legs);
				drawRecolored(torso);
			}
			else
			{
				if (itRT) itRT->blit(this);
				if (itLT) itLT->blit(this);
				drawRecolored(leftArm);
				drawRecolored(rightArm);
				drawRecolored(legs);
				drawRecolored(torso);
			}
	}

	torso->setX(0);
	legs->setX(0);
	leftArm->setX(0);
	rightArm->setX(0);
	if (itRT) itRT->setX(0);
	if (itLT) itLT->setX(0);
}

/**
 * Drawing routine for floaters and waspites.
 */
void UnitSprite::drawRoutine1()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int // magic static numbers
		stand = 16,
		walk  = 24,
		die   = 64,

		rarm      =  0,
		larm      =  8,
		rarm2H    = 75,
		larm2H    = 67,
		rarmShoot = 83,
		rarm1H    = 91, // note that arms are switched vs "normal" sheets

		yoffWalk[8]	= { 0,  0,  0,  0,  0,  0,  0,  0 }, // bobbing up and down
		offX[8]		= { 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8]		= {-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8]	= {-8,  3,  7, 13,  6, -3, -5,-13 }, // for the weapons
		offY2[8]	= { 1, -4, -1,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8]	= { 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8]	= {-4, -4, -1,  0,  5,  0,  1,  0 }, // for the left handed rifles

		offXAiming = 16;

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
		drawRecolored(torso);
		return;
	}

	const int
		unitDir		= _unit->getUnitDirection(),
		walkPhase	= _unit->getWalkPhase();

	Surface
		* rightArm	= _unitSet->getFrame(unitDir + rarm),
		* leftArm	= _unitSet->getFrame(unitDir + larm),
		* itRT		= nullptr,
		* itLT		= nullptr;

	if (_unit->getUnitStatus() == STATUS_WALKING
		|| _unit->getUnitStatus() == STATUS_FLYING)
	{
		torso = _unitSet->getFrame(
								  unitDir * 5
								+ static_cast<int>(
								  static_cast<float>(walkPhase) / 1.6f)
								+ walk);
		torso->setY(yoffWalk[walkPhase]);
	}
	else
		torso = _unitSet->getFrame(unitDir + stand);

	sortHandObjects();

	if (_itRT != nullptr)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itRT->getRules()->isTwoHanded() == true)
		{
			itRT = _itSetRT->getFrame((unitDir + 2) % 8 + _itRT->getRules()->getHandSprite());
			itRT->setX(offX[unitDir]);
			itRT->setY(offY[unitDir]);
		}
		else
		{
			itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());
			itRT->setX(0);
			itRT->setY(0);
		}

		if (_itRT->getRules()->isTwoHanded() == true)
		{
			leftArm = _unitSet->getFrame(unitDir + larm2H);
			if (_unit->getUnitStatus() == STATUS_AIMING)
				rightArm = _unitSet->getFrame(unitDir + rarmShoot);
			else
				rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}
		else
			rightArm = _unitSet->getFrame(unitDir + rarm1H);
	}

	if (_itLT != nullptr)
	{
		leftArm = _unitSet->getFrame(unitDir + larm2H);
		itLT = _itSetLT->getFrame(unitDir + _itLT->getRules()->getHandSprite());
		if (_itLT->getRules()->isTwoHanded() == false)
		{
			itLT->setX(offX2[unitDir]);
			itLT->setY(offY2[unitDir]);
		}
		else
		{
			itLT->setX(0);
			itLT->setY(0);
			rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}

		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itLT->getRules()->isTwoHanded() == true)
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());
			itLT->setX(offX3[unitDir]);
			itLT->setY(offY3[unitDir]);
			rightArm = _unitSet->getFrame(unitDir + rarmShoot);
		}

		if (_unit->getUnitStatus() == STATUS_WALKING
			|| _unit->getUnitStatus() == STATUS_FLYING)
		{
			leftArm->setY(yoffWalk[walkPhase]);
			itLT->setY(itLT->getY() + yoffWalk[walkPhase]);
			if (_itLT->getRules()->isTwoHanded() == true)
				rightArm->setY(yoffWalk[walkPhase]);
		}
	}

	if (_unit->getUnitStatus() != STATUS_WALKING
		&& _unit->getUnitStatus() != STATUS_FLYING)
	{
		torso->		setY(0);
		rightArm->	setY(0);
		leftArm->	setY(0);
	}

	if (_unit->getUnitStatus() == STATUS_AIMING)
	{
		torso->		setX(offXAiming);
		rightArm->	setX(offXAiming);
		leftArm->	setX(offXAiming);
		if (itRT) itRT->setX(itRT->getX() + offXAiming);
		if (itLT) itLT->setX(itLT->getX() + offXAiming);
	}

	switch (unitDir)
	{
		case 0:
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(leftArm);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
		case 2:
			drawRecolored(leftArm);
			drawRecolored(torso);
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 3:
		case 4:
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 5:
			drawRecolored(rightArm);
			drawRecolored(torso);
			drawRecolored(leftArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 6:
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(leftArm);
			drawRecolored(torso);
	}

	torso->setX(0);
	leftArm->setX(0);
	rightArm->setX(0);
	if (itRT) itRT->setX(0);
	if (itLT) itLT->setX(0);
}

/**
 * Drawing routine for xCom tanks.
 */
void UnitSprite::drawRoutine2()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int walk = 104; // magic static number

	Surface* quad;

	const int
		turret = _unit->getTurretType(),
		hover  = (_unit->getMoveTypeUnit() == MT_FLY) ? 32 : 0;

	if (_quad != 0 && hover != 0)
	{
		quad = _unitSet->getFrame((_quad - 1) * 8 + _aniFrame + walk);
		drawRecolored(quad);
	}

	// This is a fix, more of a workaround for tank's reverse strafing move.
	// There's a problem somewhere that keeps switching BattleUnit::_direction
	// back and forth ... in reverse gears. That is, _direction should remain
	// constant throughout a single-tile strafe move with tanks. But at least
	// '_faceDirection' seems constant during these sprite-frames.
	int unitDir;
	if (_unit->getFaceDirection() != -1)
		unitDir = _unit->getFaceDirection();
	else
		unitDir = _unit->getUnitDirection();

	quad = _unitSet->getFrame(hover + (_quad * 8) + unitDir);
	drawRecolored(quad);

/*	if (_quad == 3 && turret != -1)
	{
		quad = _unitSet->getFrame(64 + (turret * 8) + _unit->getTurretDirection());
		int
			turretOffsetX = 0,
			turretOffsetY = -4;
		if (hover != 0)
		{
			turretOffsetX += offX[unitDir];
			turretOffsetY += offY[unitDir];
		}

		quad->setX(turretOffsetX);
		quad->setY(turretOffsetY);
		drawRecolored(quad);
	} */
	if (turret != -1)
	{
		quad = _unitSet->getFrame(64 + (turret * 8) + _unit->getTurretDirection());
		int
			turretOffsetX,
			turretOffsetY;

		if (_quad == 0)
		{
			turretOffsetX = 0,
			turretOffsetY = 12;
		}
		else if (_quad == 1)
		{
			turretOffsetX = -16,
			turretOffsetY = 4;
		}
		else if (_quad == 2)
		{
			turretOffsetX = 16,
			turretOffsetY = 4;
		}
		else // _quad== 3
		{
			turretOffsetX = 0,
			turretOffsetY = -4;
		}

		if (hover != 0)
		{
			static const int
				offX[8] = {-2,-7,-5, 0, 5, 7, 2, 0},
				offY[8] = {-1,-3,-4,-5,-4,-3,-1,-1};

			turretOffsetX += offX[unitDir];
			turretOffsetY += offY[unitDir];
		}

		quad->setX(turretOffsetX);
		quad->setY(turretOffsetY);
		drawRecolored(quad);
	}
}

/**
 * Drawing routine for cyberdiscs.
 */
void UnitSprite::drawRoutine3()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int walk = 32; // magic static number

	Surface* quad;
	if (_quad != 0)
	{
		quad = _unitSet->getFrame((_quad - 1) * 8 + _aniFrame + walk);
		drawRecolored(quad);
	}

	quad = _unitSet->getFrame((_quad * 8) + _unit->getUnitDirection());
	drawRecolored(quad);
}

/**
 * Drawing routine for stock civilians, ethereals, zombies, dogs, cybermites,
 * and scout-drones.
 */
void UnitSprite::drawRoutine4()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int // magic static numbers
		stand =  0,
		walk  =  8,
		die   = 72,

		offXAiming = 16,

		offX[8]		= { 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8]		= {-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8]	= {-8,  3,  5, 12,  6, -1, -5,-13 }, // for the weapons
		offY2[8]	= { 1, -4, -2,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8]	= { 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8]	= {-4, -4, -1,  0,  5,  0,  1,  0 }; // for the left handed rifles

	Surface* sprite;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		sprite = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
		drawRecolored(sprite);
		return;
	}

	Surface
		* itRT = nullptr,
		* itLT = nullptr;

	const int unitDir = _unit->getUnitDirection();

	if (_unit->getUnitStatus() == STATUS_WALKING
		&& _unit->getRaceString() != "STR_ETHEREAL") // stop the ethereal shuffle.
	{
		sprite = _unitSet->getFrame(unitDir * 8 + _unit->getWalkPhase() + walk);
	}
	else
		sprite = _unitSet->getFrame(stand + unitDir);

	sortHandObjects();

	if (_itRT != nullptr && _itRT->getRules()->isFixed() == false)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itRT->getRules()->isTwoHanded() == true)
		{
			itRT = _itSetRT->getFrame((unitDir + 2) % 8 + _itRT->getRules()->getHandSprite());
			itRT->setX(offX[unitDir]);
			itRT->setY(offY[unitDir]);
		}
		else
		{
			if (_itRT->getInventorySection()->getSectionType() == ST_RIGHTHAND) // wtf. how could right-item not be in right-hand: Dual-wield sorting hand-obs.
			{
				itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());
				itRT->setX(0);
				itRT->setY(0);
			}
			else
			{
				itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());
				itRT->setX(offX2[unitDir]);
				itRT->setY(offY2[unitDir]);
			}
		}
	}

	if (_itLT != nullptr && _itLT->getRules()->isFixed() == false)
	{
		itLT = _itSetLT->getFrame(unitDir + _itLT->getRules()->getHandSprite());
		if (_itLT->getRules()->isTwoHanded() == false)
		{
			itLT->setX(offX2[unitDir]);
			itLT->setY(offY2[unitDir]);
		}
		else
		{
			itLT->setX(0);
			itLT->setY(0);
		}

		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itLT->getRules()->isTwoHanded() == true)
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());
			itLT->setX(offX3[unitDir]);
			itLT->setY(offY3[unitDir]);
		}
	}

	if (_unit->getUnitStatus() == STATUS_AIMING)
	{
		sprite->setX(offXAiming);
		if (itRT) itRT->setX(itRT->getX() + offXAiming);
		if (itLT) itLT->setX(itLT->getX() + offXAiming);
	}

	switch (unitDir)
	{
		case 0:
			if (itLT) itLT->blit(this);
			if (itRT) itRT->blit(this);
			drawRecolored(sprite);
		break;
		case 1:
			if (itLT) itLT->blit(this);
			drawRecolored(sprite);
			if (itRT) itRT->blit(this);
		break;
		case 2:
			drawRecolored(sprite);
			if (itLT) itLT->blit(this);
			if (itRT) itRT->blit(this);
		break;
		case 3:
		case 4:
			drawRecolored(sprite);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 5:
			if (itRT) itRT->blit(this);
			drawRecolored(sprite);
			if (itLT) itLT->blit(this);
		break;
		case 6:
		case 7:
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(sprite);
	}

	sprite->setX(0);
	if (itRT) itRT->setX(0);
	if (itLT) itLT->setX(0);
}

/**
 * Drawing routine for sectopods and reapers.
 */
void UnitSprite::drawRoutine5()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int walk = 32; // magic static number

	Surface* quad;
	if (_unit->getUnitStatus() == STATUS_WALKING)
		quad = _unitSet->getFrame(_unit->getUnitDirection() * 16
								+ _quad * 4
								+ _unit->getWalkPhase() / 2 % 4
								+ walk);
	else
		quad = _unitSet->getFrame(_unit->getUnitDirection() + _quad * 8);

	drawRecolored(quad);
}

/**
 * Drawing routine for snakemans.
 */
void UnitSprite::drawRoutine6()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int // magic static numbers
		body		= 24,
		legsStand	= 16,
		die			= 96,

		larmStand	=   0,
		rarmStand	=   8,
		rarm1H		=  99,
		larm2H		= 107,
		rarm2H		= 115,
		rarmShoot	= 123,

		offXAiming	= 16,

		legsWalk[8]		= {32, 40, 48, 56, 64, 72, 80, 88},
		yoffWalk[8]		= { 3,  3,  2,  1,  0,  0,  1,  2}, // bobbing up and down
		xoffWalka[8]	= { 0,  0,  1,  2,  3,  3,  2,  1},
		xoffWalkb[8]	= { 0,  0, -1, -2, -3, -3, -2, -1},
		yoffStand[8]	= { 2,  1,  1,  0,  0,  0,  0,  0},
		offX[8]			= { 8, 10,  5,  2, -8,-10, -5, -2}, // for the weapons
		offY[8]			= {-6, -3,  0,  0,  2, -3, -7, -9}, // for the weapons
		offX2[8]		= {-8,  2,  7, 13,  7,  0, -3,-15}, // for the weapons
		offY2[8]		= { 1, -4, -2,  0,  3,  3,  5,  0}, // for the weapons
		offX3[8]		= { 0,  6,  6, 12, -4, -5, -5,-13}, // for the left handed rifles
		offY3[8]		= {-4, -4, -1,  0,  5,  0,  1,  0}; // for the left handed rifles

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
		drawRecolored(torso);
		return;
	}

	const int
		unitDir		= _unit->getUnitDirection(),
		walkPhase	= _unit->getWalkPhase();

	Surface
		* legs,
		* rightArm	= _unitSet->getFrame(unitDir + rarmStand),
		* leftArm	= _unitSet->getFrame(unitDir + larmStand),
		* itRT		= nullptr,
		* itLT		= nullptr;

	torso = _unitSet->getFrame(unitDir + body);

	if (_unit->getUnitStatus() == STATUS_WALKING)
	{
		int xoffWalk;
		if (unitDir < 3)
			xoffWalk = xoffWalka[walkPhase];
		if (unitDir < 7 && unitDir > 3)
			xoffWalk = xoffWalkb[walkPhase];
		else
			xoffWalk = 0;

		torso->		setX(xoffWalk);
		torso->		setY(yoffWalk[walkPhase]);
		rightArm->	setX(xoffWalk);
		rightArm->	setY(yoffWalk[walkPhase]);
		leftArm->	setX(xoffWalk);
		leftArm->	setY(yoffWalk[walkPhase]);

		legs = _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
	}
	else
		legs = _unitSet->getFrame(unitDir + legsStand);

	sortHandObjects();

	if (_itRT != nullptr)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itRT->getRules()->isTwoHanded() == true)
		{
			itRT = _itSetRT->getFrame((unitDir + 2) % 8 + _itRT->getRules()->getHandSprite());
			itRT->setX(offX[unitDir]);
			itRT->setY(offY[unitDir]);
		}
		else
		{
			itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());
			itRT->setX(0);
			itRT->setY(0);

			if (_itRT->getRules()->isTwoHanded() == false)
				itRT->setY(yoffStand[unitDir]);
		}

		if (_itRT->getRules()->isTwoHanded() == true)
		{
			leftArm = _unitSet->getFrame(unitDir + larm2H);
			if (_unit->getUnitStatus() == STATUS_AIMING)
				rightArm = _unitSet->getFrame(unitDir + rarmShoot);
			else
				rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}
		else
			rightArm = _unitSet->getFrame(unitDir + rarm1H);

		if (_unit->getUnitStatus() == STATUS_WALKING)
		{
			itRT->setY(yoffWalk[walkPhase]);
			rightArm->setY(yoffWalk[walkPhase]);
			if (_itRT->getRules()->isTwoHanded() == true)
				leftArm->setY(yoffWalk[walkPhase]);
		}
	}

	if (_itLT != nullptr)
	{
		leftArm = _unitSet->getFrame(unitDir + larm2H);
		itLT = _itSetLT->getFrame(unitDir + _itLT->getRules()->getHandSprite());
		if (_itLT->getRules()->isTwoHanded() == false)
		{
			itLT->setX(offX2[unitDir]);
			itLT->setY(offY2[unitDir]);
		}
		else
		{
			itLT->setX(0);
			itLT->setY(0);

			if (_itLT->getRules()->isTwoHanded() == false)
				itLT->setY(yoffStand[unitDir]);

			rightArm = _unitSet->getFrame(unitDir + rarm2H);
		}

		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itLT->getRules()->isTwoHanded() == true)
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());
			itLT->setX(offX3[unitDir]);
			itLT->setY(offY3[unitDir]);
			rightArm = _unitSet->getFrame(unitDir + rarmShoot);
		}

		if (_unit->getUnitStatus() == STATUS_WALKING)
		{
			leftArm->setY(yoffWalk[walkPhase]);
			itLT->setY(offY2[unitDir] + yoffWalk[walkPhase]);
			if (_itLT->getRules()->isTwoHanded() == true)
				rightArm->setY(yoffWalk[walkPhase]);
		}
	}

	if (_unit->getUnitStatus() != STATUS_WALKING)
	{
		torso->		setY(0);
		rightArm->	setY(0);
		leftArm->	setY(0);
	}

	if (_unit->getUnitStatus() == STATUS_AIMING)
	{
		if (_itRT == nullptr && _itLT == nullptr) // using Universal Fist. so PUNCH!! ( this is so funny )
			rightArm = _unitSet->getFrame(unitDir + rarmShoot);

		torso->		setX(offXAiming);
		rightArm->	setX(offXAiming);
		leftArm->	setX(offXAiming);
		legs->		setX(offXAiming);
		if (itRT) itRT->setX(itRT->getX() + offXAiming);
		if (itLT) itLT->setX(itLT->getX() + offXAiming);
	}

	switch (unitDir)
	{
		case 0:
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 1:
			drawRecolored(leftArm);
			drawRecolored(legs);
			if (itLT) itLT->blit(this);
			drawRecolored(torso);
			if (itRT) itRT->blit(this);
			drawRecolored(rightArm);
		break;
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 3:
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 4:
		case 5:
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
		break;
		case 6:
			drawRecolored(rightArm);
			drawRecolored(legs);
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			if (itRT) itRT->blit(this);
			if (itLT) itLT->blit(this);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
	}

	torso->		setX(0);
	rightArm->	setX(0);
	leftArm->	setX(0);
	legs->		setX(0);
	if (itRT) itRT->setX(0); // was getX
	if (itLT) itLT->setX(0); // was getX
}

/**
 * Drawing routine for chryssalid.
 */
void UnitSprite::drawRoutine7()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	// what no _redraw!

	static const int // magic static numbers
		body		=  24,
		legsStand	=  16,
		die			= 224,

		larmStand	= 0,
		rarmStand	= 8,

		legsWalk[8] = { 48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7 },
		larmWalk[8] = { 32, 32+24, 32+24*2, 32+24*3, 32+24*4, 32+24*5, 32+24*6, 32+24*7 },
		rarmWalk[8] = { 40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7 },
		yoffWalk[8] = {  1,     0,      -1,       0,       1,       0,      -1,       0 }; // bobbing up and down

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
		drawRecolored(torso);
		return;
	}

	const int unitDir = _unit->getUnitDirection();

	torso = _unitSet->getFrame(unitDir + body);

	Surface
		* rightArm,
		* leftArm,
		* legs;

	if (_unit->getUnitStatus() == STATUS_WALKING)
	{
		const int walkPhase	= _unit->getWalkPhase();
		torso->setY(yoffWalk[walkPhase]);
		rightArm	= _unitSet->getFrame(walkPhase + rarmWalk[unitDir]);
		leftArm		= _unitSet->getFrame(walkPhase + larmWalk[unitDir]);
		legs		= _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
	}
	else
	{
		torso->setY(0);
		rightArm	= _unitSet->getFrame(unitDir + rarmStand);
		leftArm		= _unitSet->getFrame(unitDir + larmStand);
		legs		= _unitSet->getFrame(unitDir + legsStand);
	}

	switch (unitDir)
	{
		case 0:
		case 1:
		case 2:
			drawRecolored(leftArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(rightArm);
		break;
		case 3:
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
			drawRecolored(rightArm);
		break;
		case 4:
		case 5:
		case 6:
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
			drawRecolored(leftArm);
		break;
		case 7:
			drawRecolored(leftArm);
			drawRecolored(rightArm);
			drawRecolored(legs);
			drawRecolored(torso);
	}
}

/**
 * Drawing routine for silacoids.
 */
void UnitSprite::drawRoutine8()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	_redraw = true;

	static const int // magic static numbers
		body = 0,
		aim  = 5,
		die  = 6,

		pulsate[8] = {0,1,2,3,4,3,2,1};

	Surface* sprite;
	switch (_unit->getUnitStatus())
	{
		case STATUS_COLLAPSING:
			sprite = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
			break;
		case STATUS_AIMING:
			sprite = _unitSet->getFrame(aim);
			break;

		default:
			sprite = _unitSet->getFrame(body + pulsate[_aniFrame]);
	}

	drawRecolored(sprite);
}

/**
 * Drawing routine for celatids.
 */
void UnitSprite::drawRoutine9()
{
	if (_unit->isOut_t(OUT_STAT) == true)
		return;

	_redraw = true;

	static const int // magic static numbers
		body  =  0,
		die   = 25,
		shoot =  8; // frames 8..23 or ..24 (24 is merely a green ball sprite)

	Surface* sprite;
	switch (_unit->getUnitStatus())
	{
		case STATUS_COLLAPSING:
			sprite = _unitSet->getFrame(_unit->getCollapsingPhase() + die);
			break;

		case STATUS_AIMING:
		{
			const int framesTotal = _unit->getArmor()->getShootFrames();
			int
				phase = _unit->getAimingPhase(),
				extra;

			if (phase == framesTotal)
				extra = 2; // bounce back one frame at the end.
			else
				extra = 0;

			sprite = _unitSet->getFrame(std::min(
											shoot + phase - extra,
											shoot + framesTotal - 1));
			// Clamp that because slow (read, long) think()->draw intervals
			// cause it to exceed the upper bound of total shootFrames.
			_unit->setAimingPhase(++phase);
			// -> let BattleUnit::keepAiming() iterate the final aimPhase. nix
			// that; super-slow animation speed doesn't even let keepAiming()
			// get called. ... not sure how the animation is ended in that case
			// but something does it.
			break;
		}

		default:
			sprite = _unitSet->getFrame(body + _aniFrame);
	}

	drawRecolored(sprite);
}

/**
 * Determines which hand-objects to display.
 */
void UnitSprite::sortHandObjects()
{
	// this is the draw active-hand code:
	if (_itRT != nullptr && _itLT != nullptr)
	{
		if (_unit->getActiveHand() == AH_LEFT)
			_itRT = _itLT;

		_itLT = nullptr;
	}
	// this is the draw dual-wield code:
/*	if (_itRT && _itRT->getRules()->isTwoHanded() == true)
	{
		if (_itLT && _itLT->getRules()->isTwoHanded() == true)
		{
			if (_unit->getActiveHand() == AH_LEFT)
				_itRT = _itLT;

			_itLT = nullptr;
		}
		else if (_unit->getUnitStatus() != STATUS_AIMING)
			_itLT = nullptr;
	}
	else if (_itLT && _itLT->getRules()->isTwoHanded() == true)
	{
		if (_unit->getUnitStatus() != STATUS_AIMING)
			_itRT = nullptr;
	} */
}

}
