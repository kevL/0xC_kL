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

#ifndef OPENXCOM_BASE_H
#define OPENXCOM_BASE_H

//#include <cstdint>
//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "Target.h"


namespace OpenXcom
{

enum RecallType
{
	RCL_SOLDIER,	// 0
	RCL_TRANSFER,	// 1
	RCL_PURCHASE,	// 2
	RCL_SELL		// 3
};

enum PurchaseSellTransferType
{
	PST_SOLDIER,	// 0
	PST_SCIENTIST,	// 1
	PST_ENGINEER,	// 2
	PST_CRAFT,		// 3
	PST_ITEM		// 4
};


class BaseFacility;
class Craft;
class ItemContainer;
class Language;
class Production;
class ResearchProject;
class Ruleset;
class SavedGame;
class Soldier;
class Transfer;


/**
 * Represents a player's Base on the Globe.
 * @note Bases can contain facilities, personnel, crafts and equipment.
 */
class Base final
	:
		public Target
{

private:
	bool
		_exposed,
		_isQuickDefense,
		_placed,
		_tactical;
	int
		_cashIncome,
		_cashSpent,
		_defenseResult,
		_engineers,
		_scientists;
	size_t
		_recallPurchase,
		_recallSell,
		_recallSoldier,
		_recallTransfer;

	ItemContainer* _items;
	const Ruleset* const _rules;
	SavedGame* _gameSave;

	std::wstring _label;

	std::vector<BaseFacility*>
		_defenses,
		_facilities;
	std::vector<Craft*> _crafts;
	std::vector<Production*> _productions;
	std::vector<ResearchProject*> _researchProjects;
	std::vector<Soldier*> _soldiers;
	std::vector<Transfer*> _transfers;

	/// Gets soldier-factor for Research Help.
	static double getSoldierHelp(const std::string& rp);
	/// Gets navigator-factor for Research Help.
	static double getNavigatorHelp(const std::string& rp);
	/// Gets medic-factor for Research Help.
	static double getMedicHelp(const std::string& rp);
	/// Gets engineer-factor for Research Help.
	static double getEngineerHelp(const std::string& rp);
	/// Gets leader-factor for Research Help.
	static double getLeaderHelp(const std::string& rp);
	/// Gets commander-factor for Research Help.
	static double getCommanderHelp(const std::string& rp);

	/// Calculates the chance that aLiens have to detect the Base.
	static int exposedChance(
			int diff,
			int facQty,
			int shields);


	public:
		static const size_t
			MAX_BASES = 8u,
			BASE_SIZE = 6u;

		/// Creates a Base.
		Base(
				const Ruleset* const rules,
				SavedGame* const gameSave);
		/// Cleans up the Base.
		~Base();

		/// Loads the Base from YAML.
		void loadBase(
				const YAML::Node& node,
				bool isFirstBase = false,
				bool isQuickBattle = false);
		/// Saves the Base to YAML.
		YAML::Node save() const override;
		/// Saves the Base's unique-ID to YAML.
		YAML::Node saveIdentificator() const override;

		/// Gets the Base's label.
		std::wstring getLabel(
				const Language* const lang = nullptr,
				bool id = true) const override;
		/// Sets the Base's label.
		void setLabel(const std::wstring& label);
		/// Gets the Base's globe-marker.
		int getMarker() const override;

		/// Gets the Base's facilities.
		std::vector<BaseFacility*>* getFacilities();
		/// Gets the Base's soldiers.
		std::vector<Soldier*>* getSoldiers();
		/// Gets the Base's crafts.
		std::vector<Craft*>* getCrafts();
		/// Gets the Base's transfers.
		std::vector<Transfer*>* getTransfers();

		/// Gets the Base's items.
		ItemContainer* getStorageItems();

		/// Gets the Base's scientists.
		int getScientists() const;
		/// Sets the Base's scientists.
		void setScientists(int scientists);
		/// Gets the Base's engineers.
		int getEngineers() const;
		/// Sets the Base's engineers.
		void setEngineers(int engineers);

		/// Gets the Base's combat-ready soldiers.
		int getAvailableSoldiers() const;
		/// Gets the Base's non-wounded soldiers.
		int getHealthySoldiers() const;
		/// Gets the Base's total soldiers.
		int getTotalSoldiers() const;

		/// Gets the Base's total scientists.
		int getTotalScientists() const;
		/// Gets the number of scientists at work.
		int getAllocatedScientists() const;
		/// Gets the Base's total engineers.
		int getTotalEngineers() const;
		/// Gets the number of engineers at work.
		int getAllocatedEngineers() const;

		/// Gets the Base's used living quarters.
		int getUsedQuarters() const;
		/// Gets the Base's available living quarters.
		int getTotalQuarters() const;
		/// Gets the Base's total free personel space.
		int getFreeQuarters() const;

		/// Gets the Base's used storage space.
		double getUsedStores() const;
		/// Gets the Base's available storage space.
		int getTotalStores() const;
		/// Checks if the Base's stores are overfull.
		bool storesOverfull(double offset = 0.) const;

		/// Gets the Base's used laboratory space.
		int getUsedLaboratories() const;
		/// Gets the Base's available laboratory space.
		int getTotalLaboratories() const;
		/// Gets the number of available space lab (not used by a ResearchProject).
		int getFreeLaboratories() const;
		/// Checks if the Base has research facilities.
		bool hasResearch() const;

		/// Gets the Base's used workshop space.
		int getUsedWorkshops() const;
		/// Gets the Base's available workshop space.
		int getTotalWorkshops() const;
		/// Gets the number of available space lab (not used by a Production).
		int getFreeWorkshops() const;
		/// Checks if the Base has production facilities.
		bool hasProduction() const;

		/// Gets the Base's used psi lab space.
		int getUsedPsiLabs() const;
		/// Gets the Base's total available psi lab space.
		int getTotalPsiLabs() const;
		/// Gets the Base's total free psi lab space.
		int getFreePsiLabs() const;
		/// Checks if the Base has psi labs.
		bool hasPsiLabs() const;

		/// Gets the total amount of used containment space.
		int getUsedContainment() const;
		/// Gets the total amount of containment space.
		int getTotalContainment() const;
		/// Gets the Base's total free containment space.
		int getFreeContainment() const;
		/// Checks if the Base has alien containment.
		bool hasContainment() const;
		/// Gets the quantity of aLiens currently under interrogation.
		int getInterrogatedAliens() const;

		/// Gets the Base's used hangars.
		int getUsedHangars() const;
		/// Gets the Base's available hangars.
		int getTotalHangars() const;
		/// Gets the Base's total free hangar space.
		int getFreeHangars() const;

		/// Gets the Base's Soldiers of a specified type.
		int getSoldierCount(const std::string& type) const;
		/// Gets the Base's Crafts of a specified type.
		int getCraftCount(const std::string& type) const;

		/// Adds a Production to the Base.
		void addProduction(Production* const prod);
		/// Removes a Production from the Base.
		void removeProduction(const Production* const prod);
		/// Gets a list of the Base's Productions.
		const std::vector<Production*>& getProductions() const;

		/// Gets the list of the Base's ResearchProjects.
		const std::vector<ResearchProject*>& getResearch() const;
		/// Adds a fresh ResearchProject to the Base.
		void addResearch(ResearchProject* const project);
		/// Removes a ResearchProject from the Base.
		void removeResearch(
				ResearchProject* const project,
				bool grantHelp = false,
				bool goOffline = false);
		/// Research Help ala XcomUtil.
		void researchHelp(const std::string& aLien);

		/// Sets the Base's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets if the Base is in the battlescape.
		bool getTactical() const;

		/// Sets the Base as eligible for alien retaliation.
		void setBaseExposed(bool exposed = true);
		/// Gets if the Base is eligible for alien retaliation.
		bool getBaseExposed() const;

		/// Flags the Base as placed and in operation.
		void placeBase();
		/// Checks if the Base has been placed on the Globe.
		bool isBasePlaced() const;

		/// Checks if the Base is hyper-wave equipped.
		bool getHyperDetection() const;
		/// Gets the Base's short range detection.
//		int getShortRangeDetection() const;
		/// Gets the Base's short range detection value.
		int getShortRangeTotal() const;
		/// Gets the Base's long range detection.
//		int getLongRangeDetection() const;
		/// Gets the Base's long range detection.
		int getLongRangeTotal() const;

		/// Checks if a target is detected by the Base's radar.
		int detect(Target* const target) const;
		/// Checks if a target is inside the Base's radar range.
		double insideRadarRange(const Target* const target) const;

		/// Gets the detection chance for the Base.
		int getExposedChance(
				int diff,
				int* facQty = nullptr,
				int* shields = nullptr) const;

		/// Gets how many grav-shields the Base has.
		size_t getGravShields() const;

		/// Gets the Base's defense-value.
		int getDefenseTotal() const;
		/// Sets up the Base defenses.
		bool setupBaseDefense();
		/// Cleans up the Base defenses.
		void clearBaseDefense();
		/// Sets the result of the Base's defense against aLien attacks.
		void setDefenseResult(int result);
		/// Gets the result of the Base's defense against aLien attacks.
		int getDefenseResult() const;
		/// Clears the result of the Base's defense against aLien attacks.
		void clearDefenseResult();
		/// Gets a list of defensive Facilities.
		std::vector<BaseFacility*>* getDefenses();

		/// Destroys all disconnected facilities in the Base.
		void destroyDisconnectedFacilities();
		/// Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
		std::list<std::vector<BaseFacility*>::const_iterator> getDisconnectedFacilities(const BaseFacility* const ignoreFac = nullptr);
		/// Destroys a facility and deals with the side-effects.
		std::vector<BaseFacility*>::const_iterator destroyFacility(std::vector<BaseFacility*>::const_iterator pFac);

		/// Gets the Base's craft-maintenance.
		int getCraftMaintenance() const;
		/// Gets the Base's personnel-maintenance.
		int getPersonnelMaintenance() const;
		/// Gets the Base's facility-maintenance.
		int getFacilityMaintenance() const;
		/// Gets the Base's total monthly maintenance.
		int getMonthlyMaintenace() const;

		/// Adds a specified quantity to the Base's total-income value.
		void addCashIncome(int cash);
		/// Gets the Base's total-income value.
		int getCashIncome() const;
		/// Zeros the Base's total-income value.
		void zeroCashIncome();
		/// Adds a specified quantity to the Base's total-expenditure value.
		void addCashSpent(int cash);
		/// Gets the Base's total-expenditure value.
		int getCashSpent() const;
		/// Zeros the Base's total-expenditure value.
		void zeroCashSpent();

		/// Sets various recalls for the Base.
		void setRecallRow(
				RecallType recallType,
				size_t row);
		/// Gets various recalls for the Base.
		size_t getRecallRow(RecallType recallType) const;

		/// Calculates the bonus-expense for Soldiers by rank.
		int getOperationalExpenses(const Craft* const craft = nullptr) const;
		/// Calculates a Soldier's bonus-pay for doing a tactical mission.
		int expenseSoldier(
				const Soldier* const sol,
				bool dead = false);
		/// Calculates the expense of sending HWPs/doggies on a tactical mission.
		int expenseSupport(
				const int quadrants,
				bool dead = false);
		/// Calculates the expense of sending a transport-craft on a tactical mission.
		int expenseCraft(const Craft* const craft);

		/// Sorts the Base's Soldiers according to a pre-determined algorithm.
		void sortSoldiers();

		/// Calculates the penalty-score for losing the Base.
		int calcLostScore() const;

		/// Checks if any Craft at the Base can be refurbished.
		void refurbishCraft(const std::string& itType);

		/// Checks if the Base is a quick-battle base for a base-defense tactical.
		bool isQuickDefense() const;
		/// Sets the Base as a quick-battle base for a base-defense tactical.
		void setQuickDefense(bool quickDefense = true);
};

}

#endif
