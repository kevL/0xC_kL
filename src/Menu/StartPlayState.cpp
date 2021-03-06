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

#include "StartPlayState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Menu/ListSaveState.h"

#include "../Geoscape/BuildBaseState.h"
#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the StartPlay window.
 */
StartPlayState::StartPlayState()
{
	_window			= new Window(this, 192, 180, 64, 10, POPUP_VERTICAL);

	_txtTitle		= new Text(192, 17, 64, 18);

	_btnBeginner	= new TextButton(160, 18, 80,  35);
	_btnExperienced	= new TextButton(160, 18, 80,  55);
	_btnVeteran		= new TextButton(160, 18, 80,  75);
	_btnGenius		= new TextButton(160, 18, 80,  95);
	_btnSuperhuman	= new TextButton(160, 18, 80, 115);

	_btnIronman		= new ToggleTextButton(78, 18, 80, 139);
	_txtIronman		= new Text(90, 24, 162, 135);

	_btnCancel		= new TextButton(78, 16,  80, 164);
	_btnOk			= new TextButton(78, 16, 162, 164);

	setInterface("newGameMenu");

	add(_window,			"window",	"newGameMenu");
	add(_txtTitle,			"text",		"newGameMenu");
	add(_btnBeginner,		"button",	"newGameMenu");
	add(_btnExperienced,	"button",	"newGameMenu");
	add(_btnVeteran,		"button",	"newGameMenu");
	add(_btnGenius,			"button",	"newGameMenu");
	add(_btnSuperhuman,		"button",	"newGameMenu");
	add(_btnIronman,		"ironman",	"newGameMenu");
	add(_txtIronman,		"ironman",	"newGameMenu");
	add(_btnOk,				"button",	"newGameMenu");
	add(_btnCancel,			"button",	"newGameMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setText(tr("STR_SELECT_DIFFICULTY_LEVEL"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_difficulty = _btnBeginner;

	_btnBeginner->setText(tr("STR_1_BEGINNER"));
	_btnBeginner->setGroup(&_difficulty);

	_btnExperienced->setText(tr("STR_2_EXPERIENCED"));
	_btnExperienced->setGroup(&_difficulty);

	_btnVeteran->setText(tr("STR_3_VETERAN"));
	_btnVeteran->setGroup(&_difficulty);

	_btnGenius->setText(tr("STR_4_GENIUS"));
	_btnGenius->setGroup(&_difficulty);

	_btnSuperhuman->setText(tr("STR_5_SUPERHUMAN"));
	_btnSuperhuman->setGroup(&_difficulty);

	_btnIronman->setText(tr("STR_IRONMAN"));

	_txtIronman->setText(tr("STR_IRONMAN_DESC"));
	_txtIronman->setVerticalAlign(ALIGN_MIDDLE);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&StartPlayState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StartPlayState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&StartPlayState::btnOkClick),
							Options::keyOkKeypad);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&StartPlayState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&StartPlayState::btnCancelClick),
								Options::keyCancel);
}

/**
 * dTor.
 */
StartPlayState::~StartPlayState()
{}

/**
 * Sets up a SavedGame and jumps to the Geoscape.
 * @param action - pointer to an Action
 */
void StartPlayState::btnOkClick(Action*)
{
	DifficultyLevel diff;

	if      (_difficulty == _btnSuperhuman)  diff = DIFF_SUPERHUMAN;
	else if (_difficulty == _btnGenius)      diff = DIFF_GENIUS;
	else if (_difficulty == _btnVeteran)     diff = DIFF_VETERAN;
	else if (_difficulty == _btnExperienced) diff = DIFF_EXPERIENCED;
	else                                     diff = DIFF_BEGINNER;

	bool ironballs = (_btnIronman->getPressed() == true);

	SavedGame* const playSave (_game->getRuleset()->createSave(_game));
	playSave->setDifficulty(diff);
	playSave->setIronman(ironballs);

	// TODO: An ironballs save should be done here.

	GeoscapeState* const geo (new GeoscapeState());
	_game->setState(geo);
	geo->init();

	_game->getResourcePack()->fadeMusic(_game, 1306);
	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);

	_game->pushState(new BuildBaseState(
									playSave->getBases()->back(),
									geo->getGlobe(),
									true));

	if (ironballs)
		geo->popupGeo(new ListSaveState(OPT_GEOSCAPE));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void StartPlayState::btnCancelClick(Action*)
{
	_game->setSavedGame();
	_game->popState();
}

}
