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

#ifndef OPENXCOM_PRODUCTION_H
#define OPENXCOM_PRODUCTION_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Base;
class RuleManufacture;
class Ruleset;
class SavedGame;


// Used by Manufacture and construction both by ProductionCompleteState in geoscape.
enum ProductionProgress
{
	PROGRESS_NOT_COMPLETE,			// 0
	PROGRESS_COMPLETE,				// 1
	PROGRESS_NOT_ENOUGH_MONEY,		// 2
	PROGRESS_NOT_ENOUGH_MATERIALS,	// 3
	PROGRESS_CONSTRUCTION			// 4
};


class Production
{

private:
	const RuleManufacture* _manfRule;

	bool
		_infinite,
		_sell;
	int
		_units,
		_engineers,
		_timeSpent;

	/// Checks if there is enough funds to continue production.
	bool enoughMoney(const SavedGame* const gameSave) const;
	/// Checks if there is enough resource material to continue production.
	bool enoughMaterials(
			Base* const base,
			const Ruleset* const rules) const;


	public:
		/// Tracks a Base manufacturing project.
		explicit Production(const RuleManufacture* const manfRule);
		/// Cleans the Base manufacturing project.
		~Production();

		/// Loads from YAML.
		void load(const YAML::Node& node);
		/// Saves to YAML.
		YAML::Node save() const;

		/// Gets the rules for the Production.
		const RuleManufacture* getRules() const;

		/// Gets the total quantity to produce.
		int getProductionTotal() const;
		/// Sets the total quantity to produce.
		void setProductionTotal(int quantity);

		/// Gets if the Production is to produce an infinite quantity.
		bool getInfinite() const;
		/// Sets if the Production is to produce an infinite quantity.
		void setInfinite(bool infinite);

		/// Gets the quantity of assigned engineers to the Production.
		int getAssignedEngineers() const;
		/// Gets the quantity of assigned engineers to the Production.
		void setAssignedEngineers(int engineers);

		/// Gets if the produced items are to be sold immediately.
		bool getAutoSales() const;
		/// Sets if the produced items are to be sold immediately.
		void setAutoSales(bool sell);

		/// Gets the quantity of produced items so far.
		int getProducedQuantity() const;

		/// Advances the Production by a step.
		ProductionProgress step(
				Base* const base,
				SavedGame* const gameSave,
				const Ruleset* const rules);

		/// Starts the Production.
		void startProduction(
				Base* const base,
				SavedGame* const gameSave,
				const Ruleset* const rules) const;

		/// Gets the time till the Production is completed.
		bool tillFinish(
				int& days,
				int& hours) const;

		/// Gets the time spent on the Production so far.
//		int getTimeSpent() const;
		/// Sets the time spent on the Production so far.
//		void setTimeSpent(int spent);
};

}

#endif
