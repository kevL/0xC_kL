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

#ifndef OPENXCOM_TRANSFER_H
#define OPENXCOM_TRANSFER_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "Base.h"


namespace OpenXcom
{

class Base;
class Craft;
class Language;
class Ruleset;
class SavedGame;
class Soldier;


/**
 * Represents a Transfer whether purchased or from Base to Base.
 * @note Items are placed in transit whenever they are purchased or transfered
 * between bases.
 */
class Transfer
{

private:
	bool _delivered;
	int
		_engineers,
		_hours,
		_itemQty,
		_scientists;

	std::string _itemId;

	Craft* _craft;
	Soldier* _soldier;


	public:
		/// Creates a Transfer.
		explicit Transfer(int hours);
		/// Cleans up the Transfer.
		~Transfer();

		/// Loads the Transfer from YAML.
		bool load(
				const YAML::Node& node,
				Base* const base,
				const Ruleset* const rule);
		/// Saves the Transfer to YAML.
		YAML::Node save() const;

		/// Gets the Soldier of the Transfer.
		Soldier* getSoldier() const;
		/// Sets the Soldier of the Transfer.
		void setSoldier(Soldier* const soldier);
		/// Sets the scientists of the Transfer.
		void setScientists(int scientists);
		/// Sets the engineers of the Transfer.
		void setEngineers(int engineers);
		/// Sets the Craft of the Transfer.
		void setCraft(Craft* const craft);
		/// Gets the Craft of the Transfer.
		Craft* getCraft() const;

		/// Gets the item-types of the Transfer.
		std::string getTransferItems() const;
		/// Sets the item-types of the Transfer.
		void setTransferItems(
				const std::string& id,
				int qty = 1);

		/// Gets the label of the Transfer.
		std::wstring getLabel(const Language* const lang) const;

		/// Gets the hours remaining of the Transfer.
		int getHours() const;
		/// Gets the quantity of the Transfer.
		int getQuantity() const;
		/// Gets the type of the Transfer.
		PurchaseSellTransferType getTransferType() const;

		/// Advances the Transfer.
		void advance(Base* const base);
};

}

#endif
