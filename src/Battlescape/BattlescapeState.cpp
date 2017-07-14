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

#include "BattlescapeState.h"

//#include <algorithm>
#include <cstring>
#include <iomanip>
//#include <sstream>

#include <SDL/SDL_gfxPrimitives.h>

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
#include "../Engine/Logger.h"
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

#include "../Ruleset/MapDataSet.h" // debug: mapClick()
#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleCountry.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnitStatistics.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Tile.h"
#include "../Savegame/Ufo.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape HUD and instantiates the
 * battlefield Surface.
 */
BattlescapeState::BattlescapeState()
	:
		_playSave(_game->getSavedGame()),
		_battleSave(_game->getSavedGame()->getBattleSave()),
		_rules(_game->getRuleset()),
//		_reserve(0),
		_dragScrollActivated(false),
//		_dragScrollStepped(false),
		_dragScrollPastPixelThreshold(false),
		_dragScrollStartTick(0u),
		_dragScrollX(0),
		_dragScrollY(0),
		_init(true),
		_mouseOverIcons(false),
		_cycleFuse(0u),
		_showConsole(2),
		_cycleTargeter(0u),
		_showSoldierData(false),
		_iconsHidden(false),
		_isOverweight(false),
		_autosave(false)
{
	//Log(LOG_INFO) << "Create BattlescapeState";
	STATE_INTERVAL_ALIEN	= static_cast<Uint32>(Options::battleAlienSpeed);
	STATE_INTERVAL_XCOM		= static_cast<Uint32>(Options::battleXcomSpeed);
	STATE_INTERVAL_XCOMDASH	= (STATE_INTERVAL_XCOM << 1u) / 3u;

	const RuleInterface* const uiRule (_rules->getInterface("battlescape"));
	const int
		screenWidth		(Options::baseXResolution),
		screenHeight	(Options::baseYResolution),
		iconsWidth		(uiRule->getElement("icons")->w), // 320
		iconsHeight		(uiRule->getElement("icons")->h), // 56
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

	_icons = new InteractiveSurface(	// the HUD Surface at bottom-center of Screen.
								iconsWidth,
								iconsHeight,
								x,y);
	_map = new Map(						// battlefield Surface here.
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

	_btnUnitUp		= new BattlescapeButton(32,  16, x +  48, y);
	_btnUnitDown	= new BattlescapeButton(32,  16, x +  48, y + 16);
	_btnMapUp		= new BattlescapeButton(32,  16, x +  80, y);
	_btnMapDown		= new BattlescapeButton(32,  16, x +  80, y + 16);
	_btnMiniMap		= new BattlescapeButton(32,  16, x + 112, y);
	_btnKneel		= new BattlescapeButton(32,  16, x + 112, y + 16);
	_btnInventory	= new BattlescapeButton(32,  16, x + 144, y);
	_btnCenter		= new BattlescapeButton(32,  16, x + 144, y + 16);
	_btnNextUnit	= new BattlescapeButton(32,  16, x + 176, y);
	_btnNextStop	= new BattlescapeButton(32,  16, x + 176, y + 16);
	_btnShowLayers	= new BattlescapeButton(32,  16, x + 208, y);
	_btnOptions		= new BattlescapeButton(32,  16, x + 208, y + 16);
	_btnEndTurn		= new BattlescapeButton(32,  16, x + 240, y);
	_btnAbort		= new BattlescapeButton(32,  16, x + 240, y + 16);

/*	_btnReserveNone		= new BattlescapeButton(17, 11, x + 60, y + 33);
	_btnReserveSnap		= new BattlescapeButton(17, 11, x + 78, y + 33);
	_btnReserveAimed	= new BattlescapeButton(17, 11, x + 60, y + 45);
	_btnReserveAuto		= new BattlescapeButton(17, 11, x + 78, y + 45);
	_btnReserveKneel	= new BattlescapeButton(10, 23, x + 96, y + 33);
	_btnZeroTUs			= new BattlescapeButton(10, 23, x + 49, y + 33); */

	_isfStats		= new InteractiveSurface(164, 23, x + 107, y + 33);
	_isfLogo		= new InteractiveSurface( 57, 23, x +  49, y + 33);

	_isfLeftHand	= new InteractiveSurface(32, 48, x +   8, y + 5);
	_isfRightHand	= new InteractiveSurface(32, 48, x + 280, y + 5);
	_numAmmoL		= new NumberText(7, 5, x +  33, y + 5);
	_numAmmoR		= new NumberText(7, 5, x + 305, y + 5);

	_numFuseL		= new NumberText(7, 5, x +   8, y + 5);
	_numFuseR		= new NumberText(7, 5, x + 280, y + 5);

	_numTwohandL	= new NumberText(7, 5, x +  33, y + 46, true);
	_numTwohandR	= new NumberText(7, 5, x + 305, y + 46, true);

	_numMediL1		= new NumberText(7, 5, x +   9, y + 32);
	_numMediL2		= new NumberText(7, 5, x +   9, y + 39);
	_numMediL3		= new NumberText(7, 5, x +   9, y + 46);
	_numMediR1		= new NumberText(7, 5, x + 281, y + 32);
	_numMediR2		= new NumberText(7, 5, x + 281, y + 39);
	_numMediR3		= new NumberText(7, 5, x + 281, y + 46);
//	const int
//		visibleUnitX = uiRule->getElement("visibleUnits")->x,
//		visibleUnitY = uiRule->getElement("visibleUnits")->y;

	_srfTargeter = new Surface(
							32,40,
							(screenWidth >> 1u) - 16,
							playableHeight >> 1u);

	std::fill_n(
			_hostileUnits,
			ICONS_HOSTILE,
			static_cast<BattleUnit*>(nullptr));

	int offsetX (0);
	for (size_t
			i = 0u;
			i != ICONS_HOSTILE;
			++i)
	{
		if (i > 9) offsetX = 15;

		_isfHostiles[i] = new InteractiveSurface(
										15,13,
										x + iconsWidth - 21 - offsetX,
										y - 16 - (static_cast<int>(i) * 13));
		_numHostiles[i] = new NumberText(
										8,6,
										x + iconsWidth - 15 - offsetX,
										y - 12 - (static_cast<int>(i) * 13));
	}

	for (size_t // center 10+ on buttons
			i = 9u;
			i != ICONS_HOSTILE;
			++i)
	{
		_numHostiles[i]->setX(_numHostiles[i]->getX() - 2);
	}


	std::fill_n(
			_tileMedic,
			ICONS_MEDIC,
			static_cast<Tile*>(nullptr));

	for (size_t
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		_isfMedic[i] = new InteractiveSurface(
											9,7,
											x + 5,
											y - 17 - (static_cast<int>(i) * 14));
		_numMedic[i] = new NumberText(
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

	if (uiRule->getElement("pathfinding") != nullptr)
	{
		const Element* const el (uiRule->getElement("pathfinding"));
		Pathfinding::green	= static_cast<Uint8>(el->color);
		Pathfinding::yellow	= static_cast<Uint8>(el->color2);
		Pathfinding::red	= static_cast<Uint8>(el->border);
	}

	add(_map);
	_map->init();
	_map->onMouseOver(	static_cast<ActionHandler>(&BattlescapeState::mapOver));
	_map->onMousePress(	static_cast<ActionHandler>(&BattlescapeState::mapPress));
	_map->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::mapClick),
						0u);
//	_map->onMouseIn(	static_cast<ActionHandler>(&BattlescapeState::mapIn));


	const ResourcePack* const res (_game->getResourcePack());

	add(_icons);
	Surface* const icons (res->getSurface("TacIcons"));
	if (res->getSurface("Logo") != nullptr)
	{
		Surface* const logo (res->getSurface("Logo"));
		logo->setX(48);
		logo->setY(32);
		logo->blit(icons);
	}
	icons->blit(_icons);

	_srtBigobs			= res->getSurfaceSet("BIGOBS.PCK");
	_srtIconsOverlay	= res->getSurfaceSet("TacIconsOverlay");
	_srtScanG			= res->getSurfaceSet("SCANG.DAT");
	_srtTargeter		= res->getSurfaceSet("Targeter");


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
	add(_isfStats,			"buttonStats",			"battlescape", _icons);
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
	add(_isfLogo,			"buttonZeroTUs",		"battlescape", _icons);
	add(_isfLeftHand,		"buttonLeftHand",		"battlescape", _icons);
	add(_isfRightHand,		"buttonRightHand",		"battlescape", _icons);
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
			i != ICONS_HOSTILE;
			++i)
	{
		add(_isfHostiles[i]);
		add(_numHostiles[i]);
	}

	for (size_t
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		add(_isfMedic[i]);
		add(_numMedic[i]);
	}


	add(_btnLaunch);
	add(_btnPsi);

	SurfaceSet* const srt (res->getSurfaceSet("SPICONS.DAT"));
	srt->getFrame(0)->blit(_btnLaunch);
	_btnLaunch->onMousePress(static_cast<ActionHandler>(&BattlescapeState::btnLaunchPress));
	_btnLaunch->setVisible(false);

	srt->getFrame(1)->blit(_btnPsi);
	_btnPsi->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnPsiClick));
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

	_warning->setTextColor(static_cast<Uint8>(uiRule->getElement("warning")->color));
	_warning->setColor(    static_cast<Uint8>(uiRule->getElement("warning")->color2));

	if (_battleSave->getOperation().empty() == false)
	{
		_txtOperationTitle->setText(_battleSave->getOperation());
		_txtOperationTitle->setHighContrast();
		_txtOperationTitle->setAlign(ALIGN_CENTER);
		_txtOperationTitle->setBig();

		const Sint16
			text_width	(static_cast<Sint16>(_txtOperationTitle->getTextWidth() + 14)),
			x_left		(static_cast<Sint16>((screenWidth - text_width) >> 1u)),
			x_right		(static_cast<Sint16>(x_left + text_width)),
			y_high		(static_cast<Sint16>(_srfTitle->getY() + 1)),
			y_low		(static_cast<Sint16>(_srfTitle->getHeight() - 2));

		_srfTitle->drawLine( // left line
						x_left, y_high,
						x_left, y_low,
						WHITE);
		_srfTitle->drawLine( // right line
						x_right, y_high,
						x_right, y_low,
						WHITE); // TODO: Get color (Uint8) of OperationTitle [#0].

		_srfTitle->drawLine( // left line shadow
						static_cast<Sint16>(x_left + 1), static_cast<Sint16>(y_high + 1),
						static_cast<Sint16>(x_left + 1), static_cast<Sint16>(y_low  + 1),
						BLACK);
		_srfTitle->drawLine( // right line shadow
						static_cast<Sint16>(x_right + 1), static_cast<Sint16>(y_high + 1),
						static_cast<Sint16>(x_right + 1), static_cast<Sint16>(y_low  + 1),
						BLACK);
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
			i = _playSave->getBases()->begin();
			i != _playSave->getBases()->end() && baseLabel.empty() == true;
			++i)
	{
		if ((*i)->getTactical() == true)
		{
			target = dynamic_cast<Target*>(*i);
			baseLabel = (*i)->getLabel();
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
				baseLabel = (*i)->getLabel();
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
				i = _playSave->getUfos()->begin();
				i != _playSave->getUfos()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);

				if ((*i)->isCrashed() == true)
					woststr << tr("STR_UFO_CRASH_RECOVERY");
				else
					woststr << tr("STR_UFO_GROUND_ASSAULT");

				woststr << L"> " << (*i)->getLabel(_game->getLanguage());
			}
		}

		for (std::vector<TerrorSite*>::const_iterator
				i = _playSave->getTerrorSites()->begin();
				i != _playSave->getTerrorSites()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);
				woststr << tr("STR_TERROR_MISSION") << L"> " << (*i)->getLabel(_game->getLanguage()); // <- not necessarily a Terror Mission ...
			}
		}

		for (std::vector<AlienBase*>::const_iterator
				i = _playSave->getAlienBases()->begin();
				i != _playSave->getAlienBases()->end() && woststr.str().empty() == true;
				++i)
		{
			if ((*i)->getTactical() == true)
			{
				target = dynamic_cast<Target*>(*i);
				woststr << tr("STR_ALIEN_BASE_ASSAULT") << L"> " << (*i)->getLabel(_game->getLanguage());
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
				i = _playSave->getRegions()->begin();
				i != _playSave->getRegions()->end();
				++i)
		{
			if ((*i)->getRules()->insideRegion(lon,lat) == true)
			{
				woststr << tr((*i)->getRules()->getType());
				break;
			}
		}

		for (std::vector<Country*>::const_iterator
				i = _playSave->getCountries()->begin();
				i != _playSave->getCountries()->end();
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

	_icons->onMouseIn(	static_cast<ActionHandler>(&BattlescapeState::mouseInIcons));
	_icons->onMouseOut(	static_cast<ActionHandler>(&BattlescapeState::mouseOutIcons));

	_btnUnitUp->onMousePress(	static_cast<ActionHandler>(&BattlescapeState::btnUnitUpPress),
								SDL_BUTTON_LEFT);
	_btnUnitUp->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnUnitUpRelease),
								SDL_BUTTON_LEFT);
//	_btnUnitUp->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnUnitUp->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnUnitUp->setTooltip("STR_UNIT_LEVEL_ABOVE");

	_btnUnitDown->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnUnitDownPress),
									SDL_BUTTON_LEFT);
	_btnUnitDown->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnUnitDownRelease),
									SDL_BUTTON_LEFT);
//	_btnUnitDown->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnUnitDown->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnUnitDown->setTooltip("STR_UNIT_LEVEL_BELOW");

	_btnMapUp->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnMapUpPress),
									SDL_BUTTON_LEFT);
	_btnMapUp->onKeyboardPress(		static_cast<ActionHandler>(&BattlescapeState::btnMapUpPress),
									Options::keyBattleLevelUp);
	_btnMapUp->onMouseRelease(		static_cast<ActionHandler>(&BattlescapeState::btnMapUpRelease),
									SDL_BUTTON_LEFT);
	_btnMapUp->onKeyboardRelease(	static_cast<ActionHandler>(&BattlescapeState::btnMapUpRelease),
									Options::keyBattleLevelUp);
//	_btnMapUp->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnMapUp->onMouseOut(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnMapUp->setTooltip("STR_VIEW_LEVEL_ABOVE");

	_btnMapDown->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnMapDownPress),
									SDL_BUTTON_LEFT);
	_btnMapDown->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnMapDownPress),
									Options::keyBattleLevelDown);
	_btnMapDown->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnMapDownRelease),
									SDL_BUTTON_LEFT);
	_btnMapDown->onKeyboardRelease(	static_cast<ActionHandler>(&BattlescapeState::btnMapDownRelease),
									Options::keyBattleLevelDown);
//	_btnMapDown->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnMapDown->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnMapDown->setTooltip("STR_VIEW_LEVEL_BELOW");

	_btnMiniMap->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnMinimapClick));
	_btnMiniMap->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnMinimapClick),
									Options::keyBattleMap);
//	_btnMiniMap->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnMiniMap->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnMiniMap->setTooltip("STR_MINIMAP");

	_btnKneel->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnKneelClick));
	_btnKneel->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnKneelClick),
								Options::keyBattleKneel);
//	_btnKneel->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnKneel->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnKneel->setTooltip("STR_KNEEL");

	_btnInventory->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnInventoryClick));
	_btnInventory->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnInventoryClick),
									Options::keyBattleInventory);
//	_btnInventory->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnInventory->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnInventory->setTooltip("STR_INVENTORY");

	_btnCenter->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnCenterPress),
									SDL_BUTTON_LEFT);
	_btnCenter->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnCenterPress),
									Options::keyBattleCenterUnit);
	_btnCenter->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnCenterPress),
									SDLK_KP5);
	_btnCenter->onMouseRelease(		static_cast<ActionHandler>(&BattlescapeState::btnCenterRelease),
									SDL_BUTTON_LEFT);
	_btnCenter->onKeyboardRelease(	static_cast<ActionHandler>(&BattlescapeState::btnCenterRelease),
									Options::keyBattleCenterUnit);
	_btnCenter->onKeyboardRelease(	static_cast<ActionHandler>(&BattlescapeState::btnCenterRelease),
									SDLK_KP5);
//	_btnCenter->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnCenter->onMouseOut(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnCenter->setTooltip("STR_CENTER_SELECTED_UNIT");

	_btnNextUnit->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnNextUnitPress),
									SDL_BUTTON_LEFT);
	_btnNextUnit->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnNextUnitPress),
									Options::keyBattleNextUnit);
	_btnNextUnit->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnNextUnitRelease),
									SDL_BUTTON_LEFT);
	_btnNextUnit->onKeyboardRelease(static_cast<ActionHandler>(&BattlescapeState::btnNextUnitRelease),
									Options::keyBattleNextUnit);
	_btnNextUnit->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnPrevUnitPress),
									SDL_BUTTON_RIGHT);
	_btnNextUnit->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnPrevUnitPress),
									Options::keyBattlePrevUnit);
	_btnNextUnit->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnPrevUnitRelease),
									SDL_BUTTON_RIGHT);
	_btnNextUnit->onKeyboardRelease(static_cast<ActionHandler>(&BattlescapeState::btnPrevUnitRelease),
									Options::keyBattlePrevUnit);
//	_btnNextUnit->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnNextUnit->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnNextUnit->setTooltip("STR_NEXT_UNIT");

	_btnNextStop->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnNextStopPress),
									SDL_BUTTON_LEFT);
	_btnNextStop->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnNextStopPress),
									Options::keyBattleDeselectUnit);
	_btnNextStop->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnNextStopRelease),
									SDL_BUTTON_LEFT);
	_btnNextStop->onKeyboardRelease(static_cast<ActionHandler>(&BattlescapeState::btnNextStopRelease),
									Options::keyBattleDeselectUnit);
	_btnNextStop->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnPrevStopPress), // NOTE: There's no Option for 'keyBattleDeselectPrevUnit'.
									SDL_BUTTON_RIGHT);
	_btnNextStop->onMouseRelease(	static_cast<ActionHandler>(&BattlescapeState::btnPrevStopRelease),
									SDL_BUTTON_RIGHT);
//	_btnNextStop->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnNextStop->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnNextStop->setTooltip("STR_DESELECT_UNIT");

	_btnShowLayers->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnShowLayersClick));
//	_btnShowLayers->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnShowLayers->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnShowLayers->setTooltip("STR_MULTI_LEVEL_VIEW");

	_btnOptions->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnBattleOptionsClick));
	_btnOptions->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnBattleOptionsClick),
									Options::keyBattleOptions); // = keyCancel. [Escape]
//	_btnOptions->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnOptions->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnOptions->setTooltip("STR_OPTIONS");

	_btnEndTurn->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnEndTurnClick));
	_btnEndTurn->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnEndTurnClick),
									Options::keyBattleEndTurn);
//	_btnEndTurn->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnEndTurn->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnEndTurn->setTooltip("STR_END_TURN");

	_btnAbort->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnAbortClick));
	_btnAbort->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnAbortClick),
								Options::keyBattleAbort);
//	_btnAbort->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnAbort->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnAbort->setTooltip("STR_ABORT_MISSION");

	_isfStats->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnStatsClick));
	_isfStats->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnStatsClick),
								Options::keyBattleStats);
//	_isfStats->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_isfStats->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_isfStats->setTooltip("STR_UNIT_STATS");

	_isfLeftHand->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnLeftHandLeftClick),
									SDL_BUTTON_LEFT);
	_isfLeftHand->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnLeftHandRightClick),
									SDL_BUTTON_RIGHT);
	_isfLeftHand->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnLeftHandLeftClick),
									Options::keyBattleUseLeftHand);
//	_isfLeftHand->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_isfLeftHand->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_isfLeftHand->setTooltip("STR_USE_LEFT_HAND");

	_isfRightHand->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnRightHandLeftClick),
									SDL_BUTTON_LEFT);
	_isfRightHand->onMouseClick(	static_cast<ActionHandler>(&BattlescapeState::btnRightHandRightClick),
									SDL_BUTTON_RIGHT);
	_isfRightHand->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnRightHandLeftClick),
									Options::keyBattleUseRightHand);
//	_isfRightHand->onMouseIn(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_isfRightHand->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_isfRightHand->setTooltip("STR_USE_RIGHT_HAND");

/*	_btnReserveNone->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnReserveClick));
	_btnReserveNone->onKeyboardPress(
					static_cast<ActionHandler>(&BattlescapeState::btnReserveClick),
					Options::keyBattleReserveNone);
	_btnReserveNone->onMouseIn(static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
	_btnReserveNone->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
	_btnReserveNone->setTooltip("STR_DONT_RESERVE_TIME_UNITS");

	_btnReserveSnap->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnReserveClick));
	_btnReserveSnap->onKeyboardPress(
					static_cast<ActionHandler>(&BattlescapeState::btnReserveClick),
					Options::keyBattleReserveSnap);
	_btnReserveSnap->onMouseIn(static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
	_btnReserveSnap->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
	_btnReserveSnap->setTooltip("STR_RESERVE_TIME_UNITS_FOR_SNAP_SHOT");

	_btnReserveAimed->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnReserveClick));
	_btnReserveAimed->onKeyboardPress(
					static_cast<ActionHandler>(&BattlescapeState::btnReserveClick),
					Options::keyBattleReserveAimed);
	_btnReserveAimed->onMouseIn(static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
	_btnReserveAimed->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
	_btnReserveAimed->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AIMED_SHOT");

	_btnReserveAuto->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnReserveClick));
	_btnReserveAuto->onKeyboardPress(
					static_cast<ActionHandler>(&BattlescapeState::btnReserveClick),
					Options::keyBattleReserveAuto);
	_btnReserveAuto->onMouseIn(static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
	_btnReserveAuto->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
	_btnReserveAuto->setTooltip("STR_RESERVE_TIME_UNITS_FOR_AUTO_SHOT");

	_btnReserveKneel->onMouseClick(static_cast<ActionHandler>(&BattlescapeState::btnReserveKneelClick));
	_btnReserveKneel->onKeyboardPress(
					static_cast<ActionHandler>(&BattlescapeState::btnReserveKneelClick),
					Options::keyBattleReserveKneel);
	_btnReserveKneel->onMouseIn(static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
	_btnReserveKneel->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
	_btnReserveKneel->allowToggleInversion();
	_btnReserveKneel->setTooltip("STR_RESERVE_TIME_UNITS_FOR_KNEEL"); */

	_isfLogo->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnZeroTuClick));
	_isfLogo->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::keyZeroTuPress));		// NOTE: Can't use a specific SDLKey on this because it requires CTRL.
	_isfLogo->onMouseClick(		static_cast<ActionHandler>(&BattlescapeState::btnUfoPaediaClick),	// ... InteractiveSurface handlers do not like that.
								SDL_BUTTON_RIGHT);
	_isfLogo->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnUfoPaediaClick),
								Options::keyGeoUfopedia);

//	_btnZeroTUs->onMouseIn(	static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//	_btnZeroTUs->onMouseOut(static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//	_btnZeroTUs->allowClickInversion();
//	_btnZeroTUs->setTooltip("STR_EXPEND_ALL_TIME_UNITS");

	// NOTE: The following shortcuts do not have a specific surface-button graphic.
//	_isfStats->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnReloadClick), // NOTE: Reloading uses advanced conditions in the Inventory.
//								Options::keyBattleReload);
	_isfStats->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::keyUnitLight),
								Options::keyBattlePersonalLighting);
	_isfStats->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::keyConsoleToggle),
								Options::keyBattleConsole);
	_isfStats->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::keyTurnUnit));	// NOTE: Can't use a specific SDLKey on this because it can require CTRL.
																								// ... InteractiveSurface handlers do not like that.

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

	const Uint8 color (static_cast<Uint8>(uiRule->getElement("visibleUnits")->color));
	for (size_t
			i = 0u;
			i != ICONS_HOSTILE;
			++i)
	{
		_isfHostiles[i]->onMousePress(		static_cast<ActionHandler>(&BattlescapeState::btnHostileUnitPress));
//		_isfHostiles[i]->onKeyboardPress(	static_cast<ActionHandler>(&BattlescapeState::btnHostileUnitPress),
//											buttons[i]);
//		_isfHostiles[i]->onMouseIn(			static_cast<ActionHandler>(&BattlescapeState::txtTooltipIn));
//		_isfHostiles[i]->onMouseOut(		static_cast<ActionHandler>(&BattlescapeState::txtTooltipOut));
//		std::ostringstream tooltip;
//		tooltip << "STR_CENTER_ON_ENEMY_" << (i + 1);
//		_isfHostiles[i]->setTooltip(tooltip.str());

		_numHostiles[i]->setColor(color);
		_numHostiles[i]->setValue(i + 1u);
	}

	for (size_t
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		_isfMedic[i]->onMousePress(static_cast<ActionHandler>(&BattlescapeState::btnWoundedPress));
	}

	_txtName->setHighContrast();

	_numTwohandR->setValue(2u);
	_numTwohandL->setValue(2u);

//	_btnReserveNone->setGroup(&_reserve);
//	_btnReserveSnap->setGroup(&_reserve);
//	_btnReserveAimed->setGroup(&_reserve);
//	_btnReserveAuto->setGroup(&_reserve);

	_game->getFpsCounter()->setY(screenHeight - 6);

	_timerAnimate = new Timer(STATE_INTERVAL_TILE);			// setStateInterval() does NOT change this <-
	_timerAnimate->onTimer(static_cast<StateHandler>(&BattlescapeState::animate));

	_timerTactical = new Timer(STATE_INTERVAL_STANDARD);	// setStateInterval() will change this <-
	_timerTactical->onTimer(static_cast<StateHandler>(&BattlescapeState::handleState));
	//_timerTactical->debug("BattlescapeState");

	_battleGame = new BattlescapeGame(_battleSave, this);
	//Log(LOG_INFO) << "Create BattlescapeState EXIT";
}

/**
 * Deletes this BattlescapeState.
 */
BattlescapeState::~BattlescapeState()
{
	//Log(LOG_INFO) << "Delete BattlescapeState";
	delete _timerAnimate;
	delete _timerTactical;
	delete _battleGame;
}

/**
 * Initializes this BattlescapeState.
 */
void BattlescapeState::init()
{
	//Log(LOG_INFO) << "BattlescapeState::init()";
	State::init();

	if (_init == true)
	{
		_init = false;

		if (_timerAnimate->isRunning() == false) // do NOT restart timers if/when 2nd stage starts.
			_timerAnimate->start();

		if (_timerTactical->isRunning() == false)
			_timerTactical->start();

//		_map->setFocus();	// -> NOTE: InteractiveSurface is initialized w/ (_isFocused=TRUE)
							// and only State::resetSurfaces() sets a Surface unfocused; resetSurfaces()
							// is called from Game::run() on each state-initialization, but
							// that's back-asswards. So (_isFocused=FALSE) has been disabled
							// in resetSurfaces() .... See also GeoscapeState::init().
		_map->cacheUnitSprites();
		_map->draw();

		_battleGame->getTileEngine()->calcFovTiles_all();
		_battleGame->getTileEngine()->calcFovUnits_all();

//		if (playableUnitSelected() == false)
//			selectNextPlayerUnit();

		_battleGame->setupSelector();
		updateSoldierInfo(false);

		if (playableUnitSelected() == true)
			_map->getCamera()->centerPosition(_battleSave->getSelectedUnit()->getPosition(), false);

		_numLayers->setValue(static_cast<unsigned>(_map->getCamera()->getViewLevel()) + 1);

		std::string // start music ->
			track,
			terrain;
		_battleSave->calibrateMusic(track, terrain);
		_game->getResourcePack()->playMusic(track, terrain);

//		switch (_battleSave->getBatReserved())
//		{
//			case BA_SNAPSHOT:	_reserve = _btnReserveSnap;  break;
//			case BA_AIMEDSHOT:	_reserve = _btnReserveAimed; break;
//			case BA_AUTOSHOT:	_reserve = _btnReserveAuto;  break;
//			default:			_reserve = _btnReserveNone;  break;
//		}
//		_btnReserveNone->setGroup(&_reserve);
//		_btnReserveSnap->setGroup(&_reserve);
//		_btnReserveAimed->setGroup(&_reserve);
//		_btnReserveAuto->setGroup(&_reserve);
	}

	_txtControlDestroyed->setVisible(_battleSave->getControlDestroyed() == true
								  && _iconsHidden == false);

	if (_autosave == true) // flagged by NextTurnState::nextTurn()
	{
		_autosave = false;
		if (_playSave->isIronman() == true)
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
}

/**
 * Sets a flag to re-initialize this BattlescapeState.
 * @note Called by BattlescapeGenerator::nextStage().
 */
void BattlescapeState::reinit()
{
	_init = true;
}

/**
 * Runs the Timers and handles popups.
 * @note Called by the engine-core.
 */
void BattlescapeState::think()
{
	//Log(LOG_INFO) << "BattlescapeState::think()";
	if (_timerTactical->isRunning() == true)
	{
		static bool popped (false);

		if (_popups.empty() == false) // handle popups. NOTE: Only showActionMenu() uses 'popups'.
		{
			popped = true;
			_game->pushState(_popups.front());
			_popups.erase(_popups.begin());
		}
		else
		{
			State::think();

			//Log(LOG_INFO) << "BattlescapeState::think() -> _battleGame.think()";
			_battleGame->think();
			//Log(LOG_INFO) << "BattlescapeState::think() -> _timerAnimate.think()";
			_timerAnimate->think(this, nullptr);
			//Log(LOG_INFO) << "BattlescapeState::think() -> _timerTactical.think()";
			_timerTactical->think(this, nullptr);
			//Log(LOG_INFO) << "BattlescapeState::think() -> back from thinks";

			if (popped == true)
			{
				popped = false;
				_battleGame->handleNonTargetAction();
			}
		}
	}
	//Log(LOG_INFO) << "BattlescapeState::think() EXIT";
}

/**
 * Adds a popup-window to the popups-queue.
 * @note This prevents popups from overlapping. wtf is "overlap"
 * @param state - pointer to popup State
 */
void BattlescapeState::popupTac(State* const state)
{
	_popups.push_back(state);
}

/**
 * Prints contents of a hovered Tile's inventory to screen.
 * @note This should have been done w/ vectors but it works as-is.
 * @param tile - pointer to a mouse-overed tile
 */
void BattlescapeState::printTileInventory(Tile* const tile) // private.
{
	_txtConsole1->setText(L"");
	_txtConsole2->setText(L"");

	bool showInfo;

	if (tile != nullptr
		&& tile->isRevealed() == true
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

				if (item->getBodyUnit() != nullptr)
				{
					if (item->getBodyUnit()->getType().compare(0u,11u, "STR_FLOATER") == 0) // See medikit w/ Autopsy OR inventory w/ Autopsy+Race research.
					{
						wst1 += tr("STR_FLOATER");
//						wst1 += L" (status doubtful)";
					}
					else
					{
						switch (item->getBodyUnit()->getUnitStatus())
						{
							case STATUS_UNCONSCIOUS:
								wst1 += item->getBodyUnit()->getLabel(_game->getLanguage());

								if (item->getBodyUnit()->getGeoscapeSoldier() != nullptr)
									wst1 += L" (" + Text::intWide(item->getBodyUnit()->getHealth() - item->getBodyUnit()->getStun() - 1) + L")";
								break;

							case STATUS_DEAD:
								wst1 += tr(itRule->getType());

								if (item->getBodyUnit()->getGeoscapeSoldier() != nullptr)
									wst1 += L" (" + item->getBodyUnit()->getLabel(_game->getLanguage()) + L")";
						}
					}
				}
				else if (_playSave->isResearched(itRule->getRequiredResearch()) == true)
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

/* void logDetails(Action* action)
{
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN || action->getDetails()->type == SDL_MOUSEBUTTONUP)
	{
		Log(LOG_INFO) << "button= " << (int)action->getDetails()->button.button;
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			Log(LOG_INFO) << "BUTTON_LEFT";
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
			Log(LOG_INFO) << "BUTTON_RIGHT";
		else if (action->getDetails()->button.button == SDL_BUTTON_MIDDLE)
			Log(LOG_INFO) << "BUTTON_MIDDLE";
		else
			Log(LOG_INFO) << "button other";
	}

	Log(LOG_INFO) << "type= " << (int)action->getDetails()->type;
	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
		Log(LOG_INFO) << "MOUSEDOWN";
	else if (action->getDetails()->type == SDL_MOUSEBUTTONUP)
		Log(LOG_INFO) << "MOUSEUP";
	else if (action->getDetails()->type == SDL_MOUSEMOTION)
		Log(LOG_INFO) << "MOUSEMOTION";
	else if (action->getDetails()->type == SDL_KEYDOWN)
		Log(LOG_INFO) << "KEYDOWN";
	else if (action->getDetails()->type == SDL_KEYUP)
		Log(LOG_INFO) << "KEYUP";
	else
		Log(LOG_INFO) << "type other";
} */

/**
 * Handles drag-scrolling and printing mouse-overed tile-info.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapOver(Action* action)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeState::mapOver()";

	if (action != nullptr
		&& action->getDetails() != nullptr
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		//logDetails(action);

		_dragScrollX += static_cast<int>(action->getDetails()->motion.xrel);
		_dragScrollY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_dragScrollPastPixelThreshold == false)
			_dragScrollPastPixelThreshold = std::abs(_dragScrollX) > Options::dragScrollPixelTolerance
										 || std::abs(_dragScrollY) > Options::dragScrollPixelTolerance;

		if (_dragScrollActivated == true && _dragScrollPastPixelThreshold == true)
		{
//			if (Options::battleDragScrollInvert == true) // scroll. I don't use inverted scrolling.
//				_map->getCamera()->scroll(
//										static_cast<int>(static_cast<double>(-action->getDetails()->motion.xrel) / action->getScaleX()),
//										static_cast<int>(static_cast<double>(-action->getDetails()->motion.yrel) / action->getScaleY()),
//										false);
//			else
			_map->getCamera()->scroll(
									static_cast<int>(static_cast<double>(action->getDetails()->motion.xrel) * 3.5 / action->getScaleX()),
									static_cast<int>(static_cast<double>(action->getDetails()->motion.yrel) * 3.5 / action->getScaleY()),
									false);

			_game->getCursor()->handle(action);
		}
		else
			handleTileInfo();
	}
}

/**
 * Updates mouse-overed tile-data and inventory-info.
 */
void BattlescapeState::handleTileInfo() // private.
{
	if (_mouseOverIcons == false && allowButtons() == true
		&& _game->getCursor()->getHidden() == false)
	{
		Position pos;
		_map->getSelectorPosition(pos);

		Tile* const tile (_battleSave->getTile(pos));
		updateTileInfo(tile);

		if (_showConsole > 0)
			printTileInventory(tile);
	}
//	else if (_mouseOverIcons == true){} // might need to erase some info here.
}

/**
 * Initiates drag-scrolling.
 * @note Apparently, for whatever reason, this handler fires only as the result
 * of a mouse-down event.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapPress(Action* action)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeState::mapPress()";
	//if (action != nullptr) logDetails(action);

	if (_mouseOverIcons == false)
	{
		_dragScrollStartTick = SDL_GetTicks();
		// NOTE: This is how ticks will be used.
		//
		// If the mouse-button is released under the threshold, any drag-scroll
		// is cancelled. The mouse-button event will register as a primary or
		// secondary action (and then only if the pixel-threshold is not
		// exceeded).
		//
		// If the mouse-button is released over the threshold, any primary or
		// secondary action will be cancelled. The mouse-button event will
		// register only as a drag-scroll (and then only if the pixel-threshold
		// is exceeded).

		_dragScrollX =
		_dragScrollY = 0;
		_dragScrollPastPixelThreshold = false;

		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_dragScrollActivated = true;
//			_dragScrollStartPos = _map->getCamera()->getMapOffset();
		}
	}
}

/**
 * Invokes a primary or secondary tactical-action, also finalizes drag-scrolling.
 * @note Apparently, for whatever reason, this handler fires only as the result
 * of a mouse-up event.
 * @param action - pointer to an Action
 */
void BattlescapeState::mapClick(Action* action)
{
	//Log(LOG_INFO) << "";
	//Log(LOG_INFO) << "BattlescapeState::mapClick()";
	//if (action != nullptr) logDetails(action);

//	if (_dragScrollActivated == true
//		&& action->getDetails()->button.button != Options::battleDragScrollButton)
//	{
//		return; // other buttons are ineffective while scrolling
//	}

	_dragScrollActivated = false;

	if (_dragScrollPastPixelThreshold == false
		&& SDL_GetTicks() - _dragScrollStartTick < static_cast<Uint32>(Options::dragScrollTimeTolerance))
	{
//		if (_dragScrollActivated == true)
//			_map->getCamera()->setMapOffset(_dragScrollStartPos);

		if ((action->getDetails()->button.button != SDL_BUTTON_RIGHT	// right-click removes pathPreview or aborts walking state
				|| _battleGame->cancelTacticalAction() == false)		// or skips projectile trajectory, etc.
			&& (_mouseOverIcons == false
				&& _map->getSelectorType() != CT_NONE
				&& _battleGame->isBusy() == false))
		{
			Position pos;
			_map->getSelectorPosition(pos);

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


				std::wostringstream woststr; // onScreen debug ->

				const BattleUnit* const unit (_battleSave->getTile(pos)->getTileUnit());
				if (unit != nullptr && unit->getUnitVisible() == true)
					woststr	<< L"unit "
							<< unit->getId()
							<< L" ";

				woststr << L"pos " << pos;
				printDebug(woststr.str());

				if (_battleSave->getDebugTac() == true) // print tile-info to Log ->
				{
					Log(LOG_INFO) << "";
					Log(LOG_INFO) << "data sets";
					size_t id (0u);
					for (std::vector<MapDataSet*>::const_iterator
							i = _battleSave->getBattleDataSets()->begin();
							i != _battleSave->getBattleDataSets()->end();
							++i, ++id)
					{
						Log(LOG_INFO) << ". " << id << " - " << (*i)->getType();
					}


					Log(LOG_INFO) << "";
					Log(LOG_INFO) << "tile info " << pos;

					int
						partSetId,
						partId;

					const Tile* const tile (_battleSave->getTile(pos));

					Log(LOG_INFO) << ". is Floored= " << tile->isFloored(tile->getTileBelow(_battleSave));

					if (tile->getMapData(O_FLOOR) != nullptr)
					{
						tile->getMapData(&partId, &partSetId, O_FLOOR);
						Log(LOG_INFO) << ". FLOOR partSetId= "	<< partSetId << " - " << tile->getMapData(O_FLOOR)->getDataset()->getType();
						Log(LOG_INFO) << ". FLOOR partId= "		<< partId;
					}
					else Log(LOG_INFO) << ". no FLOOR";

					if (tile->getMapData(O_WESTWALL) != nullptr)
					{
						tile->getMapData(&partId, &partSetId, O_WESTWALL);
						Log(LOG_INFO) << ". WESTWALL partSetId= "	<< partSetId << " - " << tile->getMapData(O_WESTWALL)->getDataset()->getType();
						Log(LOG_INFO) << ". WESTWALL partId= "		<< partId;
					}
					else Log(LOG_INFO) << ". no WESTWALL";

					if (tile->getMapData(O_NORTHWALL) != nullptr)
					{
						tile->getMapData(&partId, &partSetId, O_NORTHWALL);
						Log(LOG_INFO) << ". NORTHWALL partSetId= "	<< partSetId << " - " << tile->getMapData(O_NORTHWALL)->getDataset()->getType();
						Log(LOG_INFO) << ". NORTHWALL partId= "		<< partId;
					}
					else Log(LOG_INFO) << ". no NORTHWALL";

					if (tile->getMapData(O_OBJECT) != nullptr)
					{
						tile->getMapData(&partId, &partSetId, O_OBJECT);
						Log(LOG_INFO) << ". OBJECT partSetId= "	<< partSetId << " - " << tile->getMapData(O_OBJECT)->getDataset()->getType();
						Log(LOG_INFO) << ". OBJECT partId= "	<< partId;
					}
					else Log(LOG_INFO) << ". no OBJECT";
				}
			}
		}
	}
	else
		handleTileInfo();

//	else if (_dragScrollActivated == true
//		&& _dragScrollPastPixelThreshold == false)
//	{
//		_map->getCamera()->setMapOffset(_dragScrollStartPos);
//	}
}

/**
 * Handles mouse entering the Map surface.
 * @param action - pointer to an Action
 *
void BattlescapeState::mapIn(Action*)
{
	_dragScrollActivated = false;
	_map->setButtonsPressed(static_cast<Uint8>(Options::battleDragScrollButton), false);
} */

/**
 * Takes care of any events from the core-engine.
 * @param action - pointer to an Action
 */
inline void BattlescapeState::handle(Action* action)
{
	if (_init == true)
		return;

	bool doit;
	if (_game->getCursor()->getVisible() == true)
		doit = true;
	else
	{
		doit = false;
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_RIGHT: // -> not sure what this is on about; but here's a refactor of that.
				switch (action->getDetails()->type)
				{
					case SDL_MOUSEBUTTONDOWN:
					case SDL_MOUSEBUTTONUP:
						doit = true;
				}
		}
	}

	if (doit == true)
	{
		State::handle(action);

		switch (action->getDetails()->type)
		{
			case SDL_KEYDOWN:
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
//											(*i)->takeDamage(Position(0,0,0), 1000, DT_AP, true);
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
//											(*i)->takeDamage(Position(0,0,0), 1000, DT_STUN, true);
										}
									}
									break;

								// TODO: vet this ->
//								case SDLK_w:										// "ctrl-w" - warp unit.
//									beep = true; //MB_ICONWARNING
//									printDebug(L"beam me up Scotty");
//									BattleUnit* unit (_battleSave->getSelectedUnit());
//									if (unit != nullptr)
//									{
//										Position pos;
//										_map->getSelectorPosition(&pos);
//										if (pos.x > -1) // TODO: a better check ... getTileValid eg.
//										{
//											unit->getTile()->setUnit(nullptr);
//											unit->setPosition(pos);
//											_battleSave->getTile(pos)->setUnit(unit);
//											_battleSave->getTileEngine()->calculateUnitLighting();
//											_battleSave->getBattleGame()->handleBattleState(); // why. Let the regular call by the tactical-timer handle it.
//										}
//									}
							}

							if (casualties == true)
							{
								_battleGame->checkCasualties(nullptr, nullptr, true);
//								_battleGame->handleBattleState(); // why. Let the regular call by the tactical-timer handle it.
							}
						}
					}
					else
					{
						switch (action->getDetails()->key.keysym.sym)
						{
//							case SDLK_F10:													// f10 - voxel-map dump. - moved below_
//								beep = true;
//								saveVoxelMap();
//								break;

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
					saveVoxelMaps();
				}
				else if (_playSave->isIronman() == false)
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

				if (beep == true)
				{
#ifdef _WIN32
					MessageBeep(MB_OK);
#endif
					break; // <- is not comprehensive; some conditions above^ will pass through. no biggie.
				}
				// no break;
			}

			case SDL_KEYUP:
				switch (action->getDetails()->key.keysym.sym)								// Alt - handleTileInfo.
				{
					case SDLK_LALT:
					case SDLK_RALT:
						handleTileInfo();
				}
//				break;
//
//			case SDL_MOUSEBUTTONDOWN:
//				if (action->getDetails()->button.button == SDL_BUTTON_X1)
//					btnNextUnitPress(action);
//				else if (action->getDetails()->button.button == SDL_BUTTON_X2)
//					btnPrevUnitPress(action);
		}
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
					warning(BattlescapeGame::PLAYER_ERROR[12u]); // cant fly
					break;

				case FLY_BLOCKED:
					warning(BattlescapeGame::PLAYER_ERROR[13u]); // not allowed: roof
					break;

				case FLY_GRAVLIFT:
				case FLY_GOOD:
					_srtIconsOverlay->getFrame(0)->blit(_btnUnitUp);
					_battleGame->cancelTacticalAction();
					_battleGame->moveUpDown(
										_battleSave->getSelectedUnit(),
										Pathfinding::DIR_UP);
			}
		}
		else
			warning(BattlescapeGame::PLAYER_ERROR[12u]); // cant fly
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
				warning(BattlescapeGame::PLAYER_ERROR[14u]); // not allowed: floor
				break;

			case FLY_GRAVLIFT:
			case FLY_GOOD:
				_srtIconsOverlay->getFrame(7)->blit(_btnUnitDown);
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
	if (allowButtons() == true && _map->getCamera()->up() == true)
	{
		if (action != nullptr) // prevent hotSqrs from depressing btn.
			_srtIconsOverlay->getFrame(1)->blit(_btnMapUp);

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
	if (allowButtons() == true && _map->getCamera()->down() == true)
	{
		if (action != nullptr) // prevent hotSqrs from depressing btn.
			_srtIconsOverlay->getFrame(8)->blit(_btnMapDown);

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
//		_srtIconsOverlay->getFrame(2)->blit(_btnMiniMap); // -> hidden by MiniMap itself atm.
		_game->pushState(new MiniMapState());
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
			if (_battleGame->kneelToggle(unit) == true)
			{
				_battleGame->getTileEngine()->calcFovTiles(unit); // always Faction_Player here.
				_battleGame->getTileEngine()->calcFovUnits_pos(unit->getPosition(), true);

				updateSoldierInfo(false);

				if (_battleGame->getTileEngine()->checkReactionFire(unit) == true)
					_battleSave->rfTriggerOffset(_map->getCamera()->getMapOffset());

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
//				(*i)->prepareUnit();
//			updateSoldierInfo();
//		}
//	}

	if (playableUnitSelected() == true)
	{
		const BattleUnit* const unit (_battleSave->getSelectedUnit());

		if ((unit->isMechanical() == false && unit->getRankString() != "STR_LIVE_TERRORIST")
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
//			_srtIconsOverlay->getFrame(3)->blit(_btnInventory); // clear() not implemented @ InventoryState.
			_game->pushState(new InventoryState(
											true, //_battleSave->getDebugTac() == false, // CHEAT For debugging.
											this));
			_game->getScreen()->fadeScreen();
		}
	}
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
			_srtIconsOverlay->getFrame(10)->blit(_btnCenter);

		_map->getCamera()->centerPosition(_battleSave->getSelectedUnit()->getPosition());
		refreshMousePosition();
	}
}

/**
 * Releases the Center btn.
 * @ note For most of these buttons, refreshMousePosition() is enough; but for
 * Center, it also wants to update tile-info since if the hot-key is used to
 * center the selected unit, the tile that the selector is over will likely
 * change.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnCenterRelease(Action*)
{
	_btnCenter->clear();
	handleTileInfo();
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
		_srtIconsOverlay->getFrame(4)->blit(_btnNextUnit);
		selectNextPlayerUnit(false, true);
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
		_srtIconsOverlay->getFrame(11)->blit(_btnNextStop);
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
		_srtIconsOverlay->getFrame(4)->blit(_btnNextUnit);
		selectPreviousPlayerUnit(false, true);
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
		_srtIconsOverlay->getFrame(11)->blit(_btnNextStop);
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
 * @param dontReselect		- true to set the current unit's reselectable flag FALSE (default false)
 * @param checkReselect		- true to check the next unit's reselectable flag (default false)
 * @param checkInventory	- true to check if the next unit has no inventory (default false)
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattlescapeState::selectNextPlayerUnit(
		bool dontReselect,
		bool checkReselect,
		bool checkInventory)
{
//	if (allowButtons() == true && _battleGame->getTacticalAction()->type == BA_NONE)
//	{
	BattleUnit* const unit (_battleSave->selectNextUnit(
													dontReselect,
													checkReselect,
													checkInventory));
	updateSoldierInfo(false); // try no calcFov()

	if (unit != nullptr)
		_map->getCamera()->centerPosition(unit->getPosition());

	_battleGame->cancelTacticalAction();
	_battleGame->setupSelector();

	return unit;
//	}
}

/**
 * Selects the player's previous BattleUnit.
 * @param dontReselect		- true to set the current unit's reselectable flag FALSE (default false)
 * @param checkReselect		- true to check the next unit's reselectable flag (default false)
 * @param checkInventory	- true to check if the next unit has no inventory (default false)
 * @return, pointer to a BattleUnit
 */
BattleUnit* BattlescapeState::selectPreviousPlayerUnit(
		bool dontReselect,
		bool checkReselect,
		bool checkInventory)
{
//	if (allowButtons() == true && _battleGame->getTacticalAction()->type == BA_NONE)
//	{
	BattleUnit* const unit (_battleSave->selectPrevUnit(
													dontReselect,
													checkReselect,
													checkInventory));
	updateSoldierInfo(false); // try no calcFov()

	if (unit != nullptr)
		_map->getCamera()->centerPosition(unit->getPosition());

	_battleGame->cancelTacticalAction();
	_battleGame->setupSelector();

	return unit;
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
			_srtIconsOverlay->getFrame(5)->blit(_btnShowLayers);
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
		_srtIconsOverlay->getFrame(12)->blit(_btnOptions);
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
	}
}

/**
 * Clears the Options btn.
 * @note To be called from PauseState::btnSaveClick() and ::btnCancelClick().
 */
void BattlescapeState::clearOptionsOverlay()
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
//		_srtIconsOverlay->getFrame(6)->blit(_btnEndTurn);
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
		_srtIconsOverlay->getFrame(13)->blit(_btnAbort);
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
			_battleGame->getTacticalAction()->waypoints.clear();	// but i don't want to look it up.
			_map->getWaypoints()->clear();
			_btnLaunch->setVisible(false);
		}

		_battleGame->cancelTacticalAction(true);
		_game->pushState(new UnitInfoState(
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
		BattleUnit* const unit (_battleSave->getSelectedUnit());

		activateHand(unit, AH_LEFT);
		showActionMenu(
					unit->getItem(ST_LEFTHAND),
					unit->getFatals(BODYPART_LEFTARM) != 0);
	}
}

/**
 * Sets left-hand as the Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnLeftHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
		activateHand(
				_battleSave->getSelectedUnit(),
				AH_LEFT);
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
		BattleUnit* const unit (_battleSave->getSelectedUnit());

		activateHand(unit, AH_RIGHT);
		showActionMenu(
					unit->getItem(ST_RIGHTHAND),
					unit->getFatals(BODYPART_RIGHTARM) != 0);
	}
}

/**
 * Sets right-hand as the Active Hand.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnRightHandRightClick(Action*)
{
	if (playableUnitSelected() == true)
		activateHand(
				_battleSave->getSelectedUnit(),
				AH_RIGHT);
}

/**
 * Activates the left or right hand on a hand-click.
 * @param unit - pointer to a BattleUnit
 * @param hand - the hand to activate
 */
void BattlescapeState::activateHand( // private.
		BattleUnit* const unit,
		ActiveHand hand)
{
	_battleGame->cancelTacticalAction();

	unit->setActiveHand(hand);
	updateSoldierInfo(false);

	_map->cacheUnitSprite(unit);
	_map->draw();
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
					i != ICONS_HOSTILE;
					++i)
			{
				if (_isfHostiles[i] == action->getSender())
					break;
			}

			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_LEFT:
				{
					const Position& pos (_hostileUnits[i]->getPosition());

					Camera* const camera (_map->getCamera());
					if (camera->focusPosition(pos, false) == true)
					{
						_srfTargeter->setX((Options::baseXResolution >> 1u) - 16);
						_srfTargeter->setY((Options::baseYResolution - _rules->getInterface("battlescape")->getElement("icons")->h) >> 1u);
					}
					else
					{
						Position posScreen;
						camera->convertMapToScreen(
												pos,
												&posScreen);
						posScreen += camera->getMapOffset();
						_srfTargeter->setX(posScreen.x);
						_srfTargeter->setY(posScreen.y);
					}

					_srfTargeter->setVisible();
					_cycleTargeter = 0u;
					break;
				}

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
						j = _battleSave->getUnits()->begin() + static_cast<std::ptrdiff_t>(curIter);
						j != _battleSave->getUnits()->end();
						++j)
					{
						if ((*j)->getFaction() == FACTION_PLAYER
							&& (*j)->isOut_t(OUT_STAT) == false
							&& std::find(
									(*j)->getHostileUnits().begin(),
									(*j)->getHostileUnits().end(),
									_hostileUnits[i]) != (*j)->getHostileUnits().end())
						{
							nextSpotter = *j;
							break;
						}
					}

					if (nextSpotter == nullptr)
					{
						for (std::vector<BattleUnit*>::const_iterator
							j = _battleSave->getUnits()->begin();
							j != _battleSave->getUnits()->end() - static_cast<std::ptrdiff_t>(_battleSave->getUnits()->size() - curIter);
							++j)
						{
							if ((*j)->getFaction() == FACTION_PLAYER
								&& (*j)->isOut_t(OUT_STAT) == false
								&& std::find(
										(*j)->getHostileUnits().begin(),
										(*j)->getHostileUnits().end(),
										_hostileUnits[i]) != (*j)->getHostileUnits().end())
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
						_map->getCamera()->focusPosition(nextSpotter->getPosition());
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
					i != ICONS_MEDIC;
					++i)
			{
				if (_isfMedic[i] == action->getSender())
				{
					_map->getCamera()->centerPosition(_tileMedic[i]->getPosition());

					if (btnId == SDL_BUTTON_LEFT)
					{
						BattleUnit* const unit (_tileMedic[i]->getTileUnit());
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
 * Zeroes TU of the currently selected BattleUnit w/ mouse-click.
 * @note Requires CTRL-key down and BattleStates inactive.
 * @param action - pointer to an Action
 */
void BattlescapeState::btnZeroTuClick(Action*)
{
	if ((SDL_GetModState() & KMOD_CTRL) != 0
		&& playableUnitSelected() == true)
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (_battleGame->noActionsPending(unit) == true)
		{
			unit->setTu();
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
		&& (   action->getDetails()->key.keysym.sym == Options::keyBattleZeroTUs
			|| action->getDetails()->key.keysym.sym == SDLK_KP_PERIOD)
		&& playableUnitSelected() == true)
	{
		BattleUnit* const unit (_battleSave->getSelectedUnit());
		if (_battleGame->noActionsPending(unit) == true)
		{
			unit->setTu();
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
	const SDLKey keyId (action->getDetails()->key.keysym.sym);

	if (keyId == Options::keyBattlePivotCcw || keyId == Options::keyBattlePivotCw
		&& playableUnitSelected() == true)
	{
		BattleAction* const tacAction (_battleGame->getTacticalAction());
		tacAction->actor = _battleSave->getSelectedUnit();
		tacAction->targeting = false;

		int dir;
		if (keyId == Options::keyBattlePivotCcw)
			dir = -1; // pivot unit counter-clockwise
		else
			dir = +1; // pivot unit clockwise

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

		_battleGame->stateBPushBack(new UnitTurnBState(_battleGame, *tacAction));
	}
}

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
					|| _init == true) // huh.
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
 * Updates the currently selected unit's OnScreen stats & info.
 * @param spot - true to run calcFovUnits() (default true)
 */
void BattlescapeState::updateSoldierInfo(bool spot)
{
	clearHostileIcons();

	_srfRank		->clear();
	_isfRightHand	->clear();
	_isfLeftHand	->clear();
	_btnKneel		->clear();

	_isfLeftHand	->setVisible(false);
	_isfRightHand	->setVisible(false);
	_numAmmoL		->setVisible(false);
	_numAmmoR		->setVisible(false);
	_numFuseL		->setVisible(false);
	_numFuseR		->setVisible(false);

	_numTwohandL	->setVisible(false);
	_numTwohandR	->setVisible(false);

	_numMediL1		->setVisible(false);
	_numMediL2		->setVisible(false);
	_numMediL3		->setVisible(false);
	_numMediR1		->setVisible(false);
	_numMediR2		->setVisible(false);
	_numMediR3		->setVisible(false);

	_srfOverweight	->setVisible(false);
	_numDir			->setVisible(false);
	_numDirTur		->setVisible(false);

	_numTULaunch	->setVisible(false);
	_numTUAim		->setVisible(false);
	_numTUAuto		->setVisible(false);
	_numTUSnap		->setVisible(false);

	_isOverweight = false;

	_txtOrder->setText(L"");


	if (playableUnitSelected() == false) // not a controlled unit; ie. aLien or civilian turn
	{
		showPsiButton(false);

		_txtName->setText(L"");

		_srfRank		->setVisible(false);

		_numTimeUnits	->setVisible(false);
		_barTimeUnits	->setVisible(false);

		_numEnergy		->setVisible(false);
		_barEnergy		->setVisible(false);

		_numHealth		->setVisible(false);
		_barHealth		->setVisible(false);

		_numMorale		->setVisible(false);
		_barMorale		->setVisible(false);

		return;
	}
//	else not aLien nor civilian; ie. a controlled unit ->>

	_srfRank		->setVisible();

	_numTimeUnits	->setVisible();
	_barTimeUnits	->setVisible();

	_numEnergy		->setVisible();
	_barEnergy		->setVisible();

	_numHealth		->setVisible();
	_barHealth		->setVisible();

	_numMorale		->setVisible();
	_barMorale		->setVisible();


	BattleUnit* const selUnit (_battleSave->getSelectedUnit());
	if (spot == true)
		_battleSave->getTileEngine()->calcFovUnits(selUnit);

	if (_battleSave->getSide() == FACTION_PLAYER)
		updateHostileIcons();

	_txtName->setText(selUnit->getLabel(
									_game->getLanguage(),
									false));

	const Soldier* const sol (selUnit->getGeoscapeSoldier());
	if (sol != nullptr)
	{
		static SurfaceSet* const texture (_game->getResourcePack()->getSurfaceSet("SMOKE.PCK"));
		texture->getFrame(20 + sol->getRank())->blit(_srfRank);

		if (selUnit->isKneeled() == true)
			_srtIconsOverlay->getFrame(9)->blit(_btnKneel);

		_txtOrder->setText(tr("STR_ORDER")
							.arg(static_cast<int>(selUnit->getBattleOrder())));
	}

	if (selUnit->getCarriedWeight() > selUnit->getStrength())
		_isOverweight = true;

	_numDir->setValue(static_cast<unsigned>(selUnit->getUnitDirection()));
	_numDir->setVisible();

	if (selUnit->getTurretType() != TRT_NONE)
	{
		_numDirTur->setValue(static_cast<unsigned>(selUnit->getTurretDirection()));
		_numDirTur->setVisible();
	}


	double stat (static_cast<double>(selUnit->getBattleStats()->tu));
	const int tu (selUnit->getTu());
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

	ActiveHand ah (selUnit->deterActiveHand());
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
				switch (rtItem->getRules()->getBattleType())
				{
					case BT_FIREARM:
					case BT_MELEE:
						tuLaunch = selUnit->getActionTu(BA_LAUNCH, rtItem);
						tuAim = selUnit->getActionTu(BA_AIMEDSHOT, rtItem);
						tuAuto = selUnit->getActionTu(BA_AUTOSHOT, rtItem);
						tuSnap = selUnit->getActionTu(BA_SNAPSHOT, rtItem);
						if (tuLaunch  == 0
							&& tuAim  == 0
							&& tuAuto == 0
							&& tuSnap == 0)
						{
							tuSnap = selUnit->getActionTu(BA_MELEE, rtItem);
						}
				}
				break;

			case AH_LEFT:
				switch (ltItem->getRules()->getBattleType())
				{
					case BT_FIREARM:
					case BT_MELEE:
						tuLaunch = selUnit->getActionTu(BA_LAUNCH, ltItem);
						tuAim = selUnit->getActionTu(BA_AIMEDSHOT, ltItem);
						tuAuto = selUnit->getActionTu(BA_AUTOSHOT, ltItem);
						tuSnap = selUnit->getActionTu(BA_SNAPSHOT, ltItem);
						if (tuLaunch  == 0
							&& tuAim  == 0
							&& tuAuto == 0
							&& tuSnap == 0)
						{
							tuSnap = selUnit->getActionTu(BA_MELEE, ltItem);
						}
				}
		}

		if (tuLaunch != 0)
		{
			_numTULaunch->setValue(static_cast<unsigned>(tuLaunch));
			_numTULaunch->setVisible();
		}

		if (tuAim != 0)
		{
			_numTUAim->setValue(static_cast<unsigned>(tuAim));
			_numTUAim->setVisible();
		}

		if (tuAuto != 0)
		{
			_numTUAuto->setValue(static_cast<unsigned>(tuAuto));
			_numTUAuto->setVisible();
		}

		if (tuSnap != 0)
		{
			_numTUSnap->setValue(static_cast<unsigned>(tuSnap));
			_numTUSnap->setVisible();
		}
	}

	if (rtItem != nullptr)
	{
		itRule = rtItem->getRules();
		itRule->drawHandSprite(
							_srtBigobs,
							_isfRightHand);
		_isfRightHand->setVisible();

		if (itRule->isFixed() == false && itRule->isTwoHanded() == true)
			_numTwohandR->setVisible();

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
//			case BT_MELEE:
				if (rtItem->selfPowered() == false || rtItem->selfExpended() == true)
				{
					Uint8 color;

					const BattleItem* const aItem (rtItem->getAmmoItem());
					if (aItem != nullptr)
					{
						const int load (aItem->getAmmoQuantity());
						_numAmmoR->setValue(static_cast<unsigned>(load));

						int fullclip;
						if (itRule->isFixed() == true)
							fullclip = itRule->getFullClip();
						else
							fullclip = aItem->getRules()->getFullClip();

						if		(load ==  fullclip)			color = GREEN_D;
						else if	(load >= (fullclip >> 1u))	color = YELLOW_D;
						else								color = ORANGE_D;
					}
					else
					{
						_numAmmoR->setValue(0u);
						color = RED_M;
					}
					_numAmmoR->setColor(color);
					_numAmmoR->setVisible();
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
							_srtBigobs,
							_isfLeftHand);
		_isfLeftHand->setVisible();

		if (itRule->isFixed() == false && itRule->isTwoHanded() == true)
			_numTwohandL->setVisible();

		switch (itRule->getBattleType())
		{
			case BT_FIREARM:
//			case BT_MELEE:
				if (ltItem->selfPowered() == false || ltItem->selfExpended() == true)
				{
					Uint8 color;

					const BattleItem* const aItem (ltItem->getAmmoItem());
					if (aItem != nullptr)
					{
						const int load (aItem->getAmmoQuantity());
						_numAmmoL->setValue(static_cast<unsigned>(load));

						int fullclip;
						if (itRule->isFixed() == true)
							fullclip = itRule->getFullClip();
						else
							fullclip = aItem->getRules()->getFullClip();

						if		(load ==  fullclip)			color = GREEN_D;
						else if	(load >= (fullclip >> 1u))	color = YELLOW_D;
						else								color = ORANGE_D;
					}
					else
					{
						_numAmmoL->setValue(0u);
						color = RED_M;
					}
					_numAmmoL->setColor(color);
					_numAmmoL->setVisible();
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
void BattlescapeState::clearHostileIcons()
{
	for (size_t // hide target indicators & clear targets
			i = 0u;
			i != ICONS_HOSTILE;
			++i)
	{
		_isfHostiles[i]->setVisible(false);
		_numHostiles[i]->setVisible(false);
		_hostileUnits[i] = nullptr;
	}
}

/**
 * Updates the hostile unit indicator squares.
 */
void BattlescapeState::updateHostileIcons()
{
	size_t j (0u);
	for (std::vector<BattleUnit*>::const_iterator
		i = _battleSave->getUnits()->begin();
		i != _battleSave->getUnits()->end() && j != ICONS_HOSTILE;
		++i)
	{
		if (   (*i)->getFaction()     == FACTION_HOSTILE
			&& (*i)->getUnitStatus()  == STATUS_STANDING
			&& (*i)->getUnitVisible() == true)
		{
			_isfHostiles[j]->setVisible();
			_numHostiles[j]->setVisible();
			_hostileUnits[j++] = *i;
		}
	}
}

/**
 * Updates the wounded units indicators.
 */
void BattlescapeState::updateMedicIcons()
{
	static Surface* const srfBadge (_game->getResourcePack()->getSurface("RANK_ROOKIE"));

	for (size_t // hide target indicators & clear tiles
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		_isfMedic[i]->clear();
		_isfMedic[i]->setVisible(false);
		_numMedic[i]->setVisible(false);
		_tileMedic[i] = nullptr;
	}

	const bool vis (_battleSave->getSide() == FACTION_PLAYER);

	const BattleUnit* unit;
	Tile* tile;
	for (size_t
			i = 0u, k = 0u;
			i != _battleSave->getMapSizeXYZ() && k != ICONS_MEDIC;
			++i)
	{
		tile = _battleSave->getTiles()[i];

		if ((unit = tile->getTileUnit()) != nullptr
			&& unit->getFatalsTotal() != 0
			&& unit->getFaction() == FACTION_PLAYER
			&& unit->isMindControlled() == false
			&& unit->getGeoscapeSoldier() != nullptr
			&& unit->isOut_t(OUT_HEALTH) == false)
		{
			srfBadge->blit(_isfMedic[k]);
			_isfMedic[k]->setVisible(vis);

			_numMedic[k]->setValue(static_cast<unsigned>(unit->getFatalsTotal()));
			_numMedic[k]->setVisible(vis);

			_tileMedic[k++] = tile;
		}

		for (std::vector<BattleItem*>::const_iterator
				j = tile->getInventory()->begin();
				j != tile->getInventory()->end();
				++j)
		{
			if ((unit = (*j)->getBodyUnit()) != nullptr
				&& unit->getUnitStatus() == STATUS_UNCONSCIOUS
				&& unit->getFatalsTotal() != 0
				&& unit->getFaction() == FACTION_PLAYER
				&& unit->getGeoscapeSoldier() != nullptr)
			{
				srfBadge->blit(_isfMedic[k]);
				_isfMedic[k]->setVisible(vis);

				_numMedic[k]->setValue(static_cast<unsigned>(unit->getFatalsTotal()));
				_numMedic[k]->setVisible(vis);

				_tileMedic[k++] = tile;
			}
		}
	}
}

/**
 * Animates red cross icon(s) on Wounded hot-icons.
 */
void BattlescapeState::cycleMedic() // private.
{
	static Surface* const srf (_srtScanG->getFrame(11)); // dark gray cross
	static int phase (0);

	for (size_t
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		if (_isfMedic[i]->getVisible() == true)
		{
			_isfMedic[i]->lock();
			srf->blitNShade(
						_isfMedic[i],
						_isfMedic[i]->getX() + 2,
						_isfMedic[i]->getY() + 1,
						phase, false, 3); // red
			_isfMedic[i]->unlock();

			_numMedic[i]->setColor(static_cast<Uint8>(YELLOW + phase));
		}
		else
			break;
	}

	if ((phase += 2) == 16) phase = 0;
}

/**
 * Blinks the health-bar when selected unit has fatal wounds.
 */
void BattlescapeState::cycleHealthBar() // private.
{
	static const int TICKS (5);
	static int vis (0);

	_barHealth->setVisible(++vis > (TICKS >> 1u));

	if (vis == TICKS) vis = 0;
}

/**
 * Shows a selected unit's kneeled state.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeState::toggleKneelButton(const BattleUnit* const unit)
{
	if (unit != nullptr && unit->isKneeled() == true)
		_srtIconsOverlay->getFrame(9)->blit(_btnKneel);
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
			cycleMedic();

			BattleUnit* const selUnit (_battleSave->getSelectedUnit());
			if (selUnit != nullptr)
			{
				cycleHostileIcons(selUnit);
				cycleFuses(selUnit);

				if (selUnit->getFatalsTotal() != 0)
					cycleHealthBar();

				if (_srfTargeter->getVisible() == true)
					cycleTargeter();

				if (_battleGame->getLiquidate() == true)
					liquidationExplosion();

				if (_isOverweight == true && RNG::seedless(0,3) == 0)
					_srfOverweight->setVisible(!_srfOverweight->getVisible());

				static int stickyTiks (0);
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
	static Surface* const srf (_srtScanG->getFrame(9)); // light gray cross
	static const int pulse[PHASE_FUSE] { 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,
										13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3};

	if (_cycleFuse == PHASE_FUSE)
		_cycleFuse = 0u;

	const BattleItem* item (selUnit->getItem(ST_LEFTHAND));
	if (item != nullptr && item->getFuse() != -1)
	{
		switch (item->getRules()->getBattleType())
		{
			case BT_GRENADE:
			case BT_PROXYGRENADE:
			case BT_FLARE:
				_isfLeftHand->lock();
				srf->blitNShade(
							_isfLeftHand,
							_isfLeftHand->getX() + 27,
							_isfLeftHand->getY() -  1,
							pulse[_cycleFuse],
							false, 3); // red
				_isfLeftHand->unlock();
		}
	}

	if ((item = selUnit->getItem(ST_RIGHTHAND)) != nullptr && item->getFuse() != -1)
	{
		switch (item->getRules()->getBattleType())
		{
			case BT_GRENADE:
			case BT_PROXYGRENADE:
			case BT_FLARE:
				_isfRightHand->lock();
				srf->blitNShade(
							_isfRightHand,
							_isfRightHand->getX() + 27,
							_isfRightHand->getY() -  1,
							pulse[_cycleFuse],
							false, 3); // red
				_isfRightHand->unlock();
		}
	}
	++_cycleFuse;
}

/**
 * Shifts the colors of the hostileUnit buttons' backgrounds.
 * @param selUnit - the currently selected BattleUnit
 */
void BattlescapeState::cycleHostileIcons(BattleUnit* const selUnit) // private.
{
	static int
		delta		  (1),
		colorRed	 (36), // currently selected unit sees other unit
		colorBlue	(116), // another unit can see other unit
		color_border (13); // very.dark.gray

	Uint8 color;
	bool isSpotted;

	for (size_t
			i = 0u;
			i != ICONS_HOSTILE;
			++i)
	{
		if (_isfHostiles[i]->getVisible() == true)
		{
			isSpotted = false;
			for (std::vector<BattleUnit*>::const_iterator
				j = _battleSave->getUnits()->begin();
				j != _battleSave->getUnits()->end() && isSpotted == false;
				++j)
			{
				if ((*j)->getFaction() == FACTION_PLAYER)
				{
					switch ((*j)->getUnitStatus())
					{
						case STATUS_STANDING:
						case STATUS_WALKING:
						case STATUS_FLYING:
						case STATUS_TURNING:
						case STATUS_AIMING:
							if (std::find(
										(*j)->getHostileUnits().begin(),
										(*j)->getHostileUnits().end(),
										_hostileUnits[i]) != (*j)->getHostileUnits().end())
							{
								isSpotted = true;
							}
//						STATUS_COLLAPSING
//						STATUS_DEAD
//						STATUS_UNCONSCIOUS
//						STATUS_PANICKING
//						STATUS_BERSERK
//						STATUS_LATENT
//						STATUS_LATENT_START
					}
				}
			}

			if (isSpotted == false)
				color = GREEN_D; // hostile unit is visible but not currently in LoS of friendly units; ergo do not cycle colors.
			else if (std::find(
							selUnit->getHostileUnits().begin(),
							selUnit->getHostileUnits().end(),
							_hostileUnits[i]) != selUnit->getHostileUnits().end())
			{
				color = static_cast<Uint8>(colorRed);
			}
			else
				color = static_cast<Uint8>(colorBlue);

			_isfHostiles[i]->drawRect(0,0, 15,13, static_cast<Uint8>(color_border));
			_isfHostiles[i]->drawRect(1,1, 13,11, color);
		}
		else
			break;
	}

	switch (colorRed)
	{
		case 36: delta = +1; break;
		case 45: delta = -1;
	}

	colorRed += delta;
	colorBlue += delta;
	color_border -= delta;
}

/**
 * Animates a targeter over a hostile unit when the hot-square for a
 * corresponding hostile unit is mouse-pressed.
 */
void BattlescapeState::cycleTargeter() // private.
{
	static const int targeterFrames[PHASE_TARGET] {0,1,2,3,4,0};

	Surface* const srfTargeter (_srtTargeter->getFrame(targeterFrames[_cycleTargeter]));
	srfTargeter->blit(_srfTargeter);

	if (++_cycleTargeter == PHASE_TARGET)
		_srfTargeter->setVisible(false); // NOTE: The last phase will not be drawn.
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
		_battleSave->getSelectedUnit()->toggleShoot();
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
 * Popups a context-sensitive-list of BattleActions for the player.
 * @param item		- pointer to the BattleItem (righthand/lefthand)
 * @param injured	- true if the arm using @a item is injured (default false)
 */
void BattlescapeState::showActionMenu( // private.
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
			// TODO: This can be generalized later; right now the only
			// 'meleeWeapon' is "STR_FIST" - the Universal Fist!!!
//			const RuleItem* const itRule (_rules->getItemRule(action->actor->getUnitRules()->getMeleeWeapon()));
			action->weapon = action->actor->getMeleeWeapon();
		}

		if (action->weapon != nullptr)
			popupTac(new ActionMenuState(
									action,
									_icons->getX(),
									_icons->getY() + 16,
									injured));
	}
}

/**
 * Handles the top BattleState.
 * @note Called by the tactical-timer.
 */
void BattlescapeState::handleState()
{
	_battleGame->handleBattleState();
}

/**
 * Sets the '_timerTactical' interval for think() calls of the state.
 * @param interval - an interval in ms
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_timerTactical->setInterval(interval);
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
	return _playSave;
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
 * Prints a debug-message onScreen.
 * @param wst - reference to a widestring
 */
void BattlescapeState::printDebug(const std::wstring& wst)
{
//	if (_battleSave->getDebugTac() == true)
	_txtDebug->setText(wst);
}

/**
 * Shows a red warning message that fades overlaid on the HUD-icons.
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

	const RuleAlienDeployment* const ruleDeploy (_rules->getDeployment(_battleSave->getTacticalType()));

	std::string nextStage;
	if (ruleDeploy != nullptr)
	{
		switch (_battleSave->getTacType())
		{
			case TCT_BASEASSAULT: // no check for next-stage if Ufo_Crashed or _Landed.
			case TCT_BASEDEFENSE:
			case TCT_TERRORSITE:
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
		_timerAnimate->stop();
		_timerTactical->stop();
		_game->popState();


		bool debrief;
		if (_playSave->getMonthsElapsed() != -1)
		{
			if (ruleDeploy != nullptr)
			{
				if (ruleDeploy->isFinalMission() == true) // must fulfill objectives
				{
					debrief = false;
					if (aborted == true
						|| _battleSave->allObjectivesDestroyed() == false) //&& playerUnits < 1
					{
						_playSave->setEnding(END_LOSE);
						_game->pushState(new DefeatState());
					}
					else //if (_battleSave->allObjectivesDestroyed() == true)
					{
						_playSave->setEnding(END_WIN);
						_game->pushState(new VictoryState());
					}
				}
				else if (ruleDeploy->isNoRetreat() == true)
				{
					if (aborted == true || playerUnits < 1)
					{
						debrief = false;
						_playSave->setEnding(END_LOSE);
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
			ironsave = (_playSave->isIronman() == true);

		if (ironsave == true)
		{
			_playSave->setBattleSave();
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
				&& _playSave->getMonthsElapsed() != -1)
			{
				_playSave->setEnding(END_LOSE);
				ironsave = _playSave->isIronman() == true;
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
				&& _playSave->getMonthsElapsed() != -1)
			{
				_playSave->setEnding(END_WIN);
				ironsave = _playSave->isIronman() == true;
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
				ts = _playSave->getTerrorSites()->begin();
				ts != _playSave->getTerrorSites()->end() && nextStageRace.empty() == true;
				++ts)
		{
			if ((*ts)->getTactical() == true)
				nextStageRace = (*ts)->getAlienRace();
		}
		for (std::vector<AlienBase*>::const_iterator
				ab = _playSave->getAlienBases()->begin();
				ab != _playSave->getAlienBases()->end() && nextStageRace.empty() == true;
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
 * @param show - true to show PSI button
 */
void BattlescapeState::showPsiButton(bool show)
{
	_btnPsi->setVisible(show);
}

/**
 * Clears drag-scrolling.
 * @param doInfo - true to update tile-info under selector (default false)
 */
void BattlescapeState::clearDragScroll(bool doInfo)
{
	_dragScrollActivated = false;

	if (doInfo == true) handleTileInfo();
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
	_map->getCamera()->warp(dX >> 1u, dY >> 1u);

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
 * Updates the turn-text.
 */
void BattlescapeState::updateTurnText()
{
	_txtTurn->setText(tr("STR_TURN").arg(_battleSave->getTurn()));
}

/**
 * Toggles the icons' surfaces' visibility for Hidden Movement.
 * @param vis - true to show show icons and info
 */
void BattlescapeState::toggleIcons(bool vis)
{
	//Log(LOG_INFO) << "bs:toggleIcons() vis= " << vis;
	_iconsHidden = !vis;

	_icons			->setVisible(vis);

	_numLayers		->setVisible(vis);

	_btnUnitUp		->setVisible(vis);
	_btnUnitDown	->setVisible(vis);
	_btnMapUp		->setVisible(vis);
	_btnMapDown		->setVisible(vis);
	_btnMiniMap		->setVisible(vis);
	_btnKneel		->setVisible(vis);
	_btnInventory	->setVisible(vis);
	_btnCenter		->setVisible(vis);
	_btnNextUnit	->setVisible(vis);
	_btnNextStop	->setVisible(vis);
	_btnShowLayers	->setVisible(vis);
	_btnOptions		->setVisible(vis);
	_btnEndTurn		->setVisible(vis);
	_btnAbort		->setVisible(vis);

	_txtOrder		->setVisible(vis);
	_lstSoldierInfo	->setVisible(vis);
	_srfAlienIcon	->setVisible(vis && allowAlienIcons() == true);

	_showSoldierData = vis;

//	_txtControlDestroyed->setVisible(vis);
	_txtMissionLabel	->setVisible(vis);
	_lstTileInfo		->setVisible(vis);

	_srfOverweight->setVisible(vis && _isOverweight == true);

	for (size_t
			i = 0u;
			i != ICONS_MEDIC;
			++i)
	{
		if (_tileMedic[i] != nullptr)
		{
			_isfMedic[i]->setVisible(vis);
			_numMedic[i]->setVisible(vis);
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
			static Surface* const srfAlien (_game->getResourcePack()->getSurface("AlienIcon"));
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

	if (tile != nullptr && tile->isRevealed() == true)
	{
		static const int BLOCKED = 255;

		size_t rows (3u);

		int tuCost;
		const BattleUnit* const selUnit (_battleSave->getSelectedUnit());
		if (selUnit != nullptr
			&& selUnit->getFaction() == FACTION_PLAYER)
		{
			++rows;

			MoveType mType (selUnit->getMoveTypeUnit());
			if (mType == MT_FLY
				&& selUnit->getGeoscapeSoldier() != nullptr
				&& (SDL_GetModState() & KMOD_ALT) != 0) // forced walk.
			{
				mType = MT_WALK;
			}

			if (tile->getMapData(O_OBJECT) != nullptr)
				tuCost = 4 + tile->getTuCostTile(O_OBJECT, mType);
			else if (tile->isFloored()					// <- do *not* check for a tileBelow yet
				&& tile->getMapData(O_FLOOR) != nullptr)
			{
				tuCost = tile->getTuCostTile(O_FLOOR, mType);
			}
			else
			{
				const Tile* const tileBelow (tile->getTileBelow(_battleSave));
				if (tileBelow != nullptr && tileBelow->getTerrainLevel() == -24)
					tuCost = 4;
				else
				{
					switch (mType)
					{
						case MT_FLOAT: // wft.
						case MT_FLY: tuCost = 4;
							break;

						default: // avoid g++ warning.
						case MT_WALK:
						case MT_SLIDE: tuCost = BLOCKED;
					}
				}
			}
		}
		else
			tuCost = 0;


		const int info[]
		{
			static_cast<int>(tile->isFloored(tile->getTileBelow(_battleSave))),
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
				switch (info[i])
				{
					default:
					case 0:					// NO Floor
						hasFloor = L"-";
						color = ORANGE;
						break;
					case 1:					// Floor
						hasFloor = L"F";
						color = GREEN;
				}

				_lstTileInfo->addRow(
								2,
								hasFloor.c_str(),
								infoType.at(i).c_str());
			}
			else if (i < 3u) // smoke & fire
			{
				switch (i)
				{
					case 1u: color = BROWN_L; break;	// smoke
					case 2u:							// fire
					default: color = RED;
				}

				std::wstring value;
				switch (info[i])
				{
					case 0:  value = L""; break;
					default: value = Text::intWide(info[i]);
				}

				_lstTileInfo->addRow(
								2,
								value.c_str(),
								infoType.at(i).c_str());
			}
			else if (selUnit != nullptr) // tuCost
			{
				color = BLUE;

				std::wstring cost;
				if (info[i] < BLOCKED)	cost = Text::intWide(info[i]);
				else					cost = L"-";

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
 * @note Called from NextTurnState::nextTurn().
 */
void BattlescapeState::requestAutosave()
{
	_autosave = true;
}

/**
 * Saves a first-person voxel-view of the battlefield.
 */
void BattlescapeState::saveVoxelView() // private.
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
//						|| (tile->isRevealed() && (voxelTest == 1 || voxelTest == 4))
//						|| voxelTest == 5)
					if (debugTac == true)
						black = false;
					else
					{
						switch (voxelTest)
						{
							case 1:
							case 4:
								if (tile->isRevealed() == true)
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
	while (i < 1000 && CrossPlatform::fileExists(oststr.str()) == true);

	if (i != 1000)
	{
		unsigned error (lodepng::encode(
									oststr.str(),
									pic,
									512u,512u,
									LCT_RGB));
		if (error != 0u)
			Log(LOG_WARNING) << "bs::saveVoxelView() Saving to PNG failed: " << lodepng_error_text(error);
#ifdef _WIN32
		else
		{
			const std::string& st ("\"C:\\Program Files (x86)\\IrfanView\\i_view32.exe\"");
			if (CrossPlatform::fileExists(st))
			{
				std::wstring wst (Language::fsToWstr(st + " " + oststr.str()));

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
		}
#endif
	}
	else
		Log(LOG_WARNING) << "bs::saveVoxelView() Limit of 1000 fpslook screenshots exceeded. File not saved.";
}

/**
 * Saves each layer of voxels on the battlefield as a png.
 */
void BattlescapeState::saveVoxelMaps() // private.
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
				int voxelTest (static_cast<int>(_battleSave->getTileEngine()->voxelCheck(Position(x,y,z << 1u))) + 1);
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
									static_cast<unsigned>(_battleSave->getMapSizeX() << 4u),
									static_cast<unsigned>(_battleSave->getMapSizeY() << 4u),
									LCT_RGB));
		if (error != 0u)
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap() // private.
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
										0x0000ffu,
										0x00ff00u,
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

			if ((tile = _battleSave->getTile(posTile)) != nullptr && tile->isRevealed() == true)
			{
				rect.x = static_cast<Sint16>(x * static_cast<int>(rect.w));
				rect.y = static_cast<Sint16>(y * static_cast<int>(rect.h));

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
					if (z > 0 && tile->isFloored(_battleSave->getTile(pos)) == true)
						break; // no seeing through floors
				}

				if (   tile->getMapData(O_NORTHWALL) != nullptr
					&& tile->getMapData(O_NORTHWALL)->getTuCostPart(MT_FLY) == 255)
				{
					lineRGBA(
							img,
							rect.x, rect.y,
							static_cast<Sint16>(rect.x + rect.w),
							rect.y,
							0x50u,
							0x50u,
							0x50u,
							255u);
				}

				if (   tile->getMapData(O_WESTWALL) != nullptr
					&& tile->getMapData(O_WESTWALL)->getTuCostPart(MT_FLY) == 255)
				{
					lineRGBA(
							img,
							rect.x, rect.y,
							rect.x,
							static_cast<Sint16>(rect.y + rect.h),
							0x50u,
							0x50u,
							0x50u,
							255u);
				}
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
 * Reserves time units.
 * @param action - pointer to an Action
 *
void BattlescapeState::btnReserveClick(Action* action)
{
	if (allowButtons())
	{
		action->getSender()->mousePress(_game->getFakeMouseActionD(), this);

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
		action->getSender()->mousePress(_game->getFakeMouseActionD(), this);

		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());
//		_btnReserveKneel->invert(_btnReserveKneel->getColor() + 3);
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

}
