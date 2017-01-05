/*
 * Copyright 2010-2017 OpenXcom Developers.
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
 * Sets up a UnitSprite of the specified size.
 * @param width		- width in pixels
 * @param height	- height in pixels
 */
UnitSprite::UnitSprite(
		int width,
		int height)
	:
		Surface(
				width,
				height),
		_unit(nullptr),
		_itRT(nullptr),
		_itLT(nullptr),
		_unitSet(nullptr),
		_itSetRT(nullptr),
		_itSetLT(nullptr),
		_quad(0),
		_aniCycle(0),
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
 * Sets the SurfaceSets for this UnitSprite to get resources for rendering.
 * @param unitSet - pointer to the unit's SurfaceSet
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
 * Links this sprite to a BattleUnit from which to get data for rendering.
 * @param unit		- pointer to a BattleUnit
 * @param quadrant	- quadrant
 */
void UnitSprite::setBattleUnit(
		BattleUnit* const unit,
		size_t quadrant)
{
	_drawRoutine = unit->getArmor()->getDrawRoutine();

	_unit = unit;
	_quad = static_cast<int>(quadrant);

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
 * Links this sprite's right-hand to a BattleItem from which to get data for
 * rendering.
 * @param item - pointer to a BattleItem (default nullptr)
 */
void UnitSprite::setBattleItRH(const BattleItem* const item)
{
	_itRT = item;
	_redraw = true;
}

/**
 * Links this sprite's left-hand to a BattleItem from which to get data for
 * rendering.
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
		ColorGroup = 15 << 4u,
		ColorShade = 15;

	///
	static inline bool loop(
			Uint8& dest,
			const Uint8& src,
			const std::pair<Uint8, Uint8>& face_color)
	{
		if ((src & ColorGroup) == face_color.first)
		{
			dest = static_cast<Uint8>(face_color.second + (src & ColorShade));
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
		if (src != 0u)
		{
			for (size_t
					i = 0u;
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


void UnitSprite::drawRecolored(Surface* const src) // private.
{
	switch (_colorSize)
	{
		case 0:
			src->blit(this);
			break;

		default:
			lock();
			ShaderDraw<ColorReplace>(
								ShaderSurface(this),
								ShaderSurface(src),
								ShaderScalar(_color),
								ShaderScalar(_colorSize));
			unlock();
	}
}

/**
 * Sets the animation-phase for the BattleUnit.
 * @param cycle - cycle of battlescape Map
 */
void UnitSprite::setSpriteCycle(int cycle)
{
	_aniCycle = cycle;
}

/**
 * Draws a BattleUnit using the drawing rules for that unit.
 * @note This function is called by Map for each unit in the viewable area of
 * the Screen.
 *
 * STATUS_STANDING,		//  0
 * STATUS_WALKING,		//  1
 * STATUS_FLYING,		//  2
 * STATUS_TURNING,		//  3
 * STATUS_AIMING,		//  4
 * STATUS_COLLAPSING,	//  5
 * STATUS_DEAD,			//  6
 * STATUS_UNCONSCIOUS,	//  7
 * STATUS_PANICKING,	//  8
 * STATUS_BERSERK,		//  9
 * STATUS_LATENT,		// 10
 * STATUS_LATENT_START	// 11
*/
void UnitSprite::draw()
{
	//Log(LOG_INFO) << "UnitSprite::draw() routine " << _drawRoutine;
	Surface::draw(); // clears the Surface. sets '_redraw' FALSE.

	static void (UnitSprite::*routines[])() // Array of drawing routines.
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
		&UnitSprite::drawRoutine0 // 10
	};

	switch (_unit->getUnitStatus())
	{
		case STATUS_STANDING:
		case STATUS_WALKING:
		case STATUS_FLYING:
		case STATUS_TURNING:
		case STATUS_AIMING:
		case STATUS_COLLAPSING:
		case STATUS_PANICKING:
		case STATUS_BERSERK:
			(this->*(routines[_drawRoutine]))(); // Call the matching routine.
	}
}

/**
 * Drawing routine for soldiers, sectoids, and non-stock civilians (all routine 0),
 * or mutons (subroutine 10).
 */
void UnitSprite::drawRoutine0() // private.
{
	// what no _redraw!

	static const int // magic static numbers
		maleTorso	( 32),
		femaleTorso	(267),
		legsStand	( 16),
		legsKneel	( 24),
		death		(264),
		legsFloat	(275),

		larmStand	(  0),
		rarmStand	(  8),
		rarm1H		(232),
		larm2H		(240),
		rarm2H		(248),
		rarmShoot	(256),

		offYKneel	(  4),

// direction:          0      1        2        3        4        5        6        7
// #firstFrame:       56     80      104      128      152      176      200      224
		legsWalk[8u] {56, 56+24, 56+24*2, 56+24*3, 56+24*4, 56+24*5, 56+24*6, 56+24*7},
// #firstFrame:       40     64       88      112      136      160      184      208
		larmWalk[8u] {40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7},
// #firstFrame:       48     72       96      120      144      168      192      216
		rarmWalk[8u] {48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7},

		yoffWalk[8u]	{ 1,  0, -1,  0,  1,  0, -1,  0}, // bobbing up and down
		yoffWalk_10[8u]	{ 1,  1,  0,  0,  1,  1,  0,  0}, // bobbing up and down (muton)
		offX[8u]		{ 8, 10,  7,  4, -9,-11, -7, -3}, // for the weapons
		offY[8u]		{-6, -3,  0,  2,  0, -4, -7, -9}, // for the weapons
		offX2[8u]		{-8,  3,  5, 12,  6, -1, -5,-13}, // for the left handed weapons
		offY2[8u]		{ 1, -4, -2,  0,  3,  3,  5,  0}, // for the left handed weapons
		offX3[8u]		{ 0,  0,  2,  2,  0,  0,  0,  0}, // for the weapons (muton)
		offY3[8u]		{-3, -3, -1, -1, -1, -3, -3, -2}, // for the weapons (muton)
		offX4[8u]		{-8,  2,  7, 14,  7, -2, -4, -8}, // for the left handed weapons
		offY4[8u]		{-3, -3, -1,  0,  3,  3,  0,  1}, // for the left handed weapons
		offX5[8u]		{-1,  1,  1,  2,  0, -1,  0,  0}, // for the weapons (muton)
		offY5[8u]		{ 1, -1, -1, -1, -1, -1, -3,  0}, // for the weapons (muton)
		offX6[8u]		{ 0,  6,  6,  12,-4, -5, -5,-13}, // for the left handed rifles
		offY6[8u]		{-4, -4, -1,  0,  5,  0,  1,  0}, // for the left handed rifles
		offX7[8u]		{ 0,  6,  8, 12,  2, -5, -5,-13}, // for the left handed rifles (muton)
		offY7[8u]		{-4, -6, -1,  0,  3,  0,  1,  0}, // for the left handed rifles (muton)

		expectedUnitHeight (22);

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsePhase() + death);
		torso->setX(OFFSET);
		drawRecolored(torso);
		return;
	}

	const int unitDir (_unit->getUnitDirection());

	int walkPhase;
	if (_unit->isStrafeBackwards() == true)
	{
		if ((walkPhase = 8 - _unit->getWalkPhase()) == 8)
			walkPhase = 0;
	}
	else
		walkPhase = _unit->getWalkPhase();

	Surface
		* rightArm,
		* leftArm,
		* legs,
		* itRT (nullptr),
		* itLT (nullptr);

	switch (_drawRoutine)
	{
		default:
		case 0:
			switch (_unit->getGender())
			{
				default:
				case GENDER_MALE:
					switch (_unit->getArmor()->getForcedTorso())
					{
						default:
						case TORSO_STANDARD:
						case TORSO_POWERSUIT:
							torso = _unitSet->getFrame(unitDir + maleTorso);
							break;

						case TORSO_FLIGHTSUIT:
							torso = _unitSet->getFrame(unitDir + femaleTorso);
					}
					break;

				case GENDER_FEMALE:
					switch (_unit->getArmor()->getForcedTorso())
					{
						default:
						case TORSO_STANDARD:
						case TORSO_FLIGHTSUIT:
							torso = _unitSet->getFrame(unitDir + femaleTorso);
							break;

						case TORSO_POWERSUIT:
							torso = _unitSet->getFrame(unitDir + maleTorso);
					}
			}
			break;

		case 10:
			torso = _unitSet->getFrame(unitDir + maleTorso);
	}

	const bool
		isKneeled (_unit->isKneeled()),
		isWalking (_unit->getUnitStatus() == STATUS_WALKING);
	int torsoHandsWeaponY;

	if (isWalking == true)
	{
		switch (_drawRoutine)
		{
			default:
			case 0:
				torsoHandsWeaponY = yoffWalk[walkPhase];
				break;

			case 10:
				torsoHandsWeaponY = yoffWalk_10[walkPhase];
		}

		torso->setY(torsoHandsWeaponY);

		rightArm = _unitSet->getFrame(walkPhase + rarmWalk[unitDir]);
		leftArm  = _unitSet->getFrame(walkPhase + larmWalk[unitDir]);
		legs     = _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
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

		rightArm = _unitSet->getFrame(unitDir + rarmStand);
		leftArm  = _unitSet->getFrame(unitDir + larmStand);
	}

	sortHandObjects();

	if (_itRT != nullptr)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& (   _itRT->getRules()->isTwoHanded() == true
				|| _itRT->getRules()->getBattleType() == BT_MELEE))
		{
			itRT = _itSetRT->getFrame((unitDir + 2) % 8 + _itRT->getRules()->getHandSprite());
			itRT->setX(offX[unitDir]);
			itRT->setY(offY[unitDir]);
		}
		else
		{
			itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());

			switch (_drawRoutine)
			{
				default:
				case 0:
					itRT->setX(0);
					itRT->setY(0);
					break;

				case 10:
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
		}

		if (   _itRT->getRules()->isTwoHanded() == true
			|| _itRT->getRules()->getBattleType() == BT_MELEE)
		{
			leftArm = _unitSet->getFrame(unitDir + larm2H);

			switch (_unit->getUnitStatus())
			{
				case STATUS_AIMING:
					rightArm = _unitSet->getFrame(unitDir + rarmShoot);
					break;

				default:
					rightArm = _unitSet->getFrame(unitDir + rarm2H);
			}
		}
		else
		{
			switch (_drawRoutine)
			{
				default:
				case 0:
					rightArm = _unitSet->getFrame(unitDir + rarm1H);
					break;

				case 10:
					rightArm = _unitSet->getFrame(unitDir + rarm2H);
			}
		}

		if (isWalking == true)
		{
			rightArm->setY(torsoHandsWeaponY);
			itRT->setY(itRT->getY() + torsoHandsWeaponY);

			if (   _itRT->getRules()->isTwoHanded() == true
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
			switch (_drawRoutine)
			{
				default:
				case 0:
					itLT->setX(offX2[unitDir]);
					itLT->setY(offY2[unitDir]);
					break;

				case 10:
					itLT->setX(offX4[unitDir]);
					itLT->setY(offY4[unitDir]);
			}
		}
		else
		{
			rightArm = _unitSet->getFrame(unitDir + rarm2H);

			itLT->setX(0);
			itLT->setY(0);
		}

		if (_unit->getUnitStatus() == STATUS_AIMING
			&& (   _itLT->getRules()->isTwoHanded() == true
				|| _itLT->getRules()->getBattleType() == BT_MELEE))
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());

			switch (_drawRoutine)
			{
				default:
				case 0:
					itLT->setX(offX6[unitDir]);
					itLT->setY(offY6[unitDir]);
					break;

				case 10:
					itLT->setX(offX7[unitDir]);
					itLT->setY(offY7[unitDir]);
			}
			rightArm = _unitSet->getFrame(unitDir + rarmShoot);
		}

		if (isWalking == true)
		{
			leftArm->setY(torsoHandsWeaponY);
			itLT->setY(itLT->getY() + torsoHandsWeaponY);

			if (   _itLT->getRules()->isTwoHanded() == true
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

	if (_unit->getUnitStatus() == STATUS_AIMING
		&& _itRT == nullptr && _itLT == nullptr) // using Universal Fist. so PUNCH!! ( this is so funny )
	{
		rightArm = _unitSet->getFrame(rarmShoot + unitDir);
	}

	torso->		setX(OFFSET);
	rightArm->	setX(OFFSET);
	leftArm->	setX(OFFSET);
	legs->		setX(OFFSET);
	if (itRT) itRT->setX(itRT->getX() + OFFSET);
	if (itLT) itLT->setX(itLT->getX() + OFFSET);

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
				&& (   (_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
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
				&& (   (_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
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
				&& (   (_itRT != nullptr && _itRT->getRules()->isTwoHanded() == true)
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
void UnitSprite::drawRoutine1() // private.
{
	// what no _redraw!

	static const int // magic static numbers
		stand (16),
		walk  (24),
		death (64),

		rarm      ( 0),
		larm      ( 8),
		rarm2H    (75),
		larm2H    (67),
		rarmShoot (83),
		rarm1H    (91), // note that arms are switched vs "normal" sheets

		yoffWalk[8u]	{ 0,  0,  0,  0,  0,  0,  0,  0 }, // bobbing up and down
		offX[8u]		{ 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8u]		{-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8u]		{-8,  3,  7, 13,  6, -3, -5,-13 }, // for the weapons
		offY2[8u]		{ 1, -4, -1,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8u]		{ 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8u]		{-4, -4, -1,  0,  5,  0,  1,  0 }; // for the left handed rifles

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsePhase() + death);
		torso->setX(OFFSET);
		drawRecolored(torso);
		return;
	}

	const int
		unitDir		(_unit->getUnitDirection()),
		walkPhase	(_unit->getWalkPhase());

	Surface
		* rightArm	(_unitSet->getFrame(unitDir + rarm)),
		* leftArm	(_unitSet->getFrame(unitDir + larm)),
		* itRT		(nullptr),
		* itLT		(nullptr);

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
		case STATUS_FLYING:
			torso = _unitSet->getFrame(
									  unitDir * 5
									+ static_cast<int>(
									  static_cast<float>(walkPhase) / 1.6f)
									+ walk);
			torso->setY(yoffWalk[walkPhase]);
			break;

		default:
			torso = _unitSet->getFrame(unitDir + stand);
	}

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

			switch (_unit->getUnitStatus())
			{
				case STATUS_AIMING:
					rightArm = _unitSet->getFrame(unitDir + rarmShoot);
					break;

				default:
					rightArm = _unitSet->getFrame(unitDir + rarm2H);
			}
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

		switch (_unit->getUnitStatus())
		{
			case STATUS_WALKING:
			case STATUS_FLYING:
				leftArm->setY(yoffWalk[walkPhase]);
				itLT->setY(itLT->getY() + yoffWalk[walkPhase]);

				if (_itLT->getRules()->isTwoHanded() == true)
					rightArm->setY(yoffWalk[walkPhase]);
		}
	}

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
		case STATUS_FLYING:
			break;

		default:
			torso->		setY(0);
			rightArm->	setY(0);
			leftArm->	setY(0);
	}

	torso->		setX(OFFSET);
	rightArm->	setX(OFFSET);
	leftArm->	setX(OFFSET);
	if (itRT) itRT->setX(itRT->getX() + OFFSET);
	if (itLT) itLT->setX(itLT->getX() + OFFSET);

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
void UnitSprite::drawRoutine2() // private.
{
	// what no _redraw!

	static const int walk (104); // magic static number

	Surface* quad;

	const TurretType turret (_unit->getTurretType());
	const int hover ((_unit->getMoveTypeUnit() == MT_FLY) ? 32 : 0);

	if (_quad != 0 && hover != 0)
	{
		quad = _unitSet->getFrame(((_quad - 1) << 3u) + _aniCycle + walk);
		quad->setX(OFFSET);
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

	quad = _unitSet->getFrame(hover + (_quad << 3u) + unitDir);
	quad->setX(OFFSET);
	drawRecolored(quad);

//	if (_quad == 3 && turret != TRT_NONE)
//	{
//		quad = _unitSet->getFrame(64 + (static_cast<int>(turret) * 8) + _unit->getTurretDirection());
//		int
//			turretOffsetX = 0,
//			turretOffsetY = -4;
//		if (hover != 0)
//		{
//			turretOffsetX += offX[unitDir];
//			turretOffsetY += offY[unitDir];
//		}
//
//		quad->setX(turretOffsetX);
//		quad->setY(turretOffsetY);
//		drawRecolored(quad);
//	}
	if (turret != TRT_NONE)
	{
		quad = _unitSet->getFrame(64 + (static_cast<int>(turret) << 3u) + _unit->getTurretDirection());
		int
			turretOffsetX,
			turretOffsetY;

		switch (_quad)
		{
			default:
			case 0:
				turretOffsetX =  0,
				turretOffsetY = 12;
				break;

			case 1:
				turretOffsetX = -16,
				turretOffsetY =   4;
				break;

			case 2:
				turretOffsetX = 16,
				turretOffsetY =  4;
				break;

			case 3:
				turretOffsetX =  0,
				turretOffsetY = -4;
		}

		if (hover != 0)
		{
			static const int
				offX[8u] {-2,-7,-5, 0, 5, 7, 2, 0},
				offY[8u] {-1,-3,-4,-5,-4,-3,-1,-1};

			turretOffsetX += offX[unitDir];
			turretOffsetY += offY[unitDir];
		}

		quad->setX(turretOffsetX + OFFSET);
		quad->setY(turretOffsetY);
		drawRecolored(quad);
	}
}

/**
 * Drawing routine for cyberdiscs.
 */
void UnitSprite::drawRoutine3() // private.
{
	// what no _redraw!

	static const int walk (32); // magic static number

	Surface* quad;

	if (_quad != 0)
	{
		quad = _unitSet->getFrame(((_quad - 1) << 3u) + _aniCycle + walk);
		quad->setX(OFFSET);
		drawRecolored(quad);
	}

	quad = _unitSet->getFrame((_quad << 3u) + _unit->getUnitDirection());
	quad->setX(OFFSET);
	drawRecolored(quad);
}

/**
 * Drawing routine for stock civilians, ethereals, zombies, dogs, cybermites,
 * and scout-drones.
 */
void UnitSprite::drawRoutine4() // private.
{
	// what no _redraw!

	static const int // magic static numbers
		stand ( 0),
		walk  ( 8),
		death (72),

		offX[8u]	{ 8, 10,  7,  4, -9,-11, -7, -3 }, // for the weapons
		offY[8u]	{-6, -3,  0,  2,  0, -4, -7, -9 }, // for the weapons
		offX2[8u]	{-8,  3,  5, 12,  6, -1, -5,-13 }, // for the weapons
		offY2[8u]	{ 1, -4, -2,  0,  3,  3,  5,  0 }, // for the weapons
		offX3[8u]	{ 0,  6,  6, 12, -4, -5, -5,-13 }, // for the left handed rifles
		offY3[8u]	{-4, -4, -1,  0,  5,  0,  1,  0 }; // for the left handed rifles

	Surface* sprite;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		sprite = _unitSet->getFrame(_unit->getCollapsePhase() + death);
		sprite->setX(OFFSET);
		drawRecolored(sprite);
		return;
	}

	Surface
		* itRT (nullptr),
		* itLT (nullptr);

	const int unitDir (_unit->getUnitDirection());

	if (_unit->getUnitStatus() == STATUS_WALKING
		&& _unit->getRaceString() != "STR_ETHEREAL") // stop the ethereal shuffle.
	{
		sprite = _unitSet->getFrame((unitDir << 3u) + _unit->getWalkPhase() + walk);
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
			itRT = _itSetRT->getFrame(unitDir + _itRT->getRules()->getHandSprite());

			switch (_itRT->getInventorySection()->getSectionType())
			{
				case ST_RIGHTHAND: // wtf. how could right-item not be in right-hand: Dual-wield sorting hand-obs.
					itRT->setX(0);
					itRT->setY(0);
					break;

				default:
					itRT->setX(offX2[unitDir]);
					itRT->setY(offY2[unitDir]);
			}
		}
	}

	if (_itLT != nullptr && _itLT->getRules()->isFixed() == false)
	{
		if (_unit->getUnitStatus() == STATUS_AIMING
			&& _itLT->getRules()->isTwoHanded() == true)
		{
			itLT = _itSetLT->getFrame((unitDir + 2) % 8 + _itLT->getRules()->getHandSprite());
			itLT->setX(offX3[unitDir]);
			itLT->setY(offY3[unitDir]);
		}
		else
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
		}
	}

	sprite->setX(OFFSET);
	if (itRT) itRT->setX(itRT->getX() + OFFSET);
	if (itLT) itLT->setX(itLT->getX() + OFFSET);

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
void UnitSprite::drawRoutine5() // private.
{
	// what no _redraw!

	static const int walk (32); // magic static number

	Surface* quad;

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
			quad = _unitSet->getFrame((_unit->getUnitDirection() << 4u)
									+ (_quad << 2u)
									+ (_unit->getWalkPhase() >> 1u) % 4
									+ walk);
			break;

		default:
			quad = _unitSet->getFrame(_unit->getUnitDirection() + (_quad << 3u));
	}

	quad->setX(OFFSET);
	drawRecolored(quad);
}

/**
 * Drawing routine for snakemans.
 */
void UnitSprite::drawRoutine6() // private.
{
	// what no _redraw!

	static const int // magic static numbers
		body		(24),
		legsStand	(16),
		death		(96),

		larmStand	(  0),
		rarmStand	(  8),
		rarm1H		( 99),
		larm2H		(107),
		rarm2H		(115),
		rarmShoot	(123),

		legsWalk[8u]	{32, 40, 48, 56, 64, 72, 80, 88},
		yoffWalk[8u]	{ 3,  3,  2,  1,  0,  0,  1,  2}, // bobbing up and down
		xoffWalkA[8u]	{ 0,  0,  1,  2,  3,  3,  2,  1},
		xoffWalkB[8u]	{ 0,  0, -1, -2, -3, -3, -2, -1},
		yoffStand[8u]	{ 2,  1,  1,  0,  0,  0,  0,  0},
		offX[8u]		{ 8, 10,  5,  2, -8,-10, -5, -2}, // for the weapons
		offY[8u]		{-6, -3,  0,  0,  2, -3, -7, -9}, // for the weapons
		offX2[8u]		{-8,  2,  7, 13,  7,  0, -3,-15}, // for the weapons
		offY2[8u]		{ 1, -4, -2,  0,  3,  3,  5,  0}, // for the weapons
		offX3[8u]		{ 0,  6,  6, 12, -4, -5, -5,-13}, // for the left handed rifles
		offY3[8u]		{-4, -4, -1,  0,  5,  0,  1,  0}; // for the left handed rifles

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsePhase() + death);
		torso->setX(OFFSET);
		drawRecolored(torso);
		return;
	}

	const int
		unitDir		(_unit->getUnitDirection()),
		walkPhase	(_unit->getWalkPhase());

	Surface
		* legs,
		* rightArm	(_unitSet->getFrame(unitDir + rarmStand)),
		* leftArm	(_unitSet->getFrame(unitDir + larmStand)),
		* itRT		(nullptr),
		* itLT		(nullptr);

	torso = _unitSet->getFrame(unitDir + body);

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
		{
			int xoffWalk;
			if (unitDir < 3)
				xoffWalk = xoffWalkA[walkPhase];
			if (unitDir < 7 && unitDir > 3)
				xoffWalk = xoffWalkB[walkPhase];
			else
				xoffWalk = 0;

			torso->		setX(xoffWalk);
			torso->		setY(yoffWalk[walkPhase]);
			rightArm->	setX(xoffWalk);
			rightArm->	setY(yoffWalk[walkPhase]);
			leftArm->	setX(xoffWalk);
			leftArm->	setY(yoffWalk[walkPhase]);

			legs = _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
			break;
		}

		default:
			legs = _unitSet->getFrame(unitDir + legsStand);
	}

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

			if (_itRT->getRules()->isTwoHanded() == false)
				itRT->setY(yoffStand[unitDir]);
			else
				itRT->setY(0);
		}

		if (_itRT->getRules()->isTwoHanded() == true)
		{
			leftArm = _unitSet->getFrame(unitDir + larm2H);

			switch (_unit->getUnitStatus())
			{
				case STATUS_AIMING:
					rightArm = _unitSet->getFrame(unitDir + rarmShoot);
					break;

				default:
					rightArm = _unitSet->getFrame(unitDir + rarm2H);
			}
		}
		else
			rightArm = _unitSet->getFrame(unitDir + rarm1H);

		if (_unit->getUnitStatus() == STATUS_WALKING)
		{
			rightArm->setY(yoffWalk[walkPhase]);
			itRT->setY(yoffWalk[walkPhase]);

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

	if (_unit->getUnitStatus() == STATUS_AIMING
		&& _itRT == nullptr && _itLT == nullptr) // using Universal Fist. so PUNCH!! ( this is so funny )
	{
		rightArm = _unitSet->getFrame(unitDir + rarmShoot);
	}

	torso->		setX(OFFSET);
	rightArm->	setX(OFFSET);
	leftArm->	setX(OFFSET);
	legs->		setX(OFFSET);
	if (itRT) itRT->setX(itRT->getX() + OFFSET);
	if (itLT) itLT->setX(itLT->getX() + OFFSET);

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
void UnitSprite::drawRoutine7() // private.
{
	// what no _redraw!

	static const int // magic static numbers
		body		( 24),
		legsStand	( 16),
		death		(224),

		larmStand	(0),
		rarmStand	(8),

		legsWalk[8u] { 48, 48+24, 48+24*2, 48+24*3, 48+24*4, 48+24*5, 48+24*6, 48+24*7 },
		larmWalk[8u] { 32, 32+24, 32+24*2, 32+24*3, 32+24*4, 32+24*5, 32+24*6, 32+24*7 },
		rarmWalk[8u] { 40, 40+24, 40+24*2, 40+24*3, 40+24*4, 40+24*5, 40+24*6, 40+24*7 },
		yoffWalk[8u] {  1,     0,      -1,       0,       1,       0,      -1,       0 }; // bobbing up and down

	Surface* torso;

	if (_unit->getUnitStatus() == STATUS_COLLAPSING)
	{
		torso = _unitSet->getFrame(_unit->getCollapsePhase() + death);
		torso->setX(OFFSET);
		drawRecolored(torso);
		return;
	}

	const int unitDir (_unit->getUnitDirection());

	torso = _unitSet->getFrame(unitDir + body);

	Surface
		* rightArm,
		* leftArm,
		* legs;

	switch (_unit->getUnitStatus())
	{
		case STATUS_WALKING:
		{
			const int walkPhase	(_unit->getWalkPhase());
			torso->setY(yoffWalk[walkPhase]);
			rightArm	= _unitSet->getFrame(walkPhase + rarmWalk[unitDir]);
			leftArm		= _unitSet->getFrame(walkPhase + larmWalk[unitDir]);
			legs		= _unitSet->getFrame(walkPhase + legsWalk[unitDir]);
			break;
		}

		default:
			torso->setY(0);
			rightArm	= _unitSet->getFrame(unitDir + rarmStand);
			leftArm		= _unitSet->getFrame(unitDir + larmStand);
			legs		= _unitSet->getFrame(unitDir + legsStand);
	}

	torso->		setX(OFFSET);
	legs->		setX(OFFSET);
	leftArm->	setX(OFFSET);
	rightArm->	setX(OFFSET);

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
void UnitSprite::drawRoutine8() // private.
{
	_redraw = true;

	static const int // magic static numbers
		body  (0),
		aim   (5),
		death (6),

		pulsate[8u] {0,1,2,3,4,3,2,1};

	Surface* sprite;

	switch (_unit->getUnitStatus())
	{
		case STATUS_COLLAPSING:
			sprite = _unitSet->getFrame(_unit->getCollapsePhase() + death);
			break;

		case STATUS_AIMING:
			sprite = _unitSet->getFrame(aim);
			break;

		default:
			sprite = _unitSet->getFrame(body + pulsate[_aniCycle]);
	}

	sprite->setX(OFFSET);
	drawRecolored(sprite);
}

/**
 * Drawing routine for celatids.
 */
void UnitSprite::drawRoutine9() // private.
{
	_redraw = true;

	static const int // magic static numbers
		body  ( 0),
		death (25),
		shoot ( 8); // frames 8..23 or ..24 (24 is merely a green ball sprite)

	Surface* sprite;

	switch (_unit->getUnitStatus())
	{
		case STATUS_COLLAPSING:
			sprite = _unitSet->getFrame(_unit->getCollapsePhase() + death);
			break;

		case STATUS_AIMING:
		{
			const int
				framesTotal (_unit->getArmor()->getShootFrames()),
				phase (_unit->getAimingPhase());

			int extra;
			if (phase == framesTotal)
				extra = 2; // bounce back one frame at the end.
			else
				extra = 0;

			sprite = _unitSet->getFrame(shoot + std::min(phase - extra,
														 framesTotal - 1));
			// Clamp that because slow (read: long) think()->draw intervals
			// cause it to exceed the upper bound of total shootFrames.
			_unit->setAimingPhase(phase + 1);
			// -> let BattleUnit::keepAiming() iterate the final aimPhase. nix
			// that; a super-slow animation speed won't even let keepAiming()
			// get called. ... not sure how the animation is ended in that case
			// but something does it.
			break;
		}

		default:
			sprite = _unitSet->getFrame(body + _aniCycle);
	}

	sprite->setX(OFFSET);
	drawRecolored(sprite);
}

/**
 * Determines which hand-objects to display.
 */
void UnitSprite::sortHandObjects() // private.
{
	if (_itRT != nullptr && _itLT != nullptr) // this is the draw active-hand code ->
	{
		switch (_unit->deterActiveHand())
		{
			default:
			case AH_RIGHT:
				_itLT = nullptr;
				break;

			case AH_LEFT:
				if (_itLT->getRules()->isTwoHanded() == true)
				{
					_itRT = _itLT;
					_itLT = nullptr;
				}
				else
					_itRT = nullptr;
		}
	}
//	if (_itRT && _itRT->getRules()->isTwoHanded() == true) // this is the draw dual-wield code ->
//	{
//		if (_itLT && _itLT->getRules()->isTwoHanded() == true)
//		{
//			if (_unit->deterActiveHand() == AH_LEFT)
//				_itRT = _itLT;
//
//			_itLT = nullptr;
//		}
//		else if (_unit->getUnitStatus() != STATUS_AIMING)
//			_itLT = nullptr;
//	}
//	else if (_itLT && _itLT->getRules()->isTwoHanded() == true)
//	{
//		if (_unit->getUnitStatus() != STATUS_AIMING)
//			_itRT = nullptr;
//	}
}

}
