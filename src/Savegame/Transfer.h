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
 * Represents an item transfer.
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
		/// Creates a new transfer.
		explicit Transfer(int hours);
		/// Cleans up the transfer.
		~Transfer();

		/// Loads the transfer from YAML.
		bool load(
				const YAML::Node& node,
				Base* const base,
				const Ruleset* const rule);
		/// Saves the transfer to YAML.
		YAML::Node save() const;

		/// Gets the soldier being transfered.
		Soldier* getSoldier() const;
		/// Sets the soldier of the transfer.
		void setSoldier(Soldier* const soldier);
		/// Sets the scientists of the transfer.
		void setScientists(int scientists);
		/// Sets the engineers of the transfer.
		void setEngineers(int engineers);
		/// Sets the craft of the transfer.
		void setCraft(Craft* const craft);
		/// Gets the craft of the transfer.
		Craft* getCraft() const;

		/// Gets the item-types of the transfer.
		std::string getTransferItems() const;
		/// Sets the item-types of the transfer.
		void setTransferItems(
				const std::string& id,
				int qty = 1);

		/// Gets the name of the transfer.
		std::wstring getName(const Language* const lang) const;

		/// Gets the hours remaining of the transfer.
		int getHours() const;
		/// Gets the quantity of the transfer.
		int getQuantity() const;
		/// Gets the type of the transfer.
		PurchaseSellTransferType getTransferType() const;

		/// Advances the transfer.
		void advance(Base* const base);
};

}

#endif
