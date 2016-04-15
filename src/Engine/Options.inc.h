// This file is used only in conjunction with Options.h/Options.cpp.
//
// To add a new option add a new variable entry and a corresponding OptionInfo
// in Options.cpp.

//#ifndef OPTIONS_INC_H
//#define OPTIONS_INC_H

// General options
OPT int displayWidth, displayHeight,
	baseXResolution, baseYResolution, baseXGeoscape, baseYGeoscape, baseXBattlescape, baseYBattlescape,
	windowedModePositionX, windowedModePositionY,
	soundVolume, musicVolume, uiVolume, audioSampleRate, audioBitDepth, audioBuffer1k,
	pauseMode, autosaveFrequency,
	FPS, FPSUnfocused,
	changeValueByMouseWheel, mousewheelSpeed,
	dragScrollTimeTolerance, dragScrollPixelTolerance;
OPT bool fullscreen, asyncBlit, useScaleFilter, useHQXFilter, useXBRZFilter, useOpenGL, checkOpenGLErrors, vSyncForOpenGL, useOpenGLSmoothing,
	allowResize, borderless, debug, debugUi, fpsCounter, reSeedOnLoad, keepAspectRatio, nonSquarePixelRatio,
	cursorInBlackBandsInFullscreen, cursorInBlackBandsInWindow, cursorInBlackBandsInBorderlessWindow, maximizeInfoScreens,
	playIntro, autosave, musicAlwaysLoop, stereoSound, verboseLogging;
OPT std::string language, openGLShader;
OPT KeyboardType keyboardMode;
OPT SaveSort saveOrder;
OPT MusicFormat preferredMusic;
OPT SoundFormat preferredSound;
OPT SDL_GrabMode captureMouse;
OPT SDLKey keyOk, keyOkKeypad, keyCancel, keyScreenshot, keyFps, keyQuickLoad, keyQuickSave;

// Geoscape options
OPT int geoClockSpeed, dogfightSpeed, geoScrollSpeed, geoDragScrollButton, geoscapeScale;
OPT bool includePrimeStateInSavedLayout, spendResearchedItems, craftLaunchAlways, globeSeasons, globeDetail, globeRadarLines, globeFlightPaths,
	globeAllRadarsOnBaseBuild, canSellLiveAliens, customInitialBase, aggressiveRetaliation, geoDragScrollInvert,
	allowBuildingQueue, showFundsOnGeoscape, fieldPromotions, psiStrengthEval; //allowPsiStrengthImprovement
OPT SDLKey keyGeoLeft, keyGeoRight, keyGeoUp, keyGeoDown, keyGeoZoomIn, keyGeoZoomOut, keyGeoSpeed1, keyGeoSpeed2, keyGeoSpeed3, keyGeoSpeed4, keyGeoSpeed5,
	keyGeoSpeed6, keyGeoIntercept, keyGeoBases, keyGeoGraphs, keyGeoUfopedia, keyGeoOptions, keyGeoFunding, keyGeoToggleDetail, keyGeoToggleRadar,
	keyBaseSelect1, keyBaseSelect2, keyBaseSelect3, keyBaseSelect4, keyBaseSelect5, keyBaseSelect6, keyBaseSelect7, keyBaseSelect8;

// Battlescape options
OPT ScrollType battleEdgeScroll;
OPT PathPreview battlePreviewPath;
OPT int battleScrollSpeed, battleDragScrollButton, battleFireSpeed, battleXcomSpeed, battleAlienSpeed, battleExplosionHeight, battlescapeScale, traceAI;
OPT bool battleInstantGrenade, battleNotifyDeath, battleTooltips, battleHairBleach, battleAutoEnd, //sneakyAI
	battleStrafe, battleForceFire, battleAllowPsionicCapture, battleDragScrollInvert, battleWeaponSelfDestruction,
	battleUFOExtenderAccuracy, battleSmoothCamera, battleTFTDDamage, battleRangeBasedAccuracy, battleAlienPanicMessages, battleAlienBleeding;
OPT SDLKey keyBattleLeft, keyBattleRight, keyBattleUp, keyBattleDown, keyBattleLevelUp, keyBattleLevelDown, keyBattleCenterUnit,
	keyBattlePrevUnit, keyBattleNextUnit, keyBattleDeselectUnit,
	keyBattleUseLeftHand, keyBattleUseRightHand,
	keyBattleInventory, keyBattleMap, keyBattleOptions, keyBattleEndTurn, keyBattleAbort, keyBattleStats, keyBattleKneel, keyBattleZeroTUs,
	keyBattleReload, keyBattlePersonalLighting, keyBattleReserveNone, keyBattleReserveSnap, keyBattleReserveAimed, keyBattleReserveAuto, keyBattleReserveKneel,
	keyBattleCenterEnemy1, keyBattleCenterEnemy2, keyBattleCenterEnemy3, keyBattleCenterEnemy4, keyBattleCenterEnemy5,
	keyBattleCenterEnemy6, keyBattleCenterEnemy7, keyBattleCenterEnemy8, keyBattleCenterEnemy9, keyBattleCenterEnemy10,
	keyBattleVoxelView, keyInvClear, keyBattleConsole;
// keyInvCreateTemplate, keyInvApplyTemplate,

// Flags and other stuff that don't need OptionInfo's.
OPT bool mute, reload, newOpenGL, newScaleFilter, newHQXFilter, newXBRZFilter;
OPT int newDisplayWidth, newDisplayHeight, newBattlescapeScale, newGeoscapeScale;
OPT std::string newOpenGLShader;
OPT std::vector<std::string> rulesets, badMods;
OPT SoundFormat currentSound;

//#endif
