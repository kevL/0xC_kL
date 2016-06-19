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

#ifndef OPENXCOM_ALIENCONTAINMENTSTATE_H
#define OPENXCOM_ALIENCONTAINMENTSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Menu/OptionsBaseState.h"


namespace OpenXcom
{

class Base;
class BasescapeState;
class MiniBaseView;

class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * AlienContainment screen that lets the player manage aLiens at a particular
 * Base.
 */
class AlienContainmentState
	:
		public State
{

private:
	static const Uint8 YELLOW = 213u;

	int
		_fishFood,
		_totalSpace,
		_usedSpace;
	size_t _sel;

	OptionsOrigin _origin;

	Base* _base;
	BasescapeState* _state;
	MiniBaseView* _mini;
	Text
		* _txtBaseLabel,
		* _txtDeadAliens,
		* _txtHoverBase,
		* _txtResearch,
		* _txtInResearch,
		* _txtItem,
		* _txtLiveAliens,
		* _txtSpace,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstAliens;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::vector<int> _qty;
	std::vector<std::string> _aliens;

	std::vector<Base*>* _baseList;

	/// Gets selected quantity.
	int getQuantity();
	/// Updates the quantity-strings of the selected alien.
	void update();


	public:
		/// Creates an AlienContainment state.
		AlienContainmentState(
				Base* const base,
				OptionsOrigin origin,
				BasescapeState* const state = nullptr);
		/// Cleans up the AlienContainment state.
		~AlienContainmentState();

		/// Updates the list of Live aLiens.
		void init() override;
		/// Runs the Timers.
		void think() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for pressing a Decrease arrow in the list.
		void lstRightArrowPress(Action* action);
		/// Handler for releasing a Decrease arrow in the list.
		void lstRightArrowRelease(Action* action);
		/// Handler for clicking a Decrease arrow in the list.
		void lstRightArrowClick(Action* action);
		/// Handler for pressing an Increase arrow in the list.
		void lstLeftArrowPress(Action* action);
		/// Handler for releasing an Increase arrow in the list.
		void lstLeftArrowRelease(Action* action);
		/// Handler for clicking an Increase arrow in the list.
		void lstLeftArrowClick(Action* action);

		/// Increases the quantity of an aLien by one.
		void increase();
		/// Increases the quantity of an aLien by the given value.
		void increaseByValue(int change);
		/// Decreases the quantity of an aLien by one.
		void decrease();
		/// Decreases the quantity of an aLien by the given value.
		void decreaseByValue(int change);

		/// Handler for clicking the MiniBase view.
		void miniClick(Action* action);
		/// Handler for hovering the MiniBase view.
		void miniMouseOver(Action* action);
		/// Handler for hovering out of the MiniBase view.
		void miniMouseOut(Action* action);
};

}

#endif
