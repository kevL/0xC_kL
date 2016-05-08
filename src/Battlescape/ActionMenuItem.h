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

#ifndef OPENXCOM_ACTIONMENUITEM_H
#define OPENXCOM_ACTIONMENUITEM_H

#include "BattlescapeGame.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Frame;
class Text;


/**
 * A class that represents a single box in the ActionMenu popup.
 * @note It shows the possible battle-actions of an item along with the TU-cost
 * and accuracy. Mouse over highlights the action and when clicked the event is
 * sent to the parent-state.
 */
class ActionMenuItem final
	:
		public InteractiveSurface
{

private:
	Uint8 _highlightModifier;

	int _tu;

	BattleActionType _bat;

	Frame* _frame;
	Text
		* _txtAcc,
		* _txtDesc,
		* _txtTU;


	public:
		/// Creates an ActionMenuItem.
		ActionMenuItem(
				int id,
				const Game* const game,
				int x,
				int y);
		/// Cleans up the ActionMenuItem.
		~ActionMenuItem();

		/// Assigns battle-action details to the ActionMenuItem.
		void setAction(
				BattleActionType baType,
				const std::wstring& desc,
				const std::wstring& acu,
				const std::wstring& tu,
				int tuCost);
		/// Gets the ActionMenuItem's assigned BattleActionType.
		BattleActionType getMenuActionType() const;
		/// Gets the ActionMenuItem" assigned TU-cost.
		int getMenuActionTu() const;

		/// Sets the Palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor,
				int ncolors) override;
		/// Draws the ActionMenuItem.
		void draw() override;

		/// Handles a mouse-in event.
		void mouseIn(Action* action, State* state) override;
		/// Handles a mouse-out event.
		void mouseOut(Action* action, State* state) override;
};

}

#endif
