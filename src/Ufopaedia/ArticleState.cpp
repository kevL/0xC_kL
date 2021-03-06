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

#include "ArticleState.h"

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Surface.h"
#include "../Engine/Timer.h"

#include "../Interface/TextButton.h"


namespace OpenXcom
{

/**
 * Constructs an ArticleState.
 * @param article_id - reference to the article id of this instance
 */
ArticleState::ArticleState(const std::string& article_id) // protected.
	:
		_id(article_id)
{
	_bg			= new InteractiveSurface();
	_btnOk		= new TextButton(30, 14,  5, 5);
	_btnPrev	= new TextButton(30, 14, 40, 5);
	_btnNext	= new TextButton(30, 14, 75, 5);

	_timer = new Timer(132u);
	_timer->onTimer(static_cast<StateHandler>(&ArticleState::keyRepeat));
	_timer->start();
}

/**
 * Destructor.
 */
ArticleState::~ArticleState() // virtual/protected.
{
	delete _timer;
}

/**
 * Hits the think Timer.
 */
void ArticleState::think()
{
	State::think();
	_timer->think(this, nullptr);
}

/**
 * Advances to the next/previous Article when right/left key is depressed.
 */
void ArticleState::keyRepeat() // private.
{
	Uint8* keystate (SDL_GetKeyState(nullptr));
	if (keystate[Options::keyGeoRight] == 1 || keystate[SDLK_KP6] == 1)
		btnNextClick(nullptr);
	else if (keystate[Options::keyGeoLeft] == 1 || keystate[SDLK_KP4] == 1)
		btnPrevClick(nullptr);
}

/**
 * Gets damage-type as a string.
 * @param dType - the DamageType (RuleItem.h)
 * @return, type string
 */
std::string ArticleState::getDamageTypeText(DamageType dType) // static.
{
	switch (dType)
	{
		case DT_AP:		return "STR_DAMAGE_ARMOR_PIERCING";
		case DT_IN:		return "STR_DAMAGE_INCENDIARY";
		case DT_HE:		return "STR_DAMAGE_HIGH_EXPLOSIVE";
		case DT_LASER:	return "STR_DAMAGE_LASER_BEAM";
		case DT_PLASMA:	return "STR_DAMAGE_PLASMA_BEAM";
		case DT_STUN:	return "STR_DAMAGE_STUN";
		case DT_MELEE:	return "STR_DAMAGE_MELEE";
		case DT_ACID:	return "STR_DAMAGE_ACID";
		case DT_SMOKE:	return "STR_DAMAGE_SMOKE";
	}
	return "STR_UNKNOWN";
}

/**
 * Set Text and ActionHandlers for derived ui-elements.
 * @param contrast - true to set buttons to high-contrast (default true)
 */
void ArticleState::initLayout(bool contrast) // protected.
{
	add(_bg);
	add(_btnOk);
	add(_btnPrev);
	add(_btnNext);

	_bg->onMouseClick(	static_cast<ActionHandler>(&ArticleState::btnOkClick),
						SDL_BUTTON_RIGHT);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ArticleState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ArticleState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ArticleState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ArticleState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ArticleState::btnOkClick),
							Options::keyGeoUfopedia);

	_btnPrev->setText(L"<");
	_btnPrev->onMouseClick(		static_cast<ActionHandler>(&ArticleState::btnPrevClick));
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&ArticleState::btnPrevClick),
								Options::keyGeoLeft);
	_btnPrev->onKeyboardPress(	static_cast<ActionHandler>(&ArticleState::btnPrevClick),
								SDLK_KP4);

	_btnNext->setText(L">");
	_btnNext->onMouseClick(		static_cast<ActionHandler>(&ArticleState::btnNextClick));
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&ArticleState::btnNextClick),
								Options::keyGeoRight);
	_btnNext->onKeyboardPress(	static_cast<ActionHandler>(&ArticleState::btnNextClick),
								SDLK_KP6);

	if (contrast == true)
	{
		_btnOk	->setHighContrast();
		_btnPrev->setHighContrast();
		_btnNext->setHighContrast();
	}
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ArticleState::btnOkClick(Action*) // protected.
{
	_game->popState();
}

/**
 * Shows the previous available article. Loops to the last.
 * @param action - pointer to an Action
 */
void ArticleState::btnPrevClick(Action*) // protected.
{
	Ufopaedia::prev(_game);
}

/**
 * Shows the next available article. Loops to the first.
 * @param action - pointer to an Action
 */
void ArticleState::btnNextClick(Action*) // protected.
{
	Ufopaedia::next(_game);
}

}
