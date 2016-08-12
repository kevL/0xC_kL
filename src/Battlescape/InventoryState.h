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

#ifndef OPENXCOM_INVENTORYSTATE_H
#define OPENXCOM_INVENTORYSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattlescapeButton;
class BattlescapeState;
class BattleItem;
class BattleUnit;
class Inventory;
class NumberText;
class RuleItem;
class SavedBattleGame;
class Surface;
class Text;


/**
 * Screen that displays a Battleunit's inventory.
 */
class InventoryState final
	:
		public State
{

private:
	static const Uint8
		WHITE =  1u,
		RED   = 38u;

/*	std::string _currentTooltip; */
	const bool _tuMode;

	BattlescapeButton
		* _btnGroundL,
		* _btnGroundR,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnRank,
		* _btnUnload;
/*		* _btnCreateTemplate,
		* _btnApplyTemplate,
		* _btnClearInventory; */
	BattlescapeState* _parent;
	BattleUnit* _unit;
	Inventory* _inventoryPanel;
	NumberText
		* _battleOrder,
		* _tuCost,
		* _numHead,
		* _numTorso,
		* _numRightArm,
		* _numLeftArm,
		* _numRightLeg,
		* _numLeftLeg,
		* _numFire;
	SavedBattleGame* _battleSave;
	Surface
		* _srfBg,
		* _srfGender,
		* _srfLoad,
		* _srfRagdoll;
	Text
		* _txtName,
		* _txtItem,
		* _txtAmmo,
		* _txtWeight,
		* _txtTUs,
		* _txtFAcc,
		* _txtReact,
		* _txtThrow,
		* _txtMelee,
		* _txtPSkill,
		* _txtPStr,
		* _txtUseTU,
		* _txtThrowTU,
		* _txtPsiTU;
//	Timer* _timer;

/*	std::vector<SoldierLayout*> _curInventoryTemplate; */

	/// Advances to the next/previous Unit when right/left key is depressed.
//	void keyRepeat(); // <- too twitchy.

	/// Updates the current unit's info - weight, TU, etc.
	void updateStats();
	/// Shows woundage values.
	void updateWounds();

	/// Saves all Soldiers' equipment-layouts.
	bool saveAllLayouts() const;
	/// Saves a Soldier's equipment-layout.
	bool saveLayout(BattleUnit* const unit) const;

/*	/// Clears current unit's inventory. (was static)
	void clearInventory(Game* game, std::vector<BattleItem*>* unitInv, Tile* groundTile); */

	/// Sets the extra-info fields on mouseover and mouseclicks.
	void setExtraInfo(const BattleItem* const selOver);
	/// Update the visibility and icons for the template buttons.
/*	void _updateTemplateButtons(bool isVisible); */


	public:
		/// Creates an Inventory state.
		InventoryState(
				bool tuMode = false,
				BattlescapeState* const parent = nullptr);
		/// Cleans up the Inventory state.
		~InventoryState();

		/// Updates all unit-info.
		void init() override;
		/// Runs the timer.
//		void think();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action = nullptr);

		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);

		/// Handler for clicking the Unload button.
		void btnLoadIconClick(Action* action);
		/// Handler for right-clicking the Unload button.
		void btnSaveLayouts(Action* action);

		/// Handler for mouse-wheel to shift ground-items.
		void btnGroundPress(Action* action);
		/// Handler for left-clicking either Ground button.
		void btnGroundClick(Action* action);
		/// Handler for right-clicking the rightside Ground button.
		void btnClearUnitClick(Action* action);
		/// Handler for right-clicking the leftside Ground button.
		void btnClearGroundClick(Action* action);

		/// Handler for clicking the Rank button.
		void btnRankClick(Action* action);

		/// Handler for clicking on inventory items.
		void inClick(Action* action);
		/// Handler for showing item info.
		void inMouseOver(Action* action);
		/// Handler for hiding item info.
		void inMouseOut(Action* action);

		/// Handles keypresses.
		void handle(Action* action) override;

/*		/// Handler for showing tooltip.
		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
		void txtTooltipOut(Action* action); */
/*		/// Handler for clicking on the Create Template button.
		void btnCreateTemplateClick(Action* action);
		/// Handler for clicking the Apply Template button.
		void btnApplyTemplateClick(Action* action); */
};

}

#endif
