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

#ifndef OPENXCOM_INVENTORY_H
#define OPENXCOM_INVENTORY_H

//#include <map>
//#include <string>

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class Game;
class NumberText;
class RuleInventory;
class Timer;
class WarningMessage;


/**
 * Interactive view of an inventory.
 * @note Lets the player view and manage a soldier's equipment.
 */
class Inventory final
	:
		public InteractiveSurface
{

private:
	bool
		_atBase,
		_tuMode;
	int
		_groundOffset,
		_prime,
		_tuCost;
	size_t _fuseFrame;

	BattleItem
		* _mouseOverItem,
		* _selItem;
	BattleUnit* _selUnit;
	Game* _game;
	NumberText* _stackNumber;
	Surface
		* _srfGrid,
		* _srfItems,
		* _srfGrab;
	Timer* _animTimer;
	WarningMessage* _warning;

	std::map<int, std::map<int,int>> _stackLevel;
	std::vector<std::pair<int,int>> _grenadeFuses;


	/// Draws the inventory grids.
	void drawGrids();
	/// Draws the inventory items.
	void drawItems();
	/// Shows priming warnings on grenades.
	void drawPrimers();

	/// Gets the section in the specified mouse-position.
	RuleInventory* getSlotAtCursor(
			int* x,
			int* y) const;
	/// Moves an item to a specified section.
	void moveItem(
			BattleItem* const item,
			const RuleInventory* const inRule,
			int x = 0,
			int y = 0);
	/// Attempts to place an item in an inventory section.
	bool fitItem(
			const RuleInventory* const inRule,
			BattleItem* const item,
			bool test = false);
	/// Checks if two items can be stacked on one another.
	bool canStack(
			const BattleItem* const itemA,
			const BattleItem* const itemB);


	public:
		/// Creates a new inventory view at the specified position and size.
		Inventory(
				Game* game,
				int width,
				int height,
				int x = 0,
				int y = 0,
				bool atBase = false);
		/// Cleans up the inventory.
		~Inventory();

		/// Sets the inventory's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Draws the inventory.
		void draw() override;
		/// Handles timers.
		void think() override;
		/// Blits the inventory onto another surface.
		void blit(Surface* surface) override;

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse clicks.
		void mouseClick(Action* action, State* state) override;

		/// Sets the inventory's Time Unit mode.
		void setTuMode(bool tu);

		/// Sets the inventory's selected unit.
		void setSelectedUnitInventory(BattleUnit* const unit);

		/// Sets the currently selected item.
		void setSelectedItem(BattleItem* const item = nullptr);
		/// Gets the currently selected item.
		BattleItem* getSelectedItem() const;

		/// Sets the mouse over item.
		void setMouseOverItem(BattleItem* const item);
		/// Gets the mouse over item.
		BattleItem* getMouseOverItem() const;

		/// Arranges items on the ground.
		void arrangeGround(int dir = 0);

		/// Checks for item overlap.
		static bool isOverlap(
				BattleUnit* const unit,
				const BattleItem* const item,
				const RuleInventory* const inRule,
				int x = 0,
				int y = 0);

		/// Unloads the selected weapon.
		bool unload();

		/// Sets grenade to show a warning in Inventory.
		void setPrimeGrenade(int turn);
		/// Shows a warning message.
		void showWarning(const std::wstring& msg);

		/// Gets the TU cost for moving items around.
		int getTuCostInventory() const;
};

}

#endif
