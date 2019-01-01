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

#ifndef OPENXCOM_SELLSTATE_H
#define OPENXCOM_SELLSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

//#include "../Menu/OptionsBaseState.h"

#include "../Savegame/Base.h"


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
 * Sell/Sack screen that lets the player sell any items at a particular Base.
 */
class SellState
	:
		public State
{

private:
	static const Uint8
		WHITE	= 208u,
		YELLOW	= 213u;

	int _costTotal;
	size_t
		_hasSci,
		_hasEng,
		_sel;
	Uint8 _colorAmmo;
	double _storeSize;

//	OptionsOrigin _origin;

	Base* _base;
	Text
		* _txtFunds,
		* _txtBaseLabel,
		* _txtItem,
		* _txtQuantity,
		* _txtSales,
		* _txtSell,
		* _txtStorage,
		* _txtTitle,
		* _txtValue;
	TextButton
		* _btnOk,
		* _btnCancel;
	TextList* _lstItems;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::vector<int> _sellQty;
	std::vector<std::string> _items;
	std::vector<Craft*> _crafts;
	std::vector<Soldier*> _soldiers;

	/// Updates the quantity-strings of the selected item.
	void updateListrow();

	/// Gets selected price.
	int getPrice() const;
	/// Gets selected quantity.
	int getBaseQuantity() const;

	/// Gets the type of the selected item.
	PurchaseSellTransferType getSellType(size_t sel) const;
	/// Gets the index of selected item.
	size_t getItemIndex(size_t sel) const;
	/// Gets the index of the selected craft.
	size_t getCraftIndex(size_t sel) const;


	public:
		/// Creates a Sell state.
		explicit SellState(Base* const base);
//				OptionsOrigin origin = OPT_GEOSCAPE);
		/// Cleans up the Sell state.
		~SellState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for pressing an increase-arrow in the list.
		void lstLeftArrowPress(Action* action);
		/// Handler for releasing an increase-arrow in the list.
		void lstLeftArrowRelease(Action* action);
		/// Handler for pressing a decrease-arrow in the list.
		void lstRightArrowPress(Action *action);
		/// Handler for releasing a decrease-arrow in the list.
		void lstRightArrowRelease(Action* action);

		/// Runs the timers.
		void think() override;

		/// Increases the quantity of an item on Timer tick.
		void onIncrease();
		/// Decreases the quantity of an item on Timer tick.
		void onDecrease();
		/// Changes the quantity of an item by a given value.
		void changeByValue(
				int qtyDelta,
				int dir);
};

}

#endif
