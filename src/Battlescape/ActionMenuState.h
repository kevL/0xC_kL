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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_ACTIONMENUSTATE_H
#define OPENXCOM_ACTIONMENUSTATE_H

#include "BattlescapeGame.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class ActionMenuItem;


/**
 * A menu-popup that allows player to select a Battlescape action.
 */
class ActionMenuState
	:
		public State
{

private:
	static const size_t MENU_ITEMS = 8u;

	int _actions;

	ActionMenuItem* _menuSelect[MENU_ITEMS];
	BattleAction* _action;


	/// Adds a menu-item for a battle-action.
	void addItem(
			const BattleActionType bat,
			const std::string& desc,
			size_t* id);
	/// Checks if there is a viable execution-target nearby.
	bool canExecuteTarget();


	public:
		/// Creates an ActionMenu state.
		ActionMenuState(
				BattleAction* const action,
				int x,
				int y,
				bool injured);
		/// Cleans up the ActionMenu state.
		~ActionMenuState();

		/// Handler for hardware interrupts.
		void handle(Action* action) override;

		/// Handler for clicking an ActionMenuItem.
		void btnActionMenuClick(Action* action);

		/// Updates the resolution settings - just resized the window.
		void resize(
				int& dX,
				int& dY) override;
};

}

#endif
