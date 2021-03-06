/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#ifndef OPENXCOM_CRAFTINFOSTATE_H
#define OPENXCOM_CRAFTINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Text;
class TextButton;
class TextEdit;
class Timer;
class Surface;
class Window;


/**
 * Craft Info screen that shows all the info about a specific craft.
 */
class CraftInfoState
	:
		public State
{

private:
	static const Uint8
		RED		=  32u, // repairing
		GREEN	=  48u, // ready
		ORANGE	=  96u, // rearming
		YELLOW	= 144u; // refueling

	size_t _craftId;
	std::wstring _defaultName;

	bool _isQuickBattle;

	Base* _base;
	Craft* _craft;
	Surface
		* _sprite,
		* _weapon1,
		* _weapon2,
		* _crew,
		* _equip;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtDamage,
		* _txtFuel,
		* _txtRadar,
		* _txtStatus,
		* _txtW1Ammo,
		* _txtW1Name,
		* _txtW2Ammo,
		* _txtW2Name,
		* _txtKills;
	TextButton
		* _btnArmor,
		* _btnCrew,
		* _btnEquip,
		* _btnInventory,
		* _btnOk,
		* _btnW1,
		* _btnW2;
	TextEdit* _edtCraft;
	Timer* _timerBlink;
	Window* _window;


	public:
		/// Creates a CraftInfo state.
		CraftInfoState(
				Base* const base,
				size_t craftId);
		/// Cleans up the CraftInfo state.
		~CraftInfoState();

		/// Updates the craft-info.
		void init() override;
		/// Runs the blink timer.
		void think() override;
		/// Blinks the status text.
		void blinkStatus();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the weapon-1 button.
		void btnW1Click(Action* action);
		/// Handler for clicking the weapon-2 button.
		void btnW2Click(Action* action);
		/// Handler for clicking the crew button.
		void btnCrewClick(Action* action);
		/// Handler for clicking the equipment button.
		void btnEquipClick(Action* action);
		/// Handler for clicking the armor button.
		void btnArmorClick(Action* action);
		/// Handler for clicking the inventory button.
		void btnInventoryClick(Action* action);

		/// Handler for changing the text on the label-edit.
		void edtCraftChange(Action* action);
};

}

#endif
