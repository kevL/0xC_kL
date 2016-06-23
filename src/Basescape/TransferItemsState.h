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

#ifndef OPENXCOM_TRANSFERITEMSSTATE_H
#define OPENXCOM_TRANSFERITEMSSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Savegame/Transfer.h"


namespace OpenXcom
{

class Base;
class Craft;
class Soldier;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Transfer screen that lets the player pick what items to transfer between bases.
 */
class TransferItemsState
	:
		public State
{

private:
	static const Uint8
		WHITE	= 208u,
		YELLOW	= 213u;

	bool _resetAll;
	int
		_qtyAlien,
		_qtyCraft,
		_qtyPersonnel,
		_costTotal;
	size_t
		_hasEng,
		_hasSci,
		_sel;
	Uint8 _colorAmmo;
	double
		_distance,
		_storeSize;

	Base
		* _baseSource,
		* _baseTarget;
	Text
		* _txtQtyTarget,
//		* _txtTransferQty,
		* _txtBaseSource,
		* _txtBaseTarget,
		* _txtItem,
		* _txtQuantity,
		* _txtSpaceSource,
		* _txtSpaceTarget,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstItems;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::wstring _error;

	std::vector<int>
		_baseQty,
		_destQty,
		_transferQty;
	std::vector<std::string> _items;
	std::vector<Craft*> _crafts;
	std::vector<Soldier*> _soldiers;

	/// Gets distance between bases.
	double getDistance() const;

	/// Gets cost of selected.
	int getCost() const;
	/// Gets quantity of selected at source Base.
	int getSourceQuantity() const;

	/// Gets type of selected.
	PurchaseSellTransferType getTransferType(size_t sel) const;
	/// Gets index of selected item-type.
	size_t getItemIndex(size_t sel) const;
	/// Gets the index of the selected craft.
	size_t getCraftIndex(size_t sel) const;

	/// Updates the quantity-strings of selected row.
	void updateListrow();


	public:
		/// Creates the Transfer Items state.
		TransferItemsState(
				Base* const baseSource,
				Base* const baseTarget);
		/// Cleans up the Transfer Items state.
		~TransferItemsState();

		/// Initializes the Transfer menu, when cancelling TransferConfirmState.
		void init() override;

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Completes the transfer between bases.
		void completeTransfer();

		/// Handler for pressing an Increase arrow in the list.
		void lstLeftArrowPress(Action* action);
		/// Handler for releasing an Increase arrow in the list.
		void lstLeftArrowRelease(Action* action);
		/// Handler for pressing a Decrease arrow in the list.
		void lstRightArrowPress(Action* action);
		/// Handler for releasing a Decrease arrow in the list.
		void lstRightArrowRelease(Action* action);

		/// Runs the timers.
		void think() override;

		/// Increases the quantity of an item by one.
		void onIncrease();
		/// Increases the quantity of an item by the given value.
		void increaseByValue(int delta);
		/// Decreases the quantity of an item by one.
		void onDecrease();
		/// Decreases the quantity of an item by the given value.
		void decreaseByValue(int delta);

		/// Gets the total cost of the transfer.
		int getTotalCost() const;
};

}

#endif
