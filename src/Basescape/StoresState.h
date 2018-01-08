/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_STORESSTATE_H
#define OPENXCOM_STORESSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * Stores window that displays all the items currently stored at a Base.
 */
class StoresState
	:
		public State
{

private:
	static const Uint8
		RED		=  32u,
		WHITE	= 208u,
		YELLOW	= 213u,
		BLUE	= 218u,
		PURPLE	= 246u;

	Base* _base;
	Text
		* _txtBaseLabel,
		* _txtItem,
		* _txtQuantity,
		* _txtSpaceUsed,
		* _txtTitle,
		* _txtTotal;
	TextButton
		* _btnOk,
		* _btnTransfers;
	TextList* _lstStores;
	Timer* _timerBlink;
	Window* _window;


	public:
		/// Creates a Stores state.
		explicit StoresState(Base* const base);
		/// Cleans up the Stores state.
		~StoresState();

		/// Runs the blink Timer.
		void think() override;
		/// Blinks the message Text.
		void blink();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the in-Transit button.
		void btnIncTransClick(Action* action);
};

}

#endif
