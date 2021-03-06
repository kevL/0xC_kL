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

#include "UfopaediaStartState.h"

#include <cstring>

#include "ArticleStateItem.h" // reset FirearmInfo.
#include "UfopaediaSelectState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"


namespace OpenXcom
{

const std::string UfopaediaStartState::ped_TITLES[ped_SECTIONS]
{
	UFOPAEDIA_XCOM_CRAFT_ARMAMENT,
	UFOPAEDIA_HEAVY_WEAPONS_PLATFORMS,
	UFOPAEDIA_WEAPONS_AND_EQUIPMENT,
	UFOPAEDIA_ALIEN_ARTIFACTS,
	UFOPAEDIA_BASE_FACILITIES,
	UFOPAEDIA_ALIEN_LIFE_FORMS,
	UFOPAEDIA_ALIEN_RESEARCH,
	UFOPAEDIA_UFO_COMPONENTS,
	UFOPAEDIA_UFOS,
	UFOPAEDIA_AWARDS
};


/**
 * Constructs a UfopaediaStartState that lists all sections/topics.
 * @param tactical - true if opening UfoPaedia from battlescape (default false)
 */
UfopaediaStartState::UfopaediaStartState(bool tactical)
	:
		_tactical(tactical)
{
	int offset_x; // x - 32 to center on Globe
	if (_tactical == false)
	{
		_fullScreen = false;
//		if (Options::baseXResolution > 320 + 32)
		offset_x = -32;
	}
	else // in Battlescape ->
	{
		offset_x = 0;
		_game->getScreen()->fadeScreen();
	}

	_window = new Window( // NOTE: this is almost too tall for 320x200.
					this,
					256,194,
					offset_x + 32, 6,
					POPUP_BOTH,
					_tactical == false);
	_txtTitle = new Text(
					224,17,
					offset_x + 48, 16);

	setInterface("ufopaedia");

	add(_window,	"window",	"ufopaedia");
	add(_txtTitle,	"text",		"ufopaedia");

	int offset_y (37);
	for (size_t
			i = 0u;
			i != ped_SECTIONS;
			++i)
	{
		_btnSection[i] = new TextButton(
									224,12,
									offset_x + 48,
									offset_y);
		add(_btnSection[i], "button1", "ufopaedia");
		_btnSection[i]->setText(tr(ped_TITLES[i]));
		_btnSection[i]->onMouseClick(static_cast<ActionHandler>(&UfopaediaStartState::btnSectionClick));

		offset_y += 13;
	}

	_btnOk = new TextButton(
						112,16,
						offset_x + 104,
						offset_y + 4);
	add(_btnOk, "button1", "ufopaedia");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"), offset_x);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_UFOPAEDIA"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&UfopaediaStartState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&UfopaediaStartState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&UfopaediaStartState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&UfopaediaStartState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&UfopaediaStartState::btnOkClick),
							Options::keyGeoUfopedia);
}

/**
 * dTor.
 */
UfopaediaStartState::~UfopaediaStartState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void UfopaediaStartState::btnOkClick(Action*)
{
	kL_geoMusicPlaying = false;
	kL_geoMusicReturnState = true;

	ArticleStateItem::resetFirearmInfo();

	_game->popState();

	if (_tactical == false)
		_game->getResourcePack()->fadeMusic(_game, 228);
}

/**
 * Displays the list of articles for this section.
 * @param action - pointer to an Action
 */
void UfopaediaStartState::btnSectionClick(Action* action)
{
	for (size_t
			i = 0u;
			i != ped_SECTIONS;
			++i)
	{
		if (action->getSender() == _btnSection[i])
		{
			_game->pushState(new UfopaediaSelectState(
													ped_TITLES[i],
													_tactical == true));
			break;
		}
	}
}

}
