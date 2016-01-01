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

#ifndef OPENXCOM_PURCHASESTATE_H
#define OPENXCOM_PURCHASESTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Purchase/Hire screen that lets the player buy new items for a Base.
 */
class PurchaseState
	:
		public State
{

private:
	static const Uint8 WHITE = 208;

	int
		_qtyCraft,
		_qtyPersonnel,
		_costTotal;
	size_t _sel;
	Uint8 _colorAmmo;
	double _storeSize;

	Base* _base;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtFunds,
		* _txtItem,
		* _txtPurchases,
		* _txtQuantity,
		* _txtStorage,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstItems;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::vector<int> _orderQty;
	std::vector<std::string>
		_crafts,
		_items,
		_soldiers;

	/// Gets selected price.
	int getPrice();
	/// Updates the quantity-strings of the selected item.
	void updateItemStrings();
	/// Gets the purchase type.
	PurchaseSellTransferType getPurchaseType(size_t sel) const;
	/// Gets the index of selected item.
	size_t getItemIndex(size_t sel) const;
	/// Gets the index of selected craft.
	size_t getCraftIndex(size_t sel) const;


	public:
		/// Creates the Purchase state.
		explicit PurchaseState(Base* const base);
		/// Cleans up the Purchase state.
		~PurchaseState();

		/// Runs the timers.
		void think() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for pressing an Increase arrow in the list.
		void lstItemsLeftArrowPress(Action* action);
		/// Handler for releasing an Increase arrow in the list.
		void lstItemsLeftArrowRelease(Action* action);
		/// Handler for clicking an Increase arrow in the list.
		void lstItemsLeftArrowClick(Action* action);
		/// Handler for pressing a Decrease arrow in the list.
		void lstItemsRightArrowPress(Action* action);
		/// Handler for releasing a Decrease arrow in the list.
		void lstItemsRightArrowRelease(Action* action);
		/// Handler for clicking a Decrease arrow in the list.
		void lstItemsRightArrowClick(Action* action);
		/// Handler for pressing-down a mouse-button in the list.
//		void lstItemsMousePress(Action* action);

		/// Increases the quantity of an item by one.
		void increase();
		/// Increases the quantity of an item by the given value.
		void increaseByValue(int qtyDelta);
		/// Decreases the quantity of an item by one.
		void decrease();
		/// Decreases the quantity of an item by the given value.
		void decreaseByValue(int qtyDelta);
};

}

#endif
