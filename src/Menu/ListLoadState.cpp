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

#include "ListLoadState.h"

//#include <algorithm>	// std::find()
//#include <string>		// std::string
//#include <vector>		// std::vector

#include "ConfirmLoadState.h"
#include "LoadGameState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ListGames screen.
 * @param origin - section that originated this state
 */
ListLoadState::ListLoadState(OptionsOrigin origin)
	:
		ListGamesState(origin, 0u, true)
{
	_txtTitle->setText(tr("STR_SELECT_GAME_TO_LOAD"));

	centerAllSurfaces();
}

/**
 * dTor.
 */
ListLoadState::~ListLoadState()
{}

/**
 * Loads the pressed entry.
 * @param action - pointer to an Action
 */
void ListLoadState::lstSavesPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			kL_curBase = 0u;

			bool fade;
			switch (_origin)
			{
				default:
				case OPT_MENU:
					fade = true;
					break;

				case OPT_GEOSCAPE:
					if (_saves[_lstSaves->getSelectedRow()].mode == SM_BATTLESCAPE)
						fade = true;
					else
						fade = false;
					break;

				case OPT_BATTLESCAPE:
					if (_saves[_lstSaves->getSelectedRow()].mode == SM_GEOSCAPE)
						fade = true;
					else
						fade = false;
			}
			if (fade == true)
				_game->getResourcePack()->fadeMusic(_game, 1123);

			bool confirmLoad (false);
			for (std::vector<std::string>::const_iterator
					i = _saves[_lstSaves->getSelectedRow()].rulesets.begin();
					i != _saves[_lstSaves->getSelectedRow()].rulesets.end();
					++i)
			{
				if (std::find(
							Options::rulesets.begin(),
							Options::rulesets.end(),
							*i) == Options::rulesets.end())
				{
					confirmLoad = true;
					break;
				}
			}

			if (confirmLoad == false)
				_game->pushState(new LoadGameState(
												_origin,
												_saves[_lstSaves->getSelectedRow()].file,
												_palette,
												this));
			else
				_game->pushState(new ConfirmLoadState(
												_origin,
												_saves[_lstSaves->getSelectedRow()].file,
												this));
			break;
		}

		case SDL_BUTTON_RIGHT:
			ListGamesState::lstSavesPress(action); // -> delete file
	}
}

/**
 * Hides textual elements of this state.
 * @note Called from LoadGameState, possibly through ConfirmLoadState.
 * @param hide - true to hide elements (default true)
 */
void ListLoadState::hideElements(bool hide)
{
	const bool vis (!hide);

	_txtTitle->setVisible(vis);
	_txtDelete->setVisible(vis);
	_txtName->setVisible(vis);
	_txtDate->setVisible(vis);
	_sortName->setVisible(vis);
	_sortDate->setVisible(vis);
	_lstSaves->setVisible(vis);
	_txtDetails->setVisible(vis);
	_btnCancel->setVisible(vis);
}

}
