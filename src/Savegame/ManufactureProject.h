/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_MANUFACTUREPROJECT_H
#define OPENXCOM_MANUFACTUREPROJECT_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Ruleset;
class SavedGame;


// Used by both Manufacture and Facility construction in ManufactureCompleteState.
enum ManufactureProgress
{
	PROG_NOT_COMPLETE,			// 0
	PROG_COMPLETE,				// 1
	PROG_NOT_ENOUGH_MONEY,		// 2
	PROG_NOT_ENOUGH_MATERIALS,	// 3
	PROG_CONSTRUCTION			// 4
};


class ManufactureProject
{

private:
	const RuleManufacture* _mfRule;

	bool
		_infinite,
		_sell;
	int
		_qtyTotal,
		_engineers,
		_hoursSpent;

	/// Checks if there is enough funds to start/continue the project.
	bool hasEnoughMoney(const SavedGame* const playSave) const;
	/// Checks if there is enough material to start/continue the project.
	bool hasEnoughMaterials(
			Base* const base,
			const Ruleset* const rules) const;


	public:
		/// Tracks a Manufacture project.
		explicit ManufactureProject(const RuleManufacture* const mfRule);
		/// Cleans the Manufacture project.
		~ManufactureProject();

		/// Loads from YAML.
		void load(const YAML::Node& node);
		/// Saves to YAML.
		YAML::Node save() const;

		/// Gets the rules for the Manufacture.
		const RuleManufacture* getRules() const;

		/// Sets the total quantity to produce.
		void setManufactureTotal(int qty);
		/// Gets the total quantity to produce.
		int getManufactureTotal() const;

		/// Sets if the Manufacture is to produce an infinite quantity.
		void setInfinite(bool infinite = true);
		/// Gets if the Manufacture is to produce an infinite quantity.
		bool getInfinite() const;

		/// Gets the quantity of assigned engineers to the Manufacture.
		void setAssignedEngineers(int engineers);
		/// Gets the quantity of assigned engineers to the Manufacture.
		int getAssignedEngineers() const;

		/// Sets if the produced parts are to be sold immediately.
		void setAutoSales(bool sell);
		/// Gets if the produced parts are to be sold immediately.
		bool getAutoSales() const;

		/// Gets the quantity of iterations completed so far.
		int getQuantityManufactured() const;

		/// Starts the Manufacture.
		void startManufacture(
				Base* const base,
				SavedGame* const playSave) const;

		/// Advances the Manufacture.
		ManufactureProgress stepManufacture(
				Base* const base,
				SavedGame* const playSave);

		/// Calculates the duration till the Manufacture is completed.
		bool tillFinish(
				int& days,
				int& hours) const;

		/// Sets the hours spent on the Manufacture so far.
//		void setTimeSpent(int spent);
		/// Gets the hours spent on the Manufacture so far.
//		int getTimeSpent() const;
};

}

#endif
