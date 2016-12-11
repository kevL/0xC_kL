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
 * Screen that lists available Manufacture projects.
 */
class ManufactureListState
	:
		public State
{

private:
	static const Uint8 GREEN = 58u;

	static const std::string ALL_ITEMS;
	static std::string _recallCategory;

	size_t _recall;

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

	std::vector<const RuleManufacture*> _unlocked;
	std::vector<std::string>
		_categoryTypes,
		_unlockedTypes;

	/// Handler for clicking on the ManufactureProject list.
	void lstStartClick(Action* action);
	/// Handler for changing the manufacture-category filter.
	void cbxCategoryChange(Action* action);

	/// Fills the list with available Manufacture.
	void fillProjectList();


	public:
		/// Creates a ManufactureList state.
		explicit ManufactureListState(Base* const base);
		/// dTor.
		~ManufactureListState();

		/// Initializes state.
		void init() override;

		/// Handler for the Costs button.
		void btnCostsClick(Action* action);
		/// Handler for clicking the Ok button.
		void btnCancelClick(Action* action);
};

}

#endif
