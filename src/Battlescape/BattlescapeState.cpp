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

#include "BattlescapeState.h"

//#include <algorithm>
#include <cstring>
#include <iomanip>
//#include <sstream>

#include <SDL_gfxPrimitives.h>

//#include "../fmath.h"
#include "../lodepng.h"

#include "AbortMissionState.h"
#include "ActionMenuState.h"
#include "BattlescapeGame.h"
#include "BattlescapeGenerator.h"
#include "BriefingState.h"
#include "Camera.h"
#include "DebriefingState.h"
#include "Explosion.h"
#include "InventoryState.h"
#include "Map.h"
#include "MiniMapState.h"
#include "Pathfinding.h"
#include "TileEngine.h"
#include "UnitInfoState.h"
#include "UnitTurnBState.h"
#include "WarningMessage.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/RNG.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Geoscape/DefeatState.h"
#include "../Geoscape/VictoryState.h"

#include "../Interface/Bar.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/NumberText.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"

#include "../Menu/LoadGameState.h"
#include "../Menu/PauseState.h"
#include "../Menu/SaveGameState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape HUD.
 */
BattlescapeState::BattlescapeState()
	:
		_gameSave(_game->getSavedGame()),
		_battleSave(_game->getSavedGame()->getBattleSave()),
		_rules(_game->getRuleset()),
//		_reserve(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseOverThreshold(false),
		_firstInit(true),
		_mouseOverIcons(false),
		_isMouseScrolled(false),
		_isMouseScrolling(false),
		_mouseScrollStartTime(0),
		_fuseFrame(0),
		_showConsole(2),
		_targeterFrame(0),
		_showSoldierData(false),
		_iconsHidden(false),
		_isOverweight(false),
		_autosave(false)
{
	//Log(LOG_INFO) << "Create BattlescapeState";
	STATE_INTERVAL_ALIEN	= static_cast<Uint32>(Options::battleAlienSpeed);
	STATE_INTERVAL_XCOM		= static_cast<Uint32>(Options::battleXcomSpeed);
	STATE_INTERVAL_XCOMDASH	= (STATE_INTERVAL_XCOM << 1u) / 3u;

	const int
		screenWidth		(Options::baseXResolution),
		screenHeight	(Options::baseYResolution),
		iconsWidth		(_rules->getInterface("battlescape")->getElement("icons")->w), // 320
		iconsHeight		(_rules->getInterface("battlescape")->getElement("icons")->h), // 56
		playableHeight	(screenHeight - iconsHeight),
		x				((screenWidth - iconsWidth) >> 1u),
		y				(screenHeight - iconsHeight);

	_txtBaseLabel			= new Text(120, 9, screenWidth - 121, 0);
	_txtRegion				= new Text(120, 9, screenWidth - 121, 10);
	_lstTileInfo			= new TextList(18, 33, screenWidth - 19, 70);
	_txtControlDestroyed	= new Text(iconsWidth, 9, x, y - 20);
	_txtMissionLabel		= new Text(iconsWidth, 9, x, y - 10);
	_txtOperationTitle		= new Text(screenWidth, 16, 0, 2);
	_srfTitle				= new Surface(screenWidth, 21, 0, 0);

	// Create buttonbar - this should appear at the bottom-center of the screen
	_icons = new InteractiveSurface(
								iconsWidth,
								iconsHeight,
								x,y);

	// Create the battlefield view
	// The actual map-height is the total height minus the height of the buttonbar
	_map = new Map(
				_game,
				screenWidth,
				screenHeight,
				0,0,
				playableHeight);

	_numLayers	= new NumberText(3, 5, x + 232, y + 6);
	_numDir		= new NumberText(3, 5, x + 150, y + 6);
	_numDirTur	= new NumberText(3, 5, x + 167, y + 6);

	_srfRank		= new Surface(26, 23, x + 107, y + 33);
	_srfOverweight	= new Surface( 2,  2, x + 130, y + 34);

	_btnUnitUp			= new BattlescapeButton(32,  16, x +  48, y);
	_btnUnitDown		= new BattlescapeButton(32,  16, x +  48, y + 16);
	_btnMapUp			= new BattlescapeButton(32,  16, x +  80, y);
	_btnMapDown			= new BattlescapeButton(32,  16, x +  80, y + 16);
	_btnMiniMap			= new BattlescapeButton(32,  16, x + 112, y);
	_btnKneel			= new BattlescapeButton(32,  16, x + 112, y + 16);
	_btnInventory		= new BattlescapeButton(32,  16, x + 144, y);
	_btnCenter			= new BattlescapeButton(32,  16, x + 144, y + 16);
	_btnNextUnit		= new BattlescapeButton(32,  16, x + 176, y);
	_btnNextStop		= new BattlescapeButton(32,  16, x + 176, y + 16);
	_btnShowLayers		= new BattlescapeButton(32,  16, x + 208, y);
	_btnOptions			= new BattlescapeButton(32,  16, x + 208, y + 16);
	_btnEndTurn			= new BattlescapeButton(32,  16, x + 240, y);
	_btnAbort			= new BattlescapeButton(32,  16, x + 240, y + 16);

/*	_btnReserveNone		= new BattlescapeButton(17, 11, x + 60, y + 33);
	_btnReserveSnap		= new BattlescapeButton(17, 11, x + 78, y + 33);
	_btnReserveAimed	= new BattlescapeButton(17, 11, x + 60, y + 45);
	_btnReserveAuto		= new BattlescapeButton(17, 11, x + 78, y + 45);
	_btnReserveKneel	= new BattlescapeButton(10, 23, x + 96, y + 33);
	_btnZeroTUs			= new BattlescapeButton(10, 23, x + 49, y + 33); */
	_btnStats			= new InteractiveSurface(164, 23, x + 107, y + 33);
	_btnLogo			= new InteractiveSurface( 57, 23, x +  49, y + 33);

	_btnLeftHandItem	= new InteractiveSurface(32, 48, x +   8, y + 5);
	_btnRightHandItem	= new InteractiveSurface(32, 48, x + 280, y + 5);
	_numAmmoL			= new NumberText(7, 5, x +  33, y + 4);
	_numAmmoR			= new NumberText(7, 5, x + 305, y + 4);

	_numFuseL			= new NumberText(7, 5, x +   8, y + 4);
	_numFuseR			= new NumberText(7, 5, x + 280, y + 4);

	_numTwohandL		= new NumberText(7, 5, x +  33, y + 46, true);
	_numTwohandR		= new NumberText(7, 5, x + 305, y + 46, true);

	_numMediL1			= new NumberText(7, 5, x +   9, y + 32);
	_numMediL2			= new NumberText(7, 5, x +   9, y + 39);
	_numMediL3			= new NumberText(7, 5, x +   9, y + 46);
	_numMediR1			= new NumberText(7, 5, x + 281, y + 32);
	_numMediR2			= new NumberText(7, 5, x + 281, y + 39);
	_numMediR3			= new NumberText(7, 5, x + 281, y + 46);
//	const int
//		visibleUnitX = _rules->getInterface("battlescape")->getElement("visibleUnits")->x,
//		visibleUnitY = _rules->getInterface("battlescape")->getElement("visibleUnits")->y;

	_srfTargeter = new Surface(
							32,40,
							(screenWidth >> 1u) - 16,
							playableHeight >> 1u);

	std::fill_n(
			_hostileUnit,
			HOTSQRS,
			static_cast<BattleUnit*>(nullptr));

	int offsetX (0);
	for (size_t
			i = 0u;
			i != HOTSQRS;
			++i)
	{
		if (i > 9) offsetX = 15;

		_btnHostileUnit[i] = new InteractiveSurface(
												15,13,
												x + iconsWidth - 21 - offsetX,
												y - 16 - (static_cast<int>(i) * 13));
		_numHostileUnit[i] = new NumberText(
										8,6,
										x + iconsWidth - 15 - offsetX,
										y - 12 - (static_cast<int>(i) * 13));
	}

	for (size_t // center 10+ on buttons
			i = 9u;
			i != HOTSQRS;
			++i)
	{
		_numHostileUnit[i]->setX(_numHostileUnit[i]->getX() - 2);
	}


	std::fill_n(
			_tileWounded,
			WOUNDED,
			static_cast<Tile*>(nullptr));

	for (size_t
			i = 0u;
			i != WOUNDED;
			++i)
	{
		_btnWounded[i] = new InteractiveSurface(
											9,7,
											x + 5,
											y - 17 - (static_cast<int>(i) * 14));
		_numWounded[i] = new NumberText(
									4,6,
									x + 12,
									y - 20 - (static_cast<int>(i) * 14));
	}

	_warning		= new WarningMessage(
						224,24,
						x + 48,
						y + 32);

	_btnLaunch		= new BattlescapeButton(
						32,24,
						screenWidth - 32,
						20);
	_btnPsi			= new BattlescapeButton(
						32,24,
						screenWidth - 32,
						45);
	_srfBtnBorder	= new Surface(
						32,24,
						screenWidth - 32,
						0);

	_txtName		= new Text(136, 9, x + 135, y + 32);

	_numTULaunch	= new NumberText(8, 10, x + 230, y + 34);
	_numTUAim		= new NumberText(8, 10, x + 241, y + 34);
	_numTUAuto		= new NumberText(8, 10, x + 252, y + 34);
	_numTUSnap		= new NumberText(8, 10, x + 263, y + 34);

	_numTimeUnits	= new NumberText(15, 5, x + 136, y + 42);
	_barTimeUnits	= new Bar(102, 3, x + 170, y + 41);

	_numEnergy		= new NumberText(15, 5, x + 154, y + 42);
	_barEnergy		= new Bar(102, 3, x + 170, y + 45);

	_numHealth		= new NumberText(15, 5, x + 136, y + 50);
	_barHealth		= new Bar(102, 3, x + 170, y + 49);

	_numMorale		= new NumberText(15, 5, x + 154, y + 50);
	_barMorale		= new Bar(102, 3, x + 170, y + 53);

	_txtDebug		= new Text(145, 9, screenWidth - 145, screenHeight - 9);
//	_txtTooltip		= new Text(300, 10, x + 2, y - 10);

	_txtTerrain		= new Text(150,  9, 1,  0);
	_txtShade		= new Text( 50,  9, 1, 10);
	_txtTurn		= new Text(100, 16, 1, 20);

	_txtOrder		= new Text(55, 9, 1, 37);
	_lstSoldierInfo	= new TextList(25, 57, 1, 47);
	_srfAlienIcon	= new Surface(29, 119, 1, 105); // each icon is 9x11 px. so this can contain 3x10 alien-heads = 30.

	_txtConsole1	= new Text(screenWidth >> 1u, y, 0, 0);
	_txtConsole2	= new Text(screenWidth >> 1u, y, screenWidth >> 1u, 0);

	setPalette(PAL_BATTLESCAPE);

	if (_rules->getInterface("battlescape")->getElement("pathfinding") != nullptr)
	{
		const Element* const el (_rules->getInterface("battlescape")->getElement("pathfinding"));
		Pathfinding::green	= static_cast<Uint8>(el->color);
		Pathfinding::yellow	= static_cast<Uint8>(el->color2);
		Pathfinding::red	= static_cast<Uint8>(el->border);
	}

	add(_map);
	_map->init();
	_map->onMouseOver((ActionHandler)& BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)& BattlescapeState::mapPress);
	_map->onMouseClick((ActionHandler)& BattlescapeState::mapClick, 0u);
	_map->onMouseIn((ActionHandler)& BattlescapeState::mapIn);


	add(_icons);
	Surface* const icons (_game->getResourcePack()->getSurface("TacIcons"));
	if (_game->getResourcePack()->getSurface("Logo") != nullptr)
	{
		Surface* const logo (_game->getResourcePack()->getSurface("Logo"));
		logo->setX(48);
		logo->setY(32);
		logo->blit(icons);
	}
	icons->blit(_icons);

	_overlay = _game->getResourcePack()->getSurfaceSet("TacIconsOverlay");
	_bigobs  = _game->getResourcePack()->getSurfaceSet("BIGOBS.PCK");


	add(_srfRank,			"rank",					"battlescape", _icons);
	add(_btnUnitUp,			"buttonUnitUp",			"battlescape", _icons); // note: these are not registered in Interfaces.rul
	add(_btnUnitDown,		"buttonUnitDown",		"battlescape", _icons);
	add(_btnMapUp,			"buttonMapUp",			"battlescape", _icons);
	add(_btnMapDown,		"buttonMapDown",		"battlescape", _icons);
	add(_btnMiniMap,		"buttonShowMap",		"battlescape", _icons);
	add(_btnKneel,			"buttonKneel",			"battlescape", _icons);
	add(_btnInventory,		"buttonInventory",		"battlescape", _icons);
	add(_btnCenter,			"buttonCenter",			"battlescape", _icons);
	add(_btnNextUnit,		"buttonNextUnit",		"battlescape", _icons);
	add(_btnNextStop,		"buttonNextStop",		"battlescape", _icons);
	add(_btnShowLayers,		"buttonShowLayers",		"battlescape", _icons);
	add(_btnOptions,		"buttonHelp",			"battlescape", _icons);
	add(_btnEndTurn,		"buttonEndTurn",		"battlescape", _icons);
	add(_btnAbort,			"buttonAbort",			"battlescape", _icons);
	add(_btnStats,			"buttonStats",			"battlescape", _icons);
	add(_numDir,			"numIcons",				"battlescape", _icons);
	add(_numDirTur,			"numIcons",				"battlescape", _icons);
	add(_numLayers,			"numIcons",				"battlescape", _icons);	// goes overtop _icons
	add(_srfOverweight);													// goes overtop _srfRank
	add(_txtName,			"textName",				"battlescape", _icons);
	add(_numTULaunch,		"numDark",				"battlescape", _icons);
	add(_numTUAim,			"numDark",				"battlescape", _icons);
	add(_numTUAuto,			"numDark",				"battlescape", _icons);
	add(_numTUSnap,			"numDark",				"battlescape", _icons);
	add(_numTimeUnits,		"numTUs",				"battlescape", _icons);
	add(_numEnergy,			"numEnergy",			"battlescape", _icons);
	add(_numHealth,			"numHealth",			"battlescape", _icons);
	add(_numMorale,			"numMorale",			"battlescape", _icons);
	add(_barTimeUnits,		"barTUs",				"battlescape", _icons);
	add(_barEnergy,			"barEnergy",			"battlescape", _icons);
	add(_barHealth,			"barHealth",			"battlescape", _icons);
	add(_barMorale,			"barMorale",			"battlescape", _icons);
/*	add(_btnReserveNone,	"buttonReserveNone",	"battlescape", _icons);
	add(_btnReserveSnap,	"buttonReserveSnap",	"battlescape", _icons);
	add(_btnReserveAimed,	"buttonReserveAimed",	"battlescape", _icons);
	add(_btnReserveAuto,	"buttonReserveAuto",	"battlescape", _icons);
	add(_btnReserveKneel,	"buttonReserveKneel",	"battlescape", _icons); */
	add(_btnLogo,			"buttonZeroTUs",		"battlescape", _icons);
	add(_btnLeftHandItem,	"buttonLeftHand",		"battlescape", _icons);
	add(_btnRightHandItem,	"buttonRightHand",		"battlescape", _icons);
	add(_numAmmoL,			"numLight",				"battlescape", _icons);
	add(_numAmmoR,			"numLight",				"battlescape", _icons);
	add(_numFuseL,			"numLight",				"battlescape", _icons);
	add(_numFuseR,			"numLight",				"battlescape", _icons);
	add(_numTwohandL,		"numDark",				"battlescape", _icons);
	add(_numTwohandR,		"numDark",				"battlescape", _icons);
	add(_numMediL1,			"numDark",				"battlescape", _icons);
	add(_numMediL2,			"numDark",				"battlescape", _icons);
	add(_numMediL3,			"numDark",				"battlescape", _icons);
	add(_numMediR1,			"numDark",				"battlescape", _icons);
	add(_numMediR2,			"numDark",				"battlescape", _icons);
	add(_numMediR3,			"numDark",				"battlescape", _icons);
	add(_srfTargeter);

	_srfTargeter->setVisible(false);

	for (size_t
			i = 0u;
			i != HOTSQRS;
			++i)
	{
		add(_btnHostileUnit[i]);
		add(_numHostileUnit[i]);
	}

	for (size_t
			i = 0u;
			i != WOUNDED;
			++i)
	{
		add(_btnWounded[i]);
		add(_numWounded[i]);
	}


	add(_btnLaunch);
	add(_btnPsi);

	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(0)->blit(_btnLaunch);
	_btnLaunch->onMousePress((ActionHandler)& BattlescapeState::btnLaunchPress);
	_btnLaunch->setVisible(false);

	_game->getResourcePack()->getSurfaceSet("SPICONS.DAT")->getFrame(1)->blit(_btnPsi);
	_btnPsi->onMouseClick((ActionHandler)& BattlescapeState::btnPsiClick);
	_btnPsi->setVisible(false);

	add(_srfBtnBorder);

	_srfBtnBorder->drawRect(0,0, 32,24, WHITE);
	_srfBtnBorder->drawRect(1,1, 30,22, TRANSP);
	_srfBtnBorder->setVisible(false);

//	add(_txtTooltip, "textTooltip", "battlescape", _icons);
//	_txtTooltip->setHighContrast();

	add(_txtDebug,				"textName",			"battlescape");
	add(_warning,				"warning",			"battlescape", _icons);
	add(_txtOperationTitle,		"operationTitle",	"battlescape");
	add(_srfTitle);
	add(_txtBaseLabel,			"infoText",			"battlescape");
	add(_txtRegion,				"infoText",			"battlescape");
	add(_txtControlDestroyed,	"infoText",			"battlescape");
	add(_txtMissionLabel,		"infoText",			"battlescape");

	_txtDebug->setHighContrast();
	_txtDebug->setAlign(ALIGN_RIGHT);

	_warning->setTextColor(static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("warning")->color));
	_warning->setColor(static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("warning")->color2));

	if (_battleSave->getOperation().empty() == false)
	{
		_txtOperationTitle->setText(_battleSave->getOperation());
		_txtOperationTitle->setHighContrast();
		_txtOperationTitle->setAlign(ALIGN_CENTER);
		_txtOperationTitle->setBig();

		const Sint16
			text_width	(static_cast<Sint16>(_txtOperationTitle->getTextWidth()) + 14),
			x_left		((static_cast<Sint16>(screenWidth) - text_width) >> 1u),
			x_right		(x_left + text_width),
			y_high		(static_cast<Sint16>(_srfTitle->getY()) + 1),
			y_low		(static_cast<Sint16>(_srfTitle->getHeight()) - 2);

//		_srfTitle->drawLine( // low line
//						x_left,  y_low,
//						x_right, y_low,
//						2);
		_srfTitle->drawLine( // left line
						x_left, y_high,
						x_left, y_low,
						WHITE);
		_srfTitle->drawLine( // right line
						x_right, y_high,
						x_right, y_low,
						WHITE); // TODO: Get color (Uint8) of OperationTitle [#0].

		_srfTitle->drawLine( // left line shadow
						x_left + 1, y_high + 1,
						x_left + 1, y_low + 1,
						14u);//ORANGE_D);
		_srfTitle->drawLine( // right line shadow
						x_right + 1, y_high + 1,
						x_right + 1, y_low + 1,
						14u);//ORANGE_D);
	}
	else
		_txtOperationTitle->setVisible(false);

	_txtControlDestroyed->setText(tr("STR_ALIEN_BASE_CONTROL_DESTROYED"));
	_txtControlDestroyed->setHighContrast();
	_txtControlDestroyed->setAlign(ALIGN_CENTER);
	_txtControlDestroyed->setVisible(false);

	const Target* target (nullptr);

	std::wstring
		baseLabel,
		missionLabel;

	for (std::vector<Base*>::const_iterator
			i = _gameSave->getBases()->begin();
			i != _gameSave->getBases()->end() && baseLabel.empty() == true;
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			target = dynamic_cast<Target*>(*i);
			baseLabel = (*i)->getName();
			missionLabel = tr("STR_BASE_DEFENSE");
			break;
		}

		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end() && baseLabel.empty() == true;
				++j)
		{
			if ((*j)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*j);
				baseLabel = (*i)->getName();
			}
		}
	}
	_txtBaseLabel->setText(tr("STR_SQUAD_").arg(baseLabel)); // there'd better be a baseLabel ... or else. Pow! To the moon!!!
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setHighContrast();


	if (missionLabel.empty() == true)
	{
		std::wostringstream woststr;

		for (std::vector<Ufo*>::const_iterator
				i = _gameSave->getUfos()->begin();
				i != _gameSave->getUfos()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);

				if ((*i)->isCrashed() == true)
					woststr << tr("STR_UFO_CRASH_RECOVERY");
				else
					woststr << tr("STR_UFO_GROUND_ASSAULT");

				woststr << L"> " << (*i)->getName(_game->getLanguage());
			}
		}

		for (std::vector<MissionSite*>::const_iterator
				i = _gameSave->getMissionSites()->begin();
				i != _gameSave->getMissionSites()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);
				woststr << tr("STR_TERROR_MISSION") << L"> " << (*i)->getName(_game->getLanguage()); // <- not necessarily a Terror Mission ...
			}
		}

		for (std::vector<AlienBase*>::const_iterator
				i = _gameSave->getAlienBases()->begin();
				i != _gameSave->getAlienBases()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);
				woststr << tr("STR_ALIEN_BASE_ASSAULT") << L"> " << (*i)->getName(_game->getLanguage());
			}
		}

		missionLabel = woststr.str();
	}

	if (missionLabel.empty() == true)
		missionLabel = tr(_battleSave->getTacticalType());

	_txtMissionLabel->setText(missionLabel); // there'd better be a missionLabel ... or else. Pow! To the moon!!!
	_txtMissionLabel->setAlign(ALIGN_CENTER);
	_txtMissionLabel->setHighContrast();


	if (target != nullptr)
	{
		std::wostringstream woststr;
		const double
			lon (target->getLongitude()),
			lat (target->getLatitude());

		for (std::vector<Region*>::const_iterator
				i = _gameSave->getRegions()->begin();
				i != _gameSave->getRegions()->end();
				++i)
		{
			if ((*i)->getRules()->insideRegion(lon,lat) == true)
			{
				woststr << tr((*i)->getRules()->getType());
				break;
			}
		}

		for (std::vector<Country*>::const_iterator
				i = _gameSave->getCountries()->begin();
				i != _gameSave->getCountries()->end();
				++i)
		{
			if ((*i)->getRules()->insideCountry(lon,lat) == true)
			{
				woststr << L"> " << tr((*i)->getRules()->getType());
				break;
			}
		}

		_txtRegion->setText(woststr.str()); // there'd better be a region ... or else. Pow! To the moon!!!
		_txtRegion->setAlign(ALIGN_RIGHT);
		_txtRegion->setHighContrast();
	}


	add(_lstTileInfo,	"textName",	"battlescape"); // blue
	add(_txtConsole1,	"textName",	"battlescape");
	add(_txtConsole2,	"textName",	"battlescape");

	_lstTileInfo->setColumns(2, 11, 7);
	_lstTileInfo->setHighContrast();
	_lstTileInfo->setMargin(0);

	_txtConsole1->setHighContrast();
	_txtConsole1->setVisible(_showConsole > 0);
	_txtConsole2->setHighContrast();
	_txtConsole2->setVisible(_showConsole > 1);

	add(_txtTerrain,		"infoText",			"battlescape"); // yellow
	add(_txtShade,			"infoText",			"battlescape");
	add(_txtTurn,			"infoText",			"battlescape");
	add(_txtOrder,			"operationTitle",	"battlescape"); // white
	add(_lstSoldierInfo,	"textName",			"battlescape"); // blue
	add(_srfAlienIcon);

	_txtTerrain->setHighContrast();
	_txtTerrain->setText(tr("STR_TEXTURE_").arg(tr(_battleSave->getBattleTerrain())));

	_txtShade->setHighContrast();
	_txtShade->setText(tr("STR_SHADE_").arg(_battleSave->getTacticalShade()));

	_txtTurn->setHighContrast();
	_txtTurn->setBig();
	_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));

	_txtOrder->setHighContrast();

	_lstSoldierInfo->setHighContrast();
	_lstSoldierInfo->setColumns(2, 10,15);
	_lstSoldierInfo->setMargin(0);

	_srfAlienIcon->setVisible(false);


	_srfRank->setVisible(false);

	_srfOverweight->drawRect(0,0,2,2, RED_D);
	_srfOverweight->setVisible(false);

	_icons->onMouseIn((ActionHandler)& BattlescapeState::mouseInIcons);
	_icons->onMouseOut((ActionHandler)& BattlescapeState::mouseOutIcons);

	_btnUnitUp->onMousePress(
					(ActionHandler)& BattlescapeState::btnUnitUpPress,
					SDL_BUTTON_LEFT);
	_btnUnitUp->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnUnitUpRelease,
					SDL_BUTTON_LEFT);
//	_btnUnitUp->setTooltip("STR_UNIT_LEVEL_ABOVE");
//	_btnUnitUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnUnitUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnUnitDown->onMousePress(
					(ActionHandler)& BattlescapeState::btnUnitDownPress,
					SDL_BUTTON_LEFT);
	_btnUnitDown->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnUnitDownRelease,
					SDL_BUTTON_LEFT);
//	_btnUnitDown->setTooltip("STR_UNIT_LEVEL_BELOW");
//	_btnUnitDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnUnitDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapUp->onMousePress(
					(ActionHandler)& BattlescapeState::btnMapUpPress,
					SDL_BUTTON_LEFT);
	_btnMapUp->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnMapUpPress,
					Options::keyBattleLevelUp);

	_btnMapUp->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnMapUpRelease,
					SDL_BUTTON_LEFT);
	_btnMapUp->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnMapUpRelease,
					Options::keyBattleLevelUp);
//	_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");
//	_btnMapUp->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnMapUp->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMapDown->onMousePress(
					(ActionHandler)& BattlescapeState::btnMapDownPress,
					SDL_BUTTON_LEFT);
	_btnMapDown->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnMapDownPress,
					Options::keyBattleLevelDown);

	_btnMapDown->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnMapDownRelease,
					SDL_BUTTON_LEFT);
	_btnMapDown->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnMapDownRelease,
					Options::keyBattleLevelDown);
//	_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");
//	_btnMapDown->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnMapDown->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnMiniMap->onMouseClick((ActionHandler)& BattlescapeState::btnMinimapClick);
	_btnMiniMap->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnMinimapClick,
					Options::keyBattleMap);
//	_btnMiniMap->setTooltip("STR_MINIMAP");
//	_btnMiniMap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnMiniMap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnKneel->onMouseClick((ActionHandler)& BattlescapeState::btnKneelClick);
	_btnKneel->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnKneelClick,
					Options::keyBattleKneel);
//	_btnKneel->setTooltip("STR_KNEEL");
//	_btnKneel->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnKneel->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnInventory->onMouseClick((ActionHandler)& BattlescapeState::btnInventoryClick);
	_btnInventory->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnInventoryClick,
					Options::keyBattleInventory);
//	_btnInventory->setTooltip("STR_INVENTORY");
//	_btnInventory->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnInventory->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnCenter->onMousePress(
					(ActionHandler)& BattlescapeState::btnCenterPress,
					SDL_BUTTON_LEFT);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnCenterPress,
					Options::keyBattleCenterUnit);
	_btnCenter->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnCenterPress,
					SDLK_KP5);

	_btnCenter->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnCenterRelease,
					SDL_BUTTON_LEFT);
	_btnCenter->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnCenterRelease,
					Options::keyBattleCenterUnit);
	_btnCenter->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnCenterRelease,
					SDLK_KP5);
//	_btnCenter->setTooltip("STR_CENTER_SELECTED_UNIT");
//	_btnCenter->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnCenter->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnNextUnit->onMousePress(
					(ActionHandler)& BattlescapeState::btnNextUnitPress,
					SDL_BUTTON_LEFT);
	_btnNextUnit->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnNextUnitPress,
					Options::keyBattleNextUnit);

	_btnNextUnit->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnNextUnitRelease,
					SDL_BUTTON_LEFT);
	_btnNextUnit->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnNextUnitRelease,
					Options::keyBattleNextUnit);

	_btnNextUnit->onMousePress(
					(ActionHandler)& BattlescapeState::btnPrevUnitPress,
					SDL_BUTTON_RIGHT);
	_btnNextUnit->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnPrevUnitPress,
					Options::keyBattlePrevUnit);

	_btnNextUnit->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnPrevUnitRelease,
					SDL_BUTTON_RIGHT);
	_btnNextUnit->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnPrevUnitRelease,
					Options::keyBattlePrevUnit);
//	_btnNextUnit->setTooltip("STR_NEXT_UNIT");
//	_btnNextUnit->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnNextUnit->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnNextStop->onMousePress(
					(ActionHandler)& BattlescapeState::btnNextStopPress,
					SDL_BUTTON_LEFT);
	_btnNextStop->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnNextStopPress,
					Options::keyBattleDeselectUnit);

	_btnNextStop->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnNextStopRelease,
					SDL_BUTTON_LEFT);
	_btnNextStop->onKeyboardRelease(
					(ActionHandler)& BattlescapeState::btnNextStopRelease,
					Options::keyBattleDeselectUnit);

	_btnNextStop->onMousePress( // NOTE: There's no option for 'keyBattleDeselectPrevUnit'.
					(ActionHandler)& BattlescapeState::btnPrevStopPress,
					SDL_BUTTON_RIGHT);
	_btnNextStop->onMouseRelease(
					(ActionHandler)& BattlescapeState::btnPrevStopRelease,
					SDL_BUTTON_RIGHT);
//	_btnNextStop->setTooltip("STR_DESELECT_UNIT");
//	_btnNextStop->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnNextStop->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnShowLayers->onMouseClick((ActionHandler)& BattlescapeState::btnShowLayersClick);
//	_btnShowLayers->setTooltip("STR_MULTI_LEVEL_VIEW");
//	_btnShowLayers->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnShowLayers->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnOptions->onMouseClick((ActionHandler)& BattlescapeState::btnBattleOptionsClick);
	_btnOptions->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnBattleOptionsClick,
					Options::keyBattleOptions); // = keyCancel. [Escape]
//	_btnOptions->setTooltip("STR_OPTIONS");
//	_btnOptions->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnOptions->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnEndTurn->onMouseClick((ActionHandler)& BattlescapeState::btnEndTurnClick);
	_btnEndTurn->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnEndTurnClick,
					Options::keyBattleEndTurn);
//	_btnEndTurn->setTooltip("STR_END_TURN");
//	_btnEndTurn->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnEndTurn->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnAbort->onMouseClick((ActionHandler)& BattlescapeState::btnAbortClick);
	_btnAbort->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnAbortClick,
					Options::keyBattleAbort);
//	_btnAbort->setTooltip("STR_ABORT_MISSION");
//	_btnAbort->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnAbort->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnStats->onMouseClick((ActionHandler)& BattlescapeState::btnStatsClick);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnStatsClick,
					Options::keyBattleStats);
//	_btnStats->setTooltip("STR_UNIT_STATS");
//	_btnStats->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnStats->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//	_btnLeftHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnLeftHandItemClick);
	_btnLeftHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnLeftHandLeftClick,
					SDL_BUTTON_LEFT);
	_btnLeftHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnLeftHandRightClick,
					SDL_BUTTON_RIGHT);
	_btnLeftHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnLeftHandLeftClick,
					Options::keyBattleUseLeftHand);
//	_btnLeftHandItem->setTooltip("STR_USE_LEFT_HAND");
//	_btnLeftHandItem->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnLeftHandItem->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

//	_btnRightHandItem->onMouseClick((ActionHandler)& BattlescapeState::btnRightHandItemClick);
	_btnRightHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnRightHandLeftClick,
					SDL_BUTTON_LEFT);
	_btnRightHandItem->onMouseClick(
					(ActionHandler)& BattlescapeState::btnRightHandRightClick,
					SDL_BUTTON_RIGHT);
	_btnRightHandItem->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnRightHandLeftClick,
					Options::keyBattleUseRightHand);
//	_btnRightHandItem->setTooltip("STR_USE_RIGHT_HAND");
//	_btnRightHandItem->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnRightHandItem->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

/*	_btnReserveNone->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveNone->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveNone);
	_btnReserveNone->setTooltip("STR_DONT_RESERVE_TIME_UNITS");
	_btnReserveNone->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveNone->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveSnap->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveSnap->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveSnap);
	_btnReserveSnap->setTooltip("STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT");
	_btnReserveSnap->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveSnap->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAimed->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAimed->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveAimed);
	_btnReserveAimed->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT");
	_btnReserveAimed->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAimed->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveAuto->onMouseClick((ActionHandler)& BattlescapeState::btnReserveClick);
	_btnReserveAuto->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveClick,
					Options::keyBattleReserveAuto);
	_btnReserveAuto->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT");
	_btnReserveAuto->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveAuto->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

	_btnReserveKneel->onMouseClick((ActionHandler)& BattlescapeState::btnReserveKneelClick);
	_btnReserveKneel->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnReserveKneelClick,
					Options::keyBattleReserveKneel);
	_btnReserveKneel->setTooltip("STR_RESERVE_TIME_UNITS_FOR_KNEEL");
	_btnReserveKneel->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
	_btnReserveKneel->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);
	_btnReserveKneel->allowToggleInversion(); */

	_btnLogo->onMouseClick(
					(ActionHandler)& BattlescapeState::btnZeroTuClick,
					SDL_BUTTON_LEFT);
	// NOTE: Can't use a specific SDLKey on this because it requires CTRL.
	// InteractiveSurface handlers do not like that ....
	_btnLogo->onKeyboardPress((ActionHandler)& BattlescapeState::keyZeroTuPress);

	_btnLogo->onMouseClick(
					(ActionHandler)& BattlescapeState::btnUfoPaediaClick,
					SDL_BUTTON_RIGHT);
	_btnLogo->onKeyboardPress(
					(ActionHandler)& BattlescapeState::btnUfoPaediaClick,
					Options::keyGeoUfopedia);
//	_btnZeroTUs->setTooltip("STR_EXPEND_ALL_TIME_UNITS");
//	_btnZeroTUs->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//	_btnZeroTUs->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);
//	_btnZeroTUs->allowClickInversion();

	// NOTE: The following shortcuts do not have a specific surface-button graphic.
//	_btnStats->onKeyboardPress( // NOTE: Reloading uses advanced requirements in the Inventory.
//					(ActionHandler)& BattlescapeState::btnReloadClick,
//					Options::keyBattleReload);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::keyUnitLight,
					Options::keyBattlePersonalLighting);
	_btnStats->onKeyboardPress(
					(ActionHandler)& BattlescapeState::keyConsoleToggle,
					Options::keyBattleConsole);

	// NOTE: Can't use a specific SDLKey on this because it can require CTRL.
	// InteractiveSurface handlers do not like that ....
	_btnStats->onKeyboardPress((ActionHandler)& BattlescapeState::keyTurnUnit);



//	const SDLKey buttons[]
//	{
//		Options::keyBattleCenterEnemy1,
//		Options::keyBattleCenterEnemy2,
//		Options::keyBattleCenterEnemy3,
//		Options::keyBattleCenterEnemy4,
//		Options::keyBattleCenterEnemy5,
//		Options::keyBattleCenterEnemy6,
//		Options::keyBattleCenterEnemy7,
//		Options::keyBattleCenterEnemy8,
//		Options::keyBattleCenterEnemy9,
//		Options::keyBattleCenterEnemy10
//	};

	const Uint8 color (static_cast<Uint8>(_rules->getInterface("battlescape")->getElement("visibleUnits")->color));
	for (size_t
			i = 0u;
			i != HOTSQRS;
			++i)
	{
		_btnHostileUnit[i]->onMousePress((ActionHandler)& BattlescapeState::btnHostileUnitPress);
//		_btnHostileUnit[i]->onKeyboardPress(
//						(ActionHandler)& BattlescapeState::btnHostileUnitPress,
//						buttons[i]);

//		std::ostringstream tooltip;
//		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
//		_btnHostileUnit[i]->setTooltip(tooltip.str());
//		_btnHostileUnit[i]->onMouseIn((ActionHandler)& BattlescapeState::txtTooltipIn);
//		_btnHostileUnit[i]->onMouseOut((ActionHandler)& BattlescapeState::txtTooltipOut);

		_numHostileUnit[i]->setColor(color);
		_numHostileUnit[i]->setValue(static_cast<unsigned>(i) + 1u);
	}

	for (size_t
			i = 0u;
			i != WOUNDED;
			++i)
	{
		_btnWounded[i]->onMousePress((ActionHandler)& BattlescapeState::btnWoundedPress);
	}

	_txtName->setHighContrast();

	_numTwohandR->setValue(2u);
	_numTwohandL->setValue(2u);

/*	_btnReserveNone->setGroup(&_reserve);
	_btnReserveSnap->setGroup(&_reserve);
	_btnReserveAimed->setGroup(&_reserve);
	_btnReserveAuto->setGroup(&_reserve); */

	_game->getFpsCounter()->setY(screenHeight - 6);

	_aniTimer = new Timer(STATE_INTERVAL_TILE); // setStateInterval() does NOT change this <-
	_aniTimer->onTimer((StateHandler)& BattlescapeState::animate);

	_tacticalTimer = new Timer(STATE_INTERVAL_STANDARD); // setStateInterval() will change this <-
	_tacticalTimer->onTimer((StateHandler)& BattlescapeState::handleState);
	//_tacticalTimer->debug("BattlescapeState");

	_battleGame = new BattlescapeGame(_battleSave, this);
	//Log(LOG_INFO) << "Create BattlescapeState EXIT";
}

/**
 * Deletes this BattlescapeState.
 */
BattlescapeState::~BattlescapeState()
{
	//Log(LOG_INFO) << "Delete BattlescapeState";
	delete _aniTimer;
	delete _tacticalTimer;
	delete _battleGame;
}

/**
 * Initializes this BattlescapeState.
 */
void BattlescapeState::init()
{
	State::init();

	_aniTimer->start();
	_tacticalTimer->start();

	_map->setFocus(true);
	_map->cacheUnits();
	_map->draw();

	_battleGame->init();

	updateSoldierInfo(false); // NOTE: Does not need calcFoV, done in BattlescapeGame::init(first init).

//	switch (_battleSave->getBatReserved())
//	{
//		case BA_SNAPSHOT: _reserve = _btnReserveSnap; break;
//		case BA_AIMEDSHOT: _reserve = _btnReserveAimed; break;
//		case BA_AUTOSHOT: _reserve = _btnReserveAuto; break;
//		default: _reserve = _btnReserveNone; break;
//	}

	if (_firstInit == true && playableUnitSelected() == true)
	{
		_firstInit = false;
		_battleGame->setupSelector();

		_map->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition(), false);

		std::string
			track,
			terrain;
		_battleSave->calibrateMusic(track, terrain);
		_game->getResourcePack()->playMusic(track, terrain);

//		_btnReserveNone->setGroup(&_reserve);
//		_btnReserveSnap->setGroup(&_reserve);
//		_btnReserveAimed->setGroup(&_reserve);
//		_btnReserveAuto->setGroup(&_reserve);
	}

	_numLayers->setValue(static_cast<unsigned>(_map->getCamera()->getViewLevel()) + 1);

	if (_battleSave->getControlDestroyed() == true && _iconsHidden == false)
		_txtControlDestroyed->setVisible();
	else
		_txtControlDestroyed->setVisible(false);

	if (_autosave == true)
	{
		_autosave = false;
		if (_gameSave->isIronman() == true)
			_game->pushState(new SaveGameState(
											OPT_BATTLESCAPE,
											SAVE_IRONMAN,
											_palette));
		else if (Options::autosave == true) // NOTE: Auto-save points are fucked; they should be done *before* important events, not after.
			_game->pushState(new SaveGameState(
											OPT_BATTLESCAPE,
											SAVE_AUTO_BATTLESCAPE,
											_palette));
	}

//	_txtTooltip->setText(L"");
//	if (_battleSave->getKneelReserved())
//		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);
//	_btnReserveKneel->toggle(_battleSave->getKneelReserved());
//	_battleGame->setKneelReserved(_battleSave->getKneelReserved());

	//Log(LOG_INFO) << "BattlescapeState::init() EXIT";
}

/**
 * Runs the Timers and handles popups.
 * @note Called by the engine-core.
 */
void BattlescapeState::think()
{
	//Log(LOG_INFO) << "BattlescapeState::think()";
	if (_tacticalTimer->isRunning() == true)
	{
		static bool popped;

		if (_popups.empty() == true)
		{
			State::think();

			//Log(LOG_INFO) << "BattlescapeState::think() -> _battleGame.think()";
			_battleGame->think();
			//Log(LOG_INFO) << "BattlescapeState::think() -> _aniTimer.think()";
			_aniTimer->think(this, nullptr);
			//Log(LOG_INFO) << "BattlescapeState::think() -> _tacticalTimer.think()";
			_tacticalTimer->think(this, nullptr);
			//Log(LOG_INFO) << "BattlescapeState::think() -> back from thinks";

			if (popped == true)
			{
				popped = false;
				_battleGame->handleNonTargetAction();
			}
		}
		else // Handle popups
		{
			popped = true;
			_game->pushState(*_popups.begin());
			_popups.erase(_popups.begin());
		}
	}
	//Log(LOG_INFO) << "BattlescapeState::think() EXIT";
}

/**
 * Processes any mouse-motion over the Map.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapOver(Action* action)
{
	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// What follows is a workaround for a rare problem where sometimes the
		// mouse-release event is missed for some reason. However if SDL also
		// missed the release event then this won't work.
		//
		// This part handles the release if it's missed and another button is used.
		if ((SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_offsetPreDragScroll);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
			return;
		}

		_isMouseScrolled = true;

		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false)
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;


		if (Options::battleDragScrollInvert == true) // scroll. I don't use inverted scrolling.
		{
			_map->getCamera()->scrollXY(
									static_cast<int>(static_cast<double>(-action->getDetails()->motion.xrel) / action->getScaleX()),
									static_cast<int>(static_cast<double>(-action->getDetails()->motion.yrel) / action->getScaleY()),
									false);
			_map->setSelectorType(CT_NONE);
		}
		else
			_map->getCamera()->scrollXY(
									static_cast<int>(static_cast<double>(action->getDetails()->motion.xrel) * 3.62 / action->getScaleX()),
									static_cast<int>(static_cast<double>(action->getDetails()->motion.yrel) * 3.62 / action->getScaleY()),
									false);

		_game->getCursor()->handle(action);
	}
	else if (_mouseOverIcons == false && allowButtons() == true
		&& _game->getCursor()->getHidden() == false)
	{
		Position pos;
		_map->getSelectorPosition(&pos);

		Tile* const tile (_battleSave->getTile(pos));
		updateTileInfo(tile);

		if (_showConsole > 0)
			printTileInventory(tile);
	}
//	else if (_mouseOverIcons == true){} // might need to erase some info here.
}

/**
 * Prints contents of hovered Tile's inventory to screen.
 * @note This should have been done w/ vectors but it works as-is.
 * @param tile - mouseOver tile
 */
void BattlescapeState::printTileInventory(Tile* const tile) // private.
{
	_txtConsole1->setText(L"");
	_txtConsole2->setText(L"");

	bool showInfo;

	if (tile != nullptr
		&& tile->isRevealed(ST_CONTENT) == true
		&& tile->getInventory()->empty() == false)
	{
		showInfo = false;

		size_t row (0u);
		std::wostringstream
			woststr,	// test
			woststr1,	// Console #1
			woststr2;	// Console #2
		std::wstring
			wst,		// for transient manipulations
			wst1,		// first-pass
			wst2,		// for adding ammoQty and newline
			wst3;		// second-pass (will be compared to 1st pass to check for duplicate items)
		int qty (1);

		for (size_t
				i = 0u;
				i != tile->getInventory()->size() + 1u;
				++i)
		{
			wst1 = L"> ";

			if (i < tile->getInventory()->size())
			{
				const BattleItem* const item (tile->getInventory()->at(i));
				const RuleItem* const itRule (item->getRules());

				if (item->getUnit() != nullptr)
				{
					if (item->getUnit()->getType().compare(0u,11u, "STR_FLOATER") == 0) // See medikit w/ Autopsy OR inventory w/ Autopsy+Race research.
					{
						wst1 += tr("STR_FLOATER");
//						wst1 += L" (status doubtful)";
					}
					else
					{
						switch (item->getUnit()->getUnitStatus())
						{
							case STATUS_UNCONSCIOUS:
								wst1 += item->getUnit()->getName(_game->getLanguage());

								if (item->getUnit()->getGeoscapeSoldier() != nullptr)
									wst1 += L" (" + Text::intWide(item->getUnit()->getHealth() - item->getUnit()->getStun() - 1) + L")";
								break;

							case STATUS_DEAD:
								wst1 += tr(itRule->getType());

								if (item->getUnit()->getGeoscapeSoldier() != nullptr)
									wst1 += L" (" + item->getUnit()->getName(_game->getLanguage()) + L")";
						}
					}
				}
				else if (_gameSave->isResearched(itRule->getRequirements()) == true)
				{
					wst1 += tr(itRule->getType());

					switch (itRule->getBattleType())
					{
						case BT_AMMO:
							wst1 += L" (" + Text::intWide(item->getAmmoQuantity()) + L")";
							break;

						case BT_FIREARM:
//						case BT_MELEE:
							{
								const BattleItem* const aItem (item->getAmmoItem());
								if ((aItem != nullptr && item->selfPowered() == false)
									 || item->selfExpended() == true)
								{
									wst = tr(aItem->getRules()->getType());
									wst1 += L" | " + wst + L" (" + Text::intWide(aItem->getAmmoQuantity()) + L")";
								}
							}
							break;

						case BT_GRENADE:
							if (item->getFuse() != -1)
								wst1 += L" (" + Text::intWide(item->getFuse()) + L")";
							break;

						case BT_PROXYGRENADE:
						case BT_FLARE:
							if (item->getFuse() != -1)
								wst1 += L" (*)";
					}
				}
				else
					wst1 += tr("STR_ALIEN_ARTIFACT");
			}

			if (i == 0)
			{
				wst3 =
				wst2 = wst1;
				continue;
			}

			if (wst1 == wst3)
			{
				++qty;
				continue;
			}

			if (qty > 1)
			{
				wst2 += L" * " + Text::intWide(qty);
				qty = 1;
			}

			wst2 += L"\n";
			woststr << wst2;
			wst3 =
			wst2 = wst1;

			if (row < 26u) // Console #1
			{
				if (row == 25u)
				{
					woststr << L"> ";
					++row;
				}

				woststr1.str(L"");
				woststr1 << woststr.str();

				if (row == 26u)
				{
					if (wst1 == L"> ")
					{
						wst = woststr1.str();
						wst.erase(wst.length() - 2u);
						woststr1.str(L"");
						woststr1 << wst;
					}

					if (_showConsole == 1)
						break;

					woststr.str(L"");
				}
			}
			else // row > 25 // Console #2
			{
				if (row == 50u)
					woststr << L"> ";

				woststr2.str(L"");
				woststr2 << woststr.str();

				if (row == 51u)
				{
					if (wst1 == L"> ")
					{
						wst = woststr2.str();
						wst.erase(wst.length() - 2u);
						woststr2.str(L"");
						woststr2 << wst;
					}
					break;
				}
			}
			++row;
		}

		_txtConsole1->setText(woststr1.str());
		_txtConsole2->setText(woststr2.str());
	}
	else
		showInfo = true;

	_txtTerrain->setVisible(showInfo);
	_txtShade->setVisible(showInfo);
	_txtTurn->setVisible(showInfo);

	_txtOrder->setVisible(showInfo);
	_lstSoldierInfo->setVisible(showInfo);
	_srfAlienIcon->setVisible(showInfo && allowAlienIcons());
	_showSoldierData = showInfo;
}

/**
 * Processes any presses on the Map.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapPress(Action* action)
{
	if (_mouseOverIcons == false
		&& action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		_offsetPreDragScroll = _map->getCamera()->getMapOffset();

		_totalMouseMoveX =
		_totalMouseMoveY = 0;
		_mouseOverThreshold = false;
		_mouseScrollStartTime = SDL_GetTicks();
	}
}

/**
 * Processes any mouse-clicks on the Map.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapClick(Action* action)
{
	// What follows is a workaround for a rare problem where sometimes the
	// mouse-release event is missed for some reason. However if SDL also
	// missed the release event then this won't work.
	//
	// This part handles the release if it's missed and another button is used.
	if (_isMouseScrolling == true)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_map->getCamera()->setMapOffset(_offsetPreDragScroll);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
		}
	}

	if (_isMouseScrolling == true) // dragScroll-button release: release mouse-scroll-mode
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton) // other buttons are ineffective while scrolling
			return;

		_isMouseScrolling = false;

		// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
		}

		if (_isMouseScrolled == true) return;
	}


	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT // right-click removes pathPreview or aborts walking state
		&& _battleGame->cancelTacticalAction() == true)
	{
		return;
	}

	if (_mouseOverIcons == false
		&& _map->getSelectorType() != CT_NONE
		&& _battleGame->isBusy() == false)
	{
		Position pos;
		_map->getSelectorPosition(&pos);

		if (_battleSave->getTile(pos) != nullptr)
		{
			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_LEFT:
					_battleGame->primaryAction(pos);
					break;

				case SDL_BUTTON_RIGHT:
					if (playableUnitSelected() == true)
						_battleGame->secondaryAction(pos);
			}

//			if (_battleSave->getDebugTac() == true)
//			{
			std::wostringstream woststr; // onScreen debug ->
//			if (_battleSave->getTile(pos)->getMapData(O_OBJECT) != nullptr)
//				woststr << (int)(_battleSave->getTile(pos)->getMapData(O_OBJECT)->getBigwall()) << L" ";

			if (_battleSave->getTile(pos)->getTileUnit() != nullptr)
				woststr	<< L"unit "
						<< _battleSave->getTile(pos)->getTileUnit()->getId()
						<< L" ";

			woststr << L"pos " << pos;
			printDebug(woststr.str());
//			}
		}
	}
}

/**
 * Handles mouse entering the Map surface.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapIn(Action*)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(static_cast<Uint8>(Options::battleDragScrollButton), false);
}

/**
 * Takes care of any events from the core-engine.
 * @param action - pointer to an Action
 */
inline void BattlescapeState::handle(Action* action)
{
	if (_firstInit == true)
		return;

	bool doit;
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_RIGHT: // ... not sure what all this is on about; but here's a refactor of that.
			switch (action->getDetails()->type)
			{
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					doit = true;
					break;

				default:
					doit = false;
			}
			break;

		default:
			if (_game->getCursor()->getVisible() == true)
				doit = true;
			else
				doit = false;
	}

	if (doit == true)
//	if (_game->getCursor()->getVisible() == true
//		|| (action->getDetails()->button.button == SDL_BUTTON_RIGHT
//			&& (action->getDetails()->type == SDL_MOUSEBUTTONDOWN
//				|| action->getDetails()->type == SDL_MOUSEBUTTONUP)))
	{
		State::handle(action);

		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			bool beep (false);

			if (Options::debug == true)
			{
				if ((SDL_GetModState() & KMOD_CTRL) != 0)
				{
					if (_battleSave->getDebugTac() == false)
					{
						if (action->getDetails()->key.keysym.sym == SDLK_d		// "ctrl-d" - enable debug mode.
							&& allowButtons() == true)							// - disallow turning debug-mode on during a non-
						{														//   player turn else the HUD won't show back up.
							beep = true;
							_battleSave->debugTac();
							printDebug(L"debug set active");
						}
						else
							printDebug(L"player turn only");
					}
					else
					{
						bool casualties (false);
						switch (action->getDetails()->key.keysym.sym)
						{
							case SDLK_d:										// "ctrl-d" - debug already enabled.
								printDebug(L"debug already active");
								break;

							case SDLK_v:										// "ctrl-v" - reset tile visibility.
								beep = true;
								printDebug(L"blacking all tiles");
								_battleSave->blackTiles();
								break;

							case SDLK_k:										// "ctrl-k" - kill all aliens.
								beep = true; //MB_ICONERROR
								printDebug(L"dispersing influenza");
								for (std::vector<BattleUnit*>::const_iterator
										i = _battleSave->getUnits()->begin();
										i !=_battleSave->getUnits()->end();
										++i)
								{
									if ((*i)->getOriginalFaction() == FACTION_HOSTILE
										&& (*i)->isOut_t(OUT_HEALTH) == false)
									{
										casualties = true;
										(*i)->setHealth(0);
//										(*i)->takeDamage(Position(0,0,0), 1000, DT_AP, true);
									}
								}
								break;

							case SDLK_j:										// "ctrl-j" - stun all aliens.
								beep = true; //MB_ICONWARNING
								printDebug(L"deploying Celine Dione");
								for (std::vector<BattleUnit*>::const_iterator
									i = _battleSave->getUnits()->begin();
									i !=_battleSave->getUnits()->end();
									++i)
								{
									if ((*i)->getOriginalFaction() == FACTION_HOSTILE
										&& (*i)->isOut_t(OUT_HEALTH) == false)
									{
										casualties = true;
										(*i)->setStun((*i)->getHealth() + 1000);
//										(*i)->takeDamage(Position(0,0,0), 1000, DT_STUN, true);
									}
								}
						}

						if (casualties == true)
						{
							_battleGame->checkCasualties(nullptr, nullptr, true);
							_battleGame->handleState();
						}
					}
				}
				else
				{
					switch (action->getDetails()->key.keysym.sym)
					{
//						case SDLK_F10:													// f10 - voxel-map dump. - moved below_
//							beep = true;
//							saveVoxelMap();
//							break;

						case SDLK_F9:													// f9 - ai dump. TODO: Put in Options.
							beep = true;
							saveAIMap();
					}
				}
			}

			if (action->getDetails()->key.keysym.sym == Options::keyBattleVoxelView)	// f11 - voxel-view pic.
			{
				beep = true;
				saveVoxelView();
			}
			else if (action->getDetails()->key.keysym.sym == SDLK_F10)					// f10 - voxel-map dump. - from above^ TODO: Put in Options.
			{
				beep = true;
				saveVoxelMap();
			}
			else if (_gameSave->isIronman() == false)
			{
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)		// f6 - quickSave.
				{
					beep = true;
					_game->pushState(new SaveGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
				}
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)	// f5 - quickLoad.
				{
					beep = true;
					_game->pushState(new LoadGameState(
													OPT_BATTLESCAPE,
													SAVE_QUICK,
													_palette));
				}
			}

#ifdef _WIN32
			if (beep == true) MessageBeep(MB_OK);
#endif
		}
//		else if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
//		{
//			if (action->getDetails()->button.button == SDL_BUTTON_X1)
//				btnNextUnitPress(action);
//			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
//				btnPrevUnitPress(action);
//		}
	}
}

/**
 * Moves the selected unit up.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitUpPress(Action*)
{
	if (playableUnitSelected() == true)
	{
		Pathfinding* const pf (_battleSave->getPathfinding());
		pf->setInputModifiers();
		if (pf->isModAlt() == false)
		{
			pf->setPathingUnit(_battleSave->getSelectedUnit());
			switch (pf->validateUpDown(
									_battleSave->getSelectedUnit()->getPosition(),
									Pathfinding::DIR_UP))
			{
				case FLY_CANT:
					warning("STR_ACTION_NOT_ALLOWED_NOFLY");
					break;

				case FLY_BLOCKED:
					warning("STR_ACTION_NOT_ALLOWED_ROOF");
					break;

				case FLY_GRAVLIFT:
				case FLY_GOOD:
					_overlay->getFrame(0)->blit(_btnUnitUp);
					_battleGame->cancelTacticalAction();
					_battleGame->moveUpDown(
										_battleSave->getSelectedUnit(),
										Pathfinding::DIR_UP);
			}
		}
		else
			warning("STR_ACTION_NOT_ALLOWED_NOFLY");
	}
}

/**
 * Releases the Unitup btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitUpRelease(Action*)
{
	_btnUnitUp->clear();
}

/**
 * Moves the selected unit down.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitDownPress(Action*)
{
	if (playableUnitSelected() == true)
	{
		Pathfinding* const pf (_battleSave->getPathfinding());
		pf->setInputModifiers();
		pf->setPathingUnit(_battleSave->getSelectedUnit());

		switch (pf->validateUpDown(
								_battleSave->getSelectedUnit()->getPosition(),
								Pathfinding::DIR_DOWN))
		{
			case FLY_CANT:
			case FLY_BLOCKED:
				warning("STR_ACTION_NOT_ALLOWED_FLOOR");
				break;

			case FLY_GRAVLIFT:
			case FLY_GOOD:
				_overlay->getFrame(7)->blit(_btnUnitDown);
				_battleGame->cancelTacticalAction();
				_battleGame->moveUpDown(
									_battleSave->getSelectedUnit(),
									Pathfinding::DIR_DOWN);
		}
	}
}

/**
 * Releases the Unitdown btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUnitDownRelease(Action*)
{
	_btnUnitDown->clear();
}

/**
 * Shows the next upper map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapUpPress(Action* action)
{
	if (allowButtons() == true
		&& _map->getCamera()->up() == true)
	{
		if (action != nullptr) // prevent hotSqrs from depressing btn.
			_overlay->getFrame(1)->blit(_btnMapUp);

		refreshMousePosition();
	}
}

/**
 * Releases the Mapup btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapUpRelease(Action*)
{
	_btnMapUp->clear();
}

/**
 * Shows the next lower map layer.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapDownPress(Action* action)
{
	if (allowButtons() == true
		&& _map->getCamera()->down() == true)
	{
		if (action != nullptr) // prevent hotSqrs from depressing btn.
			_overlay->getFrame(8)->blit(_btnMapDown);

		refreshMousePosition();
	}
}

/**
 * Releases the Mapdown btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMapDownRelease(Action*)
{
	_btnMapDown->clear();
}

/**
 * Shows the MiniMap.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnMinimapClick(Action*)
{
	if (allowButtons() == true)
	{
//		_overlay->getFrame(2)->blit(_btnMiniMap); // -> hidden by MiniMap itself atm.
		_game->pushState(new MiniMapState(
										_map->getCamera(),
										_battleSave));
		_game->getScreen()->fadeScreen();
	}
}

/**
 * Clears the ShowMap btn.
 * @note To be called from MiniMapState::btnOkClick()
 *
void BattlescapeState::clearMinimapBtn()
{
	_btnMiniMap->clear();
} */

/**
 * Toggles the current unit's kneel/standup status.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnKneelClick(Action*)
{
	if (allowButtons() == true)
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (unit != nullptr)
		{
			//Log(LOG_INFO) << "BattlescapeState::btnKneelClick()";
			if (_battleGame->kneelToggle(unit) == true)
			{
				_battleGame->getTileEngine()->calcFovPos(
													unit->getPosition(),
													true);
				// need that here, so that my newVis algorithm works without
				// false positives, or true negatives as it were, when a soldier
				// stands up and walks in one go via UnitWalkBState. Because if
				// I calculate newVis in kneel() it says 'yeh you see something'
				// but the soldier wouldn't stop - so newVis has to be calculated
				// directly in UnitWalkBState.... yet by doing it here on the
				// btn-press, the enemy visibility indicator should light up.

				updateSoldierInfo(false);

				// Will check reactionFire in BattlescapeGame::kneel()
				// no, no it won't.
				_battleGame->getTileEngine()->checkReactionFire(unit);

				Pathfinding* const pf (_battleGame->getPathfinding());
				if (pf->isPathPreviewed() == true)
				{
					pf->setPathingUnit(_battleGame->getTacticalAction()->actor);
					pf->calculatePath(
								_battleGame->getTacticalAction()->actor,
								_battleGame->getTacticalAction()->posTarget);
					pf->clearPreview();
					pf->previewPath();
				}
			}
//			toggleKneelButton(unit); // <- handled by BattlescapeGame::kneel()
		}
	}
}

/**
 * Goes to the Inventory screen.
// * @note Additionally resets TUs for current side in debug mode.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnInventoryClick(Action*)
{
//	if (_battleSave->getDebugTac() == true) // CHEAT For debugging.
//	{
//		for (std::vector<BattleUnit*>::const_iterator
//				i = _battleSave->getUnits()->begin();
//				i != _battleSave->getUnits()->end();
//				++i)
//		{
//			if ((*i)->getFaction() == _battleSave->getSide())
//				(*i)->prepUnit();
//			updateSoldierInfo();
//		}
//	}

	if (playableUnitSelected() == true)
	{
		const BattleUnit* const unit (_battleSave->getSelectedUnit());

		if (unit->getGeoscapeSoldier() != nullptr
			|| (unit->getUnitRules()->isMechanical() == false
				&& unit->getRankString() != "STR_LIVE_TERRORIST")
			|| _battleSave->getDebugTac() == true)
		{
			if (_battleGame->getTacticalAction()->type == BA_LAUNCH) // clean up the waypoints
			{
				_battleGame->getTacticalAction()->waypoints.clear();
				_map->getWaypoints()->clear();
				_btnLaunch->setVisible(false);
			}

			_battleGame->cancelTacticalAction(true);
			_battleGame->setupSelector();
//			_overlay->getFrame(3)->blit(_btnInventory); // clear() not implemented @ InventoryState.
			_game->pushState(new InventoryState(
											true, //_battleSave->getDebugTac() == false, // CHEAT For debugging.
											this));
			_game->getScreen()->fadeScreen();
		}
	}
}

/**
 * Forces a transparent SDL mouse-motion event.
 * @note This is required to create an arbitrary mouseOver event for when the
 * Map is repositioned under the cursor but the cursor itself doesn't
 * necessarily move on the screen.
 * @sa ListGamesState::think()
 */
void BattlescapeState::refreshMousePosition() const
{
	_game->getCursor()->fakeMotion();

	int // doesn't do shit. FIXED.
		x,y,
		dir;
	SDL_GetMouseState(&x,&y);

	if (x == 0)	dir = +1;
	else		dir = -1;

	SDL_WarpMouse(
			static_cast<Uint16>(x + dir),
			static_cast<Uint16>(y));
	SDL_GetMouseState(&x,&y);
	SDL_WarpMouse(
			static_cast<Uint16>(x - dir),
			static_cast<Uint16>(y));
}

/**
 * Centers on the currently selected BattleUnit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnCenterPress(Action* action)
{
	if (playableUnitSelected() == true)
	{
		if (action != nullptr) // prevent NextTurnState from depressing btn.
			_overlay->getFrame(10)->blit(_btnCenter);

		_map->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());
		refreshMousePosition();
	}
}

/**
 * Releases the Center btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnCenterRelease(Action*)
{
	_btnCenter->clear();	// For most of these buttons, refreshMousePosition() is enough; but
	mapOver(nullptr);		// for Center, it also wants mapOver() - here - to update Tile info.
}

/**
 * Selects the next BattleUnit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextUnitPress(Action*)
{
	if (_battleGame->getTacticalAction()->type == BA_NONE
		&& allowButtons() == true)
	{
		_overlay->getFrame(4)->blit(_btnNextUnit);
		selectNextPlayerUnit(true);
		refreshMousePosition();
	}
}

/**
 * Releases the Nextunit btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextUnitRelease(Action*)
{
	_btnNextUnit->clear();
}

/**
 * Disables reselection of the current BattleUnit and selects the next BattleUnit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextStopPress(Action*)
{
	if (_battleGame->getTacticalAction()->type == BA_NONE
		&& allowButtons() == true)
	{
		_overlay->getFrame(11)->blit(_btnNextStop);
		selectNextPlayerUnit(true, true);
		refreshMousePosition();
	}
}

/**
 * Releases the Nextstop btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnNextStopRelease(Action*)
{
	_btnNextStop->clear();
}

/**
 * Selects next BattleUnit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevUnitPress(Action*)
{
	if (_battleGame->getTacticalAction()->type == BA_NONE
		&& allowButtons() == true)
	{
		_overlay->getFrame(4)->blit(_btnNextUnit);
		selectPreviousPlayerUnit(true);
		refreshMousePosition();
	}
}

/**
 * Releases the Prevunit btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevUnitRelease(Action*)
{
	_btnNextUnit->clear();
}

/**
 * Disables reselection of the current BattleUnit and selects the next BattleUnit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevStopPress(Action*)
{
	if (_battleGame->getTacticalAction()->type == BA_NONE
		&& allowButtons() == true)
	{
		_overlay->getFrame(11)->blit(_btnNextStop);
		selectPreviousPlayerUnit(true, true);
		refreshMousePosition();
	}
}

/**
 * Releases the Prevstop btn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPrevStopRelease(Action*)
{
	_btnNextStop->clear();
}

/**
 * Selects the player's next BattleUnit.
 * @param checkReselect		- don't select a unit that has been previously flagged (default false)
 * @param dontReselect		- flag the current unit first (default false)
 * @param checkInventory	- don't select a unit that has no inventory (default false)
 */
void BattlescapeState::selectNextPlayerUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
//	if (allowButtons() == true)
//		&& _battleGame->getTacticalAction()->type == BA_NONE)
//	{
	BattleUnit* const unit (_battleSave->selectNextFactionUnit(
															checkReselect,
															dontReselect,
															checkInventory));
	updateSoldierInfo(false); // try no calcFov()

	if (unit != nullptr)
		_map->getCamera()->centerOnPosition(unit->getPosition());

	_battleGame->cancelTacticalAction();
	_battleGame->setupSelector();
//	}
}

/**
 * Selects the player's previous BattleUnit.
 * @param checkReselect		- don't select a unit that has been previously flagged (default false)
 * @param dontReselect		- flag the current unit first (default false)
 * @param checkInventory	- don't select a unit that has no inventory (default false)
 */
void BattlescapeState::selectPreviousPlayerUnit(
		bool checkReselect,
		bool dontReselect,
		bool checkInventory)
{
//	if (allowButtons() == true)
//		&& _battleGame->getTacticalAction()->type == BA_NONE)
//	{
	BattleUnit* const unit (_battleSave->selectPreviousFactionUnit(
																checkReselect,
																dontReselect,
																checkInventory));
	updateSoldierInfo(false); // try no calcFov()

	if (unit != nullptr)
		_map->getCamera()->centerOnPosition(unit->getPosition());

	_battleGame->cancelTacticalAction();
	_battleGame->setupSelector();
//	}
}
/**
 * Shows/hides all map layers.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnShowLayersClick(Action*)
{
	if (allowButtons() == true)
	{
		if (_map->getCamera()->toggleShowLayers() == true)
			_overlay->getFrame(5)->blit(_btnShowLayers);
		else
			_btnShowLayers->clear();
	}
}

/**
 * Sets the level on the icons' Layers button.
 * @param level - Z level
 */
void BattlescapeState::setLayerValue(int level)
{
//	if (level < 0) level = 0;
	_numLayers->setValue(static_cast<unsigned>((level + 1) % 10));
}

/**
 * Shows Options.
 * @note Acts first as a Cancel button. whee, now I can cancel unit-walk
 * precisely where I want: use [Escape].
 * @param action - pointer to an Action
 */
void BattlescapeState::btnBattleOptionsClick(Action*)
{
	if (allowButtons(true) == true
		&& _battleGame->cancelTacticalAction() == false)
	{
		_overlay->getFrame(12)->blit(_btnOptions);
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
	}
}

/**
 * Clears the Options btn.
 * @note To be called from PauseState::btnOkClick()
 */
void BattlescapeState::clearOptionsBtn()
{
	_btnOptions->clear();
}

/**
 * Requests the end of turn.
 * @note This will add a NULL-state to the end of the state queue so all ongoing
 * actions like explosions are finished first before really ending a turn.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnEndTurnClick(Action*)
{
	if (allowButtons() == true)
	{
//		_txtTooltip->setText(L"");
//		_overlay->getFrame(6)->blit(_btnEndTurn);
		_battleGame->requestEndTurn();
	}
}

/**
 * Clears the EndTurn btn.
 *
void BattlescapeState::clearEndTurnBtn()
{
	_btnEndTurn->clear();
} */

/**
 * Aborts the mission.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnAbortClick(Action*)
{
	if (allowButtons() == true)
	{
		_overlay->getFrame(13)->blit(_btnAbort);
		_game->pushState(new AbortMissionState(_battleSave, this));
	}
}

/**
 * Clears the Abort btn.
 * @note To be used in AbortMissionState::btnCancelClick().
 */
void BattlescapeState::clearAbortBtn()
{
	_btnAbort->clear();
}

/**
 * Shows the selected BattleUnit's info.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnStatsClick(Action* action)
{
	if (playableUnitSelected() == true)
	{
		if (Options::battleEdgeScroll == MAP_SCROLL_TRIGGER // NOTE: This is just ... code-for-the-sake-of-code.
			&& action->getDetails()->type == SDL_MOUSEBUTTONUP
			&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			const int
				posX (action->getMouseX()),
				posY (action->getMouseY());
			if (   (posX > 0 && posX < Camera::SCROLL_BORDER * action->getScaleX())
				||  posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getScaleX()
				|| (posY > 0 && posY < Camera::SCROLL_BORDER * action->getScaleY())
				||  posY > (_map->getHeight() - Camera::SCROLL_BORDER) * action->getScaleY())
			{
				return;	// To avoid handling a mouse-release event as a click on the stats-button
			}			// when the cursor is on the scroll-border if trigger-scroll is enabled.
		}

		if (_battleGame->getTacticalAction()->type == BA_LAUNCH)	// clean up any BL-waypoints
		{															// probly handled in cancelTacticalAction() below_
			_battleGame->getTacticalAction()->waypoints.clear();	// but i don't want to look it up atm.
			_map->getWaypoints()->clear();
			_btnLaunch->setVisible(false);
		}

		_battleGame->cancelTacticalAction(true);
		popup(new UnitInfoState(
							_battleSave->getSelectedUnit(),
							this));
		_game->getScreen()->fadeScreen();
	}
}

/**
 * Shows an ActionMenu popup.
 * @note Creates a tactical battle-action when an entry is clicked.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelTacticalAction();

		BattleUnit* const unit (_battleSave->getSelectedUnit());
		unit->setActiveHand(AH_LEFT);

		_map->cacheUnit(unit);
		_map->draw();

		handAction(
				unit->getItem(ST_LEFTHAND),
				unit->getFatalWound(BODYPART_LEFTARM) != 0);
	}
}

/**
 * Sets left-hand as the Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelTacticalAction();

		BattleUnit* const unit (_battleSave->getSelectedUnit());
		unit->setActiveHand(AH_LEFT);
		updateSoldierInfo(false);

		_map->cacheUnit(unit);
		_map->draw();
	}
}

/**
 * Shows an ActionMenu popup.
 * @note Creates a tactical battle-action when an entry is clicked.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandLeftClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelTacticalAction();

		BattleUnit* const unit (_battleSave->getSelectedUnit());
		unit->setActiveHand(AH_RIGHT);

		_map->cacheUnit(unit);
		_map->draw();

		handAction(
				unit->getItem(ST_RIGHTHAND),
				unit->getFatalWound(BODYPART_LEFTARM) != 0);
	}
}

/**
 * Sets right-hand as the Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
	{
		_battleGame->cancelTacticalAction();

		BattleUnit* const unit (_battleSave->getSelectedUnit());
		unit->setActiveHand(AH_RIGHT);
		updateSoldierInfo(false);

		_map->cacheUnit(unit);
		_map->draw();
	}
}

/**
 * LMB centers on the hotile-unit corresponding to the button clicked.
 * RMB cycles through player's spotters of that unit.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnHostileUnitPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_WHEELUP:
			btnMapDownPress(nullptr);
			break;
		case SDL_BUTTON_WHEELDOWN:
			btnMapUpPress(nullptr);
			break;

		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		{
			size_t i; // find out which button was pressed
			for (
					i = 0u;
					i != HOTSQRS;
					++i)
			{
				if (_btnHostileUnit[i] == action->getSender())
					break;
			}

			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_LEFT:
					// *** cppCheck false positive ***
					// kL_note: Invoke cppCheck w/ "--inline-suppr BattlescapeState.cpp"
					// it says this is going to try accessing _hostileUnit[] at index=HOTSQRS.
					// cppcheck-suppress arrayIndexOutOfBounds
					_map->getCamera()->centerOnPosition(_hostileUnit[i]->getPosition());
					// (but note that it makes no such burp against _hostileUnit[] below_)

					_srfTargeter->setVisible();
					_targeterFrame = 0;
					break;

				case SDL_BUTTON_RIGHT:
				{
					BattleUnit* nextSpotter (nullptr);
					size_t curIter (0u);

					for (std::vector<BattleUnit*>::const_iterator
						j = _battleSave->getUnits()->begin();
						j != _battleSave->getUnits()->end();
						++j)
					{
						++curIter;
						if (*j == _battleSave->getSelectedUnit())
							break;
					}

					for (std::vector<BattleUnit*>::const_iterator
						j = _battleSave->getUnits()->begin() + curIter;
						j != _battleSave->getUnits()->end();
						++j)
					{
						if ((*j)->getFaction() == FACTION_PLAYER
							&& (*j)->isOut_t(OUT_STAT) == false
							&& std::find(
									(*j)->getHostileUnits().begin(),
									(*j)->getHostileUnits().end(),
									_hostileUnit[i]) != (*j)->getHostileUnits().end())
						{
							nextSpotter = *j;
							break;
						}
					}

					if (nextSpotter == nullptr)
					{
						for (std::vector<BattleUnit*>::const_iterator
							j = _battleSave->getUnits()->begin();
							j != _battleSave->getUnits()->end() - _battleSave->getUnits()->size() + curIter;
							++j)
						{
							if ((*j)->getFaction() == FACTION_PLAYER
								&& (*j)->isOut_t(OUT_STAT) == false
								&& std::find(
										(*j)->getHostileUnits().begin(),
										(*j)->getHostileUnits().end(),
										_hostileUnit[i]) != (*j)->getHostileUnits().end())
							{
								nextSpotter = *j;
								break;
							}
						}
					}

					if (nextSpotter != nullptr)
					{
						if (nextSpotter != _battleSave->getSelectedUnit())
						{
							_battleSave->setSelectedUnit(nextSpotter);
							updateSoldierInfo(false); // try no calcFov()

							_battleGame->cancelTacticalAction();
							_battleGame->getTacticalAction()->actor = nextSpotter;

							_battleGame->setupSelector();
						}

						Camera* const camera (_map->getCamera());
						if (camera->isOnScreen(nextSpotter->getPosition()) == false
							|| camera->getViewLevel() != nextSpotter->getPosition().z)
						{
							camera->centerOnPosition(nextSpotter->getPosition());
						}
					}
				}
			}
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Centers on a wounded Soldier.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnWoundedPress(Action* action)
{
	const Uint8 btnId (action->getDetails()->button.button);
	switch (btnId)
	{
		case SDL_BUTTON_WHEELUP:
			btnMapDownPress(nullptr);
			break;
		case SDL_BUTTON_WHEELDOWN:
			btnMapUpPress(nullptr);
			break;

		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		{
			for (size_t
					i = 0u;
					i != WOUNDED;
					++i)
			{
				if (_btnWounded[i] == action->getSender())
				{
					_map->getCamera()->centerOnPosition(_tileWounded[i]->getPosition());

					if (btnId == SDL_BUTTON_LEFT)
					{
						BattleUnit* const unit (_tileWounded[i]->getTileUnit());
						if (unit != nullptr && unit != _battleSave->getSelectedUnit())
						{
							_battleSave->setSelectedUnit(unit);
							updateSoldierInfo(false); // try no calcFov()

							_battleGame->cancelTacticalAction();
							_battleGame->getTacticalAction()->actor = unit;

							_battleGame->setupSelector();
						}
					}
					break;
				}
			}
		}
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Launches the blaster bomb.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLaunchPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			_srfBtnBorder->setY(20);
			_srfBtnBorder->setVisible();

			_battleGame->launchAction();
			break;

		case SDL_BUTTON_RIGHT:
			_battleGame->getTacticalAction()->waypoints.clear();
			_map->getWaypoints()->clear();
			_btnLaunch->setVisible(false);

			_battleGame->cancelTacticalAction(true);
			_battleGame->setupSelector();
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Player uses aLien psionics.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnPsiClick(Action* action)
{
	if (_map->getSelectorType() != CT_PSI
		&& _battleGame->getTacticalAction()->waypoints.empty() == true)
	{
		_srfBtnBorder->setY(45);
		_srfBtnBorder->setVisible();

		_battleGame->psiButtonAction();
	}

	action->getDetails()->type = SDL_NOEVENT; // consume the event
}

/**
 * Reserves time units.
 * @param action - pointer to an Action
 *
void BattlescapeState::btnReserveClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.,0., 0,0);
		action->getSender()->mousePress(&a, this);

		if		(_reserve == _btnReserveNone)	_battleGame->setReservedAction(BA_NONE);
		else if (_reserve == _btnReserveSnap)	_battleGame->setReservedAction(BA_SNAPSHOT);
		else if (_reserve == _btnReserveAimed)	_battleGame->setReservedAction(BA_AIMEDSHOT);
		else if (_reserve == _btnReserveAuto)	_battleGame->setReservedAction(BA_AUTOSHOT);

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->clearPreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
} */
/**
 * Reserves time units for kneeling.
 * @param action - pointer to an Action
 *
void BattlescapeState::btnReserveKneelClick(Action* action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;

		Action a = Action(&ev, 0.,0., 0,0);
		action->getSender()->mousePress(&a, this);
		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());
//		_btnReserveKneel->invert(_btnReserveKneel->getColor()+3);
		_btnReserveKneel->toggle(_battleGame->getKneelReserved());

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->clearPreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
} */
/**
 * Reloads a weapon in hand.
 * @note Checks right hand then left.
 * @param action - pointer to an Action
 *
void BattlescapeState::btnReloadClick(Action*)
{
	if (playableUnitSelected() == true
		&& _battleSave->getSelectedUnit()->checkReload() == true)
	{
		_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_RELOAD)
									->play(-1, _map->getSoundAngle(_battleSave->getSelectedUnit()->getPosition()));
		updateSoldierInfo(false);
	}
} */

/**
 * Zeroes TU of the currently selected BattleUnit w/ mouse-click.
 * @note Requires CTRL-key down and BattleStates inactive.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnZeroTuClick(Action* action)
{
	if ((SDL_GetModState() & KMOD_CTRL) != 0
		&& playableUnitSelected() == true)
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (_battleGame->noActionsPending(unit) == true)
		{
//			SDL_Event ev;
//			ev.type = SDL_MOUSEBUTTONDOWN;
//			ev.button.button = SDL_BUTTON_LEFT;
//			Action a (Action(&ev, 0.,0.,0,0));
//			action->getSender()->mousePress(&a, this); // why mouse event, for keyboard-press perhaps

			unit->setTimeUnits(0);
			_numTimeUnits->setValue(0u);
			_barTimeUnits->setValue(0.);

			_battleGame->cancelTacticalAction();
		}
	}
}

/**
 * Zeroes TU of the currently selected BattleUnit w/ key-press.
 * @note Requires CTRL-key down and BattleStates inactive.
 * @param action - pointer to an Action
 */
void BattlescapeState::keyZeroTuPress(Action* action)
{
	if ((SDL_GetModState() & KMOD_CTRL) != 0
		&& (action->getDetails()->key.keysym.sym == Options::keyBattleZeroTUs
			|| action->getDetails()->key.keysym.sym == SDLK_KP_PERIOD)
		&& playableUnitSelected() == true)
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (_battleGame->noActionsPending(unit) == true)
		{
			unit->setTimeUnits(0);
			_numTimeUnits->setValue(0u);
			_barTimeUnits->setValue(0.);

			_battleGame->cancelTacticalAction();
		}
	}
}

/**
 * Opens the UfoPaedia.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnUfoPaediaClick(Action*)
{
	if (allowButtons() == true)
		Ufopaedia::open(_game, true);
}

/**
 * Toggles unit's personal lighting (purely cosmetic).
 * @param action - pointer to an Action
 */
void BattlescapeState::keyUnitLight(Action*)
{
	if (allowButtons() == true)
		_battleSave->getTileEngine()->togglePersonalLighting();
}

/**
 * Toggles the display-state of the console.
 * @param action - pointer to an Action
 */
void BattlescapeState::keyConsoleToggle(Action*)
{
	if (allowButtons() == true)
	{
		switch (_showConsole)
		{
			case 0:
				_showConsole = 1;
				_txtConsole2->setText(L"");
				break;

			case 1:
				_showConsole = 2;
				break;

			case 2:
				_showConsole = 0;
				_txtConsole1->setText(L"");
				_txtConsole2->setText(L"");

				_txtTerrain->setVisible();
				_txtShade->setVisible();
				_txtTurn->setVisible();

				_txtOrder->setVisible();
				_lstSoldierInfo->setVisible();
				_srfAlienIcon->setVisible(allowAlienIcons());
				_showSoldierData = true;
		}

		_txtConsole1->setVisible(_showConsole > 0);
		_txtConsole2->setVisible(_showConsole > 1);
		refreshMousePosition();
	}
}

/**
 * Pivots the selected BattleUnit if any clockwise or counter-clockwise.
 * @param action - pointer to an Action
 */
void BattlescapeState::keyTurnUnit(Action* action)
{
	switch (action->getDetails()->key.keysym.sym)
	{
		case SDLK_COMMA:
		case SDLK_PERIOD:
			if (playableUnitSelected() == true)
			{
				BattleAction* const tacAction (_battleGame->getTacticalAction());
				tacAction->actor = _battleSave->getSelectedUnit();
				tacAction->targeting = false;

				int dir;
				switch (action->getDetails()->key.keysym.sym)
				{
					case SDLK_COMMA:  dir = -1; break;	// pivot unit counter-clockwise
					case SDLK_PERIOD: dir = +1; break;	// pivot unit clockwise

					default: dir = 0;					// should never happen.
				}

				if (tacAction->actor->getTurretType() != TRT_NONE
					&& (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					tacAction->strafe = true;
					dir += tacAction->actor->getTurretDirection();
				}
				else
				{
					tacAction->strafe = false;
					dir += tacAction->actor->getUnitDirection();
				}
				tacAction->value = (dir + 8) % 8;

				_battleGame->statePushBack(new UnitTurnBState(_battleGame, *tacAction));
			}
	}
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action - pointer to an Action
 *
void BattlescapeState::txtTooltipIn(Action* action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtTooltip->setText(tr(_currentTooltip));
	}
} */
/**
 * Clears the tooltip text.
 * @param action - pointer to an Action
 *
void BattlescapeState::txtTooltipOut(Action* action)
{
	if (allowButtons() && Options::battleTooltips)
		if (_currentTooltip == action->getSender()->getTooltip())
			_txtTooltip->setText(L"");
} */

/**
 * Determines whether the player is allowed to press buttons.
 * @note Buttons are disabled in the middle of a shot, during the alien turn,
 * and while a player's units are panicking. The save button is an exception to
 * still be able to save if something goes wrong during the alien turn and
 * submit the save file for dissection.
 * @param allowSave - true if the Options button was clicked (default false)
 * @return, true if the player can still press buttons
 */
bool BattlescapeState::allowButtons(bool allowSave) const // private
{
	return (
			(allowSave == true
					|| _battleSave->getSide() == FACTION_PLAYER
					|| _battleSave->getDebugTac() == true)
				&& (_battleGame->playerPanicHandled() == true
					|| _firstInit == true)
				&& _map->getProjectile() == nullptr);
}

/**
 * Determines whether a playable unit is selected.
 * @note Normally only player side units can be selected but in debug mode one
 * can play with aliens too :)
 * @note Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeState::playableUnitSelected()
{
	return _battleSave->getSelectedUnit() != nullptr
		&& allowButtons() == true;
}

/**
 * Updates a unit's onScreen stats & info.
 * @param calcFoV - true to run calcFov() for the unit (default true)
 */
void BattlescapeState::updateSoldierInfo(bool calcFoV)
{
	hotSqrsClear();

	_srfRank			->clear();
	_btnRightHandItem	->clear();
	_btnLeftHandItem	->clear();
	_btnKneel			->clear();

	_btnLeftHandItem	->setVisible(false);
	_btnRightHandItem	->setVisible(false);
	_numAmmoL			->setVisible(false);
	_numAmmoR			->setVisible(false);
	_numFuseL			->setVisible(false);
	_numFuseR			->setVisible(false);

	_numTwohandL		->setVisible(false);
	_numTwohandR		->setVisible(false);

	_numMediL1			->setVisible(false);
	_numMediL2			->setVisible(false);
	_numMediL3			->setVisible(false);
	_numMediR1			->setVisible(false);
	_numMediR2			->setVisible(false);
	_numMediR3			->setVisible(false);

	_srfOverweight		->setVisible(false);
	_numDir				->setVisible(false);
	_numDirTur			->setVisible(false);

	_numTULaunch		->setVisible(false);
	_numTUAim			->setVisible(false);
	_numTUAuto			->setVisible(false);
	_numTUSnap			->setVisible(false);

	_isOverweight = false;

	_txtOrder->setText(L"");


	if (playableUnitSelected() == false) // not a controlled unit; ie. aLien or civilian turn
	{
		showPsiButton(false);

		_txtName->setText(L"");

		_srfRank		->setVisible(false);

		_numTimeUnits	->setVisible(false);
		_barTimeUnits	->setVisible(false);
		_barTimeUnits	->setVisible(false);

		_numEnergy		->setVisible(false);
		_barEnergy		->setVisible(false);
		_barEnergy		->setVisible(false);

		_numHealth		->setVisible(false);
		_barHealth		->setVisible(false);
		_barHealth		->setVisible(false);

		_numMorale		->setVisible(false);
		_barMorale		->setVisible(false);
		_barMorale		->setVisible(false);

		return;
	}
//	else not aLien nor civilian; ie. a controlled unit ->>

	_srfRank		->setVisible();

	_numTimeUnits	->setVisible();
	_barTimeUnits	->setVisible();
	_barTimeUnits	->setVisible();

	_numEnergy		->setVisible();
	_barEnergy		->setVisible();
	_barEnergy		->setVisible();

	_numHealth		->setVisible();
	_barHealth		->setVisible();
	_barHealth		->setVisible();

	_numMorale		->setVisible();
	_barMorale		->setVisible();
	_barMorale		->setVisible();


	BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	if (calcFoV == true)
		_battleSave->getTileEngine()->calcFov(selUnit, false); // try no tile-reveal.

	if (_battleSave->getSide() == FACTION_PLAYER)
		hotSqrsUpdate();

	_txtName->setText(selUnit->getName(
									_game->getLanguage(),
									false));

	const Soldier* const sol (selUnit->getGeoscapeSoldier());
	if (sol != nullptr)
	{
		SurfaceSet* const texture (_game->getResourcePack()->getSurfaceSet("SMOKE.PCK"));
		texture->getFrame(20 + sol->getRank())->blit(_srfRank);

		if (selUnit->isKneeled() == true)
			_overlay->getFrame(9)->blit(_btnKneel);

		_txtOrder->setText(tr("STR_ORDER")
							.arg(static_cast<int>(selUnit->getBattleOrder())));
	}

	if (selUnit->getCarriedWeight() > selUnit->getStrength())
		_isOverweight = true;

	_numDir->setValue(selUnit->getUnitDirection());
	_numDir->setVisible();

	if (selUnit->getTurretType() != TRT_NONE)
	{
		_numDirTur->setValue(selUnit->getTurretDirection());
		_numDirTur->setVisible();
	}


	double stat (static_cast<double>(selUnit->getBattleStats()->tu));
	const int tu (selUnit->getTimeUnits());
	_numTimeUnits->setValue(static_cast<unsigned>(tu));
	_barTimeUnits->setValue(std::ceil(
							static_cast<double>(tu) / stat * 100.));

	stat = static_cast<double>(selUnit->getBattleStats()->stamina);
	const int energy (selUnit->getEnergy());
	_numEnergy->setValue(static_cast<unsigned>(energy));
	_barEnergy->setValue(std::ceil(
							static_cast<double>(energy) / stat * 100.));

	stat = static_cast<double>(selUnit->getBattleStats()->health);
	const int health (selUnit->getHealth());
	_numHealth->setValue(static_cast<unsigned>(health));
	_barHealth->setValue(std::ceil(
							static_cast<double>(health) / stat * 100.));
	_barHealth->setValue2(std::ceil(
							static_cast<double>(selUnit->getStun()) / stat * 100.));

	const int morale (selUnit->getMorale());
	_numMorale->setValue(static_cast<unsigned>(morale));
	_barMorale->setValue(morale);


	const BattleItem
		* const rtItem (selUnit->getItem(ST_RIGHTHAND)),
		* const ltItem (selUnit->getItem(ST_LEFTHAND));
	const RuleItem* itRule;

	ActiveHand ah (selUnit->getActiveHand());
	if (ah != AH_NONE)
	{
		int
			tuLaunch (0),
			tuAim    (0),
			tuAuto   (0),
			tuSnap   (0);

		switch (ah)
		{
			case AH_RIGHT:
				itRule = rtItem->getRules();
				if (itRule->getBattleType() == BT_FIREARM
					|| itRule->getBattleType() == BT_MELEE)
				{
					tuLaunch = selUnit->getActionTu(BA_LAUNCH, rtItem);
					tuAim = selUnit->getActionTu(BA_AIMEDSHOT, rtItem);
					tuAuto = selUnit->getActionTu(BA_AUTOSHOT, rtItem);
					tuSnap = selUnit->getActionTu(BA_SNAPSHOT, rtItem);
					if (tuLaunch == 0
						&& tuAim == 0
						&& tuAuto == 0
						&& tuSnap == 0)
					{
						tuSnap = selUnit->getActionTu(BA_MELEE, rtItem);
					}
				}
				break;

			case AH_LEFT:
				itRule = ltItem->getRules();
				if (itRule->getBattleType() == BT_FIREARM
					|| itRule->getBattleType() == BT_MELEE)
				{
					tuLaunch = selUnit->getActionTu(BA_LAUNCH, ltItem);
					tuAim = selUnit->getActionTu(BA_AIMEDSHOT, ltItem);
					tuAuto = selUnit->getActionTu(BA_AUTOSHOT, ltItem);
					tuSnap = selUnit->getActionTu(BA_SNAPSHOT, ltItem);
					if (tuLaunch == 0
						&& tuAim == 0
						&& tuAuto == 0
						&& tuSnap == 0)
					{
						tuSnap = selUnit->getActionTu(BA_MELEE, ltItem);
					}
				}
		}

		if (tuLaunch != 0)
		{
			_numTULaunch->setValue(tuLaunch);
			_numTULaunch->setVisible();
		}

		if (tuAim != 0)
		{
			_numTUAim->setValue(tuAim);
			_numTUAim->setVisible();
		}

		if (tuAuto != 0)
		{
			_numTUAuto->setValue(tuAuto);
			_numTUAuto->setVisible();
		}

		if (tuSnap != 0)
		{
			_numTUSnap->setValue(tuSnap);
			_numTUSnap->setVisible();
		}
	}

	if (rtItem != nullptr)
	{
		itRule = rtItem->getRules();
		itRule->drawHandSprite(
							_bigobs,
							_btnRightHandItem);
		_btnRightHandItem->setVisible();

		if (itRule->isFixed() == false
			&& itRule->isTwoHanded() == true)
		{
			_numTwohandR->setVisible();
		}

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
//			case BT_MELEE:
				{
					const BattleItem* const aItem (rtItem->getAmmoItem());
					if ((aItem != nullptr && rtItem->selfPowered() == false)
						|| rtItem->selfExpended() == true)
					{
						const int load (aItem->getAmmoQuantity());
						_numAmmoR->setValue(static_cast<unsigned>(load));
						_numAmmoR->setVisible();

						Uint8 color;
						int clip;
						if (itRule->isFixed() == true)
							clip = itRule->getFullClip();
						else
							clip = aItem->getRules()->getFullClip();

						if		(load == clip)		color = GREEN_D;
						else if	(load >= clip / 2)	color = YELLOW_D;
						else						color = ORANGE_D;

						_numAmmoR->setColor(color);
					}
				}
				break;

			case BT_AMMO:
				_numAmmoR->setVisible();
				_numAmmoR->setValue(static_cast<unsigned>(rtItem->getAmmoQuantity()));
				break;

			case BT_GRENADE:
				if (rtItem->getFuse() > 0)
				{
					_numFuseR->setVisible();
					_numFuseR->setValue(static_cast<unsigned>(rtItem->getFuse()));
				}
				break;

			case BT_MEDIKIT:
				_numMediR2->setVisible();
				_numMediR1->setVisible();
				_numMediR3->setVisible();
				_numMediR1->setValue(static_cast<unsigned>(rtItem->getPainKillerQuantity()));
				_numMediR2->setValue(static_cast<unsigned>(rtItem->getStimulantQuantity()));
				_numMediR3->setValue(static_cast<unsigned>(rtItem->getHealQuantity()));
		}
	}

	if (ltItem != nullptr)
	{
		itRule = ltItem->getRules();
		itRule->drawHandSprite(
							_bigobs,
							_btnLeftHandItem);
		_btnLeftHandItem->setVisible();

		if (itRule->isFixed() == false
			&& itRule->isTwoHanded() == true)
		{
			_numTwohandL->setVisible();
		}

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
//			case BT_MELEE:
				{
					const BattleItem* const aItem (ltItem->getAmmoItem());
					if ((aItem != nullptr && ltItem->selfPowered() == false)
						 || ltItem->selfExpended() == true)
					{
						const int load (aItem->getAmmoQuantity());
						_numAmmoL->setValue(static_cast<unsigned>(load));
						_numAmmoL->setVisible();

						Uint8 color;
						int clip;
						if (itRule->isFixed() == true)
							clip = itRule->getFullClip();
						else
							clip = aItem->getRules()->getFullClip();

						if		(load == clip)		color = GREEN_D;
						else if	(load >= clip / 2)	color = YELLOW_D;
						else						color = ORANGE_D;

						_numAmmoL->setColor(color);
					}
				}
				break;

			case BT_AMMO:
				_numAmmoL->setVisible();
				_numAmmoL->setValue(static_cast<unsigned>(ltItem->getAmmoQuantity()));
				break;

			case BT_GRENADE:
				if (ltItem->getFuse() > 0)
				{
					_numFuseL->setVisible();
					_numFuseL->setValue(static_cast<unsigned>(ltItem->getFuse()));
				}
				break;

			case BT_MEDIKIT:
				_numMediL2->setVisible();
				_numMediL1->setVisible();
				_numMediL3->setVisible();
				_numMediL1->setValue(static_cast<unsigned>(ltItem->getPainKillerQuantity()));
				_numMediL2->setValue(static_cast<unsigned>(ltItem->getStimulantQuantity()));
				_numMediL3->setValue(static_cast<unsigned>(ltItem->getHealQuantity()));
		}
	}

	showPsiButton( // getSpecialWeapon() != nullptr
			selUnit->getOriginalFaction() == FACTION_HOSTILE
			&& selUnit->getBattleStats()->psiSkill != 0);
}

/**
 * Clears the hostile unit indicator squares.
 */
void BattlescapeState::hotSqrsClear()
{
	for (size_t // hide target indicators & clear targets
			i = 0u;
			i != HOTSQRS;
			++i)
	{
		_btnHostileUnit[i]->setVisible(false);
		_numHostileUnit[i]->setVisible(false);
		_hostileUnit[i] = nullptr;
	}
}

/**
 * Updates the hostile unit indicator squares.
 */
void BattlescapeState::hotSqrsUpdate()
{
	size_t j (0u);
	for (std::vector<BattleUnit*>::const_iterator
		i = _battleSave->getUnits()->begin();
		i != _battleSave->getUnits()->end() && j != HOTSQRS;
		++i)
	{
		if ((*i)->getFaction() == FACTION_HOSTILE
			&& (*i)->getUnitVisible() == true
			&& (*i)->isOut_t(OUT_STAT) == false)
		{
			_btnHostileUnit[j]->setVisible();
			_numHostileUnit[j]->setVisible();
			_hostileUnit[j++] = *i;
		}
	}
}

/**
 * Refreshes the wounded units indicators.
 */
void BattlescapeState::hotWoundsRefresh()
{
	static Surface* const srfBadge (_game->getResourcePack()->getSurface("RANK_ROOKIE"));

	for (size_t // hide target indicators & clear tiles
			i = 0u;
			i != WOUNDED;
			++i)
	{
		_btnWounded[i]->clear();
		_btnWounded[i]->setVisible(false);
		_numWounded[i]->setVisible(false);
		_tileWounded[i] = nullptr;
	}

	const bool vis (_battleSave->getSide() == FACTION_PLAYER);
	const BattleUnit* unit;
	Tile* tile;
	for (size_t
			i = 0u, k = 0u;
			i != _battleSave->getMapSizeXYZ() && k != WOUNDED;
			++i)
	{
		tile = _battleSave->getTiles()[i];

		unit = tile->getTileUnit();
		if (unit != nullptr
			&& unit->getFatalWounds() != 0
			&& unit->getFaction() == FACTION_PLAYER
			&& unit->isMindControlled() == false
			&& unit->getGeoscapeSoldier() != nullptr
			&& unit->isOut_t(OUT_HEALTH) == false)
		{
			srfBadge->blit(_btnWounded[k]);
			_btnWounded[k]->setVisible(vis);

			_numWounded[k]->setValue(static_cast<int>(unit->getFatalWounds()));
			_numWounded[k]->setVisible(vis);

			_tileWounded[k++] = tile;
		}

		for (std::vector<BattleItem*>::const_iterator
				j = tile->getInventory()->begin();
				j != tile->getInventory()->end();
				++j)
		{
			unit = (*j)->getUnit();
			if (unit != nullptr
				&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
				&& unit->getFatalWounds() != 0
				&& unit->getFaction() == FACTION_PLAYER
				&& unit->getGeoscapeSoldier() != nullptr)
			{
				srfBadge->blit(_btnWounded[k]);
				_btnWounded[k]->setVisible(vis);

				_numWounded[k]->setValue(static_cast<int>(unit->getFatalWounds()));
				_numWounded[k]->setVisible(vis);

				_tileWounded[k++] = tile;
			}
		}
	}
}

/**
 * Animates red cross icon(s) on Wounded hot-icons.
 */
void BattlescapeState::flashMedic() // private.
{
	static int phase;
	static Surface* const srfCross (_game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(11)); // gray cross

	for (size_t
			i = 0u;
			i != WOUNDED;
			++i)
	{
		if (_btnWounded[i]->getVisible() == true)
		{
			_btnWounded[i]->lock();
			srfCross->blitNShade(
							_btnWounded[i],
							_btnWounded[i]->getX() + 2,
							_btnWounded[i]->getY() + 1,
							phase, false, 3); // red
			_btnWounded[i]->unlock();

			_numWounded[i]->setColor(static_cast<Uint8>(YELLOW + phase));
		}
		else
			break;
	}

	if ((phase += 2) == 16) phase = 0;
}

/**
 * Blinks the health bar when selected unit has fatal wounds.
 */
void BattlescapeState::blinkHealthBar() // private.
{
	static const int TICKS (5);
	static int vis;

	_barHealth->setVisible(++vis > TICKS / 2);

	if (vis == TICKS) vis = 0;
}

/**
 * Shows a selected unit's kneeled state.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeState::toggleKneelButton(BattleUnit* unit)
{
	if (unit != nullptr && unit->isKneeled() == true)
		_overlay->getFrame(9)->blit(_btnKneel);
	else
		_btnKneel->clear();
}

/**
 * Animates things on the Map and in the HUD.
 */
void BattlescapeState::animate()
{
	_map->animateMap(_battleGame->isBusy() == false);	// this needs to happen regardless so that UFO
														// doors (&tc) do not stall walking units (&tc)
	if (_map->getMapHidden() == false)
	{
		if (_battleGame->getShotgun() == true)
			shotgunExplosion();

		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			flashMedic();

			BattleUnit* const selUnit (_battleSave->getSelectedUnit());
			if (selUnit != nullptr)
			{
				hotSqrsCycle(selUnit);
				cycleFuses(selUnit);

				if (selUnit->getFatalWounds() != 0)
					blinkHealthBar();

				if (_srfTargeter->getVisible() == true)
					hostileTargeter();

				if (_battleGame->getLiquidate() == true)
					liquidationExplosion();

				if (_isOverweight == true && RNG::seedless(0,3) == 0)
					_srfOverweight->setVisible(!_srfOverweight->getVisible());

				static int stickyTiks;
				if (_srfBtnBorder->getVisible() == true
					&& ++stickyTiks == 3)
				{
					stickyTiks = 0;
					_srfBtnBorder->setVisible(false);
				}
			}
		}
	}
}

/**
 * Animates primer warnings on hand-held live grenades.
 * @param selUnit - the currently selected BattleUnit
 */
void BattlescapeState::cycleFuses(BattleUnit* const selUnit) // private.
{
	static Surface* const srf (_game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9)); // plus sign
	static const int pulse[PULSE_FRAMES] { 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,
										  13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3 };

	if (_fuseFrame == PULSE_FRAMES)
		_fuseFrame = 0u;

	const BattleItem* item (selUnit->getItem(ST_LEFTHAND));
	if (item != nullptr && item->getFuse() != -1)
	{
		switch (item->getRules()->getBattleType())
		{
			case BT_GRENADE:
			case BT_PROXYGRENADE:
			case BT_FLARE:
				_btnLeftHandItem->lock();
				srf->blitNShade(
							_btnLeftHandItem,
							_btnLeftHandItem->getX() + 27,
							_btnLeftHandItem->getY() -  1,
							pulse[_fuseFrame],
							false, 3); // red
				_btnLeftHandItem->unlock();
		}
	}

	if ((item = selUnit->getItem(ST_RIGHTHAND)) != nullptr && item->getFuse() != -1)
	{
		switch (item->getRules()->getBattleType())
		{
			case BT_GRENADE:
			case BT_PROXYGRENADE:
			case BT_FLARE:
				_btnRightHandItem->lock();
				srf->blitNShade(
							_btnRightHandItem,
							_btnRightHandItem->getX() + 27,
							_btnRightHandItem->getY() -  1,
							pulse[_fuseFrame],
							false, 3); // red
				_btnRightHandItem->unlock();
		}
	}
	++_fuseFrame;
}

/**
 * Shifts the colors of the hostileUnit buttons' backgrounds.
 * @param selUnit - the currently selected BattleUnit
 */
void BattlescapeState::hotSqrsCycle(BattleUnit* const selUnit) // private.
{
	static int
		delta		  (1),
		colorRed	 (34), // currently selected unit sees other unit
		colorBlue	(114), // another unit can see other unit
		color_border (15); // dark.gray

	Uint8 color;
	bool isSpotted;

	for (size_t
			i = 0u;
			i != HOTSQRS;
			++i)
	{
		if (_btnHostileUnit[i]->getVisible() == true)
		{
			isSpotted = false;

			for (std::vector<BattleUnit*>::const_iterator
				j = _battleSave->getUnits()->begin();
				j != _battleSave->getUnits()->end();
				++j)
			{
				if ((*j)->getFaction() == FACTION_PLAYER
					&& (*j)->isOut_t(OUT_STAT) == false)
				{
					if (std::find(
								(*j)->getHostileUnits().begin(),
								(*j)->getHostileUnits().end(),
								_hostileUnit[i]) != (*j)->getHostileUnits().end())
					{
						isSpotted = true;
						break;
					}
				}
			}

			if (isSpotted == false)
				color = GREEN_D; // hostile unit is visible but not currently in LoS of friendly units; ergo do not cycle colors.
			else if (std::find(
							selUnit->getHostileUnits().begin(),
							selUnit->getHostileUnits().end(),
							_hostileUnit[i]) != selUnit->getHostileUnits().end())
			{
				color = static_cast<Uint8>(colorRed);
			}
			else
				color = static_cast<Uint8>(colorBlue);

			_btnHostileUnit[i]->drawRect(0,0, 15,13, static_cast<Uint8>(color_border));
			_btnHostileUnit[i]->drawRect(1,1, 13,11, color);
		}
		else
			break;
	}

	switch (colorRed)
	{
		case 34: delta = +1; break;
		case 45: delta = -1;
	}

	colorRed += delta;
	colorBlue += delta;
	color_border -= delta;
}

/**
 * Animates a target cursor over hostile unit when hostileUnit indicator is clicked.
 */
void BattlescapeState::hostileTargeter() // private.
{
	static const int cursorFrames[TARGET_FRAMES] {0,1,2,3,4,0}; // NOTE: Does not show the last frame.

	Surface* const targetCursor (_game->getResourcePack()->getSurfaceSet("Targeter")->getFrame(cursorFrames[_targeterFrame]));
	targetCursor->blit(_srfTargeter);

	if (++_targeterFrame == TARGET_FRAMES)
		_srfTargeter->setVisible(false);
}

/**
 * Draws an execution explosion on the Map.
 */
void BattlescapeState::liquidationExplosion() // private.
{
	std::list<Explosion*>* const explList (_map->getExplosions());

	for (std::list<Explosion*>::const_iterator
			i = explList->begin();
			i != explList->end();
			)
	{
		if ((*i)->animate() == false) // done.
		{
			delete *i;
			i = explList->erase(i);
		}
		else
			++i;
	}

	if (explList->empty() == true)
	{
		_battleGame->endLiquidate();

		BattleUnit* const selUnit (_battleSave->getSelectedUnit());
		selUnit->aim(false);
		_map->cacheUnit(selUnit);
	}
}

/**
 * Draws a shotgun explosion on the Map.
 */
void BattlescapeState::shotgunExplosion() // private.
{
	std::list<Explosion*>* const explList (_map->getExplosions());

	for (std::list<Explosion*>::const_iterator
			i = explList->begin();
			i != explList->end();
			)
	{
		if ((*i)->animate() == false) // done.
		{
			delete *i;
			i = explList->erase(i);
		}
		else
			++i;
	}

	if (explList->empty() == true)
		_battleGame->setShotgun(false);
}

/**
 * Popups a context-sensitive-list of battle-actions for the player.
 * @param item		- pointer to the BattleItem (righthand/lefthand)
 * @param injured	- true if the arm using @a item is injured (default false)
 */
void BattlescapeState::handAction( // private.
		BattleItem* const item,
		bool injured)
{
	if (_battleGame->isBusy() == false)
	{
		BattleAction* const action (_battleGame->getTacticalAction());
//		action->weapon = nullptr; // safety.

		if (item != nullptr)
			action->weapon = item;
		else //if (action->actor->getMeleeWeapon() != nullptr)
//		else if (action->actor->getUnitRules() != nullptr
//			&& action->actor->getUnitRules()->getMeleeWeapon() == "STR_FIST")
		{
			// TODO: This can be generalized later; right now the only 'meleeWeapon' is "STR_FIST" - the Universal Fist!!!
//			const RuleItem* const itRule = _rules->getItemRule(action->actor->getUnitRules()->getMeleeWeapon());
			action->weapon = action->actor->getMeleeWeapon();
		}

		if (action->weapon != nullptr)
			popup(new ActionMenuState(
									action,
									_icons->getX(),
									_icons->getY() + 16,
									injured));
	}
}

/**
 * Handles the top battle game state.
 */
void BattlescapeState::handleState()
{
	_battleGame->handleState();
}

/**
 * Sets the '_tacticalTimer' interval for think() calls of the state.
 * @param interval - an interval in ms
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_tacticalTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return, pointer to Game
 */
Game* BattlescapeState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the SavedGame.
 * @return, pointer to SavedGame
 */
SavedGame* BattlescapeState::getSavedGame() const
{
	return _gameSave;
}

/**
 * Gets pointer to the SavedBattleGame.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* BattlescapeState::getSavedBattleGame() const
{
	return _battleSave;
}

/**
 * Gets pointer to the Map.
 * @note Some states need this info.
 * @return, pointer to Map
 */
Map* BattlescapeState::getMap() const
{
	return _map;
}

/**
 * Shows a debug message in the topleft corner.
 * @param wst - reference a debug message
 */
void BattlescapeState::printDebug(const std::wstring& wst)
{
//	if (_battleSave->getDebugTac() == true)
	_txtDebug->setText(wst);
}

/**
 * Shows a warning message.
 * @note Currently uses 'arg' only to show psi-percent & fuse-timer.
 * @param st	- reference to a message-string usually a warning
 * @param arg	- the argument to show as an integer (default INT_MAX)
 */
void BattlescapeState::warning(
		const std::string& st,
		int arg)
{
	if (arg == std::numeric_limits<int>::max())
		_warning->showMessage(tr(st));
	else
		_warning->showMessage(tr(st).arg(arg));
}

/**
 * Adds a new popup-window to the popups-queue.
 * @note This prevents popups from overlapping.
 * @param state - pointer to popup State
 */
void BattlescapeState::popup(State* const state)
{
	_popups.push_back(state);
}

/**
 * Finishes the current battle and either shuts down the Battlescape and
 * presents the debriefing-screen OR sets up another Battlescape for next-stage.
 * @note Possibly ends the game as well.
 * @param aborted		- true if the mission was aborted
 * @param playerUnits	- quantity of player-units in the exit-area (aborted) or
 *						  when battle is finished (aLiens pacified or mission-
 *						  objectives destroyed; -1 for all units are deceased
 *						  both Player and Hostile.
 */
void BattlescapeState::finishBattle(
		bool aborted,
		int playerUnits)
{
	while (_game->isState(this) == false)
		_game->popState();

	_popups.clear();

	_game->getCursor()->setVisible();
	_game->getResourcePack()->fadeMusic(_game, 975);

	const AlienDeployment* const ruleDeploy (_rules->getDeployment(_battleSave->getTacticalType()));

	std::string nextStage;
	if (ruleDeploy != nullptr)
	{
		switch (_battleSave->getTacType())
		{
			case TCT_BASEASSAULT: // no check for next-stage if Ufo_Crashed or _Landed.
			case TCT_BASEDEFENSE:
			case TCT_MISSIONSITE:
			case TCT_MARS1:
//			case TCT_MARS2:
				nextStage = ruleDeploy->getNextStage();
		}
	}

	if (nextStage.empty() == false && playerUnits > 0)
	{
		_battleSave->setTacticalType(nextStage);

		BattlescapeGenerator bGen = BattlescapeGenerator(_game);
		bGen.nextStage();

		_game->popState();
		_game->pushState(new BriefingState());
	}
	else // end Mission.
	{
		_aniTimer->stop();
		_tacticalTimer->stop();
		_game->popState();


		bool debrief;
		if (_gameSave->getMonthsPassed() != -1)
		{
			if (ruleDeploy != nullptr)
			{
				if (ruleDeploy->isFinalMission() == true) // must fulfill objectives
				{
					debrief = false;
					if (aborted == true
						|| _battleSave->allObjectivesDestroyed() == false) //&& playerUnits < 1
					{
						_gameSave->setEnding(END_LOSE);
						_game->pushState(new DefeatState());
					}
					else //if (_battleSave->allObjectivesDestroyed() == true)
					{
						_gameSave->setEnding(END_WIN);
						_game->pushState(new VictoryState());
					}
				}
				else if (ruleDeploy->isNoRetreat() == true)
				{
					if (aborted == true || playerUnits < 1)
					{
						debrief = false;
						_gameSave->setEnding(END_LOSE);
						_game->pushState(new DefeatState());
					}
					else
						debrief = true;
				}
				else // non-critical battle
					debrief = true;
			}
			else // ufo crashed or landed
				debrief = true;
		}
		else // insta-battle
			debrief = true;

		bool ironsave;
		if (debrief == true)
		{
			_game->pushState(new DebriefingState());
			ironsave = false;
		}
		else
			ironsave = _gameSave->isIronman() == true;

		if (ironsave == true)
		{
			_gameSave->setBattleSave();
			_game->pushState(new SaveGameState(
											OPT_GEOSCAPE,
											SAVE_IRONMAN,
											_palette));
		}
	}
}
// old win/lose mission determination:
/*
		if (aborted == true || playerUnits == 0)		// Abort was done or no player-units are still alive.
		{												// This concludes to defeat when in a 'noRetreat' or 'final' mission, like Mars landing or Mars aLien base.
			if (ruleDeploy != nullptr
				&& ruleDeploy->isNoRetreat() == true
				&& _gameSave->getMonthsPassed() != -1)
			{
				_gameSave->setEnding(END_LOSE);
				ironsave = _gameSave->isIronman() == true;
				_game->pushState(new DefeatState());
			}
			else
			{
				ironsave = false;
				_game->pushState(new DebriefingState());
			}
		}
		else											// No abort was done and at least one player-unit is still alive.
		{												// This concludes to victory when in a 'final' mission, like Mars aLien base.
			if (ruleDeploy != nullptr
				&& ruleDeploy->isFinalMission() == true	// <- do *not* win if all units dead both Player and Hostile
				&& _gameSave->getMonthsPassed() != -1)
			{
				_gameSave->setEnding(END_WIN);
				ironsave = _gameSave->isIronman() == true;
				_game->pushState(new VictoryState());
			}
			else
			{
				ironsave = false;
				_game->pushState(new DebriefingState());
			}
		}
*/
// old next-stage data:
/*
		std::string nextStageRace (ruleDeploy->getNextStageRace());
		for (std::vector<TerrorSite*>::const_iterator
				ts = _gameSave->getTerrorSites()->begin();
				ts != _gameSave->getTerrorSites()->end() && nextStageRace.empty() == true;
				++ts)
		{
			if ((*ts)->getTactical() == true)
				nextStageRace = (*ts)->getAlienRace();
		}
		for (std::vector<AlienBase*>::const_iterator
				ab = _gameSave->getAlienBases()->begin();
				ab != _gameSave->getAlienBases()->end() && nextStageRace.empty() == true;
				++ab)
		{
			if ((*ab)->getTactical() == true)
				nextStageRace = (*ab)->getAlienRace();
		}
		if (nextStageRace.empty() == true)
			nextStageRace = "STR_MIXED";
		else if (_rules->getAlienRace(nextStageRace) == nullptr)
		{
			throw Exception(nextStageRace + " race not found.");
		}
*/
//		bGen.setAlienRace("STR_MIXED");
//		bGen.setAlienRace(nextStageRace);

/**
 * Shows the launch button.
 * @param show - true to show launch button (default true)
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Shows the PSI button.
 * @param show - true to show PSI button (default true)
 */
void BattlescapeState::showPsiButton(bool show)
{
	_btnPsi->setVisible(show);
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void BattlescapeState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Returns a pointer to BattlescapeGame.
 * @return, pointer to BattlescapeGame
 */
BattlescapeGame* BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons disabling the tile-selector cuboid.
 * @param action - pointer to an Action
 */
void BattlescapeState::mouseInIcons(Action*)
{
	_mouseOverIcons = true;

	_txtConsole1->setText(L"");
	_txtConsole2->setText(L"");

	_txtTerrain->setVisible();
	_txtShade->setVisible();
	_txtTurn->setVisible();

	_txtOrder->setVisible();
	_lstSoldierInfo->setVisible();
	_srfAlienIcon->setVisible(allowAlienIcons());
	_showSoldierData = true;

	_lstTileInfo->setVisible(false);
}

/**
 * Handler for the mouse going out of the icons enabling the tile-selector cuboid.
 * @param action - pointer to an Action
 */
void BattlescapeState::mouseOutIcons(Action*)
{
	_mouseOverIcons = false;
	_lstTileInfo->setVisible();
}

/**
 * Checks if the mouse is over the icons.
 * @return, true if the mouse is over the icons
 */
bool BattlescapeState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
 * Updates the scale.
 * @param dX - reference to the x-delta
 * @param dY - reference to the y-delta
 */
void BattlescapeState::resize(
		int& dX,
		int& dY)
{
	int divisor;
	switch (Options::battlescapeScale)
	{
		case SCALE_SCREEN_DIV_3:
			divisor = 3;
			break;

		case SCALE_SCREEN_DIV_2:
			divisor = 2;
			break;

		case SCALE_SCREEN:
			divisor = 1;
			break;

		default:
			dX =
			dY = 0;
			return;
	}

	double pixelRatioY;
	if (Options::nonSquarePixelRatio)
		pixelRatioY = 1.2;
	else
		pixelRatioY = 1.;

	dX = Options::baseXResolution;
	dY = Options::baseYResolution;

// G++ linker wants it this way ...
//#ifdef _DEBUG
	const int
		screenWidth  (Screen::ORIGINAL_WIDTH),
		screenHeight (Screen::ORIGINAL_HEIGHT);

	Options::baseXResolution = std::max(screenWidth,
										Options::displayWidth / divisor);
	Options::baseYResolution = std::max(screenHeight,
										static_cast<int>(static_cast<double>(Options::displayHeight) / pixelRatioY / static_cast<double>(divisor)));
//#else
//	Options::baseXResolution = std::max(Screen::ORIGINAL_WIDTH,
//										Options::displayWidth / divisor);
//	Options::baseYResolution = std::max(Screen::ORIGINAL_HEIGHT,
//										static_cast<int>(static_cast<double>(Options::displayHeight) / pixelRatioY / static_cast<double>(divisor)));
//#endif

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;

	_map->setWidth(Options::baseXResolution);
	_map->setHeight(Options::baseYResolution);
	_map->getCamera()->resize();
	_map->getCamera()->jumpXY(dX / 2, dY / 2);

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (   *i != _map // TODO: Add more surfaces to exclude since I added many more.
			&& *i != _btnPsi
			&& *i != _btnLaunch
			&& *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map && *i != _txtDebug)
			(*i)->setX((*i)->getX() + dX);
	}
}

/**
 * Updates the turn text.
 */
void BattlescapeState::updateTurn()
{
	_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));
}

/**
 * Toggles the icons' surfaces' visibility for Hidden Movement.
 * @param vis - true to show show icons and info
 */
void BattlescapeState::toggleIcons(bool vis)
{
	_iconsHidden = !vis;

	_icons->setVisible(vis);

	_numLayers->setVisible(vis);

	_btnUnitUp->setVisible(vis);
	_btnUnitDown->setVisible(vis);
	_btnMapUp->setVisible(vis);
	_btnMapDown->setVisible(vis);
	_btnMiniMap->setVisible(vis);
	_btnKneel->setVisible(vis);
	_btnInventory->setVisible(vis);
	_btnCenter->setVisible(vis);
	_btnNextUnit->setVisible(vis);
	_btnNextStop->setVisible(vis);
	_btnShowLayers->setVisible(vis);
	_btnOptions->setVisible(vis);
	_btnEndTurn->setVisible(vis);
	_btnAbort->setVisible(vis);

	_txtOrder->setVisible(vis);
	_lstSoldierInfo->setVisible(vis);
	_srfAlienIcon->setVisible(vis && allowAlienIcons());
	_showSoldierData = vis;

//	_txtControlDestroyed->setVisible(vis);
	_txtMissionLabel->setVisible(vis);
	_lstTileInfo->setVisible(vis);

	_srfOverweight->setVisible(vis && _isOverweight);

	for (size_t
			i = 0u;
			i != WOUNDED;
			++i)
	{
		if (_tileWounded[i] != nullptr)
		{
			_btnWounded[i]->setVisible(vis);
			_numWounded[i]->setVisible(vis);
		}
		else
			break;
	}
}

/**
 * Gets the TimeUnits field from icons.
 * @note These are used in UnitWalkBState to update info while unit moves.
 * @return, pointer to time units NumberText
 */
NumberText* BattlescapeState::getTuField() const
{
	return _numTimeUnits;
}

/**
 * Gets the TimeUnits bar from icons.
 * @return, pointer to time units Bar
 */
Bar* BattlescapeState::getTuBar() const
{
	return _barTimeUnits;
}

/**
 * Gets the Energy field from icons.
 * @return, pointer to stamina NumberText
 */
NumberText* BattlescapeState::getEnergyField() const
{
	return _numEnergy;
}

/**
 * Gets the Energy bar from icons.
 * @return, pointer to stamina Bar
 */
Bar* BattlescapeState::getEnergyBar() const
{
	return _barEnergy;
}

/**
 * Checks if it's okay to show the aLien-icons for a Soldier's kills/stuns.
 * @return, true if okay to show icons
 */
bool BattlescapeState::allowAlienIcons() const // private.
{
	_srfAlienIcon->clear();

	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	if (selUnit != nullptr
		&& selUnit->getGeoscapeSoldier() != nullptr)
	{
		const int takedowns (selUnit->getTakedowns());
		if (takedowns != 0)
		{
			static const int ICONS (30);
			int
				x,y;

			for (int
					i = 0, j = 0;
					i != takedowns && j != ICONS;
					++i, ++j)
			{
				x = (j % 3) * 10;
				y = (j / 3) * 12;

				Surface* const srfAlien (_game->getResourcePack()->getSurface("AlienIcon"));
				srfAlien->setX(x);
				srfAlien->setY(y);
				srfAlien->blit(_srfAlienIcon);
			}
			return true;
		}
	}
	return false;
}

/**
 * Updates experience data for the currently selected Soldier.
 */
void BattlescapeState::updateExperienceInfo()
{
	_lstSoldierInfo->clearList();
	_srfAlienIcon->setVisible(false);

	if (_showSoldierData == true)
	{
		const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
		if (selUnit != nullptr
			&& selUnit->getGeoscapeSoldier() != nullptr)
		{
			_srfAlienIcon->setVisible(allowAlienIcons());

			// keep this consistent ...
			std::vector<std::wstring> xpType;
			xpType.push_back(L"f "); // firing
			xpType.push_back(L"t "); // throwing
			xpType.push_back(L"m "); // melee
			xpType.push_back(L"r "); // reactions
			xpType.push_back(L"b "); // bravery
			xpType.push_back(L"a "); // psiSkill attack
			xpType.push_back(L"d "); // psiStrength defense

			// ... consistent with this
			const int xp[]
			{
				selUnit->getExpFiring(),
				selUnit->getExpThrowing(),
				selUnit->getExpMelee(),
				selUnit->getExpReactions(),
				selUnit->getExpBravery(),
				selUnit->getExpPsiSkill(),
				selUnit->getExpPsiStrength()
			};

			Uint8 color;
			for (size_t
					i = 0u;
					i != sizeof(xp) / sizeof(xp[0u]);
					++i)
			{
				_lstSoldierInfo->addRow(
									2,
									xpType.at(i).c_str(),
									Text::intWide(xp[i]).c_str());
				if (xp[i] != 0)
				{
					if		(xp[i] > 10) color = BROWN_L;
					else if	(xp[i] >  5) color = BROWN;
					else if	(xp[i] >  2) color = ORANGE;
					else				 color = GREEN;

					_lstSoldierInfo->setCellColor(i, 1u, color, true);
				}
			}
		}
	}
}

/**
 * Updates tile-info for mouse-overs.
 * @param tile - pointer to a Tile
 */
void BattlescapeState::updateTileInfo(const Tile* const tile) // private.
{
	_lstTileInfo->clearList();

	if (tile != nullptr && tile->isRevealed(ST_CONTENT) == true)
	{
		size_t rows (3u);
		int tuCost (0);

		const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
		if (selUnit != nullptr
			&& selUnit->getFaction() == FACTION_PLAYER)
		{
			++rows;
			const MoveType mType (selUnit->getMoveTypeUnit());

			tuCost = tile->getTuCostTile(O_FLOOR, mType)
				   + tile->getTuCostTile(O_OBJECT, mType);

			if (tile->getMapData(O_FLOOR) == nullptr
				&& tile->getMapData(O_OBJECT) != nullptr)
			{
				tuCost += 4;
			}
			else if (tuCost == 0)
			{
				switch (mType)
				{
					case MT_FLOAT: // wft.
					case MT_FLY: tuCost = 4;
						break;

					case MT_WALK:
					case MT_SLIDE: tuCost = 255;
				}
			}
		}


		const int info[]
		{
			static_cast<int>(tile->hasNoFloor(_battleSave->getTile(tile->getPosition() + Position(0,0,-1)))),
			tile->getSmoke(),
			tile->getFire(),
			tuCost
		};

		std::vector<std::wstring> infoType;
		infoType.push_back(L"F"); // Floor
		infoType.push_back(L"S"); // smoke
		infoType.push_back(L"I"); // fire
		infoType.push_back(L"M"); // tuCost


		Uint8 color;
		for (size_t
				i = 0u;
				i != rows;
				++i)
		{
			if (i == 0u) // Floor
			{
				std::wstring hasFloor;
				if (info[i] == 0)
				{
					hasFloor = L"F";
					color = GREEN; // Floor
				}
				else
				{
					hasFloor = L"-";
					color = ORANGE; // NO Floor
				}

				_lstTileInfo->addRow(
								2,
								hasFloor.c_str(),
								infoType.at(i).c_str());
			}
			else if (i < 3u) // smoke & fire
			{
				if (i == 1u)
					color = BROWN_L; // smoke
				else
					color = RED; // fire

				std::wstring value;
				if (info[i] != 0)
					value = Text::intWide(info[i]);
				else
					value = L"";

				_lstTileInfo->addRow(
								2,
								value.c_str(),
								infoType.at(i).c_str());
			}
			else if (selUnit != nullptr) // tuCost
			{
				color = BLUE;

				std::wstring cost;
				if (info[i] < 255)
					cost = Text::intWide(info[i]);
				else
					cost = L"-";

				_lstTileInfo->addRow(
								2,
								cost.c_str(),
								infoType.at(i).c_str());
			}
			else
				break;

			_lstTileInfo->setCellColor(i, 0u, color, true);
		}
	}
}

/**
 * Autosave the game the next time the Battlescape is init'd.
 */
void BattlescapeState::autosave()
{
	_autosave = true;
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	if (selUnit == nullptr)
		return;

	int
		w (_battleSave->getMapSizeX()),
		h (_battleSave->getMapSizeY());

	SDL_Surface* const img (SDL_AllocSurface(
										0u,
										w << 3u, h << 3u, 24,
										0xffu,
										0xff00u,
										0xff0000u,
										0u));
	std::memset(
			img->pixels,
			0,
			static_cast<size_t>(img->pitch * static_cast<Uint16>(img->h)));

	Position posTile (selUnit->getPosition());

	SDL_Rect rect;
	rect.h =
	rect.w = 8u;

	const Tile* tile;
	const BattleUnit* unit;
	for (int
			y = 0;
			y != h;
			++y)
	{
		posTile.y = y;
		for (int
				x = 0;
				x != w;
				++x)
		{
			posTile.x = x;
			tile = _battleSave->getTile(posTile);

			if (tile == nullptr || tile->isRevealed(ST_CONTENT) == false)
				continue;

			rect.x = static_cast<Sint16>(x) * static_cast<Sint16>(rect.w);
			rect.y = static_cast<Sint16>(y) * static_cast<Sint16>(rect.h);

			if (tile->getTuCostTile(O_FLOOR, MT_FLY) != 255
				&& tile->getTuCostTile(O_OBJECT, MT_FLY) != 255)
			{
				SDL_FillRect(
						img,
						&rect,
						SDL_MapRGB(
								img->format,
								255u,
								0u,
								0x20u));
				characterRGBA(
						img,
						rect.x, rect.y,
						'*',
						0x7fu,
						0x7fu,
						0x7fu,
						0x7fu);
			}
			else
			{
				if (tile->getTileUnit() == nullptr)
					SDL_FillRect(
							img,
							&rect,
							SDL_MapRGB(
									img->format,
									0x50u,
									0x50u,
									0x50u)); // gray for blocked tile
			}

			for (int
					z = posTile.z;
					z >= 0;
					--z)
			{
				Position pos(
							posTile.x,
							posTile.y,
							z);

				tile = _battleSave->getTile(pos);
				unit = tile->getTileUnit();
				if (unit != nullptr)
				{
					switch (unit->getFaction())
					{
						case FACTION_HOSTILE:
							characterRGBA(
										img,
										rect.x, rect.y,
										(posTile.z - z) ? 'a' : 'A',
										0x40u, // #4080C0 is Volutar Blue. CONGRATULATIONz!!!
										0x80u,
										0xC0u,
										0xffu);
							break;
						case FACTION_PLAYER:
							characterRGBA(
										img,
										rect.x, rect.y,
										(posTile.z - z) ? 'x' : 'X',
										255u,
										255u,
										127u,
										0xffu);
							break;
						case FACTION_NEUTRAL:
							characterRGBA(
										img,
										rect.x, rect.y,
										(posTile.z - z) ? 'c' : 'C',
										255u,
										127u,
										127u,
										0xffu);
					}
					break;
				}

				--pos.z;
				if (z > 0 && tile->hasNoFloor(_battleSave->getTile(pos)) == false)
					break; // no seeing through floors
			}

			if (tile->getMapData(O_NORTHWALL) != nullptr
				&& tile->getMapData(O_NORTHWALL)->getTuCostPart(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						rect.x, rect.y,
						rect.x + static_cast<Sint16>(rect.w),
						rect.y,
						0x50u,
						0x50u,
						0x50u,
						255u);
			}

			if (tile->getMapData(O_WESTWALL) != nullptr
				&& tile->getMapData(O_WESTWALL)->getTuCostPart(MT_FLY) == 255)
			{
				lineRGBA(
						img,
						rect.x, rect.y,
						rect.x,
						rect.y + static_cast<Sint16>(rect.h),
						0x50u,
						0x50u,
						0x50u,
						255u);
			}
		}
	}

	std::ostringstream oststr;

	oststr.str("");
	oststr << "z = " << posTile.z;
	stringRGBA(
			img,
			12,12,
			oststr.str().c_str(),
			0u,0u,0u,
			0x7fu);

	int i (0);
	do
	{
		oststr.str("");
		oststr << Options::getUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";
		++i;
	}
	while (CrossPlatform::fileExists(oststr.str()));


	unsigned error (lodepng::encode(
								oststr.str(),
								static_cast<const unsigned char*>(img->pixels),
								static_cast<unsigned>(img->w),
								static_cast<unsigned>(img->h),
								LCT_RGB));
	if (error != 0u)
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);

	SDL_FreeSurface(img);
}

/**
 * Saves a first-person voxel-view of the battlefield.
 */
void BattlescapeState::saveVoxelView()
{
	static const unsigned char pal[30u]
	{
		  0u,   0u,   0u,
		224u, 224u, 224u,	// ground
		192u, 224u, 255u,	// west wall
		255u, 224u, 192u,	// north wall
		128u, 255u, 128u,	// object
		192u,   0u, 255u,	// enemy unit
		  0u,   0u,   0u,
		255u, 255u, 255u,
		224u, 192u,   0u,	// xcom unit
		255u,  64u, 128u	// neutral unit
	};

	const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	if (selUnit == nullptr) // no unit selected
		return;

	bool
		debugTac (_battleSave->getDebugTac()),
		black;
	int voxelTest;
	double
		angleX,
		angleY,
		dist (0.),
		dir;

	if (selUnit->getTurretType() != TRT_NONE
		&& Options::battleStrafe == true)
	{
		dir = static_cast<double>(selUnit->getTurretDirection() + 4) / 4. * M_PI;
	}
	else
		dir = static_cast<double>(selUnit->getUnitDirection() + 4) / 4. * M_PI;

	std::vector<unsigned char> pic;
	std::vector<Position> trj;

	Position
		originVoxel (_battleGame->getTileEngine()->getSightOriginVoxel(selUnit)),
		targetVoxel,
		pos;
	const Tile* tile (nullptr);


//	pic.clear();

	for (int
			y = -256 + 32;
			y <  256 + 32;
			++y)
	{
		angleY = (static_cast<double>(y) / 640. * M_PI) + (M_PI / 2.);

		for (int
				x = -256;
				x <  256;
				++x)
		{
			angleX = (static_cast<double>(x) / 1024. * M_PI) + dir;

			targetVoxel.x = originVoxel.x + (static_cast<int>(-std::sin(angleX) * 1024 * std::sin(angleY)));
			targetVoxel.y = originVoxel.y + (static_cast<int>( std::cos(angleX) * 1024 * std::sin(angleY)));
			targetVoxel.z = originVoxel.z + (static_cast<int>( std::cos(angleY) * 1024));

			trj.clear();

			black = true;
			voxelTest = static_cast<int>(_battleSave->getTileEngine()->plotLine(
																			originVoxel,
																			targetVoxel,
																			false,
																			&trj,
																			selUnit,
																			true,
																			debugTac == false)) + 1;
			switch (voxelTest)
			{
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					tile = _battleSave->getTile(Position(
													trj.at(0u).x >> 4u,
													trj.at(0u).y >> 4u,
													trj.at(0u).z / 24));
//					if (debugTac == true
//						|| (tile->isRevealed(ST_WEST) && voxelTest == 2)
//						|| (tile->isRevealed(ST_NORTH) && voxelTest == 3)
//						|| (tile->isRevealed(ST_CONTENT) && (voxelTest == 1 || voxelTest == 4))
//						|| voxelTest == 5)
					if (debugTac == true)
						black = false;
					else
					{
						switch (voxelTest)
						{
							case 1:
							case 4:
								if (tile->isRevealed(ST_CONTENT) == true)
									black = false;
								break;

							case 2:
								if (tile->isRevealed(ST_WEST) == true)
									black = false;
								break;

							case 3:
								if (tile->isRevealed(ST_NORTH) == true)
									black = false;
								break;

							case 5:
								black = false;
						}
					}

					if (black == false)
					{
						if (voxelTest == 5) // Unit.
						{
							if (tile->getTileUnit() != nullptr)
							{
								switch (tile->getTileUnit()->getFaction())
								{
									case FACTION_NEUTRAL:
										voxelTest = 9;
										break;
									case FACTION_PLAYER:
										voxelTest = 8;
								}
							}
							else // check tileBelow.
							{
								tile = _battleSave->getTile(Position(
																trj.at(0u).x >> 4u,
																trj.at(0u).y >> 4u,
																trj.at(0u).z / 24 - 1));
								if (tile != nullptr && tile->getTileUnit() != nullptr)
								{
									switch (tile->getTileUnit()->getFaction())
									{
										case FACTION_NEUTRAL:
											voxelTest = 9;
											break;
										case FACTION_PLAYER:
											voxelTest = 8;
									}
								}
							}
						}

						pos = Position(
									trj.at(0u).x,
									trj.at(0u).y,
									trj.at(0u).z);
						dist = std::sqrt(static_cast<double>(
									  (pos.x - originVoxel.x) * (pos.x - originVoxel.x)
									+ (pos.y - originVoxel.y) * (pos.y - originVoxel.y)
									+ (pos.z - originVoxel.z) * (pos.z - originVoxel.z)));

						black = false;
					}
			}

			if (black == true)
				dist = 0.;
			else
			{
				if		(dist > 1000.)	dist = 1000.;
				else if	(dist < 1.)		dist = 1.;

				dist = (1000. - ((std::log(dist) * 140.)) / 700.);

				if (pos.x % 16 == 15)
					dist *= 0.9;

				if (pos.y % 16 == 15)
					dist *= 0.9;

				if (pos.z % 24 == 23)
					dist *= 0.9;

				if (dist > 1.) dist = 1.;

				if (tile != nullptr)
					dist *= (16. - static_cast<double>(tile->getShade())) / 16.;
			}

			pic.push_back(static_cast<unsigned char>(static_cast<double>(pal[static_cast<size_t>(voxelTest * 3 + 0)]) * dist));
			pic.push_back(static_cast<unsigned char>(static_cast<double>(pal[static_cast<size_t>(voxelTest * 3 + 1)]) * dist));
			pic.push_back(static_cast<unsigned char>(static_cast<double>(pal[static_cast<size_t>(voxelTest * 3 + 2)]) * dist));
		}
	}


	std::ostringstream oststr;

	int i (0);
	do
	{
		oststr.str("");
		oststr << Options::getUserFolder() << "fpslook" << std::setfill('0') << std::setw(3) << i << ".png";
		++i;
	}
	while (CrossPlatform::fileExists(oststr.str()) == true && i < 999);


	unsigned error (lodepng::encode(
								oststr.str(),
								pic,
								512u,512u,
								LCT_RGB));
	if (error != 0u)
		Log(LOG_ERROR) << "bs::saveVoxelView() Saving to PNG failed: " << lodepng_error_text(error);
#ifdef _WIN32
	else
	{
		std::wstring wst (Language::fsToWstr("\"C:\\Program Files\\IrfanView\\i_view32.exe\" \"" + oststr.str() + "\""));

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		if (CreateProcess(									//BOOL WINAPI CreateProcess
						nullptr,							//  _In_opt_     LPCTSTR lpApplicationName,
						const_cast<LPWSTR>(wst.c_str()),	//  _Inout_opt_  LPTSTR lpCommandLine,
						nullptr,							//  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
						nullptr,							//  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
						FALSE,								//  _In_         BOOL bInheritHandles,
						0,									//  _In_         DWORD dwCreationFlags,
						nullptr,							//  _In_opt_     LPVOID lpEnvironment,
						nullptr,							//  _In_opt_     LPCTSTR lpCurrentDirectory,
						&si,								//  _In_         LPSTARTUPINFO lpStartupInfo,
						&pi) == false)						//  _Out_        LPPROCESS_INFORMATION lpProcessInformation
		{
			Log(LOG_ERROR) << "bs::saveVoxelView() CreateProcess() failed";
		}
		else
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
#endif
}

/**
 * Saves each layer of voxels on the battlefield as a png.
 */
void BattlescapeState::saveVoxelMap()
{
	std::ostringstream oststr;
	std::vector<unsigned char> image;
	static const unsigned char pal[30u]
	{
		255u, 255u, 255u,
		224u, 224u, 224u,
		128u, 160u, 255u,
		255u, 160u, 128u,
		128u, 255u, 128u,
		192u,   0u, 255u,
		255u, 255u, 255u,
		255u, 255u, 255u,
		224u, 192u,   0u,
		255u,  64u, 128u
	};

	Tile* tile;

	for (int
			z = 0;
			z < _battleSave->getMapSizeZ() * 12;
			++z)
	{
		image.clear();

		for (int
				y = 0;
				y < (_battleSave->getMapSizeY() << 4u);
				++y)
		{
			for (int
					x = 0;
					x < (_battleSave->getMapSizeX() << 4u);
					++x)
			{
				int voxelTest (static_cast<int>(_battleSave->getTileEngine()->detVoxelType(Position(x,y,z << 1u))) + 1);
				float dist (1.f);

				if (x % 16 == 15)
					dist *= 0.9f;

				if (y % 16 == 15)
					dist *= 0.9f;

				if (voxelTest == 5) // Unit.
				{
					tile = _battleSave->getTile(Position(
														x >> 4u,
														y >> 4u,
														z / 12));
					if (tile->getTileUnit() != nullptr)
					{
						switch (tile->getTileUnit()->getFaction())
						{
							case FACTION_NEUTRAL:
								voxelTest = 9;
								break;
							case FACTION_PLAYER:
								voxelTest = 8;
						}
					}
					else // check tileBelow.
					{
						tile = _battleSave->getTile(Position(
															x >> 4u,
															y >> 4u,
															z / 12 - 1));
						if (tile != nullptr && tile->getTileUnit() != nullptr)
						{
							switch (tile->getTileUnit()->getFaction())
							{
								case FACTION_NEUTRAL:
									voxelTest = 9;
									break;
								case FACTION_PLAYER:
									voxelTest = 8;
							}
						}
					}
				}

				image.push_back(static_cast<unsigned char>(static_cast<float>(pal[static_cast<size_t>(voxelTest * 3 + 0)]) * dist));
				image.push_back(static_cast<unsigned char>(static_cast<float>(pal[static_cast<size_t>(voxelTest * 3 + 1)]) * dist));
				image.push_back(static_cast<unsigned char>(static_cast<float>(pal[static_cast<size_t>(voxelTest * 3 + 2)]) * dist));
			}
		}

		oststr.str("");
		oststr << Options::getUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";

		unsigned error (lodepng::encode(
									oststr.str(),
									image,
									_battleSave->getMapSizeX() << 4u,
									_battleSave->getMapSizeY() << 4u,
									LCT_RGB));
		if (error != 0u)
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}
}

}
