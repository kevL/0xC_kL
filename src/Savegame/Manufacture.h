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

#ifndef OPENXCOM_MANUFACTURE_H
#define OPENXCOM_MANUFACTURE_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Ruleset;
class SavedGame;


// Used by Manufacture and construction both by ManufactureCompleteState in geoscape.
enum ManufactureProgress
{
	PROGRESS_NOT_COMPLETE,			// 0
	PROGRESS_COMPLETE,				// 1
	PROGRESS_NOT_ENOUGH_MONEY,		// 2
	PROGRESS_NOT_ENOUGH_MATERIALS,	// 3
	PROGRESS_CONSTRUCTION			// 4
};


class Manufacture
{

private:
	const RuleManufacture* _mfRule;

	bool
		_infinite,
		_sell;
	int
		_units,
		_engineers,
		_timeSpent;

	/// Checks if there is enough funds to continue Manufacture.
	bool hasEnoughMoney(const SavedGame* const gameSave) const;
	/// Checks if there is enough resource material to continue Manufacture.
	bool hasEnoughMaterials(
			Base* const base,
			const Ruleset* const rules) const;


	public:
		/// Tracks a Manufacture project.
		explicit Manufacture(const RuleManufacture* const mfRule);
		/// Cleans the Manufacture project.
		~Manufacture();

		/// Loads from YAML.
		void load(const YAML::Node& node);
		/// Saves to YAML.
		YAML::Node save() const;

		/// Gets the rules for the Manufacture.
		const RuleManufacture* getRules() const;

		/// Sets the total quantity to produce.
		void setManufactureTotal(int quantity);
		/// Gets the total quantity to produce.
		int getManufactureTotal() const;

		/// Sets if the Manufacture is to produce an infinite quantity.
		void setInfinite(bool infinite);
		/// Gets if the Manufacture is to produce an infinite quantity.
		bool getInfinite() const;

		/// Gets the quantity of assigned engineers to the Manufacture.
		void setAssignedEngineers(int engineers);
		/// Gets the quantity of assigned engineers to the Manufacture.
		int getAssignedEngineers() const;

		/// Sets if the produced items are to be sold immediately.
		void setAutoSales(bool sell);
		/// Gets if the produced items are to be sold immediately.
		bool getAutoSales() const;

		/// Gets the quantity of items manufactured so far.
		int getQuantityManufactured() const;

		/// Advances the Manufacture.
		ManufactureProgress step(
				Base* const base,
				SavedGame* const gameSave);

		/// Starts the Manufacture.
		void startManufacture(
				Base* const base,
				SavedGame* const gameSave) const;

		/// Gets the time till the Manufacture is completed.
		bool tillFinish(
				int& days,
				int& hours) const;

		/// Gets the time spent on the Manufacture so far.
//		int getTimeSpent() const;
		/// Sets the time spent on the Manufacture so far.
//		void setTimeSpent(int spent);
};

}

#endif
