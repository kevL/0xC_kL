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
 * @param origin - section that originated this state (OptionsBaseState.h)
 */
ListLoadState::ListLoadState(OptionsOrigin origin)
	:
		ListGamesState(origin, 0u, true)
{
	_txtTitle->setText(tr("STR_SELECT_GAME_TO_LOAD"));

	centerSurfaces();
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
void ListLoadState::lstPress(Action* action)
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
				case OPT_MAIN_START:
					fade = true;
					break;

				case OPT_GEOSCAPE:
					switch (_info[_lstSaves->getSelectedRow()].mode)
					{
						case SM_BATTLESCAPE:
							fade = true;
							break;
						default:
							fade = false;
					}
					break;

				case OPT_BATTLESCAPE:
					switch (_info[_lstSaves->getSelectedRow()].mode)
					{
						case SM_GEOSCAPE:
							fade = true;
							break;
						default:
							fade = false;
					}
			}
			if (fade == true)
				_game->getResourcePack()->fadeMusic(_game, 1123);

			bool verified (true);
			for (std::vector<std::string>::const_iterator
					i  = _info[_lstSaves->getSelectedRow()].rulesets.begin(),
					j  = _info[_lstSaves->getSelectedRow()].rulesets.end();
					i != j;
					++i)
			{
				if (std::find(
							Options::rulesets.begin(),
							Options::rulesets.end(),
							*i) == Options::rulesets.end())
				{
					verified = false;
					break;
				}
			}

			if (verified == true)
			{
				hideList();
				_game->pushState(new LoadGameState(
												_origin,
												_info[_lstSaves->getSelectedRow()].file,
												_palette,
												this));
			}
			else
				_game->pushState(new ConfirmLoadState(
												_origin,
												_info[_lstSaves->getSelectedRow()].file,
												this));
			break;
		}

		case SDL_BUTTON_RIGHT:
			ListGamesState::lstPress(action); // -> delete file
	}
}

}
