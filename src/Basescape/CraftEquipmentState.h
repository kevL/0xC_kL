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

#ifndef OPENXCOM_CRAFTEQUIPMENTSTATE_H
#define OPENXCOM_CRAFTEQUIPMENTSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Ruleset;

class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Screen that lets the player load equipment onto a Craft.
 */
class CraftEquipmentState
	:
		public State
{

private:
	static const int
		COLOR_ERROR		= 241,
		COLOR_ERROR_BG	= 2;

	bool _isQuickBattle;
	size_t
		_sel,
		_selUnitId;
	Uint8 _ammoColor;

	Base* _base;
	Craft* _craft;
	const Ruleset* _rules;

	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtItem,
		* _txtLoad,
		* _txtStores,
		* _txtCraft,
		* _txtTitle,
		* _txtSpace;
	TextButton
		* _btnClear,
		* _btnInventory,
		* _btnOk;
	TextList* _lstEquipment;
	Timer
		* _timerLeft,
		* _timerRight;
	Window* _window;

	std::wstring _error;

	std::vector<std::string> _items;

	/// Updates quantities of item.
	void updateQuantity();
	/// Sets current cost to send the Craft on a mission.
	void calculateTacticalCost();
	/// Decides whether to show extra buttons - Unload and Inventory.
	void displayExtraButtons() const;


	public:
		/// Creates the Craft Equipment state.
		CraftEquipmentState(
				Base* base,
				size_t craftId);
		/// Cleans up the Craft Equipment state.
		~CraftEquipmentState();

		/// Resets state.
		void init() override;
		/// Runs the timers.
		void think() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for pressing a Move Left arrow in the list.
		void lstLeftArrowPress(Action* action);
		/// Handler for releasing a Move Left arrow in the list.
		void lstLeftArrowRelease(Action* action);
		/// Handler for pressing a Move Right arrow in the list.
		void lstRightArrowPress(Action* action);
		/// Handler for releasing a Move Right arrow in the list.
		void lstRightArrowRelease(Action* action);

		/// Handler for pressing-down a mouse-button in the list.
//		void lstMousePress(Action* action);

		/// Moves an item to the Craft.
		void moveRight();
		/// Moves the given number of items to the Craft.
		void moveRightByValue(int qtyDelta);
		/// Moves an item to the Base.
		void moveLeft();
		/// Moves the given number of items to the Base.
		void moveLeftByValue(int qtyDelta);

		/// Empties the contents of the Craft, moving all of the items back to the Base.
		void btnUnloadCraftClick(Action* action);
		/// Handler for clicking the Inventory button.
		void btnInventoryClick(Action* action);
};

}

#endif
