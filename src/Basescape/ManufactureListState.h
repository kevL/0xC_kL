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

#ifndef OPENXCOM_MANUFACTURELISTSTATE_H
#define OPENXCOM_MANUFACTURELISTSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleManufacture;

class ComboBox;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that lists possible productions.
 */
class ManufactureListState
	:
		public State
{

private:
	static const std::string ALL_ITEMS;
	static std::string _recallCatString;

	size_t _scroll;

	Base* _base;
	ComboBox* _cbxCategory;
	Text
		* _txtTitle,
		* _txtItem,
		* _txtCategory;
	TextButton
		* _btnCancel,
		* _btnCostTable;
	TextList* _lstManufacture;
	Window* _window;

	std::vector<const RuleManufacture*> _available;
	std::vector<std::string>
		_catStrings,
		_manfStrings;

	/// Fills the list of possible productions.
	void fillProductionList();


	public:
		/// Creates a NewManufactureList state.
		explicit ManufactureListState(Base* const base);
		/// dTor.
		~ManufactureListState();

		/// Initializes state.
		void init() override;

		/// Handler for the Costs button.
		void btnCostsClick(Action* action);
		/// Handler for clicking the OK button.
		void btnCancelClick(Action* action);
		/// Handler for clicking on the list.
		void lstProdClick(Action* action);
		/// Handler for changing the category filter.
		void cbxCategoryChange(Action* action);
};

}

#endif
