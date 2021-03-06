/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#include "GeoscapeState.h"

//#include <algorithm>
//#include <functional>
#include <iomanip>
//#include <sstream>

#include "../fmath.h"

#include "AlienBaseDetectedState.h"
#include "BaseDefenseState.h"
#include "BaseDestroyedState.h"
#include "ConfirmLandingState.h"
#include "CraftErrorState.h"
#include "CraftPatrolState.h"
#include "CraftReadyState.h"
#include "DefeatState.h"
#include "DogfightState.h"
#include "FundingState.h"
#include "GeoscapeCraftState.h"
#include "Globe.h"
#include "GraphsState.h"
#include "InterceptState.h"
#include "ItemsArrivingState.h"
#include "LowFuelState.h"
#include "ManufactureUnlockedState.h"
#include "MonthlyReportState.h"
#include "MonthNearEndState.h"
#include "ManufactureCompleteState.h"
#include "ResearchCompleteState.h"
#include "ResearchRequiredState.h"
#include "ResearchUnlockedState.h"
#include "SelectTargetState.h"
#include "SoldierDiedState.h"
#include "TerrorDetectedState.h"
#include "UfoDetectedState.h"
#include "UfoLostState.h"

#include "../Basescape/BasescapeState.h"
#include "../Basescape/SellState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Action.h"
#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Menu/ErrorMessageState.h"
#include "../Menu/ListSaveState.h"
#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCountry.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleMissionScript.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"
#include "../Ruleset/UfoTrajectory.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/AlienStrategy.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ManufactureProject.h"
#include "../Savegame/Region.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedBattleGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/TacticalStatistics.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Transfer.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"

#include "../Ufopaedia/TechTreeViewerState.h"
#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

size_t kL_curBase = 0u;
bool
	kL_geoMusicPlaying     = false,
	kL_geoMusicReturnState = false;

const double
	arcToRads    = (1. / 60.) * (M_PI / 180.),			// converts a minute of arc to radians
	radius_earth = 3437.7467707849392526078892888463;	// radius of Earth in nautical miles is set by definition of arcToRads.
														// NOTE: A nautical mile on the Earth's surface is equivalent to 1/60th of a degree.

// UFO-blobs graphics ...
const int GeoscapeState::_ufoBlobs[8u][BLOBSIZE][BLOBSIZE]
{
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 0 - STR_VERY_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 3, 5, 3, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 3, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 1 - STR_SMALL
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 4, 5, 4, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 2 - STR_MEDIUM_UC
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 5, 5, 5, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0},
		{0, 0, 0, 1, 2, 3, 3, 3, 2, 1, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // 3 - STR_LARGE
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // 4 - STR_VERY_LARGE
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{0, 1, 2, 3, 4, 5, 5, 5, 4, 3, 2, 1, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0},
		{0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}
	},
	{
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}, // 5 - STR_HUGE
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{1, 2, 3, 4, 5, 5, 5, 5, 5, 4, 3, 2, 1},
		{1, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 1},
		{0, 1, 2, 3, 3, 4, 4, 4, 3, 3, 2, 1, 0},
		{0, 0, 1, 2, 2, 3, 3, 3, 2, 2, 1, 0, 0},
		{0, 0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0, 0}
	},
	{
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}, // 6 - STR_VERY_HUGE :p
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{2, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 2},
		{0, 2, 3, 4, 4, 5, 5, 5, 4, 4, 3, 2, 0},
		{0, 0, 2, 3, 3, 4, 4, 4, 3, 3, 2, 0, 0},
		{0, 0, 0, 2, 2, 3, 3, 3, 2, 2, 0, 0, 0}
	},
	{
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}, // 7 - STR_ENORMOUS
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 3},
		{0, 3, 4, 5, 5, 5, 5, 5, 5, 5, 4, 3, 0},
		{0, 0, 3, 4, 4, 5, 5, 5, 4, 4, 3, 0, 0},
		{0, 0, 0, 3, 3, 4, 4, 4, 3, 3, 0, 0, 0}
	}
};

//
// struct definitions used when enqueuing notification events
//

/**
 * Info about a Manufacture project that just finished.
 */
struct ManufactureCompleteInfo
{
	bool allocate;
	std::wstring item;

	Base* base;

	ManufactureProgress doneType;

	/// cTor.
	ManufactureCompleteInfo(
			Base* const base_,
			const std::wstring& item_,
			bool allocate_,
			ManufactureProgress doneType_)
		:
			base(base_),
			item(item_),
			allocate(allocate_),
			doneType(doneType_)
	{}
};

/**
 * Info about a Research project that can now be started.
 */
struct ResearchUnlockedInfo
{
	bool allocate;
	std::vector<const RuleResearch*> unlockedProjects;

	Base* base;

	/// cTor.
	ResearchUnlockedInfo(
			Base* const base_,
			const std::vector<const RuleResearch*>& unlockedProjects_,
			bool allocate_)
		:
			base(base_),
			unlockedProjects(unlockedProjects_),
			allocate(allocate_)
	{}
};

/**
 * Info about a Manufacture project that can now be started.
 */
struct ManufactureUnlockedInfo
{
	bool allocate;
	std::vector<const RuleManufacture*> unlockedProjects;

	Base* base;

	/// cTor.
	ManufactureUnlockedInfo(
			Base* const base_,
			const std::vector<const RuleManufacture*>& unlockedProjects_,
			bool allocate_)
		:
			base(base_),
			unlockedProjects(unlockedProjects_),
			allocate(allocate_)
	{}
};


/**
 * Initializes all the elements in the Geoscape screen.
 */
GeoscapeState::GeoscapeState()
	:
		_playSave(_game->getSavedGame()),
		_rules(_game->getRuleset()),
		_pause(false),
		_pauseHard(false),
		_dfZout(true),
		_dfCenterCurrentCoords(false),
		_dfCCC_lon(0.),
		_dfCCC_lat(0.),
		_dfReduced(0u),
		_day(-1),
		_month(-1),
		_year(-1),
		_ufosDetected(0),
		_delayMusicDfCheck(0),
		_timeSurplus(0),
		_score(0)
{
	const int
		screenWidth  (Options::baseXGeoscape),
		screenHeight (Options::baseYGeoscape),
		halfHeight   (screenHeight >> 1u);

	_txtRadars = new Text(65, 9, 3, screenHeight - 32); // lower-left corner
	_txtLabels = new Text(65, 9, 3, screenHeight - 22);
	_txtZoom   = new Text(65, 9, 3, screenHeight - 12); // <- must instantiate before '_globe' (reasons.)

	_globe        = new Globe(
							_game,
							this, // <- (reason is here. Instantiating the Globe will set the zoom which updates the zoom-text)
							(screenWidth - 64) >> 1u,
							halfHeight,
							screenWidth - 64,
							screenHeight);
	_srfSpace     = new Surface(
							screenWidth,
							screenHeight);
	_srfSideBlack = new Surface(
							64,
							screenHeight,
							screenWidth - 64,
							0);

	// revert to ImageButtons. Stock build uses TextButton's
	_btnIntercept = new ImageButton(63, 11, screenWidth - 63, halfHeight - 100);
	_btnBases     = new ImageButton(63, 11, screenWidth - 63, halfHeight -  88);
	_btnGraphs    = new ImageButton(63, 11, screenWidth - 63, halfHeight -  76);
	_btnUfopaedia = new ImageButton(63, 11, screenWidth - 63, halfHeight -  64);
	_btnOptions   = new ImageButton(63, 11, screenWidth - 63, halfHeight -  52);
	_btnFunding   = new ImageButton(63, 11, screenWidth - 63, halfHeight -  40);
//	_btnOptions   = new ImageButton(63, 11, screenWidth - 63, halfHeight - 100); // change the GeoGraphic first .... ->
//	_btnUfopaedia = new ImageButton(63, 11, screenWidth - 63, halfHeight -  88);
//	_btnFunding   = new ImageButton(63, 11, screenWidth - 63, halfHeight -  76);
//	_btnGraphs    = new ImageButton(63, 11, screenWidth - 63, halfHeight -  64);
//	_btnIntercept = new ImageButton(63, 11, screenWidth - 63, halfHeight -  52);
//	_btnBases     = new ImageButton(63, 11, screenWidth - 63, halfHeight -  40);

	_btn5Secs     = new ImageButton(31, 13, screenWidth - 63, halfHeight + 12);
	_btn1Min      = new ImageButton(31, 13, screenWidth - 31, halfHeight + 12);
	_btn5Mins     = new ImageButton(31, 13, screenWidth - 63, halfHeight + 26);
	_btn30Mins    = new ImageButton(31, 13, screenWidth - 31, halfHeight + 26);
	_btn1Hour     = new ImageButton(31, 13, screenWidth - 63, halfHeight + 40);
	_btn1Day      = new ImageButton(31, 13, screenWidth - 31, halfHeight + 40);
//	_btn5Secs     = new ImageButton(31, 13, screenWidth - 63, halfHeight + 12); // change the GeoGraphic first .... ->
//	_btn1Min      = new ImageButton(31, 13, screenWidth - 63, halfHeight + 26);
//	_btn5Mins     = new ImageButton(31, 13, screenWidth - 63, halfHeight + 40);
//	_btn30Mins    = new ImageButton(31, 13, screenWidth - 31, halfHeight + 12);
//	_btn1Hour     = new ImageButton(31, 13, screenWidth - 31, halfHeight + 26);
//	_btn1Day      = new ImageButton(31, 13, screenWidth - 31, halfHeight + 40);

	// The old Globe rotate buttons have become the Detail & Radar toggles.
	_btnDetail      = new ImageButton(63, 46, screenWidth - 63, halfHeight + 54);
//	_btnRotateLeft  = new InteractiveSurface(12, 10, screenWidth-61, screenHeight/2+76);
//	_btnRotateRight = new InteractiveSurface(12, 10, screenWidth-37, screenHeight/2+76);
//	_btnRotateUp    = new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+62);
//	_btnRotateDown  = new InteractiveSurface(13, 12, screenWidth-49, screenHeight/2+87);
//	_btnZoomIn      = new InteractiveSurface(23, 23, screenWidth-25, screenHeight/2+56);
//	_btnZoomOut     = new InteractiveSurface(13, 17, screenWidth-20, screenHeight/2+82);

	const int height (((screenHeight - Screen::ORIGINAL_HEIGHT) >> 1u) - 12);
	_sideTop    = new TextButton(
							64,
							height,
							screenWidth - 64,
							halfHeight - (Screen::ORIGINAL_HEIGHT >> 1u) - (height + 12));
	_sideBottom = new TextButton(
							64,
							height,
							screenWidth - 64,
							halfHeight + (Screen::ORIGINAL_HEIGHT >> 1u) + 12);

	_ufoDetected = new Text(17, 16, _sideBottom->getX() + 6, _sideBottom->getY() + 4);

	std::fill_n(
			_hostileUfos,
			UFO_HOTBLOBS,
			static_cast<Ufo*>(nullptr));

	int
		x,y,
		offset_x,
		offset_y;
	for (size_t
			i = 0u;
			i != UFO_HOTBLOBS;
			++i)
	{
		offset_x = ((static_cast<int>(i) % 4) * 13); // 4 UFOs per row on top sidebar
		offset_y = ((static_cast<int>(i) / 4) * 13); // 4 rows going up
		x = _sideTop->getX() + offset_x + 3;
		y = _sideTop->getY() + height - offset_y - 16;

		_isfUfoBlobs[i] = new InteractiveSurface(13, 13, x, y);
		_numUfoBlobs[i] = new NumberText(11, 5, x + 1, y + 8);
	}

	_isfTime	= new InteractiveSurface(63, 39, screenWidth - 63, halfHeight - 28);

	_txtHour   = new Text(19, 17, screenWidth - 54, halfHeight - 22);
	_txtColon  = new Text( 5, 17, screenWidth - 35, halfHeight - 22);
	_txtMin    = new Text(19, 17, screenWidth - 30, halfHeight - 22);
	_txtSec    = new Text( 6,  9, screenWidth -  8, halfHeight - 26);

	_txtDay    = new Text(11, 9, screenWidth - 57, halfHeight - 4);
	_txtMonth  = new Text(17, 9, screenWidth - 45, halfHeight - 4);
	_txtYear   = new Text(21, 9, screenWidth - 27, halfHeight - 4);

	_txtFunds  = new Text(63, 8, screenWidth - 64, halfHeight - 110);
	_txtScore  = new Text(63, 8, screenWidth - 64, halfHeight + 102);

	_btnGroup  = _btn5Secs;

	_trGeo     = new Timer(FAST_GEO_INTERVAL); // Volutar_smoothGlobe terminator.

	_trDf      = new Timer(static_cast<Uint32>(Options::dogfightSpeed));
	_trDfStart = new Timer(static_cast<Uint32>(Options::dogfightSpeed));
	_trDfZinn  = new Timer(static_cast<Uint32>(Options::geoClockSpeed));
	_trDfZout  = new Timer(static_cast<Uint32>(Options::geoClockSpeed));

	_txtDebug = new Text(320, 27);


	setInterface("geoscape");

	add(_srfSpace);
	add(_srfSideBlack);
	add(_globe);

	add(_btnDetail);

	add(_btnIntercept, "button", "geoscape");
	add(_btnBases,     "button", "geoscape");
	add(_btnGraphs,    "button", "geoscape");
	add(_btnUfopaedia, "button", "geoscape");
	add(_btnOptions,   "button", "geoscape");
	add(_btnFunding,   "button", "geoscape");

	add(_btn5Secs,     "button", "geoscape");
	add(_btn1Min,      "button", "geoscape");
	add(_btn5Mins,     "button", "geoscape");
	add(_btn30Mins,    "button", "geoscape");
	add(_btn1Hour,     "button", "geoscape");
	add(_btn1Day,      "button", "geoscape");

//	add(_btnRotateLeft);
//	add(_btnRotateRight);
//	add(_btnRotateUp);
//	add(_btnRotateDown);
//	add(_btnZoomIn);
//	add(_btnZoomOut);

	add(_sideTop,    "button", "geoscape");
	add(_sideBottom, "button", "geoscape");
	add(_ufoDetected);

	for (size_t
			i = 0u;
			i != UFO_HOTBLOBS;
			++i)
	{
		add(_isfUfoBlobs[i]);
		_isfUfoBlobs[i]->setVisible(false);
		_isfUfoBlobs[i]->onMousePress(   static_cast<ActionHandler>(&GeoscapeState::btnUfoBlobPress));
//		_isfUfoBlobs[i]->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnUfoBlobPress),
//										 buttons[i]);

		add(_numUfoBlobs[i]);
		_numUfoBlobs[i]->setVisible(false);
		_numUfoBlobs[i]->setColor(YELLOW_D);
	}

	add(_isfTime);
	add(_txtDebug,  "text", "geoscape");
	add(_txtFunds,  "text", "geoscape");
	add(_txtScore,  "text", "geoscape");
	add(_txtHour);
	add(_txtColon);
	add(_txtMin);
	add(_txtSec);
	add(_txtDay);
	add(_txtMonth);
	add(_txtYear);
	add(_txtRadars, "text", "geoscape");
	add(_txtLabels, "text", "geoscape");
	add(_txtZoom,   "text", "geoscape");

	_game->getResourcePack()->getSurface("Cygnus_BG")->blit(_srfSpace);

	_srfSideBlack->drawRect(
						0,0,
						static_cast<Sint16>(_srfSideBlack->getWidth()),
						static_cast<Sint16>(_srfSideBlack->getHeight()),
						BLACK);

/*	_btnIntercept->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnInterceptClick));
	_btnIntercept->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnInterceptClick), Options::keyGeoIntercept);
	_btnIntercept->setGeoscapeButton(true);

	_btnBases->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnBases->setText(tr("STR_BASES"));
	_btnBases->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnBasesClick));
	_btnBases->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnBasesClick), Options::keyGeoBases);
	_btnBases->setGeoscapeButton(true);

	_btnGraphs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnGraphs->setText(tr("STR_GRAPHS"));
	_btnGraphs->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnGraphsClick);
	_btnGraphs->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnGraphsClick), Options::keyGeoGraphs);
	_btnGraphs->setGeoscapeButton(true);

	_btnUfopaedia->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnUfopaedia->setText(tr("STR_UFOPAEDIA_UC"));
	_btnUfopaedia->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnUfopaediaClick));
	_btnUfopaedia->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnUfopaediaClick), Options::keyGeoUfopedia);
	_btnUfopaedia->setGeoscapeButton(true);

	_btnOptions->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnOptions->setText(tr("STR_OPTIONS_UC"));
	_btnOptions->onMouseClick(		static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick));
	_btnOptions->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick), Options::keyGeoOptions);
	_btnOptions->setGeoscapeButton(true);

	_btnFunding->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btnFunding->setText(tr("STR_FUNDING_UC"));
	_btnFunding->onMouseClick(		static_cast<ActionHandler>(&GeoscapeState::btnFundingClick));
	_btnFunding->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnFundingClick), Options::keyGeoFunding);
	_btnFunding->setGeoscapeButton(true);

	_btn5Secs->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Secs->setText(tr("STR_5_SECONDS"));
	_btn5Secs->setBig();
	_btn5Secs->setGroup(&_btnGroup);
	_btn5Secs->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed1);
	_btn5Secs->setGeoscapeButton(true);

	_btn1Min->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Min->setText(tr("STR_1_MINUTE"));
	_btn1Min->setBig();
	_btn1Min->setGroup(&_btnGroup);
	_btn1Min->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed2);
	_btn1Min->setGeoscapeButton(true);

	_btn5Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn5Mins->setText(tr("STR_5_MINUTES"));
	_btn5Mins->setBig();
	_btn5Mins->setGroup(&_btnGroup);
	_btn5Mins->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed3);
	_btn5Mins->setGeoscapeButton(true);

	_btn30Mins->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn30Mins->setText(tr("STR_30_MINUTES"));
	_btn30Mins->setBig();
	_btn30Mins->setGroup(&_btnGroup);
	_btn30Mins->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed4);
	_btn30Mins->setGeoscapeButton(true);

	_btn1Hour->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Hour->setText(tr("STR_1_HOUR"));
	_btn1Hour->setBig();
	_btn1Hour->setGroup(&_btnGroup);
	_btn1Hour->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed5);
	_btn1Hour->setGeoscapeButton(true);

	_btn1Day->initText(_game->getResourcePack()->getFont("FONT_GEO_BIG"), _game->getResourcePack()->getFont("FONT_GEO_SMALL"), _game->getLanguage());
	_btn1Day->setText(tr("STR_1_DAY"));
	_btn1Day->setBig();
	_btn1Day->setGroup(&_btnGroup);
	_btn1Day->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression), Options::keyGeoSpeed6);
	_btn1Day->setGeoscapeButton(true); */

	// revert to ImageButtons.
	Surface* const geobord (_game->getResourcePack()->getSurface("GEOBORD.SCR"));
	geobord->setX( screenWidth  - geobord->getWidth());
	geobord->setY((screenHeight - geobord->getHeight()) >> 1u);

	_btnIntercept->copy(geobord);
	_btnIntercept->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnInterceptClick));
	_btnIntercept->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnInterceptClick),
								   Options::keyGeoIntercept);

	_btnBases->copy(geobord);
	_btnBases->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnBasesClick));
	_btnBases->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnBasesClick),
							   Options::keyGeoBases);

	_btnGraphs->copy(geobord);
	_btnGraphs->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnGraphsClick));
	_btnGraphs->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnGraphsClick),
								Options::keyGeoGraphs);

	_btnUfopaedia->copy(geobord);
	_btnUfopaedia->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnUfopaediaClick));
	_btnUfopaedia->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnUfopaediaClick),
								   Options::keyGeoUfopedia);

	_btnOptions->copy(geobord);
	_btnOptions->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick));	// why the f are these buttons calling btnTimeCompression() AND EVEN GETTING
	_btnOptions->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick),	// THE SENDER RIGHT when a true click on a time-compression btn does not.
								 Options::keyGeoOptions); // Escape key.							// Note that happens only if the 'resetTimeCache' is on a Press, not a Click!
																									// holy diana
//	_btnOptions->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick, // note: These would interfere w/ opening minimized Dogfights.
//								 Options::keyOk);
//	_btnOptions->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnOptionsClick,
//								 Options::keyOkKeypad);

	_btnFunding->copy(geobord);
	_btnFunding->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnFundingClick));
	_btnFunding->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnFundingClick),
								 Options::keyGeoFunding);


	_isfTime->copy(geobord);
	_isfTime->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnPauseClick),
							  0u);
	_isfTime->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnPauseClick),
							  SDLK_SPACE);

	_btn5Secs->copy(geobord);
	_btn5Secs->setGroup(&_btnGroup);
	_btn5Secs->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
							   Options::keyGeoSpeed1);
	_btn5Secs->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));

	_btn1Min->copy(geobord);
	_btn1Min->setGroup(&_btnGroup);
	_btn1Min->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
							  Options::keyGeoSpeed2);
	_btn1Min->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));

	_btn5Mins->copy(geobord);
	_btn5Mins->setGroup(&_btnGroup);
	_btn5Mins->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
							   Options::keyGeoSpeed3);
	_btn5Mins->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));

	_btn30Mins->copy(geobord);
	_btn30Mins->setGroup(&_btnGroup);
	_btn30Mins->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
								Options::keyGeoSpeed4);
	_btn30Mins->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));

	_btn1Hour->copy(geobord);
	_btn1Hour->setGroup(&_btnGroup);
	_btn1Hour->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
							   Options::keyGeoSpeed5);
	_btn1Hour->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));

	_btn1Day->copy(geobord);
	_btn1Day->setGroup(&_btnGroup);
	_btn1Day->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::keyTimeCompression),
							  Options::keyGeoSpeed6);
	_btn1Day->onMouseClick(   static_cast<ActionHandler>(&GeoscapeState::btnTimeCompression));


	_btnDetail->copy(geobord);
	_btnDetail->setColor(PURPLE_D);
	_btnDetail->onMousePress(static_cast<ActionHandler>(&GeoscapeState::btnDetailPress));

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftPress),
								  Options::keyGeoLeft);
	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftPress),
								  SDLK_KP4);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLonStop),
								  Options::keyGeoLeft);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLonStop),
								  SDLK_KP4);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateRightPress),
								  Options::keyGeoRight);
	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateRightPress),
								  SDLK_KP6);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLonStop),
								  Options::keyGeoRight);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLonStop),
								  SDLK_KP6);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateUpPress),
								  Options::keyGeoUp);
	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateUpPress),
								  SDLK_KP8);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLatStop),
								  Options::keyGeoUp);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLatStop),
								  SDLK_KP8);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateDownPress),
								  Options::keyGeoDown);
	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateDownPress),
								  SDLK_KP2);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLatStop),
								  Options::keyGeoDown);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateLatStop),
								  SDLK_KP2);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftUpPress),
								  SDLK_KP7);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateStop),
								  SDLK_KP7);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftDownPress),
								  SDLK_KP1);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateStop),
								  SDLK_KP1);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateRightUpPress),
								  SDLK_KP9);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateStop),
								  SDLK_KP9);

	_btnDetail->onKeyboardPress(  static_cast<ActionHandler>(&GeoscapeState::btnRotateRightDownPress),
								  SDLK_KP3);
	_btnDetail->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateStop),
								  SDLK_KP3);

	_btnDetail->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnZoomInLeftClick),
								Options::keyGeoZoomIn);
	_btnDetail->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnZoomInLeftClick),
								SDLK_KP_PLUS);
	_btnDetail->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnZoomOutLeftClick),
								Options::keyGeoZoomOut);
	_btnDetail->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnZoomOutLeftClick),
								SDLK_KP_MINUS);

	_ufoDetected->setColor(SLATE);
	_ufoDetected->setBig();
	_ufoDetected->setVisible(false);

/*	_btnRotateLeft->onMousePress(		static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftPress));
	_btnRotateLeft->onMouseRelease(		static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftRelease));
	_btnRotateLeft->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftPress),   Options::keyGeoLeft);
	_btnRotateLeft->onKeyboardRelease(	static_cast<ActionHandler>(&GeoscapeState::btnRotateLeftRelease), Options::keyGeoLeft);

	_btnRotateRight->onMousePress(		static_cast<ActionHandler>(&GeoscapeState::btnRotateRightPress));
	_btnRotateRight->onMouseRelease(	static_cast<ActionHandler>(&GeoscapeState::btnRotateRightRelease));
	_btnRotateRight->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnRotateRightPress),   Options::keyGeoRight);
	_btnRotateRight->onKeyboardRelease(	static_cast<ActionHandler>(&GeoscapeState::btnRotateRightRelease), Options::keyGeoRight);

	_btnRotateUp->onMousePress(		static_cast<ActionHandler>(&GeoscapeState::btnRotateUpPress));
	_btnRotateUp->onMouseRelease(	static_cast<ActionHandler>(&GeoscapeState::btnRotateUpRelease));
	_btnRotateUp->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnRotateUpPress),   Options::keyGeoUp);
	_btnRotateUp->onKeyboardRelease(static_cast<ActionHandler>(&GeoscapeState::btnRotateUpRelease), Options::keyGeoUp);

	_btnRotateDown->onMousePress(		static_cast<ActionHandler>(&GeoscapeState::btnRotateDownPress));
	_btnRotateDown->onMouseRelease(		static_cast<ActionHandler>(&GeoscapeState::btnRotateDownRelease));
	_btnRotateDown->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnRotateDownPress),   Options::keyGeoDown);
	_btnRotateDown->onKeyboardRelease(	static_cast<ActionHandler>(&GeoscapeState::btnRotateDownRelease), Options::keyGeoDown);

	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnZoomInLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomIn->onMouseClick(	static_cast<ActionHandler>(&GeoscapeState::btnZoomInRightClick), SDL_BUTTON_RIGHT);
	_btnZoomIn->onKeyboardPress(static_cast<ActionHandler>(&GeoscapeState::btnZoomInLeftClick),  Options::keyGeoZoomIn);

	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&GeoscapeState::btnZoomOutLeftClick),  SDL_BUTTON_LEFT);
	_btnZoomOut->onMouseClick(		static_cast<ActionHandler>(&GeoscapeState::btnZoomOutRightClick), SDL_BUTTON_RIGHT);
	_btnZoomOut->onKeyboardPress(	static_cast<ActionHandler>(&GeoscapeState::btnZoomOutLeftClick),  Options::keyGeoZoomOut); */

	if (Options::showFundsOnGeoscape == true)
	{
		_txtFunds->setAlign(ALIGN_CENTER);
		_txtScore->setAlign(ALIGN_CENTER);
	}
	else
	{
		_txtFunds->setVisible(false);
		_txtScore->setVisible(false);
	}

	_txtHour->setColor(GREEN_SEA);
	_txtHour->setAlign(ALIGN_RIGHT);
	_txtHour->setBig();

	_txtColon->setColor(GREEN_SEA);
	_txtColon->setText(L":");
	_txtColon->setBig();

	_txtMin->setColor(GREEN_SEA);
	_txtMin->setBig();

	_txtSec->setColor(GREEN_SEA);
	_txtSec->setText(L".");

	_txtDay->setColor(GREEN_SEA);
	_txtDay->setAlign(ALIGN_RIGHT);

	_txtMonth->setColor(GREEN_SEA);
	_txtMonth->setAlign(ALIGN_CENTER);

	_txtYear->setColor(GREEN_SEA);


	std::string st;
	switch (_playSave->getRadarDetail())
	{
		case GRD_NONE:  st = "STR_RADARS_NONE_UC";  break;
		case GRD_CRAFT: st = "STR_RADARS_CRAFT_UC"; break;
		case GRD_BASE:  st = "STR_RADARS_BASE_UC";  break;
		case GRD_ALL:   st = "STR_RADARS_ALL_UC";
	}
	_txtRadars->setText(tr("STR_RADARS_").arg(tr(st)));

	if (_playSave->isGlobeDetail() == true)
		st = "STR_ON_UC";
	else
		st = "STR_OFF_UC";
	_txtLabels->setText(tr("STR_LABELS_").arg(tr(st)));


	_trGeo->onTimer(static_cast<StateHandler>(&GeoscapeState::timeAdvance));
	_trGeo->start();

	_trDf     ->onTimer(static_cast<StateHandler>(&GeoscapeState::thinkDogfights));
	_trDfStart->onTimer(static_cast<StateHandler>(&GeoscapeState::startDogfight));
	_trDfZinn ->onTimer(static_cast<StateHandler>(&GeoscapeState::dfZoomIn));
	_trDfZout ->onTimer(static_cast<StateHandler>(&GeoscapeState::dfZoomOut));

	kL_geoMusicPlaying =
	kL_geoMusicReturnState = false;
}

/**
 * Deletes timers and clears dogfights.
 */
GeoscapeState::~GeoscapeState()
{
	delete _trGeo;
	delete _trDf;
	delete _trDfStart;
	delete _trDfZinn;
	delete _trDfZout;

	std::list<DogfightState*>::const_iterator i (_dogfights.begin());
	while (i != _dogfights.end())
	{
		(*i)->clearCraft();
		(*i)->clearUfo();

		delete *i;
		i = _dogfights.erase(i);
	}

	i = _dogfightsToStart.begin();
	while (i != _dogfightsToStart.end())
	{
		(*i)->clearCraft();
		(*i)->clearUfo();

		delete *i;
		i = _dogfightsToStart.erase(i);
	}
}

/**
 * Handle blitting of Geoscape and Dogfights.
 */
void GeoscapeState::blit()
{
	State::blit();

	for (std::list<DogfightState*>::const_iterator
			i  = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		(*i)->blit();
	}
}

/**
 * Handles keyboard-shortcuts.
 * @param action - pointer to an Action
 */
void GeoscapeState::handle(Action* action)
{
	if (inputDisabled() == false)
	{
		if (_dogfights.size() == _dfReduced)
			State::handle(action);

		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			bool beep (false);

			if (Options::debug == true
				&& (SDL_GetModState() & KMOD_CTRL) != 0)
			{
				if (action->getDetails()->key.keysym.sym == SDLK_d)						// "ctrl-d" - enable/disable debug mode
				{
					beep = true;
					if (_playSave->toggleDebugActive() == true)
						fabricateDebugPretext();
					else
					{
						_txtDebug->setText(L"");
						_stDebug = "";
					}
				}
				else if (_playSave->getDebugGeo() == true)
				{
					switch (action->getDetails()->key.keysym.sym)
					{
						case SDLK_c:													// "ctrl-c" - cycle areas
							beep = true;												// NOTE: Also handled in Game::run() where the 'cycle' is determined.
							_txtDebug->setText(L"");
							fabricateDebugPretext();
							break;

						case SDLK_a:													// "ctrl-a" - delete soldier awards
							beep = true;
							_txtDebug->setText(L"SOLDIER AWARDS DELETED");
							for (std::vector<Base*>::const_iterator						// clear Awards from living Soldiers ->
									i  = _playSave->getBases()->begin();
									i != _playSave->getBases()->end();
									++i)
							{
								for (std::vector<Soldier*>::const_iterator
										j  = (*i)->getSoldiers()->begin();
										j != (*i)->getSoldiers()->end();
										++j)
								{
									for (std::vector<SoldierAward*>::const_iterator
											k  = (*j)->getDiary()->getSoldierAwards().begin();
											k != (*j)->getDiary()->getSoldierAwards().end();
											++k)
									{
										delete *k;
									}
									(*j)->getDiary()->getSoldierAwards().clear();
								}
							}

							for (std::vector<SoldierDead*>::const_iterator				// clear Awards from dead Soldiers ->
									i  = _playSave->getDeadSoldiers()->begin();
									i != _playSave->getDeadSoldiers()->end();
									++i)
							{
								for (std::vector<SoldierAward*>::const_iterator
										j  = (*i)->getDiary()->getSoldierAwards().begin();
										j != (*i)->getDiary()->getSoldierAwards().end();
										++j)
								{
									delete *j;
								}
								(*i)->getDiary()->getSoldierAwards().clear();
							}
							break;

						case SDLK_b:													// "ctrl-b" - update soldier awards
							beep = true;
							_txtDebug->setText(L"SOLDIER AWARDS UPDATED");
							for (std::vector<Base*>::const_iterator						// update Awards for living Soldiers ->
									i  = _playSave->getBases()->begin();
									i != _playSave->getBases()->end();
									++i)
							{
								for (std::vector<Soldier*>::const_iterator
										j  = (*i)->getSoldiers()->begin();
										j != (*i)->getSoldiers()->end();
										++j)
								{
									(*j)->getDiary()->updateAwards(
																_rules,
																_playSave->getTacticalStatistics());
								}
							}

							for (std::vector<SoldierDead*>::const_iterator				// update Awards for dead Soldiers ->
									i  = _playSave->getDeadSoldiers()->begin();
									i != _playSave->getDeadSoldiers()->end();
									++i)
							{
								(*i)->getDiary()->updateAwards(
															_rules,
															_playSave->getTacticalStatistics());
							}
							break;

						case SDLK_t:
							_game->pushState(new TechTreeViewerState());				// "ctrl-t" - open TechTree viewer
							break;

						default:
							_txtDebug->setText(L"");
					}
				}
			}

			if (_playSave->isIronman() == false)										// quick-save and quick-load
			{
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)		// f6 - quickSave
				{
					beep = true;
					popupGeo(new SaveGameState(
										OPT_GEOSCAPE,
										SAVE_QUICK,
										_palette));
				}
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)	// f5 - quickLoad
				{
					beep = true;
					popupGeo(new LoadGameState(
										OPT_GEOSCAPE,
										_palette));
				}
			}

#ifdef _WIN32
			if (beep == true) MessageBeep(MB_OK);
#endif
		}

		if (_dogfights.empty() == false)
		{
			for (std::list<DogfightState*>::const_iterator
					i  = _dogfights.begin();
					i != _dogfights.end();
					++i)
			{
				(*i)->handle(action);
			}
			_dfReduced = getReducedDogfights();
		}
	}
}

/**
 * Creates the prefix for a debugging message.
 */
void GeoscapeState::fabricateDebugPretext() // private.
{
	_stDebug = "DEBUG MODE : ";
	switch (_globe->getDebugType())
	{
		case DTG_COUNTRY: _stDebug += "country : "; break;
		case DTG_REGION:  _stDebug += "region : ";  break;
		case DTG_ZONE:    _stDebug += "zones : ";
	}
}

/**
 * Updates the timer display and resets the palette and checks for/ plays
 * geoscape music since these are bound to change on other screens.
 * @note Also sets up funds and missions at game start.
 */
void GeoscapeState::init()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "GeoscapeState::init()";
	State::init();

	_timeSurplus = 0;
	updateTimeDisplay();

	_globe->onMouseClick(static_cast<ActionHandler>(&GeoscapeState::globeClick));
	_globe->onMouseOver(nullptr);
//	_globe->rotateStop();

//	_globe->setFocus();	// -> NOTE: InteractiveSurface is initialized w/ (_isFocused=TRUE)
						// and only State::resetSurfaces() sets a Surface unfocused; resetSurfaces()
						// is called from Game::run() on each state-initialization, but
						// that's back-asswards. So (_isFocused=FALSE) has been disabled
						// in resetSurfaces() .... See also BattlescapeState::init().
	_globe->draw();

	if (kL_geoMusicPlaying == true)
	{
		std::string track;
		if (_dogfights.empty() == false
			|| _trDfStart->isRunning() == true)
		{
			track = OpenXcom::res_MUSIC_GEO_INTERCEPT;
		}
		else
			track = OpenXcom::res_MUSIC_GEO_GLOBE;

		if (_game->getResourcePack()->isMusicPlaying(track) == false)
		{
			_game->getResourcePack()->fadeMusic(_game, 425);
			_game->getResourcePack()->playMusic(track);
		}
	}

	if (_playSave->getMonthsElapsed() == -1							// run once
		&& _playSave->getBases()->empty() == false					// as long as there's a base
		&& _playSave->getBases()->front()->isBasePlaced() == true)	// THIS prevents missions running prior to the first base being placed.
	{
		_playSave->elapseMonth();
		_playSave->setFunds(_playSave->getFunds() - static_cast<int64_t>(_playSave->getBases()->front()->getMonthlyMaintenace()));

		deterAlienMissions();
	}
}

/**
 * Runs the state-timers and handles popups.
 */
void GeoscapeState::think()
{
	//Log(LOG_INFO) << "GeoscapeState::think()";
	State::think();

	_trDfStart->think(this, nullptr);
	_trDfZinn ->think(this, nullptr);
	_trDfZout ->think(this, nullptr);

	if (_popups.empty() == false)
	{
		//Log(LOG_INFO) << ". (_popups.empty() == false)";
		_globe->rotateStop();

		_game->pushState(_popups.front());
		_popups.erase(_popups.begin());
	}
	else if (_dogfights.empty() == false)
	{
		//Log(LOG_INFO) << ". (_dogfights.empty() == false)";
		_trDf->think(this, nullptr);			// call thinkDogfights()

		if (_dogfights.size() == _dfReduced)	// if all dogfights are iconized rotate the Globe, etc.
		{
			//Log(LOG_INFO) << ". . CALL _trGeo->think() ie. timeAdvance()";
			_pause = false;
			_trGeo->think(this, nullptr);		// call timeAdvance()
		}
	}
	else if (_trDfZinn->isRunning() == false
		&&   _trDfZout->isRunning() == false)
	{
		//Log(LOG_INFO) << ". CALL _trGeo->think() ie. timeAdvance()";
		_trGeo->think(this, nullptr);			// call timeAdvance()
	}

	if (Options::debug == true
		&& _playSave->getDebugArgDone() == true // do not write info until Globe actually sets it.
		&& _stDebug.compare(0u,5u, "DEBUG") == 0)
	{
		const std::string stDebug (_stDebug + _playSave->getDebugArg());
		_txtDebug->setText(Language::fsToWstr(stDebug));
	}
}

/**
 * Draws the UFO indicators for known UFOs.
 * @note This is called by time5Seconds().
 */
void GeoscapeState::drawUfoBlobs()
{
	for (size_t
			i  = 0u;
			i != UFO_HOTBLOBS;
			++i)
	{
		_isfUfoBlobs[i]->setVisible(false);
		_numUfoBlobs[i]->setVisible(false);
		_hostileUfos[i] = nullptr;
	}

	size_t
		j (0u),
		radius;
	Uint8
		color,
		shift;

	for (std::vector<Ufo*>::const_iterator
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end() && j != UFO_HOTBLOBS;
			++i)
	{
		if ((*i)->getDetected() == true)
		{
			_isfUfoBlobs[j]->setVisible();
			_numUfoBlobs[j]->setVisible();
			_numUfoBlobs[j]->setValue(static_cast<unsigned>((*i)->getId()) % 1000); // truncate to 3 least significant digits

			_hostileUfos[j] = *i;

			radius = (*i)->getRules()->getRadius();

			if ((*i)->getTicked() == true)
				color = RED; // TODO: blink
			else
			{
				switch ((*i)->getUfoStatus())
				{
					default:
					case Ufo::FLYING:  color = SLATE_D; break;
					case Ufo::CRASHED: color = BROWN;   break;
					case Ufo::LANDED:  color = GREEN;
				}
			}

			for (size_t
					y = 0u;
					y != BLOBSIZE;
					++y)
			{
				for (size_t
						x = 0u;
						x != BLOBSIZE;
						++x)
				{
					shift = static_cast<Uint8>(_ufoBlobs[radius][y][x]);
					if (shift != 0u)
						_isfUfoBlobs[j]->setPixelColor(
													static_cast<int>(x),
													static_cast<int>(y),
													static_cast<Uint8>(color - shift));
					else
						_isfUfoBlobs[j]->setPixelColor(
													static_cast<int>(x),
													static_cast<int>(y),
													0u);
				}
			}
			++j;
		}
	}
}

/**
 * Advances time on the Geoscape according to the set time-compression interval
 * and calls the necessary triggers.
 * @note The game always advances in 5-sec cycles regardless of the compression
 * otherwise this will skip important steps. Instead it just keeps advancing
 * time at the lowest compression until the interval is done - eg. the next day
 * on 1-day speed - or until an event occurs.
 * @note The following routine incorporates Volutar's smooth-globe tweak, which
 * advances time in 1-sec cycles (but I'm not sure what difference it makes).
 */
void GeoscapeState::timeAdvance() // private.
{
	//Log(LOG_INFO) << "GeoscapeState::timeAdvance()";
	if (_pauseHard == false)
	{
		int interval;

		if      (_btnGroup == _btn1Day)   interval = 12 * 5 * 6 * 2 * 24;
		else if (_btnGroup == _btn1Hour)  interval = 12 * 5 * 6 * 2;
		else if (_btnGroup == _btn30Mins) interval = 12 * 5 * 6;
		else if (_btnGroup == _btn5Mins)  interval = 12 * 5;
		else if (_btnGroup == _btn1Min)   interval = 12;
		else                              interval = 1; // (_btnGroup == _btn5Secs)

		// interval = true one-second intervals (interval * 5 = minimum time-compression)
		interval = (interval * 5 + _timeSurplus) << 2u;

		_timeSurplus = interval % Options::geoClockSpeed;

		if ((interval /= Options::geoClockSpeed) != 0)
		{
			//Log(LOG_INFO) << ". tick";
			bool update (false);
			for (int
					i = 0;
					i != interval && _pause == false;
					++i)
			{
				const TimeTrigger trigger (_playSave->getTime()->advance());
				if (trigger != TIME_1SEC)
				{
					update = true;
					switch (trigger)
					{
						case TIME_1MONTH: time1Month(); // no breaks ->
						case TIME_1DAY:   time1Day();
						case TIME_1HOUR:  time1Hour();
						case TIME_30MIN:  time30Minutes();
						case TIME_10MIN:  time10Minutes();
						case TIME_5SEC:   time5Seconds();
					}
				}
			}

			if (update == true) updateTimeDisplay();
		}
		_pause = _dogfightsToStart.empty() == false
			  || _trDfZinn->isRunning() == true
			  || _trDfZout->isRunning() == true;

		_globe->draw();
	}
	else // pause hard
	{
		_txtSec->setColor(RED); // NOTE: Does not look red but it will do.
		_txtSec->setVisible();
	}
}

/**
 * Updates the Geoscape clock.
 * @note Also updates the player's current score.
 */
void GeoscapeState::updateTimeDisplay() // private.
{
	_txtFunds->setText(Text::formatCurrency(_playSave->getFunds()));

	if (_playSave->getMonthsElapsed() != -1) // update Player's current score
	{
		const size_t id (_playSave->getFundsList().size() - 1u); // use fundsList to determine which entries in other vectors to use for the current month.

		int score (_playSave->getResearchScores().at(id));
		for (std::vector<Region*>::const_iterator
				i  = _playSave->getRegions()->begin();
				i != _playSave->getRegions()->end();
				++i)
		{
			score += (*i)->getActivityXCom().at(id) - (*i)->getActivityAlien().at(id);
		}

		if (_score == 0 || _score != score)
		{
			std::wstring wst;
			if (_score != 0)
			{
				const int delta (score - _score);
				if (delta == 0)
					wst = L"";
				else if (delta > 0)
					wst = L" +" + Text::intWide(delta);
				else
					wst = L" " + Text::intWide(delta);
			}
			_wstScore = Text::intWide(score) + wst;
		}

		_txtScore->setText(_wstScore);
		_score = score;
	}
	else
		_txtScore->setText(Text::intWide(0));


	if (_btnGroup == _btn5Secs)
	{
		const int sec (_playSave->getTime()->getSecond());
		_txtSec->setVisible(sec % 15 > 9);
	}
	else
		_txtSec->setVisible();

	std::wostringstream
		woststr1,
		woststr2;

	woststr1 << std::setfill(L'0') << std::setw(2) << _playSave->getTime()->getMinute();
	_txtMin->setText(woststr1.str());

	woststr2 << std::setfill(L'0') << std::setw(2) << _playSave->getTime()->getHour();
	_txtHour->setText(woststr2.str());

	int date;
	if ((date = _playSave->getTime()->getDay()) != _day)
	{
		_txtDay->setText(Text::intWide(_day = date));
		if ((date = _playSave->getTime()->getMonth()) != _month)
		{
			_txtMonth->setText(convertDateToMonth(_month = date));
			if ((date = _playSave->getTime()->getYear()) != _year)
				_txtYear->setText(Text::intWide(_year = date));
		}
	}
}

/**
 * Converts the date to a month string.
 * @param date - the date
 * @return, the month as a wide-string
 */
std::wstring GeoscapeState::convertDateToMonth(int date) // private.
{
	switch (date)
	{
		case  1: return L"jan";
		case  2: return L"feb";
		case  3: return L"mar";
		case  4: return L"apr";
		case  5: return L"may";
		case  6: return L"jun";
		case  7: return L"jul";
		case  8: return L"aug";
		case  9: return L"sep";
		case 10: return L"oct";
		case 11: return L"nov";
		case 12: return L"dec";
	}
	return L"error";
}

/**
 * Takes care of any game logic that has to run every 5 seconds.
 */
void GeoscapeState::time5Seconds()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "GeoscapeState::time5Seconds()";
	if (_playSave->getBases()->empty() == true) // Game Over if there are no more bases.
	{
		_playSave->setEnding(END_LOSE);
		popupGeo(new DefeatState());
		if (_playSave->isIronman() == true)
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		return;
	}

	const Ufo* ufoExpired (nullptr); // kL, see below_


	for (std::vector<Ufo*>::const_iterator // handle UFO logic
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			++i)
	{
		//Log(LOG_INFO) << ". ufo= " << Language::wstrToFs((*i)->getLabel(_game->getLanguage()));
		//Log(LOG_INFO) << ". PAUSE HARD";
		//setPaused(); // TEST.

		switch ((*i)->getUfoStatus())
		{
			case Ufo::FLYING:
				//Log(LOG_INFO) << ". . status FLYING";
				(*i)->stepTarget();

				if ((*i)->reachedDestination() == true)
				{
					//Log(LOG_INFO) << ". . . . ufo reached destination -> handle it.";
					(*i)->setSpeed();

					const size_t terrors (_playSave->getTerrorSites()->size());
					const bool detected ((*i)->getDetected());

					AlienMission* const mission ((*i)->getAlienMission());
					mission->ufoReachedWaypoint(**i, *_rules, *_globe); // recalcs 'terrors' & 'detected'; also sets UFO's status

//					if ((*i)->getAltitude() == MovingTarget::stAltitude[0u])
					if ((*i)->getUfoStatus() == Ufo::LANDED)
					{
						_dfZout = false;
						for (std::list<DogfightState*>::const_iterator
								j  = _dogfights.begin();
								j != _dogfights.end();
								++j)
						{
							if ((*j)->getUfo() != *i) // huh, sometimes I wonder what the hell i code.
							{
								_dfZout = true;
								break;
							}
						}
					}

					if (detected != (*i)->getDetected()
						&& (*i)->getTargeters()->empty() == false
						&& !
							((*i)->getTrajectory().getType() == UfoTrajectory::XCOM_BASE_ASSAULT
								&& (*i)->getUfoStatus() == Ufo::LANDED))
					{
						resetTimer();
						popupGeo(new UfoLostState((*i)->getLabel(_game->getLanguage())));
					}

					if (terrors < _playSave->getTerrorSites()->size()) // new TerrorSite appeared when UFO reached waypoint, above^
					{
						TerrorSite* const site (_playSave->getTerrorSites()->back());
						site->setDetected();

						resetTimer();
						popupGeo(new TerrorDetectedState(site, this));
					}

					if ((*i)->getUfoStatus() == Ufo::DESTROYED) // if UFO was destroyed don't spawn missions
						return;

					Base* const base (dynamic_cast<Base*>((*i)->getTarget()));
					if (base != nullptr)
					{
						resetTimer();

						mission->resetCountdown();
						(*i)->setTarget();

						if (base->setupBaseDefense() == true)
							popupGeo(new BaseDefenseState(base, *i, this));
							// should/could this Return;
						else
						{
							baseDefenseTactical(base, *i);
							return;
						}
					}
				}
				break;

			case Ufo::LANDED:
				//Log(LOG_INFO) << ". . status LANDED";
				if ((*i)->reduceSecondsLeft() == true)
				{
					//Log(LOG_INFO) << ". 0 seconds left -> mission UFOLIFTING";
					AlienMission* const mission ((*i)->getAlienMission());
					const bool detected ((*i)->getDetected());
					mission->ufoLifting(**i, *_rules, *_globe);

					if (detected != (*i)->getDetected()
						&& (*i)->getTargeters()->empty() == false)
					{
						resetTimer();
						popupGeo(new UfoLostState((*i)->getLabel(_game->getLanguage())));
					}
				}
				//else Log(LOG_INFO) << ". has seconds remaining";
				break;

			case Ufo::CRASHED:
				//Log(LOG_INFO) << ". . status CRASHED";
				(*i)->setDetected();

				if ((*i)->getSecondsLeft() == 0)
				{
					ufoExpired = *i; // shot down while trying to outrun interceptor
					(*i)->setUfoStatus(Ufo::DESTROYED);
				}
				//break;

			//case Ufo::DESTROYED:
				//Log(LOG_INFO) << ". . status DESTROYED";
		}
	}

	bool initDfMusic (false);


	for (std::vector<Base*>::const_iterator // handle Craft logic
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j  = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				)
		{
			//Log(LOG_INFO) << ". craft= " << Language::wstrToFs((*j)->getLabel(_game->getLanguage()));
			if ((*j)->getCraftStatus() == CS_OUT) // Craft is airborne -> handle it
			{
				if ((*j)->isDestroyed() == true)
				{
					_playSave->scorePoints(
									(*j)->getLongitude(),
									(*j)->getLatitude(),
									(*j)->getRules()->getScore(),
									true);

					if ((*j)->getRules()->getSoldierCapacity() != 0) // if a transport craft has been shot down all soldiers aboard are dead.
					{
						for (std::vector<Soldier*>::const_iterator
								k  = (*i)->getSoldiers()->begin();
								k != (*i)->getSoldiers()->end();
								)
						{
							if ((*k)->getCraft() == *j)
							{
								(*k)->die(_playSave);
								delete *k;
								k = (*i)->getSoldiers()->erase(k);
							}
							else
								++k;
						}
					}
					delete *j;
					j = (*i)->getCrafts()->erase(j);
				}
				else // craft okay.
				{
					if ((*j)->getTarget() != nullptr)
					{
						// NOTE: This has to deal with Landed UFOs, Crashed UFOs, UFOs spawning TerrorSites
						// and UFOs that are on BaseAssault runs starting BaseDefense tacticals, etc etc.

						const Ufo* const ufo (dynamic_cast<Ufo*>((*j)->getTarget()));
						if (ufo != nullptr
							&& ufo->getDetected() == false	// lost radar contact
							&& ufo != ufoExpired)			// <- ie. not recently shot down while trying to outrun interceptor but it crashed into the sea instead Lol
						{									//      - perhaps not needed since Dogfight::think() has been tightened up.
							resetTimer();

							Waypoint* const wp (new Waypoint());
							wp->setLongitude((*j)->getMeetLongitude());
							wp->setLatitude((*j)->getMeetLatitude());
							wp->setId(ufo->getId());

							// NOTE: The Waypoint is the reconnaissance destination-target;
							// it also flags GeoscapeCraftState as a special instance.
							// NOTE: Do not null the Craft's destination; its dest-coords
							// will be used to position the targeter in GeoscapeCraftState.

							//Log(LOG_INFO) << ". . call GeoscapeCraftState()";
							popupGeo(new GeoscapeCraftState(*j, this, wp));
						}
					}

					(*j)->think();

					if ((*j)->reachedDestination() == true)
					{
						if (dynamic_cast<Waypoint*>((*j)->getTarget()) != nullptr)	// patrol at waypoint.
						{															// NOTE: This will also handle target-UFOs that just vanished.
							popupGeo(new CraftPatrolState(*j, this));
							(*j)->setTarget();
						}
						else
						{
							Ufo* const ufo (dynamic_cast<Ufo*>((*j)->getTarget()));
							if (ufo != nullptr)
							{
								switch (ufo->getUfoStatus())
								{
									case Ufo::FLYING:
										(*j)->interceptGroundTarget(false);

										if ((*j)->inDogfight() == false // Craft ran into a UFO ->
											&& (*j)->getRules()->getTopSpeed() >= ufo->getSpeed()
											&& _dogfights.size() + _dogfightsToStart.size() < 4u // no more than 4 interception-windows at once.
											&& AreSame((*j)->getDistance(ufo), 0.) == true) // check the distance (~again) might be unnecessary.
										{
											_dogfightsToStart.push_back(new DogfightState(_globe, *j, ufo, this));
//											if (_tmrDfStart->isRunning() == false)	// what are the odds that 2 Craft run into UFO(s) on the same think()
											if (_pause == false)					// <- use that instead.
											{
												// Actually it's not so obscure that 2+ Craft run this when a game
												// that was saved in a multi-interception dogfight is reloaded.
												_pause = true;
												resetTimer();

												// NOTE: In the obscene case that 2 UFOs get intercepted (ie. at different
												// coords on the globe) on the same think() only the first sets the PDC.
												_playSave->setPreDogfightCoords(_globe->getZoom());	// store current Globe coords & zoom;
												_globe->center(										// Globe will reset to these after dogfight ends (unless CCC is set)
															(*j)->getLongitude(),
															(*j)->getLatitude());

												if (_dogfights.empty() == true // first dogfight, start music unless reloading to another dogfight
													&& _game->getResourcePack()->isMusicPlaying(OpenXcom::res_MUSIC_GEO_INTERCEPT) == false)
												{
													_game->getResourcePack()->fadeMusic(_game, 425);
												}
											}

											// NOTE: the following call has to be outside of above scope or else 2nd
											// and following dogfights won't be added to the dogfights if the game
											// was saved during a multiple-interception at the most-zoomed-in level.
											// See function for details:
											startDogfight();

											initDfMusic = true;
											_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_INTERCEPT);
										}
										break;

									case Ufo::LANDED:	// TODO: setSpeed 1/2 (need to speed up to full if UFO takes off)
									case Ufo::CRASHED:	// TODO: setSpeed 1/2 (need to speed back up when setting a new destination)
										if ((*j)->inDogfight() == false
											&& (*j)->interceptGroundTarget() == false	// <- non-transport craft shall be set true to case the joint but can't start tactical
											&& (*j)->isTacticalReturn() == false)		// TODO: just use getQtySoldiers() ....
										{
											resetTimer();

											int // look up polygon's texId + shade
												texId,
												shade;
											_globe->getPolygonTextureAndShade(
																			ufo->getLongitude(),
																			ufo->getLatitude(),
																			&texId, &shade);
											popupGeo(new ConfirmLandingState(
																	*j, // countryside Texture; choice of Terrain made in ConfirmLandingState
																	_rules->getGlobe()->getTextureRule(texId),
																	shade,
																	(*j)->getQtySoldiers() != 0));
										}
										break;

									case Ufo::DESTROYED: // just before expiration
										popupGeo(new CraftPatrolState(*j, this));
										(*j)->setTarget();

//										if ((*j)->getQtySoldiers() != 0)
//										{
//											if ((*j)->inDogfight() == false)
//											{
//												resetTimer();
//												int // look up polygon's texId + shade
//													texId,
//													shade;
//												_globe->getPolygonTextureAndShade(
//																				ufo->getLongitude(),
//																				ufo->getLatitude(),
//																				&texId, &shade);
//												popup(new ConfirmLandingState(
//																		*j, // countryside Texture; choice of Terrain made in ConfirmLandingState
//																		_rules->getGlobe()->getTextureRule(texId),
//																		shade));
//											}
//										}
//										else if (ufo->getUfoStatus() != Ufo::LANDED)
//										{
//											popup(new CraftPatrolState(*j, this));
//											(*j)->setTarget();
//										}
								}
							}
							else
							{
								const TerrorSite* const site (dynamic_cast<TerrorSite*>((*j)->getTarget()));
								if (site != nullptr)
								{
									const int texId (site->getSiteTextureId());
									int shade;
									_globe->getPolygonShade(
														site->getLongitude(),
														site->getLatitude(),
														&shade);
									popupGeo(new ConfirmLandingState( // preset terrorSite Texture; choice of Terrain made via texture-deployment in ConfirmLandingState
																*j,
																_rules->getGlobe()->getTextureRule(texId),
																shade,
																(*j)->getQtySoldiers() != 0));
								}
								else // aLienBase
									popupGeo(new ConfirmLandingState( // choice of Terrain made in BattlescapeGenerator.
																*j,
																nullptr,
																-1,
																(*j)->getQtySoldiers() != 0));
							}
						}
					}
					++j;
				}
			}
			else // craft is based - bypass it.
				++j;
		}
	}

	for (std::vector<Ufo*>::const_iterator // Clean up dead UFOs and end Dogfights that were minimized.
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			)
	{
		switch ((*i)->getUfoStatus())
		{
			case Ufo::DESTROYED:
				if ((*i)->getTargeters()->empty() == false)
				{
					for (std::list<DogfightState*>::const_iterator // Remove all Dogfights with this UFO.
							j  = _dogfights.begin();
							j != _dogfights.end();
							)
					{
						if ((*j)->getUfo() == *i)
						{
							delete *j;
							j = _dogfights.erase(j);
						}
						else
							++j;
					}
					resetInterceptPorts();
				}

				delete *i;
				i = _playSave->getUfos()->erase(i);
				break;

			case Ufo::FLYING:
			case Ufo::LANDED:
			case Ufo::CRASHED:
				++i;
		}
	}

	drawUfoBlobs();


	// This is ONLY for allowing _dogfights to fill (or not) before deciding whether
	// to startMusic in init() - and ONLY for Loading with a Dogfight in progress:
	// But now it's also used for resuming Geoscape music on returning from another state ....
	if (kL_geoMusicPlaying == false)
	{
		if (++_delayMusicDfCheck > 3	// this is because initDfMusic takes uh 2+ passes before the _dogfight vector fills.
			|| initDfMusic == true		// and if it doesn't fill by that time I want some music playing.
			|| kL_geoMusicReturnState == true)
		{
			kL_geoMusicPlaying = true;	// if there's a Dogfight then dogfight-track will play when a save is loaded

			if (_dogfights.empty() == true
				&& _trDfStart->isRunning() == false)
			{
				_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
			}
		}
	}
}

/**
 ** FUNCTOR ***
 * Functor that attempts to detect an XCOM Base.
 */
struct DetectXCOMBase
	:
		public std::unary_function<Ufo*, bool>
{
private:
	const int _diff;
	const Base& _base;

	public:
		/**
		 * Creates a detector for a specified Base.
		 * @param base - reference to the Base
		 * @param diff - difficulty-level
		 */
		DetectXCOMBase(
				const Base& base,
				int diff)
			:
				_base(base),
				_diff(diff)
		{}

		/**
		 * Attempts detection of a Base.
		 * @param ufo - pointer to the UFO trying to detect
		 */
		bool operator ()(const Ufo* const ufo) const;
};

/**
 * Only UFOs within detection range of the base have a chance to detect it.
 * @param ufo - pointer to the UFO attempting detection
 * @return, true if base detected
 */
bool DetectXCOMBase::operator ()(const Ufo* const ufo) const
{
	if (ufo->isCrashed() == false
//		&& ufo->getMissionPoint() > 1u
		&& ufo->getMissionPoint() != 0u
		&& ufo->getTrajectory().getZoneId(ufo->getMissionPoint()) != 5u
		&& ufo->getTrajectory().getType() != UfoTrajectory::XCOM_BASE_ASSAULT
		&& (ufo->getAlienMission()->getRules().getObjectiveType() == alm_RETAL
			|| Options::aggressiveRetaliation == true))
	{
		const double
			range (static_cast<double>(ufo->getRules()->getRangeRecon())),
			dist  (_base.getDistance(ufo) * radius_earth);

		if (dist <= range)
		{
			const double inverseFactor (dist * 12. / range); // TODO: Use log() ....
			int pct (static_cast<int>(Round(
					 static_cast<double>(_base.getExposedChance(_diff) + ufo->getDetectors()) / inverseFactor)));

			if (ufo->getAlienMission()->getRules().getObjectiveType() == alm_RETAL
				&& Options::aggressiveRetaliation == true) // Player wants *aggressive* retaliation search.
			{
				pct += 3 + _diff;
			}

			return RNG::percent(pct);
		}
	}
	return false;
}


/**
 ** FUNCTOR ***
 * Functor that marks an XCOM Base for retaliation.
 * @note This is required because of the iterator type. Only used if Aggressive
 * Retaliation option is false.
 */
struct SetRetaliationStatus
	:
		public std::unary_function<std::map<const Region*, Base*>::value_type, void>
{
	/**
	 * Mark a Base as a valid/exposed retaliation target.
	 * @param i - reference to an iterator
	 */
	void operator ()(const argument_type& i) const
	{
		i.second->setBaseExposed();
	}
};


/**
 * Takes care of any game logic that runs every ten minutes.
 */
void GeoscapeState::time10Minutes()
{
	//Log(LOG_INFO) << "GeoscapeState::time10Minutes()";
	const int diff (_playSave->getDifficultyInt());

	for (std::vector<Base*>::const_iterator
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j  = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			if ((*j)->getCraftStatus() == CS_OUT && (*j)->hasLeftGround() == true)
			{
				if ((*j)->useFuel() == true)
					popupGeo(new LowFuelState(*j, this));

//				if ((*j)->getTarget() == nullptr) // Remove that: patrolling for aBases/10min was getting too bothersome.
//				{
				//Log(LOG_INFO) << ". Patrol for alienBases";
				for (std::vector<AlienBase*>::const_iterator // patrol for aLien bases.
						k  = _playSave->getAlienBases()->begin();
						k != _playSave->getAlienBases()->end();
						++k)
				{
					if ((*k)->isDetected() == false)
					{
						const double
							range (static_cast<double>((*j)->getRules()->getRangeRecon())),
							dist  ((*j)->getDistance(*k) * radius_earth);
						//Log(LOG_INFO) << ". . range = " << (int)range;
						//Log(LOG_INFO) << ". . dist = "  << (int)dist;

						if (dist <= range)
						{
							const int pct (100 - (diff * 10) - static_cast<int>(dist * 50. / range));
							//Log(LOG_INFO) << ". . . craft in Range pct= " << pct;
							if (RNG::percent(pct) == true)
							{
								//Log(LOG_INFO) << ". . . . aLienBase discovered";
								resetTimer();
								popupGeo(new AlienBaseDetectedState(*k, this));
							}
						}
					}
				}
//				}
			}
		}
	}

	if (Options::aggressiveRetaliation == true)
	{
		for (std::vector<Base*>::const_iterator // detect as many Bases as possible.
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
		{
			if ((*i)->getBaseExposed() == false)
			{
				std::vector<Ufo*>::const_iterator ufo (std::find_if( // find a UFO that detected this Base if any.
																_playSave->getUfos()->begin(),
																_playSave->getUfos()->end(),
																DetectXCOMBase(**i, diff)));
				if (ufo != _playSave->getUfos()->end())
					(*i)->setBaseExposed();
			}
		}
	}
	else
	{
		std::map<const Region*, Base*> discovered;
		for (std::vector<Base*>::const_iterator // remember only last Base in each region.
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
		{
			std::vector<Ufo*>::const_iterator ufo (std::find_if( // find a UFO that detected this Base if any.
															_playSave->getUfos()->begin(),
															_playSave->getUfos()->end(),
															DetectXCOMBase(**i, diff)));
			if (ufo != _playSave->getUfos()->end())
				discovered[_playSave->locateRegion(**i)] = *i;
		}
		std::for_each( // mark the Base(s) as discovered.
					discovered.begin(),
					discovered.end(),
					SetRetaliationStatus());
	}


	_ufosDetected = 0;

	for (std::vector<Ufo*>::const_iterator // handle UFO detection
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			++i)
	{
		//Log(LOG_INFO) << "\nufo= " << (*i)->getRules()->getType();
		switch ((*i)->getUfoStatus())
		{
			case Ufo::FLYING:
			case Ufo::LANDED:
			{
				std::vector<Base*> hyperBases; // = std::vector<Base*>();

				const bool contact0 ((*i)->getDetected());
				const bool hyper0   ((*i)->getHyperdecoded());
				//Log(LOG_INFO) << ". contact0= " << contact0;
				//Log(LOG_INFO) << ". hyper0= "   << hyper0;
				bool
					contact (false),
					hyper   (false);

				for (std::vector<Base*>::const_iterator
						j  = _playSave->getBases()->begin();
						j != _playSave->getBases()->end();
						++j)
				{
					switch ((*j)->detect(*i))
					{
						case DT_HYPERDETECTED: // both standard and hyper detected
							contact = true;
							// no break;
						case DT_HYPERDECODED: // hyper detected only
							hyper = true;

							if (hyper0 == false)
								hyperBases.push_back(*j);
							break;

						case DT_DETECTED: // standard detected only
							contact = true;
					}

					if (contact == false)
					{
						for (std::vector<Craft*>::const_iterator
								k  = (*j)->getCrafts()->begin();
								k != (*j)->getCrafts()->end();
								++k)
						{
							if ((*k)->getCraftStatus() == CS_OUT
								&& (*k)->hasLeftGround() == true
								&& (*k)->detect(*i) == true)
							{
								contact = true;
								break;
							}
						}
					}
				}
				//Log(LOG_INFO) << ". contact= " << contact;
				//Log(LOG_INFO) << ". hyper= "   << hyper;

				(*i)->setDetected(contact);
				(*i)->setHyperdecoded(hyper);

				if (contact0 == false)
				{
					if (contact == true
						|| (hyper0 == false && hyper == true))
					{
						++_ufosDetected;
						popupGeo(new UfoDetectedState(
												*i,
												this,
												true,
												hyper,
												contact,
												&hyperBases));
					}
				}
				else // ufo is already detected
				{
					if (hyper0 == false && hyper == true)
					{
						++_ufosDetected;
						popupGeo(new UfoDetectedState(
												*i,
												this,
												false,
												hyper,
												contact,
												&hyperBases));
					}

					if (contact == false
						&& (*i)->getTargeters()->empty() == false)
					{
						//Log(LOG_INFO) << ". . CALL UfoLostState()";
						resetTimer();
						popupGeo(new UfoLostState((*i)->getLabel(_game->getLanguage())));
					}
				}
			}
		}
	}


	if (_ufosDetected != 0)
	{
		_ufoDetected->setText(Text::intWide(static_cast<int>(_ufosDetected)));
		_ufoDetected->setVisible();
	}
}


/**
 ** FUNCTOR ***
 * @brief Call AlienMission::think() with proper parameters.
 * @note This function object calls AlienMission::think().
 */
struct CallThink
	:
		public std::unary_function<AlienMission*, void>
{
private:
	Game& _game;
	const Globe& _globe;

	public:
		/**
		 * Cache the parameters.
		 * @param game	- reference to the Game engine
		 * @param globe	- reference to the Globe object
		 */
		CallThink(
				Game& game,
				const Globe& globe)
			:
				_game(game),
				_globe(globe)
		{}

		/**
		 * Call AlienMission::think() with cached parameters.
		 * @param mission - pointer to an AlienMission
		 */
		void operator ()(AlienMission* const mission) const
		{
			mission->think(_game, _globe);
		}
};

/**
 ** FUNCTOR ***
 * Advance time for crashed UFOs.
 * @note This function object will decrease the expiration timer for crashed UFOs.
 */
struct ExpireCrashedUfo
	:
		public std::unary_function<Ufo*, void>
{
	/**
	 * Decreases UFO expiration timer.
	 * @param ufo - pointer to a crashed UFO
	 */
	void operator ()(Ufo* const ufo) const
	{
		if (ufo->getUfoStatus() == Ufo::CRASHED)
		{
			const int sec (ufo->getSecondsLeft());
			if (sec >= 30 * 60)
				ufo->setSecondsLeft(sec - 30 * 60);
			else
				ufo->setUfoStatus(Ufo::DESTROYED);	// mark expired UFO for removal.
		}											// wait that is also done in time5Seconds()
	}
};


/**
 * Takes care of any game logic that has to run every half-hour.
 */
void GeoscapeState::time30Minutes()
{
	//Log(LOG_INFO) << "GeoscapeState::time30Minutes()";
	std::for_each( // decrease mission countdowns
			_playSave->getAlienMissions().begin(),
			_playSave->getAlienMissions().end(),
			CallThink(*_game, *_globe));

	for (std::vector<AlienMission*>::const_iterator // remove finished missions
			i = _playSave->getAlienMissions().begin();
			i != _playSave->getAlienMissions().end();
			)
	{
		//Log(LOG_INFO) << ". aL mission type= " << (*i)->getRules().getType();
		if ((*i)->isOver() == true)
		{
			//Log(LOG_INFO) << ". . Over";
			delete *i;
			i = _playSave->getAlienMissions().erase(i);
		}
		else
			++i;
	}

	std::for_each( // handle crashed UFOs expiration
			_playSave->getUfos()->begin(),
			_playSave->getUfos()->end(),
			ExpireCrashedUfo());


	for (std::vector<Base*>::const_iterator // handle Craft maintenance.
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		for (std::vector<Craft*>::const_iterator
				j  = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
		{
			switch ((*j)->getCraftStatus())
			{
				case CS_REFUELLING:
				{
					const std::string refuelItem ((*j)->getRules()->getRefuelItem());

					if (refuelItem.empty() == true)
						(*j)->refuel();
					else
					{
						if ((*j)->getRules()->getRefuelRate() <= (*i)->getStorageItems()->getItemQuantity(refuelItem))
						{
							(*j)->refuel();
							(*i)->getStorageItems()->removeItem(
															refuelItem,
															(*j)->getRules()->getRefuelRate());
						}
						else if ((*j)->getWarned() == false)
						{
							(*j)->setWarned();
							(*j)->setWarning(CW_CANTREFUEL);
							const std::wstring wst (tr("STR_NOT_ENOUGH_ITEM_TO_REFUEL_CRAFT_AT_BASE_")
														.arg(tr(refuelItem))
														.arg((*j)->getLabel(_game->getLanguage()))
														.arg((*i)->getLabel()));
							popupGeo(new CraftErrorState(this, wst));
						}
					}
					break;
				}
				case CS_REARMING:
				{
					const std::string rearmClip ((*j)->rearm(_rules));

					if (rearmClip.empty() == false
						&& (*j)->getWarned() == false)
					{
						(*j)->setWarned();
						const std::wstring wst (tr("STR_NOT_ENOUGH_ITEM_TO_REARM_CRAFT_AT_BASE_")
													.arg(tr(rearmClip))
													.arg((*j)->getLabel(_game->getLanguage()))
													.arg((*i)->getLabel()));
						popupGeo(new CraftErrorState(this, wst));
					}
					break;
				}
				case CS_REPAIRS:
					(*j)->repair();
			}

			if ((*j)->showReady() == true)
				popupGeo(new CraftReadyState(
										this, *j,
										tr("STR_CRAFT_READY_")
											.arg((*i)->getLabel())
											.arg((*j)->getLabel(_game->getLanguage()))));
		}
	}


	if (_playSave->getUfos()->empty() == false)
		scoreUfos(false);

	for (std::vector<TerrorSite*>::const_iterator
			i  = _playSave->getTerrorSites()->begin();
			i != _playSave->getTerrorSites()->end();
			)
	{
		if (processTerrorSite(*i) == true)
			i = _playSave->getTerrorSites()->erase(i);
		else
			++i;
	}
}

/**
 * Scores points for UFOs that are Flying/Landed or Crashed.
 * @param hour - true if on the hour, false for the half-hour
 */
void GeoscapeState::scoreUfos(bool hour) const // private.
{
	const int basic (((_playSave->getMonthsElapsed() + 2) >> 2u) // basic score
					 + _playSave->getDifficultyInt());
	int score;
	for (std::vector<Ufo*>::const_iterator
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			++i)
	{
		switch ((*i)->getUfoStatus())
		{
			case Ufo::FLYING:
			case Ufo::LANDED:
				if (hour == false)
				{
					score = (*i)->getActivityPoints() + basic;
					score += (*i)->getAlienMission()->getRules().getMissionScore() / 10;
					if (score != 0)
						_playSave->scorePoints(
											(*i)->getLongitude(),
											(*i)->getLatitude(),
											score,
											true);
				}
				break;

			case Ufo::CRASHED:
				if (hour == true
					&& (score = (*i)->getActivityPoints() + basic) != 0)
				{
						_playSave->scorePoints(
											(*i)->getLongitude(),
											(*i)->getLatitude(),
											score,
											true);
				}
		}
	}
}

/**
 * Processes a TerrorSite.
 * @note This will count down towards expiring a TerrorSite and handles its
 * expiration.
 * @param terrorSite - pointer to a TerrorSite
 * @return, true if terror is finished (w/out xCom mission success)
 */
bool GeoscapeState::processTerrorSite(TerrorSite* const terrorSite) const // private.
{
	bool expired;

	const int
		diff (_playSave->getDifficultyInt()),
		elapsed (_playSave->getMonthsElapsed());
	int score;

	if (terrorSite->getSecondsLeft() > 1799)
	{
		expired = false;
		terrorSite->setSecondsLeft(terrorSite->getSecondsLeft() - 1800);

		score = terrorSite->getRules()->getMissionScore() / 10;
		score += diff * 10 + elapsed;
	}
	else
	{
		expired = true;

		score = terrorSite->getRules()->getMissionScore() * 5;
		score += diff * (235 + elapsed);
	}

	if (score != 0)
		_playSave->scorePoints(
							terrorSite->getLongitude(),
							terrorSite->getLatitude(),
							score,
							true);

	if (expired == true)
	{
		delete terrorSite;
		return true;
	}
	return false;
}

/**
 * Takes care of any game logic that has to run every hour.
 */
void GeoscapeState::time1Hour()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Hour()";
	if (_playSave->getUfos()->empty() == false)
		scoreUfos(true);

	for (std::vector<Region*>::const_iterator // step the activity-hours for Graphs blink
			i  = _playSave->getRegions()->begin();
			i != _playSave->getRegions()->end();
			++i)
	{
		(*i)->stepActivity();
	}

	for (std::vector<Country*>::const_iterator // step the activity-hours for Graphs blink
			i  = _playSave->getCountries()->begin();
			i != _playSave->getCountries()->end();
			++i)
	{
		(*i)->stepActivity();
	}


	bool arrivals (false);
	for (std::vector<Base*>::const_iterator // handle transfers
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		for (std::vector<Transfer*>::const_iterator
				j  = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			(*j)->advance(*i);

			if ((*j)->getHours() == 0)
				arrivals = true;
		}
	}


	std::vector<ManufactureCompleteInfo> runtEvents;
	// Note that if transfers arrive at the same time Manufacture(s) complete
	// the gotoBase button handling below is obviated by RMB on transfers ....
	// But that's been amended by showing Transfers after ProdCompleted screens;
	// although I'm not sure how this will interact with time1Day()'s facility
	// construction and research completed screens.

	for (std::vector<Base*>::const_iterator // handle Manufacture
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		std::map<ManufactureProject*, ManufactureProgress> progress;

		for (std::vector<ManufactureProject*>::const_iterator
				j  = (*i)->getManufacture().begin();
				j != (*i)->getManufacture().end();
				++j)
		{
			progress[*j] = (*j)->stepManufacture(*i, _playSave);
		}

		for (std::map<ManufactureProject*, ManufactureProgress>::const_iterator
				j  = progress.begin();
				j != progress.end();
				++j)
		{
			switch (j->second)
			{
				case PROG_COMPLETE:
				case PROG_NOT_ENOUGH_MONEY:
				case PROG_NOT_ENOUGH_MATERIALS:
					if (runtEvents.empty() == false) // set the previous event to NOT show btn.
						runtEvents.back().allocate = false;

					runtEvents.push_back(ManufactureCompleteInfo(
															*i,
															tr(j->first->getRules()->getType()),
															(arrivals == false),
															j->second));
					(*i)->clearManufactureProject(j->first);
			}
		}

		if ((*i)->storesOverfull() == true)
		{
			resetTimer();
			popupGeo(new ErrorMessageState(
								tr("STR_STORAGE_EXCEEDED").arg((*i)->getLabel()),
								_palette,
								_rules->getInterface("geoscape")->getElement("errorMessage")->color,
								"BACK12.SCR", // "BACK13.SCR"
								_rules->getInterface("geoscape")->getElement("errorPalette")->color));
//			popup(new SellState(*i));
		}
	}

	for (std::vector<ManufactureCompleteInfo>::const_iterator
			j  = runtEvents.begin();
			j != runtEvents.end();
			++j)
	{
		popupGeo(new ManufactureCompleteState(
										j->base,
										j->item,
										this,
										j->allocate,
										j->doneType)); // ie. Manufacture doneType.
	}

	if (arrivals == true)
		popupGeo(new ItemsArrivingState(this));


	// TFTD stuff: 'detected' see TerrorSite
	for (std::vector<TerrorSite*>::const_iterator
			i  = _playSave->getTerrorSites()->begin();
			i != _playSave->getTerrorSites()->end();
			++i)
	{
		if ((*i)->getDetected() == false)
		{
			(*i)->setDetected();

			resetTimer();
			popupGeo(new TerrorDetectedState(*i, this));
			break;
		}
	}
	//Log(LOG_INFO) << "GeoscapeState::time1Hour() EXIT";
}


/**
 ** FUNCTOR ***
 * Attempts to generate a support-mission for an AlienBase.
 * @note Each AlienBase has a chance to generate a mission.
 */
struct GenerateSupportMission
	:
		public std::unary_function<const AlienBase*, void>
{
private:
	const Ruleset& _rules;
	SavedGame& _playSave;

	public:
		/**
		 * Caches Ruleset and SavedGame for later use.
		 * @param rules		- reference to the Ruleset
		 * @param playSave	- reference to the SavedGame
		 */
		GenerateSupportMission(
				const Ruleset& rules,
				SavedGame& playSave)
			:
				_rules(rules),
				_playSave(playSave)
		{}

		/**
		 * Checks for and creates an AlienBase's support-mission.
		 * @param aBase - pointer to an AlienBase
		 */
		void operator ()(const AlienBase* const aBase) const;
};

/**
 * Checks for and creates a base-generated AlienMission for a specified
 * AlienBase.
 * @note There is a 4% chance per day of a mission getting created.
 * @param aBase - pointer to an AlienBase
 */
void GenerateSupportMission::operator ()(const AlienBase* const aBase) const
{
	const RuleAlienDeployment* const ruleDeploy (aBase->getAlienBaseDeployed());
	const std::string type (ruleDeploy->getBaseGeneratedType());
	const RuleAlienMission* const missionRule (_rules.getAlienMission(type));
	if (missionRule != nullptr)
	{
		if (RNG::percent(ruleDeploy->getBaseGeneratedPct()) == true)
		{
			AlienMission* const mission (new AlienMission(*missionRule, _playSave));
			mission->setRegion(
						_playSave.locateRegion(*aBase)->getRules()->getType(),
						_rules);
			mission->setId(_playSave.getCanonicalId(Target::stTarget[7u]));
			mission->setRace(aBase->getAlienRace());
			mission->setAlienBase(aBase);
			mission->start();

			_playSave.getAlienMissions().push_back(mission);
		}
	}
	else if (type.empty() == false)
	{
		std::string st ("AlienBase failed to generate an AlienMission from type: ");
		st += type + " - no RuleAlienMission was found.";
		throw Exception(st);
 	}
}


/**
 * Takes care of any game logic that has to run every day.
 */
void GeoscapeState::time1Day()
{
	//Log(LOG_INFO) << "GeoscapeState::time1Day()";

	// Create vectors of pending-events integrated for all Bases so that slightly
	// different dialog-layouts can be shown for the last event of each type.

	std::vector<State*> resEvents;

	std::vector<ManufactureCompleteInfo> runtEvents;

	std::vector<ResearchUnlockedInfo> resEventsPopped;
	std::vector<ManufactureUnlockedInfo> runtEventsPopped;

	std::vector<ResearchProject*> discovered;

	for (std::vector<Base*>::const_iterator
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
	{
		bool dead;
		for (std::vector<Soldier*>::const_iterator // handle Soldiers in sickbay ->
				j  = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				)
		{
			dead = false;
			if ((*j)->getSickbay() != 0)
			{
				int pctDeath ((*j)->getPctWounds());
				if (pctDeath > 10)
				{
					//Log(LOG_INFO) << "\n";
					//Log(LOG_INFO) << ". soldier id-" << (*j)->getId() << " pctWounds= " << pctDeath;
					const size_t tacId (static_cast<size_t>((*j)->getDiary()->getTacticalIdList().back()));
					const std::map<int,int>* injuryList (&_playSave->getTacticalStatistics().at(tacId)->injuryList);
					if (injuryList->find((*j)->getId()) != injuryList->end())
					{
						const float woundsLeft (static_cast<float>(injuryList->at((*j)->getId()))
											  / static_cast<float>((*j)->getCurrentStats()->health));
						//Log(LOG_INFO) << ". . total days wounded= " << injuryList->at((*j)->getId();
						//Log(LOG_INFO) << ". . current days wounded= " << woundsLeft;
						pctDeath = static_cast<int>(std::ceil(
								   static_cast<float>(pctDeath) * woundsLeft));

						const int roll (RNG::generate(1,1000));
						//Log(LOG_INFO) << ". . chance to die= " << pctDeath << " roll= " << roll;
						if (roll <= pctDeath)
						{
							//Log(LOG_INFO) << "It's dead, Jim!!";
							resetTimer();
							if ((*j)->getArmor()->isBasic() == false) // return former Soldier's armor to stores.
								(*i)->getStorageItems()->addItem((*j)->getArmor()->getStoreItem());

							popupGeo(new SoldierDiedState(
													(*j)->getLabel(),
													(*i)->getLabel()));

							(*j)->die(_playSave); // holy * This copies the SoldierDiary-object
							// so to delete Soldier-instance I need to use a copy-constructor
							// on either or both of SoldierDiary and SoldierAward.
							// Oh, and maybe an operator= assignment-overload also.
							// Learning C++ is like standing around while 20 people constantly
							// throw cow's dung at you. (But don't mention "const" or they'll throw
							// twice as fast.) i miss you, Alan Turing ....

							delete *j;
							j = (*i)->getSoldiers()->erase(j);

							dead = true;
						}
					}
				}
				if (dead == false)
					(*j)->heal();
			}
			if (dead == false)
				++j;
		}


		if ((*i)->hasPsiLabs() == true) // handle psi-training and AutoStat ->
		{
//			bool sortSoldiers = false;
			for (std::vector<Soldier*>::const_iterator
					j  = (*i)->getSoldiers()->begin();
					j != (*i)->getSoldiers()->end();
					++j)
			{
				if ((*j)->trainPsiDay() == true)
				{
					(*j)->autoStat();
//					sortSoldiers = true;
				}
//				(*j)->calcStatString(_rules->getStatStrings(), Options::psiStrengthEval && _playSave->isResearched(_rules->getPsiRequirements()));
			}
//			if (sortSoldiers == true)
//				(*i)->sortSoldiers();
		}


		for (std::vector<BaseFacility*>::const_iterator // handle BaseFacility construction ->
				j  = (*i)->getFacilities()->begin();
				j != (*i)->getFacilities()->end();
				++j)
		{
			if ((*j)->buildFinished() == false && (*j)->buildFacility() == true)
			{
				if (runtEvents.empty() == false) // set the previous event to NOT show btn.
					runtEvents.back().allocate = false;

				runtEvents.push_back(ManufactureCompleteInfo(
														*i,
														tr((*j)->getRules()->getType()),
														true,
														PROG_CONSTRUCTION));
			}
		}


		discovered.clear(); // handle ResearchProjects ->

		for (std::vector<ResearchProject*>::const_iterator
				j  = (*i)->getResearch().begin();
				j != (*i)->getResearch().end();
				++j)
		{
			if ((*j)->stepResearch() == true)
				discovered.push_back(*j);
		}

		if (discovered.empty() == false)
		{
			resetTimer();

			for (std::vector<ResearchProject*>::const_iterator
					j  = discovered.begin();
					j != discovered.end();
					++j)
			{
				const RuleResearch* const resRule ((*j)->getRules());
				const std::string& resType (resRule->getType());

				const RuleUnit* const alien (_rules->getUnitRule(resType));

				bool
					crackGof,
					crackRequested;

				if (alien != nullptr)
				{
					if (resRule->needsItem() == true && resRule->destroyItem() == true
						&& Options::grantCorpses == true)
					{
						(*i)->getStorageItems()->addItem(_rules->getArmor(alien->getArmorType())->getCorpseGeoscape());
					}

					doesAlienCrack(
								resType,
								crackGof,
								crackRequested);
				}
				else
				{
					if (resRule->needsItem() == true && resRule->destroyItem() == false)
						(*i)->getStorageItems()->addItem(resType);

					crackGof =
					crackRequested = true;
				}

				(*i)->clearResearchProject(*j, alien != nullptr && crackRequested == true);


				std::vector<const RuleResearch*> popupResearch;

				const RuleResearch* gofRule (nullptr);
				if (crackGof == true)
				{
					std::vector<std::string> gofList;

					for (std::vector<std::string>::const_iterator
							k  = resRule->getGetOneFree().begin();
							k != resRule->getGetOneFree().end();
							++k)
					{
						if (_playSave->searchResearch(*k) == false)
							gofList.push_back(*k);
					}

					if (gofList.empty() == false)
					{
						gofRule = _rules->getResearch(gofList.at(RNG::pick(gofList.size())));
						_playSave->discoverResearch(gofRule);
						_playSave->tabulatePopupResearch(popupResearch, gofRule);
					}
				}


				_playSave->discoverResearch(resRule);
				_playSave->tabulatePopupResearch(popupResearch, resRule, crackRequested);
				// TODO: Cull popupResearch vector of possible duplicates;
				// respective dependents of gofRule & resRule, if that's even applicable.

				std::vector<const RuleManufacture*> popupManufacture;
				_playSave->tabulatePopupManufacture(popupManufacture, resRule);


				const RuleResearch* resRulePedia; // NOTE: Ufopaedia article will be 'type' if there is no (explicit) 'lookup'.
				if (_playSave->isResearched(resRule->getUfopaediaEntry()) == false)
					resRulePedia = resRule; // 'resRulePedia' is not defined as the Ufopaedia lookup here but as the discovered research itself.
				else
					resRulePedia = nullptr; // resRule has already been discovered before.

				resEvents.push_back(new ResearchCompleteState(
															resRulePedia,
															gofRule,
															resRule,
															alien != nullptr && crackRequested == true));


				if (resRulePedia != nullptr)	// check for need to research a clip before the weapon itself can be manufactured.
				{								// NOTE: Appears only if research has never been discovered before.
					const std::string& parentType (resRule->getType());

					const RuleItem* const parentRule (_rules->getItemRule(parentType));
					if (   parentRule != nullptr
						&& parentRule->getBattleType() == BT_FIREARM
						&& parentRule->getClipTypes()->empty() == false)
					{
						const RuleManufacture* const mfRule (_rules->getManufacture(parentType));
						if (mfRule != nullptr)
						{
							const std::vector<std::string>& required (mfRule->getRequiredResearch());
							if (_playSave->isResearched(required) == false)
							{
								const std::string& dependentType (parentRule->getClipTypes()->front());
								if (std::find(
											required.begin(),
											required.end(),
											dependentType) != required.end())
								{
									resEvents.push_back(new ResearchRequiredState(parentType, dependentType));
								}
							}
						}
					}
				}


				if (popupResearch.empty() == false)
				{
					if (resEventsPopped.empty() == false) // only show the "allocate research" button for the last notification
						resEventsPopped.back().allocate = false;

					resEventsPopped.push_back(ResearchUnlockedInfo(*i, popupResearch, true));
				}

				if (popupManufacture.empty() == false)
				{
					if (runtEventsPopped.empty() == false) // only show the "allocate production" button for the last notification
						runtEventsPopped.back().allocate = false;

					runtEventsPopped.push_back(ManufactureUnlockedInfo(*i, popupManufacture, true));
				}


				if (alien == nullptr)
				{
					for (std::vector<Base*>::const_iterator			// iterate through all the bases and remove this completed project from their labs
							k  = _playSave->getBases()->begin();	// unless it's an alien interrogation ...
							k != _playSave->getBases()->end();		// TODO: remove Gof's that might be underway at other Bases, too
							++k)
					{
						for (std::vector<ResearchProject*>::const_iterator
								l = (*k)->getResearch().begin();
								l != (*k)->getResearch().end();
								++l)
						{
							if ((*l)->getRules() == resRule)
							{
								(*k)->clearResearchProject(*l);

								if (resRule->needsItem() == true)
									(*k)->getStorageItems()->addItem(resType);

								break;
							}
						}
					}
				}
			}
		} // DONE Research.
	}

	// if research has been discovered but no new research events are triggered
	// show an empty ResearchUnlockedState so players have a chance to
	// allocate the now-free scientists.
	// kL_note: already taken care of. Just reset time-compression to 5sec and
	// let ResearchCompleteState poke the player.

//	if (resEvents.empty() == false && resEventsPopped.empty() == true)
//		resEventsPopped.push_back(ResearchUnlockedInfo(std::vector<const RuleResearch*>(), true));


	// show Popup Events:
	for (std::vector<ManufactureCompleteInfo>::const_iterator
			i  = runtEvents.begin();
			i != runtEvents.end();
			++i)
	{
		popupGeo(new ManufactureCompleteState(
											i->base,
											i->item,
											this,
											i->allocate,
											i->doneType)); // ie. PROG_CONSTRUCTION
	}

	for (std::vector<State*>::const_iterator
			i  = resEvents.begin();
			i != resEvents.end();
			++i)
	{
		popupGeo(*i);
	}

	for (std::vector<ResearchUnlockedInfo>::const_iterator
			i  = resEventsPopped.begin();
			i != resEventsPopped.end();
			++i)
	{
		popupGeo(new ResearchUnlockedState(
										i->base,
										i->unlockedProjects,
										i->allocate));
	}

	for (std::vector<ManufactureUnlockedInfo>::const_iterator
			i  = runtEventsPopped.begin();
			i != runtEventsPopped.end();
			++i)
	{
		popupGeo(new ManufactureUnlockedState(
										i->base,
										i->unlockedProjects,
										i->allocate));
	}
	// done Popup Events.


	const RuleAlienMission* const missionRule (_rules->rollMission( // handle regional and country points for aLien-bases ->
																alm_BASE,
																static_cast<size_t>(_playSave->getMonthsElapsed())));
	const int aLienPts ((missionRule->getMissionScore() * (_playSave->getDifficultyInt() + 1)) / 100);
	if (aLienPts != 0)
	{
		for (std::vector<AlienBase*>::const_iterator
				i  = _playSave->getAlienBases()->begin();
				i != _playSave->getAlienBases()->end();
				++i)
		{
			_playSave->scorePoints(
								(*i)->getLongitude(),
								(*i)->getLatitude(),
								aLienPts,
								true);
		}
	}


	std::for_each( // handle possible generation of AlienBase support-mission ->
			_playSave->getAlienBases()->begin(),
			_playSave->getAlienBases()->end(),
			GenerateSupportMission(*_rules, *_playSave));


	const int day (_playSave->getTime()->getDay()); // handle autosave 3 times a month ->
	if (day == 10 || day == 20)
	{
		if (_playSave->isIronman() == true)
			popupGeo(new SaveGameState(
								OPT_GEOSCAPE,
								SAVE_IRONMAN,
								_palette));
		else if (Options::autosave == true) // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
			popupGeo(new SaveGameState(
								OPT_GEOSCAPE,
								SAVE_AUTO_GEOSCAPE,
								_palette));
	}

	if (_playSave->getTime()->isEndNear() == true) // TODO: Option to enable/disable end-of-month warning.
	{
		resetTimer();
		_playSave->getTime()->clearEndNear();
		popupGeo(new MonthNearEndState());
	}
	//Log(LOG_INFO) << "GeoscapeState::time1Day() EXIT";
}

/**
 * Assigns whether an aLien cracked under pressure.
 * @param alienType	- reference to the aLien-type
 * @param gof		- reference to hold if the gof-info is cracked
 * @param requested	- reference to hold if the request-info is cracked
 */
void GeoscapeState::doesAlienCrack( // private.
			const std::string& alienType,
			bool& gof,
			bool& requested) const
{
	int
		pctGof,
		pctRequest; // TODO: Make these ruleset vars in unit-rules.

	if (alienType.find("_TERRORIST") != std::string::npos)
	{
		pctGof     = 10;
		pctRequest = 50;
	}
	else if (alienType.find("_FLOATER") != std::string::npos)
	{
		pctGof     = 80;
		pctRequest = 30;
	}
	else if (alienType.find("_SECTOID") != std::string::npos)
	{
		pctGof     = 70;
		pctRequest = 40;
	}
	else if (alienType.find("_SNAKEMAN") != std::string::npos)
	{
		pctGof     = 60;
		pctRequest = 50;
	}
	else if (alienType.find("_MUTON") != std::string::npos)
	{
		pctGof     = 50;
		pctRequest = 60;
	}
	else if (alienType.find("_ETHEREAL") != std::string::npos)
	{
		pctGof     = 40;
		pctRequest = 70;
	}
	else if (alienType.find("_WASPITE") != std::string::npos)
	{
		pctGof     = 30;
		pctRequest = 80;
	}
	else // defaults
	{
		pctGof     = 100;
		pctRequest = 100;
	}

	gof       = RNG::percent(pctGof);
	requested = RNG::percent(pctRequest);
}

/**
 * Takes care of any game logic that has to run every month.
 */
void GeoscapeState::time1Month()
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "GeoscapeState::time1Month()";

	resetTimer();

	_game->getResourcePack()->fadeMusic(_game, 1232);

	popupGeo(new MonthlyReportState());

	if (_playSave->getAlienBases()->empty() == false) // handle 007 Secret Agents discovering alien bases
	{
		const int pct (70 - (_playSave->getDifficultyInt() * 5));
		for (std::vector<AlienBase*>::const_iterator
				i  = _playSave->getAlienBases()->begin();
				i != _playSave->getAlienBases()->end();
				++i)
		{
			if ((*i)->isDetected() == false && RNG::percent(pct) == true)
			{
				popupGeo(new AlienBaseDetectedState(*i, this, true));
				break;
			}
		}
	}

	_playSave->elapseMonth();

	deterAlienMissions();
}

/**
 * Slows the Geoscape timer down to the minimum 5-sec time-compression.
 */
void GeoscapeState::resetTimer()
{
	_globe->rotateStop();
	_btn5Secs->mousePress(_game->getFakeMouseActionD(), this);
}

/**
 * Checks if time-compression is set to 5-sec intervals.
 * @return, true if time-compression is set to 5-secs
 */
bool GeoscapeState::is5Sec() const
{
	return (_btnGroup == _btn5Secs);
}

/**
 * Adds a new popup-window to the popup-queue and pauses the Timer.
 * @note Doing it this way this prevents popups from overlapping.
 * @param state - pointer to popup state
 */
void GeoscapeState::popupGeo(State* const state)
{
	_pause = true;
	_popups.push_back(state);
}

/**
 * Gets a pointer to the geoscape Globe.
 * @return, pointer to the globe
 */
Globe* GeoscapeState::getGlobe() const
{
	return _globe;
}

/**
 * Checks if user-input is not allowed.
 * @return, true if the dogfight zinn/zout timers are running
 */
bool GeoscapeState::inputDisabled()
{
	return _trDfZinn->isRunning() == true
		|| _trDfZout->isRunning() == true;
}

/**
 * Processes any left-clicks on globe-markers. See the Globe for drag-scroll.
 * @param action - pointer to an Action
 */
void GeoscapeState::globeClick(Action* action)
{
	if (inputDisabled() == false)
	{
		const int
			mX (static_cast<int>(std::floor(action->getAbsoluteMouseX()))),
			mY (static_cast<int>(std::floor(action->getAbsoluteMouseY())));

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			const std::vector<Target*> targets (_globe->getTargets(mX,mY, false));
			if (targets.empty() == false)
				_game->pushState(new SelectTargetState(targets, nullptr, this));
		}

		if (_playSave->getDebugGeo() == true)
		{
			_playSave->setDebugArg("COORD");	// tells think() to stop writing area-info and display lon/lat instead.
			_stDebug = "";						// ditto

			double
				lonRad,
				latRad;
			_globe->cartToPolar(
							static_cast<Sint16>(mX),
							static_cast<Sint16>(mY),
							&lonRad, &latRad);

			const double
				lonDeg (lonRad / M_PI * 180.),
				latDeg (latRad / M_PI * 180.);
			int
				texture,
				shade;
			_globe->getPolygonTextureAndShade(lonDeg, latDeg, &texture, &shade);

			std::wostringstream woststr;
			woststr << std::fixed << std::setprecision(3)
					<< L"RAD Lon " << lonRad << L"  Lat " << latRad
					<< std::endl
					<< L"DEG Lon " << lonDeg << L"  Lat " << latDeg
					<< std::endl
					<< L"texture " << texture;
			if (texture != -1)
				woststr << L" shade " << shade;

			_txtDebug->setText(woststr.str()); // TODO: If paused redraw HUD.
		}
	}
}

/**
 * Opens the Intercept window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnInterceptClick(Action*)
{
	if (inputDisabled() == false)
		_game->pushState(new InterceptState(this));
}

/**
 * Goes to the Basescape screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnBasesClick(Action*)
{
	if (inputDisabled() == false)
	{
		resetTimer();
		kL_soundPop->play(Mix_GroupAvailable(0));
		_game->getScreen()->fadeScreen();

		if (kL_curBase < _playSave->getBases()->size())
			_game->pushState(new BasescapeState(
											_playSave->getBases()->at(kL_curBase),
											_globe));
		else
			_game->pushState(new BasescapeState(
											_playSave->getBases()->front(),
											_globe));
	}
}

/**
 * Goes to the Graphs screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnGraphsClick(Action*)
{
	if (inputDisabled() == false)
	{
		resetTimer();
		kL_soundPop->play(Mix_GroupAvailable(0));
		_game->getScreen()->fadeScreen();

		_game->pushState(new GraphsState());
	}
}

/**
 * Goes to the Ufopaedia window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnUfopaediaClick(Action*)
{
	if (inputDisabled() == false)
	{
		resetTimer();
		_game->getResourcePack()->fadeMusic(_game, 276);

		Ufopaedia::open(_game);
		_game->getResourcePack()->playMusic(
										OpenXcom::res_MUSIC_UFOPAEDIA,
										"", 1);
	}
}

/**
 * Opens the Options window.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnOptionsClick(Action*)
{
	if (inputDisabled() == false)
	{
		resetTimer();
		_game->pushState(new PauseState(OPT_GEOSCAPE));
	}
}

/**
 * Goes to the Funding screen.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnFundingClick(Action*)
{
	if (inputDisabled() == false)
	{
		resetTimer();
		_game->pushState(new FundingState());
	}
}

/**
 * Handler for clicking the Detail area.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnDetailPress(Action* action)
{
	if (inputDisabled() == false)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
				printDetailInfo(_globe->toggleDetail());
				break;

			case SDL_BUTTON_RIGHT:
				printRadarInfo(_globe->changeRadars());
		}
	}
}

/**
 * Updates the detail-info.
 */
void GeoscapeState::printDetailInfo(bool detail)
{
	std::string st;
	if (detail == true)
		st = "STR_ON_UC";
	else
		st = "STR_OFF_UC";

	_txtLabels->setText(tr("STR_LABELS_").arg(tr(st)));
}

/**
 * Updates the radar-info.
 */
void GeoscapeState::printRadarInfo(GlobeRadarDetail radars)
{
	std::string st;
	switch (radars)
	{
		case GRD_NONE:  st = "STR_RADARS_NONE_UC";  break;
		case GRD_CRAFT: st = "STR_RADARS_CRAFT_UC"; break;
		case GRD_BASE:  st = "STR_RADARS_BASE_UC";  break;
		case GRD_ALL:   st = "STR_RADARS_ALL_UC";
	}

	_txtRadars->setText(tr("STR_RADARS_").arg(tr(st)));
}

/**
 * Starts rotating the Globe to the left.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftPress(Action*)
{
	if (inputDisabled() == false)
		_globe->rotateLeft();
}

/**
 * Starts rotating the Globe to the right.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightPress(Action*)
{
	if (inputDisabled() == false)
		_globe->rotateRight();
}

/**
 * Stops rotating the Globe horizontally.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLonStop(Action*)
{
	_globe->rotateStopLon();
}

/**
 * Starts rotating the Globe upwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateUpPress(Action*)
{
	if (inputDisabled() == false)
		_globe->rotateUp();
}

/**
 * Starts rotating the Globe downwards.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateDownPress(Action*)
{
	if (inputDisabled() == false)
		_globe->rotateDown();
}

/**
 * Stops rotating the Globe vertically.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLatStop(Action*)
{
	_globe->rotateStopLat();
}

/**
 * Starts rotating the Globe to the left up.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftUpPress(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->rotateLeft();
		_globe->rotateUp();
	}
}

/**
 * Starts rotating the Globe to the left down.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateLeftDownPress(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->rotateLeft();
		_globe->rotateDown();
	}
}

/**
 * Starts rotating the Globe to the right up.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightUpPress(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->rotateRight();
		_globe->rotateUp();
	}
}

/**
 * Starts rotating the Globe to the right down.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateRightDownPress(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->rotateRight();
		_globe->rotateDown();
	}
}

/**
 * Stops rotating the Globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnRotateStop(Action*)
{
	_globe->rotateStop();
}

/**
 * Zooms into the Globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnZoomInLeftClick(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->zoomIn();
		updateZoomText();
	}
}

/**
 * Zooms out of the Globe.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnZoomOutLeftClick(Action*)
{
	if (inputDisabled() == false)
	{
		_globe->zoomOut();
		updateZoomText();
	}
}

/**
 * Updates the zoom-text in the lower left corner of the screen.
 */
void GeoscapeState::updateZoomText()
{
	_txtZoom->setText(tr("STR_ZOOM_").arg(Text::intWide(static_cast<int>(_playSave->getGlobeZoom()))));
}

/**
 * Zooms the Globe maximum.
 * @param action - pointer to an Action
 *
void GeoscapeState::btnZoomInRightClick(Action*)
{
	if (inputDisabled() == false)
		_globe->zoomMax();
} */
/**
 * Zooms the Globe minimum.
 * @param action - pointer to an Action
 *
void GeoscapeState::btnZoomOutRightClick(Action*)
{
	if (inputDisabled() == false)
		_globe->zoomMin();
} */

/**
 * Handles zoom-in to zoom-out or vice versa for a dogfight.
 * @param in - true to zoom in, false to zoom out
 */
void GeoscapeState::toggleDogfight(bool in) const
{
	if (in)
	{
		if (_trDfZout->isRunning() == true)
			_trDfZout->stop();

		if (_trDfZinn->isRunning() == false)
			_trDfZinn->start();
	}
	else
	{
		if (_trDfZinn->isRunning() == true)
			_trDfZinn->stop();

		if (_trDfZout->isRunning() == false)
			_trDfZout->start();
	}
}

/**
 * Zoom-in effect for dogfights.
 */
void GeoscapeState::dfZoomIn()
{
	if (_globe->zoomDogfightIn() == true)
		_trDfZinn->stop();
}

/**
 * Zoom-out effect for dogfights.
 */
void GeoscapeState::dfZoomOut()
{
	if (_globe->zoomDogfightOut() == true)
	{
		_trDfZout->stop();

		if (_dfCenterCurrentCoords == true)
		{
			_dfCenterCurrentCoords = false;
			_globe->center(
						_dfCCC_lon,
						_dfCCC_lat);
		}
		else
			_globe->center(
						_playSave->getDfLongitude(),
						_playSave->getDfLatitude());

		init();
	}
}

/**
 * Sets the zoom-out timer to ignore stored pre-Dogfight coordinates and use
 * current coordinates of the Dogfight instead.
 * @note Used only if UFO is breaking off from its last dogfight.
 */
void GeoscapeState::setDfCCC(
		double lon,
		double lat)
{
	_dfCenterCurrentCoords = true;
	_dfCCC_lon = lon;
	_dfCCC_lat = lat;
}

/**
 * Checks if the zoom-out timer should ignore stored pre-Dogfight coordinates
 * and use the current coordinates of the Dogfight instead.
 * @return, true if UFO is set to break off from its dogfight
 */
bool GeoscapeState::getDfCCC() const
{
	return _dfCenterCurrentCoords;
}

/**
 * Gets the quantity of iconized dogfights.
 * @return, quantity of iconized dogfights
 */
size_t GeoscapeState::getReducedDogfights() const
{
	size_t ret (0u);
	for (std::list<DogfightState*>::const_iterator
			i  = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		if ((*i)->isReduced() == true)
			++ret;
	}

	return ret;
}

/**
 * Dogfight logic.
 */
void GeoscapeState::thinkDogfights()
{
	_dfReduced = 0u;
	bool resetPorts = false;

	std::list<DogfightState*>::const_iterator i (_dogfights.begin());
	while (i != _dogfights.end())
	{
		(*i)->getUfo()->setTicked(false);
		++i;
	}

	i = _dogfights.begin();
	while (i != _dogfights.end())
	{
		if ((*i)->isFinished() == true)
		{
			resetPorts = true;

			delete *i;
			i = _dogfights.erase(i);
		}
		else
		{
			if ((*i)->isReduced() == true)
				++_dfReduced;

			(*i)->think();
			++i;
		}
	}

	if (_dogfights.empty() == true)
	{
		_trDf->stop(); // stop recursion of this function.

		if (_dfZout == true) // or if a UFO just landed, reset dfCoords to current globe position: DO NOT ZOOM OUT
		{
			_trDfZout->start();
		}
		else // STOP INTERCEPTION MUSIC. Start Geo music ...
		{
			if (_game->getResourcePack()->isMusicPlaying(res_MUSIC_GEO_TERROR_SPLASH) == false) // unless a Mission/TerrorSite just popped
			{
				_game->getResourcePack()->fadeMusic(_game, 425);
				_game->getResourcePack()->playMusic(res_MUSIC_GEO_GLOBE);
			}
		}
	}
	else if (resetPorts == true)
		resetInterceptPorts();

	_dfZout = true;
}

/**
 * Starts a new Dogfight.
 */
void GeoscapeState::startDogfight() // private.
{
	if (_globe->getZoom() < _globe->getZoomLevels() - 1u)
	{
		if (_trDfStart->isRunning() == false) // TODO: get rid of '_trDfStart' and '_dogfightsToStart'
			_trDfStart->start();

		if (_trDfZinn->isRunning() == false)
			_trDfZinn->start();
	}
	else
	{
		if (_trDfStart->isRunning() == true)
			_trDfStart->stop();

		if (_trDfZinn->isRunning() == true)
			_trDfZinn->stop();

		resetTimer();

		if (_trDf->isRunning() == false)
			_trDf->start();

		while (_dogfightsToStart.empty() == false)
		{
			_dogfights.push_back(_dogfightsToStart.back());
			_dogfightsToStart.pop_back();
		}
	}

	resetInterceptPorts(); // set intercept-slot and ergo port-positions for all dogfights
}

/**
 * Updates total current intercepts for all Dogfights and repositions their
 * ports and reduced-icons accordingly.
 */
void GeoscapeState::resetInterceptPorts()
{
	const size_t total (_dogfights.size());
	size_t port (0u);
	for (std::list<DogfightState*>::const_iterator
			i  = _dogfights.begin();
			i != _dogfights.end();
			++i)
	{
		(*i)->resetInterceptPort(++port, total);
	}
}

/**
 * Gets the dogfights.
 * @return, reference to a list of pointers to DogfightStates
 */
std::list<DogfightState*>& GeoscapeState::getDogfights()
{
	return _dogfights;
}

/**
 * Starts base-defense tactical.
 * @param base	- pointer to Base to defend
 * @param ufo	- pointer to the attacking Ufo
 */
void GeoscapeState::baseDefenseTactical(
		Base* const base,
		Ufo* const ufo)
{
	ufo->setUfoStatus(Ufo::DESTROYED);

	if (base->getAvailableSoldiers() != 0)
	{
		SavedBattleGame* const battleSave (new SavedBattleGame(
														_game->getSavedGame(),
														&_rules->getOperations(),
														_rules));
		_playSave->setBattleSave(battleSave);
		battleSave->setTacticalType("STR_BASE_DEFENSE");

		BattlescapeGenerator bGen = BattlescapeGenerator(_game);
		bGen.setBase(base);
		bGen.setAlienRace(ufo->getAlienRace());
		bGen.stage();

		_pause = true;
		popupGeo(new BriefingState(nullptr, base));
	}
	else
		popupGeo(new BaseDestroyedState(base, _globe));
}

/**
 * Determines the AlienMissions to start this month.
 */
void GeoscapeState::deterAlienMissions() // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "GeoscapeState::deterAlienMissions()";
	const int elapsed (_playSave->getMonthsElapsed());
	//Log(LOG_INFO) << ". elapsed= " << elapsed;

	AlienStrategy& strategy (_playSave->getAlienStrategy());
	std::vector<RuleMissionScript*> availableMissions;
	std::map<int, bool> conditions;

	RuleMissionScript* directive;
	for (std::vector<std::string>::const_iterator
			i  = _rules->getMissionScriptList()->begin();
			i != _rules->getMissionScriptList()->end();
			++i)
	{
		directive = _rules->getMissionScript(*i);

		if (directive->getMinDifficulty() <= _playSave->getDifficulty()
			&&  directive->getFirstMonth() <= elapsed
			&& (directive->getLastMonth() == -1
				|| directive->getLastMonth() >= elapsed)
			&& (directive->getMaxRuns() == -1
				|| directive->getMaxRuns() > strategy.getMissionsRun(directive->getVarType())))
		{
			bool go (true);
			for (std::map<std::string, bool>::const_iterator
					j  = directive->getResearchTriggers().begin();
					j != directive->getResearchTriggers().end() && go == true;
					++j)
			{
				go = (_playSave->isResearched(j->first) == j->second);
			}

			if (go == true)
				availableMissions.push_back(directive);
		}
	}


	for (std::vector<RuleMissionScript*>::const_iterator
			i  = availableMissions.begin();
			i != availableMissions.end();
			++i)
	{
		bool
			process (true),
			success (false);

		for (std::vector<int>::const_iterator
				j  = (*i)->getConditions().begin();
				j != (*i)->getConditions().end() && process == true;
				++j)
		{
			std::map<int, bool>::const_iterator found (conditions.find(std::abs(*j)));
			process = (found == conditions.end()
				   || (found->second == true  && *j > 0)
				   || (found->second == false && *j < 0));
		}

		if ((*i)->getLabel() > 0
			&& conditions.find((*i)->getLabel()) != conditions.end())
		{
			std::ostringstream err;
			err << "Mission generator encountered an error: multiple commands: ["
				<< (*i)->getType() << "] and ";
			for (std::vector<RuleMissionScript*>::const_iterator
					j  = availableMissions.begin();
					j != availableMissions.end();
					++j)
			{
				if (*j != *i && (*j)->getLabel() == (*i)->getLabel())
					err << "["
						<< (*j)->getType()
						<< "]";
			}
			err << " are sharing the same label: ["
				<< (*i)->getLabel()
				<< "]";
			throw Exception(err.str());
		}

		if (process == true && RNG::percent((*i)->getExecutionOdds()) == true)
			success = processDirective(*i);

		if ((*i)->getLabel() > 0)
		{
			if (conditions.find((*i)->getLabel()) != conditions.end())
			{
				throw Exception("Error in mission scripts: " + (*i)->getType()
								+ ". Two or more commands share the same label.");
			}
			conditions[(*i)->getLabel()] = success;
		}
	}
}

/**
 * Proccesses a directive to start up a mission if possible.
 * @param directive - the directive from which to read information
 * @return, true if the command successfully produced a new mission
 */
bool GeoscapeState::processDirective(RuleMissionScript* const directive) // private.
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "GeoscapeState::processDirective()";
	const size_t elapsed (static_cast<size_t>(_playSave->getMonthsElapsed()));
	//Log(LOG_INFO) << ". elapsed= " << elapsed;

	AlienStrategy& strategy (_playSave->getAlienStrategy());
	const RuleAlienMission* missionRule;
	std::string
		typeRegion,
		typeMission,
		typeRace;
	size_t terrorZoneId (std::numeric_limits<size_t>::max()); // darn vc++ linker warning ...

	if (directive->terrorType() == true)
	{
		typeMission = directive->genDataType(elapsed, GT_MISSION);
		const std::vector<std::string> missionTypes (directive->getMissionTypes(elapsed));
		size_t
			missionsTotal (missionTypes.size()),
			missionsTest (0u);

		for (
				;
				missionsTest != missionsTotal;
				++missionsTest)
		{
			if (missionTypes[missionsTest] == typeMission)
				break;
		}

		std::vector<std::pair<std::string, size_t>> validZoneIds;

		for (size_t
				i  = 0u;
				i != missionsTotal;
				++i)
		{
			missionRule = _rules->getAlienMission(typeMission);
			terrorZoneId = missionRule->getObjectiveZone();

			std::vector<std::string> regions;
			if (directive->hasRegionWeights() == true)
				regions = directive->getRegions(elapsed);
			else
				regions = _rules->getRegionsList();

			for (std::vector<std::string>::const_iterator
					j  = regions.begin();
					j != regions.end();
					)
			{
				bool go (true);
				for (std::vector<AlienMission*>::const_iterator
						k  = _playSave->getAlienMissions().begin();
						k != _playSave->getAlienMissions().end();
						++k)
				{
					if ((*k)->getRules().getType() == missionRule->getType()
						&& (*k)->getRegion() == *j)
					{
						go = false;
						break;
					}
				}

				if (go == true)
				{
					const RuleRegion* const regionRule (_rules->getRegion(*j));
					if (regionRule->getMissionZones().size() > terrorZoneId)
					{
						size_t siteZoneTest (0u);
						const std::vector<MissionArea> areas (regionRule->getMissionZones()[terrorZoneId].areas);
						for (std::vector<MissionArea>::const_iterator
								k  = areas.begin();
								k != areas.end();
								++k, ++siteZoneTest)
						{
							if ((*k).isPoint() == true
								&& strategy.validateMissionLocation(
																directive->getVarType(),
																regionRule->getType(),
																siteZoneTest) == true)
							{
								validZoneIds.push_back(std::make_pair(
																	regionRule->getType(),
																	siteZoneTest));
							}
						}
					}
					++j;
				}
				else
					j = regions.erase(j);
			}

			if (validZoneIds.empty() == true)
			{
				if (missionsTotal > 1u && ++missionsTest == missionsTotal)
					missionsTest = 0u;

				typeMission = missionTypes[missionsTest];
			}
			else
				break;
		}

		if (validZoneIds.empty() == true)
			return false;


		terrorZoneId = std::numeric_limits<size_t>::max();
		while (terrorZoneId == std::numeric_limits<size_t>::max())
		{
			if (directive->hasRegionWeights() == true)
				typeRegion = directive->genDataType(elapsed, GT_REGION);
			else
				typeRegion = _rules->getRegionsList().at(RNG::pick(_rules->getRegionsList().size()));

			int
				low      (-1),
				high     (-1), // avoid vc++ linker warning.
				testArea ( 0);

			for (std::vector<std::pair<std::string, size_t>>::const_iterator
					i  = validZoneIds.begin();
					i != validZoneIds.end();
					++i)
			{
				if ((*i).first == typeRegion)
				{
					if (low == -1) low = testArea;
					high = testArea;
				}
				else if (low > -1)
					break;

				++testArea;
			}

			if (low != -1)
				terrorZoneId = validZoneIds[static_cast<size_t>(RNG::generate(low, high))].second;
		}

		strategy.addMissionLocation(
								directive->getVarType(),
								typeRegion,
								terrorZoneId,
								static_cast<size_t>(directive->getRepeatAvoidance()));
	}
	else if (RNG::percent(directive->getTargetBaseOdds()) == true)
	{
		std::vector<std::string> regionsPlayerBases;
		for (std::vector<Base*>::const_iterator
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
		{
			regionsPlayerBases.push_back(_playSave->locateRegion(**i)->getRules()->getType());
		}

		std::vector<std::string> missionTypes (directive->getMissionTypes(elapsed));
		if (missionTypes.empty() == false)
		{
			std::vector<std::string> regionTypes;
			const size_t missionTypesTotal (missionTypes.size());
			size_t id (RNG::pick(missionTypesTotal));

			for (size_t
					i  = 0u;
					i != missionTypesTotal;
					++i)
			{
				regionTypes = regionsPlayerBases;

				for (std::vector<AlienMission*>::const_iterator
						j  = _playSave->getAlienMissions().begin();
						j != _playSave->getAlienMissions().end();
						++j)
				{
					if (missionTypes[id] == (*j)->getRules().getType())
					{
						for (std::vector<std::string>::const_iterator
								k  = regionTypes.begin();
								k != regionTypes.end();
								)
						{
							if (*k == (*j)->getRegion())
								k = regionTypes.erase(k);
							else
								++k;
						}
					}
				}

				if (regionTypes.empty() == false)
				{
					typeMission = missionTypes[id];
					typeRegion = regionTypes[RNG::pick(regionTypes.size())];
					break;
				}

				if (missionTypesTotal > 1u && ++id == missionTypesTotal)
					id = 0u;
			}
		}
		else
		{
			for (std::vector<std::string>::const_iterator
					i  = regionsPlayerBases.begin();
					i != regionsPlayerBases.end();
					)
			{
				if (strategy.validateMissionRegion(*i) == true)
					++i;
				else
					i = regionsPlayerBases.erase(i);
			}

			if (regionsPlayerBases.empty() == true)
				return false;

			typeRegion = regionsPlayerBases[RNG::pick(regionsPlayerBases.size())];
		}
	}
	else if (directive->hasRegionWeights() == true)
		typeRegion = directive->genDataType(elapsed, GT_REGION);
	else
		typeRegion = strategy.chooseRegion(_rules);

	if (typeRegion.empty() == true)
		return false;


	if (_rules->getRegion(typeRegion) == nullptr)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", region named: " + typeRegion
						+ " is not defined");
	}

	if (typeMission.empty() == true)
	{
		if (directive->hasMissionWeights() == false)
			typeMission = strategy.chooseMission(typeRegion);
		else
			typeMission = directive->genDataType(elapsed, GT_MISSION);
	}

	if (typeMission.empty() == true)
		return false;


	missionRule = _rules->getAlienMission(typeMission);

	if (missionRule == nullptr)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", mission-type " + typeMission
						+ " is not defined");
	}

	if (directive->hasRaceWeights() == false)
		typeRace = missionRule->generateRace(elapsed);
	else
		typeRace = directive->genDataType(elapsed, GT_RACE);

	if (_rules->getAlienRace(typeRace) == nullptr)
	{
		throw Exception("Error proccessing mission script named: "
						+ directive->getType()
						+ ", race " + typeRace
						+ " is not defined");
	}


	//Log(LOG_INFO) << ". create AlienMission type= " << missionRule->getType();
	AlienMission* const mission (new AlienMission(
												*missionRule,
												*_playSave));
	mission->setRace(typeRace);
	mission->setId(_playSave->getCanonicalId(Target::stTarget[7u]));
	mission->setRegion(typeRegion, *_rules);
	mission->setTerrorZone(terrorZoneId);
	strategy.addMissionRun(directive->getVarType());

	int countdown (directive->getDelay() / 30);
	countdown = (RNG::generate(0, countdown) + (countdown >> 1u)) * 30;
	mission->start(countdown);

	_playSave->getAlienMissions().push_back(mission);

	if (directive->isTracked() == true)
		strategy.clearRegion(
						typeRegion,
						typeMission);

	return true;
}

/**
 * Determine the alien missions to start each month.
 * @note In the vanilla game a terror mission plus one other are started in
 * random regions. The very first mission is Sectoid Research in the region of
 * player's first Base.
 * @param atGameStart - true if called at start (default false)
 *
void GeoscapeState::deterAlienMissions(bool atGameStart) // private.
{
	if (atGameStart == false)
	{
		//
		// One randomly selected mission.
		//
		AlienStrategy& strategy = _playSave->getAlienStrategy();
		const std::string& region = strategy.chooseRegion(_rules);
		const std::string& mission = strategy.chooseMission(region);

		// Choose race for this mission.
		const RuleAlienMission& missionRule = *_rules->getAlienMission(mission);
		const std::string& race = missionRule.generateRace(_playSave->getMonthsElapsed());

		AlienMission* const alienMission = new AlienMission(
														missionRule,
														*_playSave);
		alienMission->setId(_playSave->getId(Target::stTarget[7u]));
		alienMission->setRegion(
							region,
							*_rules);
		alienMission->setRace(race);
		alienMission->start();

		_playSave->getAlienMissions().push_back(alienMission);

		// Make sure this combination never comes up again.
		strategy.clearRegion(
							region,
							mission);
	}
	else
	{
		//
		// Sectoid Research at base's region. haha
		//
		AlienStrategy& strategy = _playSave->getAlienStrategy();
		const std::string region = _playSave->locateRegion(*_playSave->getBases()->front())->getRules()->getType();

		// Choose race for this mission.
		const std::string mission = _rules->getAlienMissionList().front();
		const RuleAlienMission& missionRule = *_rules->getAlienMission(mission);

		AlienMission* const alienMission = new AlienMission(
														missionRule,
														*_playSave);
		alienMission->setId(_playSave->getId(Target::stTarget[7u]));
		alienMission->setRegion(
							region,
							*_rules);
		const std::string sectoid = missionRule.getTopRace(_playSave->getMonthsElapsed());
		alienMission->setRace(sectoid);
		alienMission->start(150);

		_playSave->getAlienMissions().push_back(alienMission);

		// Make sure this combination never comes up again.
		strategy.clearRegion(
							region,
							mission);
	}
} */

/**
 * Sets up a land mission. Eg TERROR!!!
 *
void GeoscapeState::setupLandMission() // private.
{
	const RuleAlienMission& missionRule = *_rules->rollMission(
															alm_TERROR,
															_playSave->getMonthsElapsed());

	// Determine a random region with a valid mission zone and no mission already running.
	const RuleRegion* regRule = nullptr; // avoid VC++ linker warning.
	bool picked = false;
	const std::vector<std::string> regionsList = _rules->getRegionsList();

	// Try 40 times to pick a valid zone for a land mission.
	for (int
			i  = 0;
			i != 40 && picked == false;
			++i)
	{
		regRule = _rules->getRegion(regionsList[static_cast<size_t>(RNG::generate(0,
																				  static_cast<int>(regionsList.size()) - 1))]);
		if (regRule->getMissionZones().size() > missionRule.getObjectiveZone()
			&& _playSave->findAlienMission(
										regRule->getType(),
										alm_TERROR) == nullptr)
		{
			const MissionZone& zone = regRule->getMissionZones().at(missionRule.getObjectiveZone());
			for (std::vector<MissionArea>::const_iterator
					j  = zone.areas.begin();
					j != zone.areas.end() && picked == false;
					++j)
			{
				if (j->isPoint() == true)
					picked = true;
			}
		}
	}

	// Choose race for land mission.
	if (picked == true) // safety.
	{
		AlienMission* const mission = new AlienMission(
													missionRule,
													*_playSave);
		mission->setId(_playSave->getId(Target::stTarget[7u]));
		mission->setRegion(
						regRule->getType(),
						*_rules);
		const std::string& race = missionRule.generateRace(static_cast<size_t>(_playSave->getMonthsElapsed()));
		mission->setRace(race);
		mission->start(150);

		_playSave->getAlienMissions().push_back(mission);
	}
} */

/**
 * Handler for hot-keying time-compression.
 * @note The 0xC_kL build does not accept mouse-clicks here; now that I look at
 * it neither does the vanilla code.
 * @param action - pointer to the hot-key Action
 */
void GeoscapeState::keyTimeCompression(Action* action) // private.
{
	if (inputDisabled() == false)
	{
		ImageButton* const sender (dynamic_cast<ImageButton*>(action->getSender()));
		if (sender != _btnGroup)
		{
			_timeSurplus = 0;
			sender->mousePress(_game->getFakeMouseActionD(), this);
		}
	}
}

/**
 * Handler for clicking a time-compression button.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnTimeCompression(Action*) // private.
{
	// NOTE: User-input cannot be disabled for the time-compression buttons
	// w/out screwing around with ImageButton, etc.
	_timeSurplus = 0;
}

/**
 * Pauses and unpauses the Geoscape.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnPauseClick(Action* action) // private.
{
	if (inputDisabled() == false)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
			case SDL_BUTTON_RIGHT:
				_globe->toggleBlink();
				if ((_pauseHard = !_pauseHard) == false)
				{
					_txtSec->setColor(GREEN_SEA);
				}
//				else // done in timeAdvance()
//				{
//					_txtSec->setColor(RED); // NOTE: Does not look red but it will do.
//					_txtSec->setVisible();
//				}
		}
	}
}

/**
 * LMB Opens info.
 * RMB Centers on the UFO corresponding to a blob.
 * @param action - pointer to an Action
 */
void GeoscapeState::btnUfoBlobPress(Action* action) // private.
{
	if (inputDisabled() == false)
	{
		for (size_t // find out which blob was pressed
				i  = 0u;
				i != UFO_HOTBLOBS;
				++i)
		{
			if (_isfUfoBlobs[i] == action->getSender())
			{
				switch (action->getDetails()->button.button)
				{
					case SDL_BUTTON_LEFT:
					{
						Ufo* const ufo (_hostileUfos[i]);
						_game->pushState(new UfoDetectedState(
															ufo,
															this,
															false,
															ufo->getHyperdecoded() == true));
						break;
					}

					case SDL_BUTTON_RIGHT:
					{
						const Ufo* const ufo (_hostileUfos[i]);
						const double
							lon (ufo->getLongitude()),
							lat (ufo->getLatitude());
						_globe->center(lon,lat);
						_globe->setCrosshair(lon,lat);
					}
				}
				break;
			}
		}
	}
	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Sets hard-pause.
 */
void GeoscapeState::setPaused()
{
	_pauseHard = true;
	_globe->toggleBlink();
	resetTimer();

//	_txtSec->setColor(RED);	// NOTE: Does not look red but it will do.
//	_txtSec->setVisible();	// TODO: This won't work until the state redraws.
}							// so it's done in timeAdvance() instead

/**
 * Checks hard-pause.
 * @return, true if state is paused
 */
bool GeoscapeState::isPaused() const
{
	return _pauseHard;
}

/**
 * Examines the quantity of remaining UFO-detected popups.
 * @note Reduces the number by one and decides whether to display the value.
 */
void GeoscapeState::assessUfoPopups()
{
	if (--_ufosDetected == 0)
		_ufoDetected->setVisible(false);
	else
		_ufoDetected->setText(Text::intWide(static_cast<int>(_ufosDetected)));
}

/**
 * Updates the scale.
 * @param dX - reference to x-delta
 * @param dY - reference to y-delta
 */
void GeoscapeState::resize(
		int& dX,
		int& dY)
{
	if (_playSave->getBattleSave() != nullptr)
		return;

	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

	int divisor = 1;
	double pixelRatioY = 1.;

	if (Options::nonSquarePixelRatio)
		pixelRatioY = 1.2;

	switch (Options::geoscapeScale)
	{
		case SCALE_SCREEN_DIV_3: divisor = 3; break;
		case SCALE_SCREEN_DIV_2: divisor = 2; break;
		case SCALE_SCREEN: break;

		default:
			dX =
			dY = 0;
			return;
	}

// G++ linker wants it this way ...
//#ifdef _DEBUG
	const int
		screenWidth  = Screen::ORIGINAL_WIDTH,
		screenHeight = Screen::ORIGINAL_HEIGHT;

	Options::baseXResolution = std::max(screenWidth,
										Options::displayWidth / divisor);
	Options::baseYResolution = std::max(screenHeight,
										static_cast<int>(static_cast<double>(Options::displayHeight)
										 / pixelRatioY / static_cast<double>(divisor)));
//#else
//	Options::baseXResolution = std::max(Screen::ORIGINAL_WIDTH,
//										Options::displayWidth / divisor);
//	Options::baseYResolution = std::max(Screen::ORIGINAL_HEIGHT,
//										static_cast<int>(static_cast<double>(Options::displayHeight)
//										 / pixelRatioY / static_cast<double>(divisor)));
//#endif

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	_globe->resize();

	for (std::vector<Surface*>::const_iterator
			i  = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (*i != _globe)
		{
			(*i)->setX((*i)->getX() + dX);
			(*i)->setY((*i)->getY() + dY / 2);
		}
	}
}

}
