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

#include "TechTreeHelpState.h"

#include "../Savegame/Base.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes all the elements on the UI.
 */
TechTreeHelpState::TechTreeHelpState(const std::string& selTopic)
{
	_fullScreen = false;

	_window		= new Window(this, 230, 140, 45, 30);

	_txtTitle	= new Text(230, 16, 45, 42);
	_txtTopic	= new Text(230,  9, 45, 62);

	_lstAliens	= new TextList(131, 49, 95, 76);

	_btnOk		= new TextButton(206, 16, 57, 145);

	setInterface("allocateResearch");

	add(_window,	"window",	"allocateResearch");
	add(_txtTitle,	"text",		"allocateResearch");
	add(_txtTopic,	"text",		"allocateResearch");
	add(_lstAliens,	"list",		"researchMenu");
	add(_btnOk,		"button2",	"allocateResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_ALIEN_ASSISTANCE"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtTopic->setText(tr(selTopic));
	_txtTopic->setAlign(ALIGN_CENTER);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TechTreeHelpState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeHelpState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeHelpState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeHelpState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TechTreeHelpState::btnOkClick),
							SDLK_h);

	_lstAliens->setColumns(2, 97,32);
	_lstAliens->setBackground(_window);
	_lstAliens->setSelectable();
	_lstAliens->setMargin(10);

	std::wostringstream woststr;

	double help (Base::getSoldierHelp(selTopic));
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_SOLDIER").c_str(),
					woststr.str().c_str());

	help = Base::getNavigatorHelp(selTopic);
	woststr.str(L"");
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_NAVIGATOR").c_str(),
					woststr.str().c_str());

	help = Base::getMedicHelp(selTopic);
	woststr.str(L"");
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_MEDIC").c_str(),
					woststr.str().c_str());

	help = Base::getEngineerHelp(selTopic);
	woststr.str(L"");
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_ENGINEER").c_str(),
					woststr.str().c_str());

	help = Base::getLeaderHelp(selTopic);
	woststr.str(L"");
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_LEADER").c_str(),
					woststr.str().c_str());

	help = Base::getCommanderHelp(selTopic);
	woststr.str(L"");
	woststr << help;
	_lstAliens->addRow(
					2,
					tr("STR_LIVE_COMMANDER").c_str(),
					woststr.str().c_str());
}

/**
 * dTor.
 */
TechTreeHelpState::~TechTreeHelpState()
{}

/**
 *
 *
void TechTreeHelpState::init()
{
	State::init();
} */

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void TechTreeHelpState::btnOkClick(Action*)
{
	_game->popState();
}

}
