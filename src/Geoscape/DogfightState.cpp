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

#include "DogfightState.h"

//#include <cmath>
//#include <cstdlib>
//#include <sstream>

#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/ImageButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCountry.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"

#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/CraftWeapon.h"
#include "../Savegame/CraftWeaponProjectile.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

// UFO blobs graphics ... MOVED TO GEOSCAPESTATE.

// Projectile blobs
const int DogfightState::_projectileBlobs[4u][6u][3u]
{
	{
		{0, 1, 0}, // 0 - STR_STINGRAY_MISSILE
		{1, 9, 1},
		{1, 4, 1},
		{0, 3, 0},
		{0, 2, 0},
		{0, 1, 0}
	},
	{
		{1, 2, 1}, // 1 - STR_AVALANCHE_MISSILE
		{2, 9, 2},
		{2, 5, 2},
		{1, 3, 1},
		{0, 2, 0},
		{0, 1, 0}
	},
	{
		{0, 0, 0}, // 2 - STR_CANNON_ROUND
		{0, 7, 0},
		{0, 2, 0},
		{0, 1, 0},
		{0, 0, 0},
		{0, 0, 0}
	},
	{
		{2, 4, 2}, // 3 - STR_FUSION_BALL
		{4, 9, 4},
		{2, 4, 2},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}
	}
};


/**
 * Initializes all the elements in the Dogfight window.
 * @param globe		- pointer to the Globe
 * @param craft		- pointer to the Craft intercepting
 * @param ufo		- pointer to the UFO getting intercepted
 * @param geoState	- pointer to GeoscapeState
 */
DogfightState::DogfightState(
		Globe* const globe,
		Craft* const craft,
		Ufo* const ufo,
		GeoscapeState* const geoState)
	:
		_globe(globe),
		_craft(craft),
		_ufo(ufo),
		_geoState(geoState),
		_playSave(_game->getSavedGame()),
		_diff(_game->getSavedGame()->getDifficultyInt()),
		_textPersistence(STAT_PERSIST),
		_dist(DIST_ENGAGE),
		_desired(DIST_STANDOFF),
		_destroyCraft(false),
		_destroyUfo(false),
		_ufoBreakingOff(false),
		_w1Enabled(true),
		_w2Enabled(true),
		_reduced(false),
		_finish(false),
		_finishRequest(false),
		_cautionLevel(CAUTION_HIGH),
		_ufoSize(static_cast<int>(ufo->getRules()->getRadius())),
		_craftHeight(0),
		_craftHeight_pre(0),
		_refreshCraft(0),
		_slot(0u),
		_slotsTotal(0u),
		_x(0),
		_y(0),
		_restoreIconX(5), // x-offset won't change
		_restoreIconY(0), // y-offset will change
		_w1FireCountdown(0),
		_w2FireCountdown(0),
		_w1FireInterval(0),
		_w2FireInterval(0)
{
	_fullScreen = false;

	_craft->inDogfight(true);

	_window					= new Surface(160, 96, _x, _y);

	_battleScope			= new Surface(77, 74, _x +  3, _y +  3);
	_srfHull				= new Surface(22, 25, _x + 93, _y + 40);
	_srfCwRange1			= new Surface(21, 74, _x + 19, _y +  3);
	_srfCwRange2			= new Surface(21, 74, _x + 43, _y +  3);
	_isfCw1					= new InteractiveSurface(15, 17, _x +  4, _y + 52);
	_isfCw2					= new InteractiveSurface(15, 17, _x + 64, _y + 52);

	_btnReduce				= new InteractiveSurface( 12, 12, _x, _y);
	_previewUfo				= new InteractiveSurface(160, 96, _x, _y);

	_btnDisengage			= new ImageButton(36, 15, _x + 83, _y +  4);
	_btnUfo					= new ImageButton(36, 15, _x + 83, _y + 20);

	_btnCautious			= new ImageButton(36, 15, _x + 120, _y +  4);
	_btnStandard			= new ImageButton(36, 15, _x + 120, _y + 20);
	_btnAggressive			= new ImageButton(36, 15, _x + 120, _y + 36);
	_btnStandoff			= new ImageButton(36, 17, _x + 120, _y + 52);
	_craftStance = _btnStandoff;

	_srfTexIcon				= new Surface(9, 9, _x + 147, _y + 72);

	_txtLoad1				= new Text( 16, 9, _x +   4, _y + 70);
	_txtLoad2				= new Text( 16, 9, _x +  64, _y + 70);
	_txtDistance			= new Text( 40, 9, _x + 116, _y + 72);
	_txtStatus				= new Text(150, 9, _x +   4, _y + 85);
	_txtTitle				= new Text(160, 9, _x,       _y -  9);

	_btnRestoreIcon			= new InteractiveSurface(32, 20);	// NOTE: x & y offset are set in placePort().
	_txtRestoreIcon			= new Text(150, 9);					// ditto.

	setInterface("dogfight");

	add(_window);
	add(_battleScope);
	add(_isfCw1);
	add(_srfCwRange1);
	add(_isfCw2);
	add(_srfCwRange2);
	add(_srfHull);
	add(_btnReduce);
	add(_btnDisengage,		"disengageButton",	"dogfight", _window);
	add(_btnUfo,			"ufoButton",		"dogfight", _window);
	add(_btnAggressive,		"aggressiveButton",	"dogfight", _window);
	add(_btnStandard,		"standardButton",	"dogfight", _window);
	add(_btnCautious,		"cautiousButton",	"dogfight", _window);
	add(_btnStandoff,		"standoffButton",	"dogfight", _window);
	add(_srfTexIcon);
	add(_txtLoad1,			"numbers",			"dogfight", _window);
	add(_txtLoad2,			"numbers",			"dogfight", _window);
	add(_txtDistance,		"distance",			"dogfight", _window);
	add(_previewUfo);
	add(_txtStatus,			"text",				"dogfight", _window);
	add(_txtTitle,			"ufoButton",		"dogfight", _window);

	add(_btnRestoreIcon);
	add(_txtRestoreIcon,	"iconText",			"dogfight");

	_btnStandoff  ->invalidate(false);
	_btnCautious  ->invalidate(false);
	_btnStandard  ->invalidate(false);
	_btnAggressive->invalidate(false);
	_btnDisengage ->invalidate(false);
	_btnUfo       ->invalidate(false);

/*	Surface* graphic;
	graphic = _game->getResourcePack()->getSurface("INTERWIN.DAT");
	graphic->setX(0);
	graphic->setY(0);
	graphic->getCrop()->x = 0;
	graphic->getCrop()->y = 0;
	graphic->getCrop()->w = 160;
	graphic->getCrop()->h = 96;
	_window->drawRect(graphic->getCrop(), 15);
	graphic->blit(_window);

	_previewUfo->drawRect(graphic->getCrop(), 15);
	graphic->getCrop()->y = 96;
	graphic->getCrop()->h = 15;
	graphic->blit(_previewUfo);

	graphic->setY(67);
	graphic->getCrop()->y = 111;
	graphic->getCrop()->h = 29;
	graphic->blit(_previewUfo);

	if (ufo->getRules()->getSpriteString().empty())
	{
		graphic->setY(15);
		graphic->getCrop()->y = 140 + 52 * _ufo->getRules()->getSprite();
		graphic->getCrop()->h = 52;
	}
	else
	{
		graphic = _game->getResourcePack()->getSurface(ufo->getRules()->getSpriteString());
		graphic->setX(0);
		graphic->setY(15);
	}
	graphic->blit(_previewUfo); */

	Surface* srf (_game->getResourcePack()->getSurface("INTERWIN"));
	if (srf != nullptr)
		srf->blit(_window);

	if ((srf = _game->getResourcePack()->getSurface("INTERWIN_")) != nullptr)
		srf->blit(_previewUfo);

	std::ostringstream sprite;
	sprite << "INTERWIN_" << _ufo->getRules()->getSprite();
	if ((srf = _game->getResourcePack()->getSurface(sprite.str())) != nullptr)
	{
		srf->setY(15);
		srf->blit(_previewUfo);
	}
	_previewUfo->setVisible(false);
	_previewUfo->onMouseClick(	static_cast<ActionHandler>(&DogfightState::previewClick),
								0u);

	_btnReduce->onMouseClick(static_cast<ActionHandler>(&DogfightState::btnReduceToIconClick));

	_btnUfo->copy(_window);
	_btnUfo->onMouseClick(	static_cast<ActionHandler>(&DogfightState::btnUfoClick),
							0u);

	_btnDisengage->copy(_window);
	_btnDisengage->setGroup(&_craftStance);
	_btnDisengage->onMouseClick(static_cast<ActionHandler>(&DogfightState::btnDisengageClick)); // TODO: Key-presses.

	_btnCautious->copy(_window);
	_btnCautious->setGroup(&_craftStance);
	_btnCautious->onMouseClick(static_cast<ActionHandler>(&DogfightState::btnCautiousClick));

	_btnStandard->copy(_window);
	_btnStandard->setGroup(&_craftStance);
	_btnStandard->onMouseClick(static_cast<ActionHandler>(&DogfightState::btnStandardClick));

	_btnAggressive->copy(_window);
	_btnAggressive->setGroup(&_craftStance);
	_btnAggressive->onMouseClick(static_cast<ActionHandler>(&DogfightState::btnAggressiveClick));

	_btnStandoff->copy(_window);
	_btnStandoff->setGroup(&_craftStance);
	_btnStandoff->onMouseClick(		static_cast<ActionHandler>(&DogfightState::btnStandoffClick));
	_btnStandoff->onKeyboardPress(	static_cast<ActionHandler>(&DogfightState::keyEscape),
									Options::keyCancel);

	if ((srf = _game->getResourcePack()->getSurface(getTextureIcon())) != nullptr)
		srf->blit(_srfTexIcon);
	else Log(LOG_INFO) << "ERROR: no texture icon for dogfight";

	_txtDistance->setText(Text::intWide(DIST_ENGAGE));

	_txtStatus->setAlign(ALIGN_CENTER);
	_txtStatus->setText(tr("STR_STANDOFF"));

	SurfaceSet* const srtInticon (_game->getResourcePack()->getSurfaceSet("INTICON.PCK"));

	// Create the minimized Dogfight icon.
	srf = srtInticon->getFrame(_craft->getRules()->getSprite());
//	srf->setX(0);
//	srf->setY(0);
	srf->blit(_btnRestoreIcon);
	_btnRestoreIcon->onMousePress(		static_cast<ActionHandler>(&DogfightState::btnShowPortPress));
	_btnRestoreIcon->onKeyboardPress(	static_cast<ActionHandler>(&DogfightState::btnShowPortPress),
										Options::keyOk);		// used to Maximize all minimized interceptor icons.
	_btnRestoreIcon->onKeyboardPress(	static_cast<ActionHandler>(&DogfightState::btnShowPortPress),
										Options::keyOkKeypad);	// used to Maximize all minimized interceptor icons.
	_btnRestoreIcon->setVisible(false);

	std::wostringstream woststr;
	woststr << _craft->getLabel(_game->getLanguage()) << L" >" << _craft->getBase()->getLabel();
	_txtTitle->setText(woststr.str());

	_txtRestoreIcon->setText(woststr.str());
	_txtRestoreIcon->setVisible(false);

	// Define the colors to be used. Note these have been further tweaked in Interfaces.rul
	const RuleInterface* const dfInterface (_game->getRuleset()->getInterface("dogfight"));

	_colors[CRAFT_MIN]       = static_cast<Uint8>(dfInterface->getElement("craftRange")->color);		// 160 (10)+ 0 slate gray
	_colors[CRAFT_MAX]       = static_cast<Uint8>(dfInterface->getElement("craftRange")->color2);		// 176 (11)+ 0 purple
	_colors[RADAR_MIN]       = static_cast<Uint8>(dfInterface->getElement("radarRange")->color);		// 112  (7)+ 0 green
	_colors[RADAR_MAX]       = static_cast<Uint8>(dfInterface->getElement("radarRange")->color2);		// 128  (8)+ 0 pure red
	_colors[DAMAGE_RED]      = static_cast<Uint8>(dfInterface->getElement("damageRange")->color);		//  12  (0)+12 yellow (not) do -> (0)+13 brightred
	_colors[DAMAGE_YEL]      = static_cast<Uint8>(dfInterface->getElement("damageRange")->color2);		//  14  (0)+14 red    (not) do -> (0)+12 darkyellow
	_colors[BLOB_MIN]        = static_cast<Uint8>(dfInterface->getElement("radarDetail")->color);		// 108  (6)+12 light green
	_colors[BLOB_MAX]        = 127u;																	//      (7)+15 dark olive green
	_colors[RANGE_METER]     = static_cast<Uint8>(dfInterface->getElement("radarDetail")->color2);		// 111  (6)+15 green
	_colors[DISABLED_WEAPON] = static_cast<Uint8>(dfInterface->getElement("disabledWeapon")->color);	//  24
	_colors[DISABLED_RANGE]  = static_cast<Uint8>(dfInterface->getElement("disabledWeapon")->color2);	//   7
	_colors[DISABLED_AMMO]   = static_cast<Uint8>(dfInterface->getElement("disabledAmmo")->color);		//  24
	_colors[UFO_BEAM]        = 128u;																	//      (8)+ 0 pure red


	const CraftWeapon* cw;
	InteractiveSurface* cwSprite;
	Surface* cwRange;
	Text* cwLoad;
	int
		x1,x2,
		rangeY, // 1 km = 1 pixel
		connectY (57),
		minY,
		maxY;

	const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
	for (size_t
			i = 0u;
			i != hardpoints;
			++i)
	{
		switch (i)
		{
			default:
			case 0u:
				cwSprite = _isfCw1;
				cwRange  = _srfCwRange1;
				cwLoad   = _txtLoad1;
				break;
			case 1u:
				cwSprite = _isfCw2;
				cwRange  = _srfCwRange2;
				cwLoad   = _txtLoad2;
		}

		if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr)
		{
			switch (i)
			{
				default:
				case 0u:
					_w1FireInterval = _craft->getCraftWeapons()->at(i)->getRules()->getStandardReload();
					cwSprite->onMouseClick(static_cast<ActionHandler>(&DogfightState::weapon1Click));
					x1 = 2; x2 = 0;
					break;
				case 1u:
					_w2FireInterval = _craft->getCraftWeapons()->at(i)->getRules()->getStandardReload();
					cwSprite->onMouseClick(static_cast<ActionHandler>(&DogfightState::weapon2Click));
					x1 = 0; x2 = 18;
			}

			srf = srtInticon->getFrame(cw->getRules()->getSprite() + 5);
//			srf->setX(0);
//			srf->setY(0);
			srf->blit(cwSprite);

			woststr.str(L"");
			woststr << cw->getCwLoad();
			cwLoad->setText(woststr.str());

			cwRange->lock();
			rangeY = cwRange->getHeight() - cw->getRules()->getRange();

			for (int
					x = x1;
					x < x1 + 19;
					x += 2)
			{
				cwRange->setPixelColor(
									x, rangeY,
									_colors[RANGE_METER]);
			}

			minY =
			maxY = 0;

			if (rangeY < connectY)
			{
				minY = rangeY;
				maxY = connectY;
			}
			else if (rangeY > connectY)
			{
				minY = connectY;
				maxY = rangeY;
			}

			for (int
					y = minY;
					y != maxY + 1;
					++y)
			{
				cwRange->setPixelColor(
									x1 + x2, y,
									_colors[RANGE_METER]);
			}

			for (int
					x = x2;
					x != x2 + 3;
					++x)
			{
				cwRange->setPixelColor(
									x, connectY,
									_colors[RANGE_METER]);
			}
			cwRange->unlock();

			switch (i) // disable weapons that were disabled on a prior dogfight
			{
				default:
				case 0u:
					if (_craft->getWeaponDisabled(1))
						weapon1Click(nullptr);
					break;
				case 1u:
					if (_craft->getWeaponDisabled(2))
						weapon2Click(nullptr);
					break;
			}
		}
		else
		{
			cwSprite->setVisible(false);
			cwRange ->setVisible(false);
			cwLoad  ->setVisible(false);
		}
	}

	srf = srtInticon->getFrame(_craft->getRules()->getSprite() + 11);
	srf->blit(_srfHull);

	if (_ufo->getEscapeCountdown() == 0) // UFO is *not* engaged already in a different dogfight/Intercept slot.
	{
		_ufo->setFireCountdown(0); // UFO is ready to Fire pronto.

		int escape (_ufo->getRules()->getEscape());
		escape += RNG::generate(0, escape);
		escape /= _diff + 1; // escape -= _diff * 30;
		if (escape < 1) escape = 1;
		_ufo->setEscapeCountdown(escape);
	}


	Uint8 testColor;
	bool isCraftColor;

	for (int
			y = 0;
			y != _srfHull->getHeight();
			++y)
	{
		testColor = _srfHull->getPixelColor(11, y); // examine Craft down the center of its length
		isCraftColor = testColor >= _colors[CRAFT_MIN]
					&& testColor <  _colors[CRAFT_MAX];

		if (_craftHeight != 0 && isCraftColor == false)
			break;

		if (isCraftColor == true)
			++_craftHeight;
		else
			++_craftHeight_pre;
	}

	drawCraft(true);
}

/**
 * Cleans up this DogfightState.
 */
DogfightState::~DogfightState()
{
	_geoState->resetTimer();

	while (_projectiles.empty() == false)
	{
		delete _projectiles.back();
		_projectiles.pop_back();
	}

	if (_craft != nullptr)
		_craft->inDogfight(false);
}

/**
 * Runs the higher level dogfight functionality.
 * @note This is the handler for GeoscapeState::thinkDogfights() '_tmrDogfight'.
 */
void DogfightState::think()
{
	if (_reduced == true				// short-circuit
		&& (_craft->isLowFuel() == true	// these can happen only when reduced to Icon ->
			|| dynamic_cast<Ufo*>(_craft->getTarget()) != _ufo
			|| _ufo->getUfoStatus() == Ufo::LANDED))
	{
		_finish = true; // think() won't be called again.
	}
	else
		advanceDogfight();

	// NOTE: The final zoom-out is handled by GeoscapeState::thinkDogfights()
	// but only if '_dogfights' is empty. Zoom-out/in for reducing/restoring
	// dogfights is done in btnReduceToIconClick() and btnShowPortPress().
	// The following zoom-out handles the case of 1 interceptor disengaging
	// from a multi-intercepted dogfight (iff the others are currently reduced).
	if (_textPersistence == 0
		&& _projectiles.empty() == true
		&& (_dist > DIST_ENGAGE
			|| _ufo->isCrashed() == true
			|| _craft->isDestroyed() == true))
	{
		_finish = true; // think() won't be called again.

		if (_slotsTotal > 1u											// if (_totalIntercepts == 1u) let GeoscapeState handle it.
			&& _geoState->getQtyReducedDogfights() == _slotsTotal - 1u)	// if this is the only dogfight with a port that's displayed
		{
			_geoState->toggleDogfight(false);
		}
	}
}

/**
 * Animates the view-port via palette cycling.
 * @note This is called by advanceDogfight().
 */
void DogfightState::aniPort()
{
	for (int // Animate radar waves and other stuff.
			x = 0;
			x != _window->getWidth();
			++x)
	{
		for (int
				y = 0;
				y != _window->getHeight();
				++y)
		{
			Uint8 color (_window->getPixelColor(x,y));
			if (   color >= _colors[RADAR_MIN]
				&& color <  _colors[RADAR_MAX])
			{
				if (++color >= _colors[RADAR_MAX])
					color    = _colors[RADAR_MIN];

				_window->setPixelColor(x,y, color);
			}
		}
	}

	_battleScope->clear();

	if (_ufo->isDestroyed() == false)
		drawUfo();

	for (std::vector<CraftWeaponProjectile*>::const_iterator
			i = _projectiles.begin();
			i != _projectiles.end();
			++i)
	{
		drawProjectile(*i);
	}

	if (_textPersistence == 0) // clears status-text after a while
		_txtStatus->setText(L"");
	else
		--_textPersistence;


	bool last (false);
	int hit (_ufo->getHitStep()); // animate UFO hit
	if (hit != 0)
	{
		_ufo->setHitStep(--hit);
		if (hit == 0)
		{
			last = true;
		}
	}

	if (_ufo->isCrashed() == true // animate UFO crash landing
		&& _ufo->getHitStep() == 0
		&& last == false)
	{
		--_ufoSize;
	}
}

/**
 * Advances this Dogfight and updates all the elements in the view-port.
 * @note Includes ufo movement, weapons fire, projectile movement, ufo escape
 * conditions, craft and ufo destruction conditions, and retaliation mission
 * generation as applicable.
 */
void DogfightState::advanceDogfight()
{
	if (_reduced == false)
	{
		aniPort();

		if (_refreshCraft != 0
			&& --_refreshCraft == 0)
		{
			drawCraft();
		}

		if (_ufo->isCrashed()        == false
			&& _craft->isDestroyed() == false
			&& _ufo->getTicked()     == false)	// NOTE: a UFO ticks only once per GeoscapeState::thinkDogfights()
		{										// (regardless of how many intercepts have engaged it).
			_ufo->setTicked();

			int escapeTicks (_ufo->getEscapeCountdown());
			if (escapeTicks > 0)
			{
				_geoState->drawUfoBlobs();

				if (_dist < DIST_STANDOFF)
					_ufo->setEscapeCountdown(--escapeTicks);

				if (escapeTicks == 0) // UFO is breaking off.
					_ufo->setSpeed(_ufo->getRules()->getTopSpeed());
			}

			const int fireTicks (_ufo->getFireCountdown());
			if (fireTicks > 0)
				_ufo->setFireCountdown(fireTicks - 1);
		}
	}

	bool stop (false);

	if (_ufo->getSpeed() > _craft->getRules()->getTopSpeed()) // Crappy craft is chasing UFO.
	{
		_ufoBreakingOff = true;
		stop = true;
		updateStatus("STR_UFO_OUTRUNNING_INTERCEPTOR");

		if (_geoState->getDfCCC() == false) // should need to run this only once per.
		{
			int qtyCraftVsUfo (0);
			for (std::list<DogfightState*>::const_iterator
					i = _geoState->getDogfights().begin();
					i != _geoState->getDogfights().end();
					++i)
			{
				if ((*i)->getUfo() == _ufo
					&& (*i)->getCraft()->isDestroyed() == false)
				{
					++qtyCraftVsUfo;
				}
			}

			if (qtyCraftVsUfo == 1)
				_geoState->setDfCCC(
								_craft->getLongitude(),
								_craft->getLatitude());
		}
	}
	else // UFO cannot break off because it's crappier than the crappy craft.
		_ufoBreakingOff = false;


	if (_reduced == false)
	{
		int delta; // Update distance.
		const int accel ((_craft->getRules()->getAcceleration()
						- _ufo->getRules()->getAcceleration()) >> 1u); // could be negative.

		if (_ufoBreakingOff == false)
		{
			if (_dist != _desired)
			{
				if (_ufo->isCrashed() == false
					&& _craft->isDestroyed() == false)
				{
					delta = std::max(2,
									 8 + accel);

					if (_dist < _desired)			// Craft vs UFO receding
					{
						if (_dist + delta > _desired)
							delta = _desired - _dist;
					}
					else //if (_dist > _desired)	// Craft vs UFO closing
						delta = -delta;
				}
				else
					delta = 0;

				if (delta > 0)
				{
					for (std::vector<CraftWeaponProjectile*>::const_iterator
							i = _projectiles.begin();
							i != _projectiles.end();
							++i)
					{
						if ((*i)->getGlobalType() == PGT_MISSILE) //&& (*i)->getDirection() == PD_CRAFT)
							(*i)->setCwpRange((*i)->getCwpRange() + delta); // Don't let interceptor mystically push or pull its fired projectiles. Sorta ....
					}
				}
			}
			else
				delta = 0;
		}
		else // _ufoBreakingOff== true
			delta = std::max(6,				// UFOs can try to outrun the missiles; don't adjust projectile positions here.
							 12 + accel);	// If UFOs ever fire anything but beams those positions need to be adjusted here though.

		_dist += delta;
		_txtDistance->setText(Text::intWide(_dist));

		int pos;

		for (std::vector<CraftWeaponProjectile*>::const_iterator // Move projectiles and check for hits.
				i = _projectiles.begin();
				i != _projectiles.end();
				)
		{
			(*i)->stepProjectile();

			switch ((*i)->getDirection())
			{
				case PD_CRAFT: // Projectiles fired by interceptor.
					if (_ufo->isCrashed() == false
						&& (*i)->getPassed() == false
						&& ((*i)->getCwpPosition() >= _dist // Projectile reached the UFO - determine if it's been hit.
							|| ((*i)->getGlobalType() == PGT_BEAM && (*i)->getFinished() == true)))
					{
						const int hitprob = (*i)->getAccuracy() // NOTE: Could include UFO speed here ... and/or acceleration-delta.
										  + (_craft->getKills() << 1u)
										  + _ufoSize * 3
										  - _diff * 5;

						//Log(LOG_INFO) << "df: Craft pType = " << (*i)->getType() << " hp = " << hitprob;
						if (RNG::percent(hitprob) == true)
						{
							(*i)->setFinished();

							const int power (RNG::generate(
													((*i)->getPower() + 1) >> 1u, // Round up.
													 (*i)->getPower()));
							// TODO: Subtract UFO-armor value ala OXCE.
							std::string status;
							if (power < 1)
								status = "STR_UFO_HIT_NO_DAMAGE"; // TODO: sFx play "ping" sound.
							else
							{
								_ufo->setUfoHull(power);

								if (power < (_ufo->getRules()->getUfoHullCap() + 19) / 20) // 5% rounded up.
									status = "STR_UFO_HIT_GLANCING";
								else
									status = "STR_UFO_HIT";

								if (_ufo->isCrashed() == true)
								{
									_ufo->setShotDownByCraftId(_craft->getIdentificator());
									_ufo->setSpeed();
									_craft->addKill();

									_ufoBreakingOff = // if the ufo got shotdown here these no longer apply ->
									_finishRequest =
									stop = false;
								}

								_game->getResourcePack()->playSoundFx(
																ResourcePack::UFO_HIT,
																true);
							}

							updateStatus(status);

							_ufo->setHitStep(3);
						}
						else // Missed.
						{
							switch ((*i)->getGlobalType())
							{
								case PGT_MISSILE:
									(*i)->setPassed();
									break;

								case PGT_BEAM:
									(*i)->setFinished();
							}
						}
					}
					break;

				case PD_UFO: // Projectiles fired by UFO.
					if ((*i)->getFinished() == true)
					{
						int hitprob ((*i)->getAccuracy() - _craft->getKills());

						if (   _craftStance == _btnCautious
							|| _craftStance == _btnStandoff
							|| _craftStance == _btnDisengage)
						{
							hitprob -= 20;
						}
						else if (_craftStance == _btnAggressive)
							hitprob += 15;

						//Log(LOG_INFO) << "df: UFO pType = " << (*i)->getType() << " hp = " << hitprob;
						if (RNG::percent(hitprob) == true)
						{
							const int power (RNG::generate(
													(_ufo->getRules()->getWeaponPower() + 9) / 10, // Round up.
													 _ufo->getRules()->getWeaponPower()));
							// TODO: Subtract Craft-armor value ala OXCE.
							// TODO: Rig-up damage messages ala above^ under Craft projectiles.
							if (power != 0)
							{
								_craft->setCraftHull(power);
								drawCraft();
								_refreshCraft = 3; // the hull will turn red when hit and '_refreshCraft' will turn it yellow after "3" iterations.

								updateStatus("STR_INTERCEPTOR_DAMAGED");
								_game->getResourcePack()->playSoundFx(
																ResourcePack::INTERCEPTOR_HIT,
																true);

								if (_cautionLevel != CAUTION_NONE
									&& checkTargets() == true)
								{
									const int hullPct (_craft->getCraftHullPct());

									bool standoff (false);
									switch (_cautionLevel)
									{
										case CAUTION_HIGH:
											if (   (_craftStance == _btnCautious && hullPct < 66)
												|| (_craftStance == _btnStandard && hullPct < 41))
											{
												standoff = true;

												if (hullPct > 40)
													_cautionLevel = CAUTION_LOW;
												else
													_cautionLevel = CAUTION_NONE;
											}
											break;

										case CAUTION_LOW:
											if (_craftStance == _btnCautious && hullPct < 41)
											{
												standoff = true;
												_cautionLevel = CAUTION_NONE;
											}
									}

									if (standoff == true)
									{
										_btnStandoff->releaseButtonGroup();

										_finishRequest = false;
										updateStatus("STR_STANDOFF");
										_desired = DIST_STANDOFF;
									}
								}
							}
						}
					}
			}

			if ((*i)->getFinished() == true // Remove projectiles that hit or missed their target.
				|| ((*i)->getPassed() == true
					&& ((pos = (*i)->getCwpPosition()) > (*i)->getCwpRange()
						|| (pos >> 3u) > _battleScope->getHeight()))) // || pos < 0 - for Ufo MISSILES (not used.)
			{
				delete *i;
				i = _projectiles.erase(i);
			}
			else
				++i;
		}


		if (_craft->isDestroyed() == false // handle Craft weapons and distance.
			&& _ufo->isCrashed() == false)
		{
			if (   _craftStance == _btnCautious
				|| _craftStance == _btnStandard
				|| _craftStance == _btnAggressive)
			{
				bool adjust = false;

				const CraftWeapon* cw;
				const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
				for (size_t
						i = 0u;
						i != hardpoints;
						++i)
				{
					if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr
						&& cw->getCwLoad() != 0)
					{
						int fire;
						switch (i)
						{
							default:
							case 0u: fire = _w1FireCountdown; break;
							case 1u: fire = _w2FireCountdown;
						}

						if (fire == 0
							&& _dist <= cw->getRules()->getRange() << 3u) // <- convert ruleset-value to IG Dogfight distance.
						{
							switch (i)
							{
								case 0u:
									if (_w1Enabled == true) fireWeapon1();
									break;
								case 1u:
									if (_w2Enabled == true) fireWeapon2();
							}
						}

						if (cw->getCwLoad() == 0)
							adjust = true;
					}
				}

				if (adjust == true) // Handle craft distance according to stance and weapon loads.
					adjustDistance();
			}

			if (_w1FireCountdown != 0) --_w1FireCountdown;
			if (_w2FireCountdown != 0) --_w2FireCountdown;
		}

		if (_ufo->isCrashed() == false // handle UFO weapon.
			&& _ufo->getFireCountdown() == 0)
		{
			const int ufoWRange (_ufo->getRules()->getWeaponRange()); // huh is this not to be multiplied by 8 like CraftWeaponRange

			if (_craft->isDestroyed() == false
				 && _dist <= ufoWRange)
			{
				if (_ufo->getShootingAt() == _slot || _ufo->getShootingAt() == 0u)
				{
					std::vector<size_t> altSlots; // Randomize UFO's target.

					for (std::list<DogfightState*>::const_iterator
							i = _geoState->getDogfights().begin();
							i != _geoState->getDogfights().end();
							++i)
					{
						if (*i != this // target can be either '_slot' OR in 'altSlots' but not both.
							&& (*i)->isReduced() == false
							&& (*i)->getDistance() <= ufoWRange
							&& (*i)->getCraft()->isDestroyed() == false
							&& (*i)->getUfo() == _ufo)
						{
							altSlots.push_back((*i)->getInterceptSlot());
						}
					}

					switch (altSlots.size())
					{
						case 0u: // this->craft.
							_ufo->setShootingAt(_slot);
							fireWeaponUfo();
							break;

						default:
						{
							int pctShoot (static_cast<int>(Round(100. / static_cast<double>(altSlots.size() + 1u)))); // +1 for this->craft.

							if (_ufo->getShootingAt() == _slot)
								pctShoot += 18; // arbitrary increase for UFO to continue shooting at this->craft.

							if (RNG::percent(pctShoot) == true)
							{
								_ufo->setShootingAt(_slot);
								fireWeaponUfo();
							}
							else // This is where the magic happens, Lulzor!!
								_ufo->setShootingAt(altSlots.at(RNG::pick(altSlots.size()))); // NOTE: Do not shoot - UFO is not on this->craft.
						}
					}
				}
			}
			else if (_ufo->getShootingAt() == _slot)
				_ufo->setShootingAt(0u);
		}
	}
	// End (_reduced == false)


	if (_finishRequest == true) // finish requested by btnDisengageClick()
	{
		if ((_ufo->isCrashed() == true && _textPersistence == 0) //(_ufo->isCrashed() || _craft->isDestroyed()) -> their code.
			|| ((_reduced == true || _dist > DIST_ENGAGE)
				&& (_ufoBreakingOff == true || _craftStance == _btnDisengage)))
		{
			_finish = true;

			if (_destroyCraft == false)
			{
				if (_ufoBreakingOff == true)
				{
					// what if this isn't the only interceptor engaging the UFO - what if one
					// of the other interceptors is faster than the UFO so the UFO can't
					// really breakoff and therefore should (assumedly) not call stepTarget().
					_ufo->stepTarget();	// <- probably required so that the dogfight doesn't instantly start again ...
										//    also required to play nice with the craft's setTarget() routine ...

					_craft->setTarget(_ufo); // <- possibly unnecessary/redundant.
				}
				else if (_destroyUfo == true || _craftStance == _btnDisengage)
				{
					_craft->returnToBase(); // NOTE: This would also be done on the UFO's dTor.
				}
			}

//			if (_ufo->isCrashed())	// their code. -> I don't think I actually *want* this. Or I've
//			{						// likely handled it with a different result ... above^ or below_
//				for (std::vector<Target*>::iterator i = _ufo->getFollowers()->begin(); i != _ufo->getFollowers()->end(); ++i)
//				{
//					Craft* c = dynamic_cast<Craft*>(*i);
//					if (c != nullptr && c->getNumSoldiers() == 0 && c->getNumVehicles() == 0)
//					{
//						c->returnToBase();
//						i = _ufo->getFollowers()->begin();
//					}
//					else
//						++i;
//				}
//			}
		}
	}
	else
	{
		if (_craft->isDestroyed() == true) // End dogfight if craft is destroyed.
		{
			updateStatus("STR_INTERCEPTOR_DESTROYED");
			_textPersistence <<= 1u;
			_game->getResourcePack()->playSoundFx(ResourcePack::INTERCEPTOR_EXPLODE);

			stop =
			_destroyCraft = true;

			if (_ufo->getShootingAt() == _slot)
				_ufo->setShootingAt(0u);
		}
		else if (_ufo->isCrashed() == true) // End dogfight if UFO is crashed or destroyed.
		{
			stop = true;

			_ufo->getAlienMission()->ufoShotDown(*_ufo);

			const double
				lon (_ufo->getLongitude()),
				lat (_ufo->getLatitude());
			
			checkRetaliation(lon,lat);

			int pts (0);

			if (_ufo->isDestroyed() == true)
			{
				_destroyUfo = true;

				if (_ufo->getShotDownByCraftId() == _craft->getIdentificator())
				{
					updateStatus("STR_UFO_DESTROYED");
					_game->getResourcePack()->playSoundFx(ResourcePack::UFO_EXPLODE);

					pts = _ufo->getRules()->getScore() << 1u;
				}
			}
			else if (_ufo->getShotDownByCraftId() == _craft->getIdentificator()) // crashed.
			{
				updateStatus("STR_UFO_CRASH_LANDS");
				_game->getResourcePack()->playSoundFx(ResourcePack::UFO_CRASH);

				pts = _ufo->getRules()->getScore();

				if (_globe->insideLand(lon,lat) == false)
				{
					_destroyUfo = true;
					_ufo->setUfoStatus(Ufo::DESTROYED);

					pts <<= 1u;
				}
				else if (_ufo->getCrashId() == 0) // Set up Crash site.
				{
					_ufo->setTarget();
					_ufo->setCrashId(_playSave->getCanonicalId(Target::stTarget[6u]));

					_ufo->setSecondsLeft(RNG::generate(24,96) * 3600); // TODO: Put min/max in UFO-rules per UFO-type.
					_ufo->setAltitude(MovingTarget::stAltitude[0u]);
				}
			}

			if (pts != 0)
				_playSave->scorePoints(lon,lat, pts, false);

			if (_ufo->getShotDownByCraftId() == _craft->getIdentificator())
			{
				for (std::list<DogfightState*>::const_iterator
					i = _geoState->getDogfights().begin();
					i != _geoState->getDogfights().end();
					++i)
				{
					if (*i != this
						&& (*i)->getUfo() == _ufo
						&& (*i)->isReduced() == false)
					{
						(*i)->setTextPersistence(STAT_PERSIST); // persist other port(s).
					}
				}
				_textPersistence <<= 1u; // persist this port twice normal duration.
			}
		}
		else if (_ufo->getUfoStatus() == Ufo::LANDED)
		{
			_textPersistence <<= 1u; // persist this port twice normal duration.
			stop = true;
			_ufo->setShootingAt(0u);
		}
	}


	if (_projectiles.empty() == true && stop == true)
		_finishRequest = true;
}

/**
 * Fires a shot from the first weapon equipped on the Craft.
 */
void DogfightState::fireWeapon1()
{
	CraftWeapon* const cw (_craft->getCraftWeapons()->at(0u));
	if (cw->setCwLoad(cw->getCwLoad() - 1))
	{
		_w1FireCountdown = _w1FireInterval;
		if (_w1FireCountdown < 1) _w1FireCountdown = 1;

		std::wostringstream woststr;
		woststr << cw->getCwLoad();
		_txtLoad1->setText(woststr.str());

		CraftWeaponProjectile* const prj (cw->fire());
		prj->setDirection(PD_CRAFT);
		prj->setHorizontalPosition(PH_LEFT);
		_projectiles.push_back(prj);

		_game->getResourcePack()->playSoundFx(
										static_cast<unsigned>(cw->getRules()->getMissileSound()),
										true);
	}
}

/**
 * Fires a shot from the second weapon equipped on the Craft.
 */
void DogfightState::fireWeapon2()
{
	CraftWeapon* const cw (_craft->getCraftWeapons()->at(1u));
	if (cw->setCwLoad(cw->getCwLoad() - 1))
	{
		_w2FireCountdown = _w2FireInterval;
		if (_w2FireCountdown < 1) _w2FireCountdown = 1;

		std::wostringstream woststr;
		woststr << cw->getCwLoad();
		_txtLoad2->setText(woststr.str());

		CraftWeaponProjectile* const prj (cw->fire());
		prj->setDirection(PD_CRAFT);
		prj->setHorizontalPosition(PH_RIGHT);
		_projectiles.push_back(prj);

		_game->getResourcePack()->playSoundFx(
										static_cast<unsigned>(cw->getRules()->getMissileSound()),
										true);
	}
}

/**
 * Each time a UFO fires its weapon a new reload interval is calculated.
 */
void DogfightState::fireWeaponUfo()
{
	updateStatus("STR_UFO_RETURN_FIRE");

	CraftWeaponProjectile* const prj (new CraftWeaponProjectile());
	prj->setType(PT_PLASMA_BEAM);
	prj->setAccuracy(60);
	prj->setPower(_ufo->getRules()->getWeaponPower());
	prj->setDirection(PD_UFO);
	prj->setHorizontalPosition(PH_CENTER);
	prj->setCwpPosition(_dist - (_ufoSize >> 1u));
	_projectiles.push_back(prj);

	_game->getResourcePack()->playSoundFx(
									_ufo->getRules()->getUfoFireSound(),
									true);


	int reload (_ufo->getRules()->getWeaponReload());
	reload += RNG::generate(0,
							reload >> 1u);
	reload -= _diff << 1u;
	if (reload < 1) reload = 1;

	_ufo->setFireCountdown(reload);
}

/**
 * Changes distance between the Craft and UFO.
 * @param hasWeapons - true if the Craft has weapons (default true)
 */
void DogfightState::adjustDistance(bool hasWeapons) // private.
{
	int dist (-1);

	if (hasWeapons == true)
	{
		if (_craftStance == _btnCautious) // find range of longest loaded weapon
		{
			for (std::vector<CraftWeapon*>::const_iterator
					i = _craft->getCraftWeapons()->begin();
					i != _craft->getCraftWeapons()->end();
					++i)
			{
				if (*i != nullptr
					&& (*i)->getCwLoad() != 0
					&& (*i)->getRules()->getRange() > dist)
				{
					dist = (*i)->getRules()->getRange();
				}
			}
		}
		else if (_craftStance == _btnStandard) // find range of shortest loaded weapon
		{
			dist = 999;
			for (std::vector<CraftWeapon*>::const_iterator
					i = _craft->getCraftWeapons()->begin();
					i != _craft->getCraftWeapons()->end();
					++i)
			{
				if (*i != nullptr
					&& (*i)->getCwLoad() != 0
					&& (*i)->getRules()->getRange() < dist)
				{
					dist = (*i)->getRules()->getRange();
				}
			}
			if (dist == 999) dist = -1;
		}
	}

	if (dist != -1)
		_desired = dist << 3u; // <- convert ruleset-value to IG Dogfight distance.
	else
		_desired = DIST_STANDOFF;
}

/**
 * Updates the status text and restarts the message-timeout counter.
 * @param status - reference to status
 */
void DogfightState::updateStatus(const std::string& status) // private.
{
	_txtStatus->setText(tr(status));
	_textPersistence = STAT_PERSIST;
}
// statii ->
// STR_INTERCEPTOR_DAMAGED
// STR_INTERCEPTOR_DESTROYED
// STR_UFO_OUTRUNNING_INTERCEPTOR
// STR_UFO_HIT
// STR_UFO_DESTROYED
// STR_UFO_CRASH_LANDS
// STR_UFO_RETURN_FIRE
// STR_STANDOFF
// STR_CAUTIOUS_ATTACK
// STR_STANDARD_ATTACK
// STR_AGGRESSIVE_ATTACK
// STR_DISENGAGING
// STR_STANDOFF_RANGE_ONLY

/**
 * Puts craft in standoff-stance if engaged.
 * @note If already in standoff it tries to iconize all dogfights (all dogfights
 * must already be in their standoff-stance).
 */
void DogfightState::keyEscape(Action*)
{
	if (_craftStance != _btnStandoff)
	{
		_btnStandoff->releaseButtonGroup();
		btnStandoffClick(nullptr);
	}
	else if (_reduced == false)
	{
		bool dont (false);
		for (std::list<DogfightState*>::const_iterator
				i = _geoState->getDogfights().begin();
				i != _geoState->getDogfights().end();
				++i)
		{
			if ((*i)->checkStandoff() == false) //_dist >= DST_STANDOFF
			{
				dont = true;
				break;
			}
		}

		if (dont == false)
			btnReduceToIconClick(nullptr);
	}
}

/**
 * Switches to Standoff mode - standoff range.
 * @param action - pointer to an Action
 */
void DogfightState::btnStandoffClick(Action*)
{
	if (checkTargets() == true)
	{
		_finishRequest = false;
		updateStatus("STR_STANDOFF");
		_desired = DIST_STANDOFF;
	}
}

/**
 * Switches to Cautious mode - maximum weapon range.
 * @param action - pointer to an Action
 */
void DogfightState::btnCautiousClick(Action*)
{
	if (checkTargets() == true)
	{
		_finishRequest = false;
		updateStatus("STR_CAUTIOUS_ATTACK");

		bool hasWeapons = false;

		const CraftWeapon* cw;
		int fireInterval;
		const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
		for (size_t
				i = 0u;
				i != hardpoints;
				++i)
		{
			if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr)
			{
				hasWeapons = true;

				fireInterval = cw->getRules()->getCautiousReload();
				switch (i)
				{
					case 0u: _w1FireInterval = fireInterval; break;
					case 1u: _w2FireInterval = fireInterval;
				}
			}
		}

		adjustDistance(hasWeapons);
	}
}

/**
 * Switches to Standard mode - minimum weapon range.
 * @param action - pointer to an Action
 */
void DogfightState::btnStandardClick(Action*)
{
	if (checkTargets() == true)
	{
		_finishRequest = false;
		updateStatus("STR_STANDARD_ATTACK");

		bool hasWeapons = false;

		const CraftWeapon* cw;
		int fireInterval;
		const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
		for (size_t
				i = 0u;
				i != hardpoints;
				++i)
		{
			if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr)
			{
				hasWeapons = true;

				fireInterval = cw->getRules()->getStandardReload();
				switch (i)
				{
					case 0u: _w1FireInterval = fireInterval; break;
					case 1u: _w2FireInterval = fireInterval;
				}
			}
		}

		adjustDistance(hasWeapons);
	}
}

/**
 * Switches to Aggressive mode - close range.
 * @param action - pointer to an Action
 */
void DogfightState::btnAggressiveClick(Action*)
{
	if (checkTargets() == true)
	{
		_finishRequest = false;
		updateStatus("STR_AGGRESSIVE_ATTACK");

		const CraftWeapon* cw;
		int fireInterval;
		const size_t hardpoints (_craft->getRules()->getWeaponCapacity());
		for (size_t
				i = 0u;
				i != hardpoints;
				++i)
		{
			if ((cw = _craft->getCraftWeapons()->at(i)) != nullptr)
			{
				fireInterval = cw->getRules()->getAggressiveReload();
				switch (i)
				{
					case 0u: _w1FireInterval = fireInterval; break;
					case 1u: _w2FireInterval = fireInterval;
				}
			}
		}

		_desired = DIST_CLOSE;
	}
}

/**
 * Disengages from the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnDisengageClick(Action*)
{
	if (checkTargets() == true)
	{
		_finishRequest = true;
		updateStatus("STR_DISENGAGING");
		_desired = DIST_ENGAGE + 10;
	}

/*	if (_geoState->getQtyReducedDogfights() == _totalIntercepts - 1u) // if this is the only dogfight with a port that's displayed
	{
		_geoState->toggleDogfight(false);
	} */
}

/**
 * Shows a side-view of the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::btnUfoClick(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
			_previewUfo		->setVisible();

			_btnStandoff	->setVisible(false); // Disable all other buttons to prevent misclicks ->
			_btnCautious	->setVisible(false);
			_btnStandard	->setVisible(false);
			_btnAggressive	->setVisible(false);
			_btnDisengage	->setVisible(false);
			_btnUfo			->setVisible(false);
			_srfTexIcon		->setVisible(false);
			_btnReduce		->setVisible(false);
			_isfCw1			->setVisible(false);
			_isfCw2			->setVisible(false);
	}
}

/**
 * Hides the sideview of the UFO.
 * @param action - pointer to an Action
 */
void DogfightState::previewClick(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			std::string article (_ufo->getRules()->getType()); // strip const. yay,
			if (_playSave->isResearched(article) == true)
				Ufopaedia::openArticle(_game, article);
			break;
		}

		case SDL_BUTTON_RIGHT:
			_previewUfo		->setVisible(false);

			_btnStandoff	->setVisible(); // Reenable all other buttons to prevent misclicks Lol ->
			_btnCautious	->setVisible();
			_btnStandard	->setVisible();
			_btnAggressive	->setVisible();
			_btnDisengage	->setVisible();
			_btnUfo			->setVisible();
			_srfTexIcon		->setVisible();
			_btnReduce		->setVisible();
			_isfCw1			->setVisible();
			_isfCw2			->setVisible();
	}
}

/**
 * Minimizes the dogfight port to its Icon.
 * @param action - pointer to an Action
 */
void DogfightState::btnReduceToIconClick(Action*)
{
	_geoState->resetTimer();

	if (checkTargets() == true)
	{
		if (_dist >= DIST_STANDOFF)
		{
			if (_projectiles.empty() == true)
			{
				_reduced = true;

				_window			->setVisible(false);
				_btnStandoff	->setVisible(false);
				_btnCautious	->setVisible(false);
				_btnStandard	->setVisible(false);
				_btnAggressive	->setVisible(false);
				_btnDisengage	->setVisible(false);
				_btnUfo			->setVisible(false);
				_srfTexIcon		->setVisible(false);
				_btnReduce		->setVisible(false);
				_battleScope	->setVisible(false);
				_isfCw1			->setVisible(false);
				_srfCwRange1	->setVisible(false);
				_isfCw2			->setVisible(false);
				_srfCwRange2	->setVisible(false);
				_srfHull		->setVisible(false);
				_txtLoad1		->setVisible(false);
				_txtLoad2		->setVisible(false);
				_txtDistance	->setVisible(false);
				_txtStatus		->setVisible(false);
				_txtTitle		->setVisible(false);
				_previewUfo		->setVisible(false);

				_btnRestoreIcon->setVisible();
				_txtRestoreIcon->setVisible();

				if (_geoState->getQtyReducedDogfights() == _slotsTotal) // if all dogfight-ports are currently reduced.
				{
					_geoState->toggleDogfight(false);
				}
				else
					_geoState->resetInterceptPorts();
			}
			else
				updateStatus("STR_PROJECTILE_IN_FLIGHT");
		}
		else
			updateStatus("STR_STANDOFF_RANGE_ONLY");
	}
}

/**
 * Restores the dogfight port from its Icon.
 * @param action - pointer to an Action
 */
void DogfightState::btnShowPortPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT: // note that this includes keyboard-press for whatever reason.
		{
			_srfTexIcon->clear();

			Surface* const srf (_game->getResourcePack()->getSurface(getTextureIcon()));
			if (srf != nullptr)
				srf->blit(_srfTexIcon);
//			else Log(LOG_WARNING) << "Texture icon for dogfight not available.";

			_reduced = false;

			_window			->setVisible();
			_btnStandoff	->setVisible();
			_btnCautious	->setVisible();
			_btnStandard	->setVisible();
			_btnAggressive	->setVisible();
			_btnDisengage	->setVisible();
			_btnUfo			->setVisible();
			_srfTexIcon		->setVisible();
			_btnReduce		->setVisible();
			_battleScope	->setVisible();
			_isfCw1			->setVisible();
			_srfCwRange1	->setVisible();
			_isfCw2			->setVisible();
			_srfCwRange2	->setVisible();
			_srfHull		->setVisible();
			_txtLoad1		->setVisible();
			_txtLoad2		->setVisible();
			_txtDistance	->setVisible();
			_txtStatus		->setVisible();
			_txtTitle		->setVisible();

			_btnRestoreIcon->setVisible(false);
			_txtRestoreIcon->setVisible(false);

			_geoState->resetInterceptPorts();

			if (_geoState->getQtyReducedDogfights() == _slotsTotal - 1u) // if this is the only dogfight with a port that's displayed.
				_geoState->storePreDogfightCoords();

			_globe->center(
						_craft->getLongitude(),
						_craft->getLatitude());

			_geoState->toggleDogfight(true);
			break;
		}

		case SDL_BUTTON_RIGHT:
			_game->pushState(new GeoscapeCraftState(_craft, _geoState));
			_globe->rotateStop(); // grrr
	}
}

/**
 * Checks if the state is iconized.
 * @return, true if minimized
 */
bool DogfightState::isReduced() const
{
	return _reduced;
}

/**
 * Checks if Craft stance is in stand-off.
 * @return, true if standing off
 */
bool DogfightState::checkStandoff() const
{
	return (_craftStance == _btnStandoff);
}

/**
 * Animates interceptor damage by changing the color according to percentage of
 * hull left and redrawing the sprite.
 * @param init - true if called from cTor (default false)
 */
void DogfightState::drawCraft(bool init) // private.
{
	if (_reduced == false)
	{
		const int hurtPct (100 - _craft->getCraftHullPct());
		if (hurtPct > 0)
		{
			int rowsToColor (static_cast<int>(std::floor(
							 static_cast<float>(_craftHeight * hurtPct) / 100.f)));

			if (rowsToColor != 0)
			{
				if (hurtPct > 99) ++rowsToColor;

				Uint8
					color,
					colorPre;
				
				if (init == true)
					color = _colors[DAMAGE_YEL];
				else
					color = _colors[DAMAGE_RED];

				for (int
						y = _craftHeight_pre;
						y != _craftHeight_pre + rowsToColor;
						++y)
				{
					for (int
							x = 1;
							x != 23;
							++x)
					{
						colorPre = _srfHull->getPixelColor(x,y);

						if ((  colorPre >= _colors[CRAFT_MIN] // is gray Craft pixel
							&& colorPre <  _colors[CRAFT_MAX])
							|| colorPre == _colors[DAMAGE_YEL])
						{
							_srfHull->setPixelColor(x,y, color);
						}
						else if (colorPre == _colors[DAMAGE_RED])
							_srfHull->setPixelColor(
												x,y,
												_colors[DAMAGE_YEL]);
					}
				}
			}
		}
	}
}

/**
 * Draws the UFO blob on the radar screen.
 * @note Currently works only for original sized blobs 13 x 13 pixels.
 */
void DogfightState::drawUfo() // private.
{
	if (_ufoSize > -1) //&& _ufo->isDestroyed() == false
	{
		const int
			ufo_x ((_battleScope->getWidth() >> 1u) - 6),
			ufo_y (_battleScope->getHeight() - (_dist >> 3u) - 6);
		Uint8 color;

		for (int
				y = 0;
				y != 13;
				++y)
		{
			for (int
					x = 0;
					x != 13;
					++x)
			{
				color = static_cast<Uint8>(GeoscapeState::_ufoBlobs[static_cast<size_t>(_ufoSize + _ufo->getHitStep())]
																   [static_cast<size_t>(y)]
																   [static_cast<size_t>(x)]);
				if (color != 0u)
				{
					if (_ufo->isCrashed() == true || _ufo->getHitStep() != 0)
						color = static_cast<Uint8>(color << 1u);

					color = static_cast<Uint8>(_window->getPixelColor(
																ufo_x + x + 3,
																ufo_y + y + 3) - color);
					if (color < _colors[BLOB_MIN])
						color = _colors[BLOB_MIN];

					_battleScope->setPixelColor(
											ufo_x + x,
											ufo_y + y,
											color);
				}
			}
		}
	}
}

/**
 * Draws projectiles on the Craft's radar-screen.
 * @note Its shape will be different depending on what type of projectile it is.
 * Currently works for original-sized blobs 3x6 pixels. Positions are plotted in
 * ruleset-kilometers, not the larger [x8] Dogfight-values. yeh, how did that
 * make it past Quality Assurance.
 * @param prj - pointer to a CraftWeaponProjectile
 */
void DogfightState::drawProjectile(const CraftWeaponProjectile* const prj) // private.
{
	int pos_x ((_battleScope->getWidth() >> 1u) + (prj->getHorizontalPosition() << 1u));
	Uint8 colorOffset;

	switch (prj->getGlobalType())
	{
		case PGT_MISSILE:
		{
			Uint8 color;

			--pos_x;
			const int pos_y (_battleScope->getHeight() - prj->getCwpPosition(true));
			for (int
					x = 0;
					x != 3;
					++x)
			{
				for (int
						y = 0;
						y != 6;
						++y)
				{
					colorOffset = static_cast<Uint8>(_projectileBlobs[static_cast<size_t>(prj->getType())]
																	 [static_cast<size_t>(y)]
																	 [static_cast<size_t>(x)]);
					if (colorOffset != 0u)
					{
						color = _window->getPixelColor(
													pos_x + x + 3,
													pos_y + y + 3);
						color = Vicegrip(
									static_cast<Uint8>(color - colorOffset),
									_colors[BLOB_MIN],
									_colors[BLOB_MAX]);

						_battleScope->setPixelColor(
												pos_x + x,
												pos_y + y,
												color);
					}
				}
			}
			break;
		}

		case PGT_BEAM:
		{
			Uint8
				colorSrc,
				colorDst;

			colorOffset = prj->getBeamPhase();
			const int
				stop  (_battleScope->getHeight() - 2),
				start (_battleScope->getHeight() - (_dist >> 3u));
			int
				intensity,
				width;

			for (int
					y = stop;
					y != start;
					--y)
			{
				switch (prj->getDirection())
				{
					case PD_CRAFT:
						intensity = prj->getPower() / 50;
						width = Vicegrip(intensity, 1,3);
						colorSrc = _window->getPixelColor(
														pos_x + 3,
														y     + 3);
						colorDst = static_cast<Uint8>(colorSrc - colorOffset - intensity);
						for (int
								x = 0;
								x != width;
								++x)
						{
							colorDst = static_cast<Uint8>(colorDst + (x << 1u));
							colorDst = Vicegrip(
											colorDst,
											_colors[BLOB_MIN],
											_colors[BLOB_MAX]);

							switch (x)
							{
								case 0:
									_battleScope->setPixelColor(pos_x,     y, colorDst);
									break;
								default:
									_battleScope->setPixelColor(pos_x + x, y, colorDst);
									_battleScope->setPixelColor(pos_x - x, y, colorDst);
							}
						}
						break;

					case PD_UFO:
						width = Vicegrip(_ufo->getRules()->getWeaponPower() / 40, 1,3);
						for (int
								x = 0;
								x != width;
								++x)
						{
							switch (x)
							{
								case 0:
									_battleScope->setPixelColor(pos_x,     y, _colors[UFO_BEAM]);
									break;
								default:
									_battleScope->setPixelColor(pos_x + x, y, _colors[UFO_BEAM]);
									_battleScope->setPixelColor(pos_x - x, y, _colors[UFO_BEAM]);
							}
						}
				}
			}
		}
	}
}

/**
 * Toggles usage of Craft weapon-1.
 * @param action - pointer to an Action
 */
void DogfightState::weapon1Click(Action*)
{
	int i;
	if (_w1Enabled = !_w1Enabled) // NOTE: reverse these:
		i = +1;
	else
		i = -1;

	_isfCw1     ->offset(_colors[DISABLED_WEAPON] * i);
	_srfCwRange1->offset(_colors[DISABLED_RANGE]  * i);
	_txtLoad1   ->offset(_colors[DISABLED_AMMO]   * i);

	_craft->setWeaponDisabled(1, !_w1Enabled);
}

/**
 * Toggles usage of Craft weapon-2.
 * @param action - pointer to an Action
 */
void DogfightState::weapon2Click(Action*)
{
	int i;
	if (_w2Enabled = !_w2Enabled) // NOTE: reverse these:
		i = +1;
	else
		i = -1;

	_isfCw2     ->offset(_colors[DISABLED_WEAPON] * i);
	_srfCwRange2->offset(_colors[DISABLED_RANGE]  * i);
	_txtLoad2   ->offset(_colors[DISABLED_AMMO]   * i);

	_craft->setWeaponDisabled(2, !_w2Enabled);
}

/**
 * Sets this Dogfight's intercept-slot.
 * @param slot - slotId
 */
void DogfightState::setInterceptSlot(size_t slot)
{
	_slot = slot;
}

/**
 * Gets this Dogfight's intercept-slot.
 * @return, slotId
 */
size_t DogfightState::getInterceptSlot() const
{
	return _slot;
}

/**
 * Sets the total-intercepts quantity.
 * @note Used to properly position the ports etc.
 * @param total - total quantity of intercept-slots
 */
void DogfightState::setTotalInterceptSlots(size_t total)
{
	_slotsTotal = total;
}

/**
 * Calculates intercept-port positions in accord with the total quantity of
 * currently displayed ports.
 * @param port			- current intercept-port iteration
 * @param portsTotal	- total quantity of displayed dogfight-ports
 */
void DogfightState::resetInterceptPort(
		size_t port,
		size_t portsTotal)
{
	if (_slot > _slotsTotal)
		_slot = _geoState->getAvailableInterceptSlot(); // keep slots filled contiguously.

	_restoreIconY = static_cast<int>((_slot * 5u) + ((_slot - 1u) << 4u));

	if (_reduced == false)
	{
		switch (portsTotal)
		{
			case 1:
				_x = 80;
				_y = 52;
				break;

			case 2:
				switch (port)
				{
					case 1:
						_x = 80;
						_y = 0;
						break;
					case 2:
						_x = 80;
						_y = 201 - _window->getHeight(); // 96;
				}
				break;

			case 3:
				switch (port)
				{
					case 1:
						_x = 80;
						_y = 0;
						break;

					case 2:
						_x = 0;
						_y = 201 - _window->getHeight(); // 96;
						break;

					case 3:
						_x = 320 - _window->getWidth();  // 160;
						_y = 201 - _window->getHeight(); // 96;
				}
				break;

			case 4:
				switch (port)
				{
					case 1:
						_x =
						_y = 0;
						break;

					case 2:
						_x = 320 - _window->getWidth(); // 160;
						_y = 0;
						break;

					case 3:
						_x = 0;
						_y = 201 - _window->getHeight(); // 96;
						break;

					case 4:
						_x = 320 - _window->getWidth();  // 160;
						_y = 201 - _window->getHeight(); // 96;
				}
		}

		_x += _game->getScreen()->getDX();
		_y += _game->getScreen()->getDY();

		placePort();
	}
}

/**
 * Relocates all dogfight-port elements to a calculated position.
 * @note This is used when multiple Dogfights are active.
 */
void DogfightState::placePort() // private.
{
	const int
		x (_window->getX() - _x),
		y (_window->getY() - _y);

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() - x);
		(*i)->setY((*i)->getY() - y);
	}

	_btnRestoreIcon->setX(_restoreIconX);
	_btnRestoreIcon->setY(_restoreIconY);
	_txtRestoreIcon->setX(_restoreIconX + 18);
	_txtRestoreIcon->setY(_restoreIconY +  6);
}

/**
 * Checks if this Dogfight should be stopped and deleted.
 * @return, true if finished
 */
bool DogfightState::isFinished() const
{
	return _finish;
}

/**
 * Gets the UFO associated with this Dogfight.
 * @return, pointer to the linked UFO object
 */
Ufo* DogfightState::getUfo() const
{
	return _ufo;
}

/**
 * Sets the UFO associated with this Dogfight to nullptr.
 * @note Used when destructing GeoscapeState.
 */
void DogfightState::clearUfo()
{
	_ufo = nullptr;
}

/**
 * Gets pointer to the xCom Craft in this Dogfight.
 * @return, pointer to linked Craft object
 */
Craft* DogfightState::getCraft() const
{
	return _craft;
}

/**
 * Sets pointer to the xCom Craft in this Dogfight to nullptr.
 * @note Used when destructing GeoscapeState.
 */
void DogfightState::clearCraft()
{
	_craft = nullptr;
}

/**
 * Gets the current distance between UFO and Craft.
 * @return, distance
 */
int DogfightState::getDistance() const
{
	return _dist;
}

/**
 * Gets the Globe's texture-icon to display for the interception.
 * @note This does not return the actual battleField terrain; that is done in
 * ConfirmLandingState. This is merely an indicator .... cf. UfoDetectedState.
 * @return, reference to a string for the icon of the texture of the globe's surface under the dogfight ha!
 */
const std::string& DogfightState::getTextureIcon() const // private.
{
	static const std::string dfTextureIconTypes[7u]
	{
		"WATER",	// 0
		"FOREST",	// 1
		"CULTA",	// 2
		"MOUNT",	// 3
		"DESERT",	// 4
		"URBAN",	// 5
		"POLAR"		// 6
	};

	int texture;
	_globe->getPolygonTexture(
						_ufo->getLongitude(),
						_ufo->getLatitude(),
						&texture);
	switch (texture)
	{
		default:
		case -1: return dfTextureIconTypes[0u];

		case  0:
		case  3:
		case  6: return dfTextureIconTypes[1u]; // 6= JUNGLE

		case  1:
		case  2: return dfTextureIconTypes[2u];

		case  5: return dfTextureIconTypes[3u];

		case  7:
		case  8: return dfTextureIconTypes[4u];

		case 10: return dfTextureIconTypes[5u];

		case  4:
		case  9:
		case 11:
		case 12: return dfTextureIconTypes[6u];
	}
}

/**
 * Checks if the Craft and UFO are still actively engaged.
 * @return, true if the fight is still on
 */
bool DogfightState::checkTargets() const // private.
{
	return _ufo->isCrashed()     == false
		&& _craft->isDestroyed() == false
		&& _ufoBreakingOff       == false;
}

/**
 * Checks for and determines a retaliation mission if applicable.
 * @param lon - the longitude
 * @param lat - the latitude
 */
void DogfightState::checkRetaliation(double lon, double lat) const // private
{
	if (_ufo->getTrajectory().getType() != UfoTrajectory::XCOM_BASE_ASSAULT) // shooting down an assault-battleship does *not* generate a Retal-Mission.
	{
		int retalCoef (_ufo->getAlienMission()->getRules().getRetaliation());
		if (retalCoef == -1)
			retalCoef = _game->getRuleset()->getRetaliation();

		if (RNG::percent((_diff + 1) * retalCoef) == true) // Check retaliation trigger. -> Spawn retaliation mission.
		{
			std::string targetRegion;
			if (RNG::percent(_diff * 10 + 10) == true)
				targetRegion = _playSave->locateRegion(*_craft->getBase())->getRules()->getType();	// Try to find the originating base.
				// TODO: If the base is removed, the mission is cancelled. nah.
			else if (RNG::generate(0,1) == 0)
				targetRegion = _ufo->getAlienMission()->getRegion();								// Retaliation vs UFO's mission region.
			else
				targetRegion = _playSave->locateRegion(lon,lat)->getRules()->getType();				// Retaliation vs UFO's shootdown region.

			// Difference from original: No retaliation until final UFO lands (Original: Is spawned).
			if (_game->getSavedGame()->findAlienMission(targetRegion, alm_RETAL) == nullptr)
			{
				const RuleAlienMission& missionRule (*_game->getRuleset()->rollMission(
																					alm_RETAL,
																					static_cast<size_t>(_game->getSavedGame()->getMonthsElapsed())));
				AlienMission* const mission (new AlienMission(missionRule, *_playSave));
				mission->setId(_playSave->getCanonicalId(Target::stTarget[7u]));
				mission->setRegion(
								targetRegion,
								*_game->getRuleset());
				mission->setRace(_ufo->getAlienRace());

				int countdown (mission->getRules().getWaveData(0u).waveTimer / 30); // delay for first wave/scout
				countdown = (RNG::generate(0, countdown) + (countdown >> 1u)) * 30;
				mission->start(countdown);

				_playSave->getAlienMissions().push_back(mission);
			}
		}
	}
}

}
