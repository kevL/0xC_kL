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

#include "OptionsBaseState.h"

//#include <SDL/SDL.h>

#include "MainMenuState.h"
#include "OptionsAdvancedState.h"
#include "OptionsAudioState.h"
#include "OptionsBattlescapeState.h"
#include "OptionsConfirmState.h"
#include "OptionsControlsState.h"
#include "OptionsDefaultsState.h"
#include "OptionsGeoscapeState.h"
#include "OptionsModsState.h"
#include "OptionsNoAudioState.h"
#include "OptionsVideoState.h"
#include "StartState.h"

#include "../Battlescape/BattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Window.h"
#include "../Interface/TextButton.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Options window.
 * @param origin - game section that originated this state
 */
OptionsBaseState::OptionsBaseState(OptionsOrigin origin)
	:
		_origin(origin)
{
	_window			= new Window(this);

	_btnVideo		= new TextButton(80, 16, 8,   8);
	_btnAudio		= new TextButton(80, 16, 8,  28);
	_btnControls	= new TextButton(80, 16, 8,  48);
	_btnGeoscape	= new TextButton(80, 16, 8,  68);
	_btnBattlescape	= new TextButton(80, 16, 8,  88);
	_btnAdvanced	= new TextButton(80, 16, 8, 108);
	_btnMods		= new TextButton(80, 16, 8, 128);

	_btnOk			= new TextButton(100, 16,   8, 176);
	_btnCancel		= new TextButton(100, 16, 110, 176);
	_btnDefault		= new TextButton(100, 16, 212, 176);

	_txtTooltip		= new Text(305, 25, 8, 148);

	setInterface(
			"optionsMenu",
			false,
			_origin == OPT_BATTLESCAPE);

	add(_window,			"window",	"optionsMenu");

	add(_btnVideo,			"button",	"optionsMenu");
	add(_btnAudio,			"button",	"optionsMenu");
	add(_btnControls,		"button",	"optionsMenu");
	add(_btnGeoscape,		"button",	"optionsMenu");
	add(_btnBattlescape,	"button",	"optionsMenu");
	add(_btnAdvanced,		"button",	"optionsMenu");
	add(_btnMods,			"button",	"optionsMenu");

	add(_btnOk,				"button",	"optionsMenu");
	add(_btnCancel,			"button",	"optionsMenu");
	add(_btnDefault,		"button",	"optionsMenu");

	add(_txtTooltip,		"tooltip",	"optionsMenu");

	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnVideo->setText(tr("STR_VIDEO"));
	_btnVideo->onMousePress(static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
							SDL_BUTTON_LEFT);

	_btnAudio->setText(tr("STR_AUDIO"));
	_btnAudio->onMousePress(static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
							SDL_BUTTON_LEFT);

	_btnControls->setText(tr("STR_CONTROLS"));
	_btnControls->onMousePress(	static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
								SDL_BUTTON_LEFT);

	_btnGeoscape->setText(tr("STR_GEOSCAPE_UC"));
	_btnGeoscape->onMousePress(	static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
								SDL_BUTTON_LEFT);

	_btnBattlescape->setText(tr("STR_BATTLESCAPE_UC"));
	_btnBattlescape->onMousePress(	static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
									SDL_BUTTON_LEFT);

	_btnAdvanced->setText(tr("STR_ADVANCED"));
	_btnAdvanced->onMousePress(	static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
								SDL_BUTTON_LEFT);

	_btnMods->setText(tr("STR_MODS"));
	_btnMods->onMousePress(	static_cast<ActionHandler>(&OptionsBaseState::btnGroupPress),
							SDL_BUTTON_LEFT);
	_btnMods->setVisible(_origin == OPT_MAIN_START); // Mods require a restart, don't enable them in-game

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&OptionsBaseState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&OptionsBaseState::btnOkClick),
							Options::keyOk);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&OptionsBaseState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&OptionsBaseState::btnCancelClick),
								Options::keyCancel);

	_btnDefault->setText(tr("STR_RESTORE_DEFAULTS"));
	_btnDefault->onMouseClick(static_cast<ActionHandler>(&OptionsBaseState::btnDefaultClick));

	_txtTooltip->setWordWrap();
}

/**
 * dTor.
 */
OptionsBaseState::~OptionsBaseState()
{}

/**
 *
 * @param origin -
 */
void OptionsBaseState::restart(OptionsOrigin origin)
{
	switch (origin)
	{
		case OPT_MAIN_START:
			_game->setState(new MainMenuState());
			break;

		case OPT_GEOSCAPE:
			_game->setState(new GeoscapeState());
			break;

		case OPT_BATTLESCAPE:
		{
			_game->setState(new GeoscapeState());

			BattlescapeState* const battleState (new BattlescapeState());
			_game->pushState(battleState);
			_game->getSavedGame()->getBattleSave()->setBattleState(battleState);
		}
	}
}

/**
 * Initializes UI colors according to origin.
 */
void OptionsBaseState::init()
{
	State::init();

	if (_origin == OPT_BATTLESCAPE)
		applyBattlescapeColors();
}

/**
 * Handles the pressed-button state for the category-buttons.
 * @param button Button to press.
 */
void OptionsBaseState::setCategory(TextButton* const button)
{
	_group = button;

	_btnVideo->setGroup(&_group);
	_btnAudio->setGroup(&_group);
	_btnControls->setGroup(&_group);
	_btnGeoscape->setGroup(&_group);
	_btnBattlescape->setGroup(&_group);
	_btnAdvanced->setGroup(&_group);
	_btnMods->setGroup(&_group);
}

/**
 * Saves the new options and returns to the proper origin-screen.
 * @param action - pointer to an Action
 */
void OptionsBaseState::btnOkClick(Action*)
{
	int
		dX (Options::baseXResolution),
		dY (Options::baseYResolution);

	Screen::updateScale(
					Options::battlescapeScale,
					Options::safeBattlescapeScale,
					Options::baseXBattlescape,
					Options::baseYBattlescape,
					_origin == OPT_BATTLESCAPE);
	Screen::updateScale(
					Options::geoscapeScale,
					Options::safeGeoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					_origin != OPT_BATTLESCAPE);

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;
	recenter(dX, dY);

	Options::switchDisplay();
	Options::save();

	_game->loadLanguage(Options::language);

	SDL_WM_GrabInput(Options::captureMouse);

	_game->getScreen()->resetDisplay();
	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);

	if (Options::reload && _origin == OPT_MAIN_START)
		_game->setState(new StartState());
	else if (  Options::displayWidth	!= Options::safeDisplayWidth // confirm any video-option changes ->
			|| Options::displayHeight	!= Options::safeDisplayHeight
			|| Options::useScaleFilter	!= Options::safeScaleFilter
			|| Options::useHQXFilter	!= Options::safeHQXFilter
			|| Options::useXBRZFilter	!= Options::safeXBRZFilter // kL
			|| Options::useOpenGL		!= Options::safeOpenGL
			|| Options::openGLShader	!= Options::safeOpenGLShader)
	{
		_game->pushState(new OptionsConfirmState(_origin));
	}
	else
		restart(_origin);
}

/**
 * Loads previous options and returns to the previous screen.
 * @param action - pointer to an Action
 */
void OptionsBaseState::btnCancelClick(Action*)
{
	Options::reload = false;
	Options::load();

	SDL_WM_GrabInput(Options::captureMouse);

	Screen::updateScale(
					Options::safeBattlescapeScale,
					Options::battlescapeScale,
					Options::baseXBattlescape,
					Options::baseYBattlescape,
					_origin == OPT_BATTLESCAPE);
	Screen::updateScale(
					Options::safeGeoscapeScale,
					Options::geoscapeScale,
					Options::baseXGeoscape,
					Options::baseYGeoscape,
					_origin != OPT_BATTLESCAPE);

	_game->setVolume(
				Options::volMusic,
				Options::volFx,
				Options::volUi);

	_game->popState();
}

/**
 * Restores the Options to default-settings.
 * @param action - pointer to an Action
 */
void OptionsBaseState::btnDefaultClick(Action*)
{
	_game->pushState(new OptionsDefaultsState(_origin, this));
}

/**
 *
 * @param action - pointer to an Action
 */
void OptionsBaseState::btnGroupPress(Action* action)
{
	const Surface* const sender (action->getSender());
//	if (sender != _group)
//	{
	_game->popState();

	if (sender == _btnVideo)
		_game->pushState(new OptionsVideoState(_origin));
	else if (sender == _btnAudio)
	{
		if (Options::mute == false)
			_game->pushState(new OptionsAudioState(_origin));
		else
			_game->pushState(new OptionsNoAudioState(_origin));
	}
	else if (sender == _btnControls)
		_game->pushState(new OptionsControlsState(_origin));
	else if (sender == _btnGeoscape)
		_game->pushState(new OptionsGeoscapeState(_origin));
	else if (sender == _btnBattlescape)
		_game->pushState(new OptionsBattlescapeState(_origin));
	else if (sender == _btnAdvanced)
		_game->pushState(new OptionsAdvancedState(_origin));
	else if (sender == _btnMods)
		_game->pushState(new OptionsModsState(_origin));
//	}
}

/**
 * Shows a tooltip for the appropriate button.
 * @param action Pointer to an action.
 *
void OptionsBaseState::txtTooltipIn(Action* action)
{
	_currentTooltip = action->getSender()->getTooltip();
	_txtTooltip->setText(tr(_currentTooltip));
} */

/**
 * Clears the tooltip text.
 * @param action Pointer to an action.
 *
void OptionsBaseState::txtTooltipOut(Action* action)
{
	if (_currentTooltip == action->getSender()->getTooltip())
		_txtTooltip->setText(L"");
} */

/**
 * Updates the scale.
 * @param dX - x-delta
 * @param dY - y-delta
 */
void OptionsBaseState::resize(
		int& dX,
		int& dY)
{
	Options::safeDisplayWidth = Options::displayWidth;
	Options::safeDisplayHeight = Options::displayHeight;

	State::resize(dX, dY);
}

}
