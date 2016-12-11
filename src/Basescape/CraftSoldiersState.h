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

#ifndef OPENXCOM_CRAFTSOLDIERSSTATE_H
#define OPENXCOM_CRAFTSOLDIERSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * A select-squad screen that lets the player pick the Soldiers that are
 * assigned to a Craft.
 */
class CraftSoldiersState final
	:
		public State
{

private:
	bool _isQuickBattle;

	Base* _base;
	Craft* _craft;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtCraft,
		* _txtLoad,
		* _txtName,
		* _txtRank,
		* _txtTitle,
		* _txtSpace;
	TextButton
		* _btnInventory,
		* _btnOk,
		* _btnUnload;
	TextList* _lstSoldiers;
	Window* _window;

	/// Decides whether to show extra buttons and tactical-costs.
	void extra() const;


	public:
		/// Creates a CraftSoldiers state.
		CraftSoldiersState(
				Base* base,
				size_t craftId);
		/// Cleans up the CraftSoldiers state.
		~CraftSoldiersState();

		/// Updates the Soldiers list.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Unload button.
		void btnUnloadClick(Action* action);

		/// Handler for clicking the Soldiers list.
		void lstSoldiersPress(Action* action);

		/// Handler for clicking the soldier-reordering button.
		void lstLeftArrowClick(Action* action);
		/// Handler for clicking the soldiers-reordering button.
		void lstRightArrowClick(Action* action);

		/// Handler for clicking the Inventory button.
		void btnInventoryClick(Action* action);
};

}

#endif
