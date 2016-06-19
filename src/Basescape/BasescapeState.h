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

#ifndef OPENXCOM_BASESCAPESTATE_H
#define OPENXCOM_BASESCAPESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class BaseView;
class Globe;
class MiniBaseView;
class Text;
class TextButton;
class TextEdit;


/**
 * Basescape screen that shows a Base's layout and lets the player manage
 * his/her Bases.
 */
class BasescapeState final
	:
		public State
{

private:
	bool _allowStoresWarning;

	Base* _base;
	BaseView* _baseLayout;
	Globe* _globe;
	MiniBaseView* _miniBases;
	Text
		* _txtFacility,
		* _txtFunds,
		* _txtRegion;
	TextEdit* _edtBase;
	TextButton
		* _btnAliens,
		* _btnBaseInfo,
		* _btnCrafts,
		* _btnDaMatrix,
		* _btnFacilities,
		* _btnGeoscape,
		* _btnIncTrans,
		* _btnManufacture,
		* _btnMemorial,
		* _btnPurchase,
		* _btnResearch,
		* _btnSell,
		* _btnSoldiers,
		* _btnStores,
		* _btnTransfer;
//		* _btnNewBase,

	std::vector<Base*>* _baseList;


	public:
		/// Creates the Basescape state.
		BasescapeState(
				Base* const base,
				Globe* const globe);
		/// Cleans up the Basescape state.
		~BasescapeState();

		/// Updates the base stats.
		void init() override;
		/// Sets a new base to display.
		void setBase(Base* const base);

		/// Handler for clicking the Base Information button.
		void btnBaseInfoClick(Action* action);
		/// Handler for clicking the Stores button.
		void btnStoresClick(Action* action);
		/// Handler for clicking the Soldiers button.
		void btnSoldiersClick(Action* action);
		/// Handler for clicking the Memorial button.
		void btnMemorialClick(Action* action);
		/// Handler for clicking the Equip Craft button.
		void btnCraftsClick(Action* action);
		/// Handler for clicking the Alien Containment button.
		void btnAliens(Action* action);
		/// Handler for clicking the Research button.
		void btnResearchClick(Action* action);
		/// Handler for clicking the Manufacture button.
		void btnManufactureClick(Action* action);
		/// Handler for clicking the Purchase/Hire button.
		void btnPurchaseClick(Action* action);
		/// Handler for clicking the Sell/Sack button.
		void btnSellClick(Action* action);
		/// Handler for clicking the Matrix button.
		void btnMatrixClick(Action* action);
		/// Handler for clicking the Transfer button.
		void btnTransferClick(Action* action);
		/// Handler for clicking the in-Transit button.
		void btnIncTransClick(Action* action);
		/// Handler for clicking the Build Facilities button.
		void btnFacilitiesClick(Action* action);
		/// Handler for clicking the Geoscape button.
		void btnGeoscapeClick(Action* action);
		/// Handler for clicking the Build New Base button.
//		void btnNewBaseClick(Action* action);

		/// Handler for left-clicking the BaseView.
		void layoutLeftClick(Action* action);
		/// Handler for right-clicking the BaseView.
		void layoutRightClick(Action* action);
		/// Handler for hovering the BaseView & MiniBaseView.
		void layoutMouseOver(Action* action);
		/// Handler for hovering out of the BaseView & MiniBaseView.
		void layoutMouseOut(Action* action);
		/// Handler for left-clicking the MiniBaseView.
		void basesLeftClick(Action* action);
		/// Handler for right-clicking the MiniBaseView.
		void basesRightClick(Action* action);
		/// Handler for pressing a Base selection hotkey.
		void handleKeyPress(Action* action);
		/// Gets the baseId for a keypress.
		size_t getKeyedBaseId(SDLKey keyId) const;

		/// Handler for changing the text in the base-name editor.
		void edtLabelChange(Action* action);

		/// Allows the storage-overfull warning to be displayed to the player.
		void resetStoresWarning();
};

}

#endif
