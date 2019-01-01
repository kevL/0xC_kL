/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "SoldierMemorialState.h"

//#include <sstream>

#include "SoldierInfoDeadState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/StatisticsState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldierMemorial screen.
 */
SoldierMemorialState::SoldierMemorialState()
{
	_window			= new Window(this);

	_txtTitle		= new Text(310, 17,   5,  9);
	_txtRecruited	= new Text(110,  9,  16, 25);
	_txtLost		= new Text(110,  9, 210, 25);
	_txtName		= new Text(132,  9,  16, 36);
	_txtRank		= new Text( 70,  9, 148, 36);
	_txtDate		= new Text( 86,  9, 218, 36);

	_lstSoldiers	= new TextList(285, 129, 16, 44);

	_btnStatistics	= new TextButton(140, 16,  16, 177);
	_btnOk			= new TextButton(140, 16, 164, 177);

	setInterface("soldierMemorial");

	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_BASE_MEMORIAL, "", 1); // no terrain, 1 loop

	add(_window,		"window",	"soldierMemorial");
	add(_txtTitle,		"text",		"soldierMemorial");
	add(_txtRecruited,	"text",		"soldierMemorial");
	add(_txtLost,		"text",		"soldierMemorial");
	add(_txtName,		"text",		"soldierMemorial");
	add(_txtRank,		"text",		"soldierMemorial");
	add(_txtDate,		"text",		"soldierMemorial");
	add(_lstSoldiers,	"list",		"soldierMemorial");
	add(_btnStatistics,	"button",	"soldierMemorial");
	add(_btnOk,			"button",	"soldierMemorial");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&SoldierMemorialState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierMemorialState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierMemorialState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&SoldierMemorialState::btnOkClick),
							Options::keyCancel);

	_btnStatistics->setText(tr("STR_STATISTICS"));
	_btnStatistics->onMouseClick(	static_cast<ActionHandler>(&SoldierMemorialState::btnStatsClick));
	_btnStatistics->onKeyboardPress(static_cast<ActionHandler>(&SoldierMemorialState::btnStatsClick),
									SDLK_s);

	_txtTitle->setText(tr("STR_MEMORIAL"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtName->setText(tr("STR_NAME_UC"));
	_txtRank->setText(tr("STR_RANK"));
	_txtDate->setText(tr("STR_DATE_DEATH"));

	int agents (static_cast<int>(_game->getSavedGame()->getDeadSoldiers()->size()));
	_txtLost->setText(tr("STR_SOLDIERS_LOST_").arg(agents));

	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		agents += (*i)->getTotalSoldiers();
	}
	_txtRecruited->setText(tr("STR_SOLDIERS_RECRUITED_").arg(agents));

	_lstSoldiers->setColumns(5, 124,70,26,23,33);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress(static_cast<ActionHandler>(&SoldierMemorialState::lstSoldiersPress));

	const SoldierDeath* death;
	for (std::vector<SoldierDead*>::const_reverse_iterator
			rit = _game->getSavedGame()->getDeadSoldiers()->rbegin();
			rit != _game->getSavedGame()->getDeadSoldiers()->rend();
			++rit)
	{
		death = (*rit)->getDeath();
		_lstSoldiers->addRow(
						5,
						(*rit)->getLabel().c_str(),
						tr((*rit)->getRankString()).c_str(),
						death->getTime()->getDayString(_game->getLanguage()).c_str(),
						tr(death->getTime()->getMonthString()).c_str(),
						Text::intWide(death->getTime()->getYear()).c_str());
	}
}

/**
 * dTor.
 */
SoldierMemorialState::~SoldierMemorialState()
{}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierMemorialState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 863);

	_game->popState();
	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
}

/**
 * Opens the Statistics screen.
 * @param action - pointer to an Action
 */
void SoldierMemorialState::btnStatsClick(Action*)
{
	_game->pushState(new StatisticsState);
}

/**
 * Shows the selected SoldierDead's info.
 * @param action - pointer to an Action
 */
void SoldierMemorialState::lstSoldiersPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		{
			const size_t solId (_game->getSavedGame()->getDeadSoldiers()->size()
							  - _lstSoldiers->getSelectedRow() - 1u);
			_game->pushState(new SoldierInfoDeadState(solId));
		}
	}
}

}
