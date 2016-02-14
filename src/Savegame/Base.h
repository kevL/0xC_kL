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
	REC_SOLDIER,	// 0
	REC_TRANSFER,	// 1
	REC_PURCHASE,	// 2
	REC_SELL		// 3
};

enum PurchaseSellTransferType
{
	PST_SOLDIER,	// 0
	PST_SCIENTIST,	// 1
	PST_ENGINEER,	// 2
	PST_ITEM,		// 3
	PST_CRAFT		// 4
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
class Vehicle;


/**
 * Represents a player base on the globe.
 * @note Bases can contain facilities, personnel, crafts and equipment.
 */
class Base final
	:
		public Target
{

private:
	bool
		_exposed,
		_tactical,
		_placed;
	int
		_cashSpent,
		_cashIncome,
		_engineers,
		_scientists,
		_defenseResult;
	size_t
		_recallPurchase,
		_recallSell,
		_recallSoldier,
		_recallTransfer;

	ItemContainer* _items;
	const Ruleset* const _rules;

	std::wstring _name;

	std::vector<BaseFacility*>
		_defenses,
		_facilities;
	std::vector<Craft*> _crafts;
	std::vector<Production*> _productions;
	std::vector<ResearchProject*> _research;
	std::vector<Soldier*> _soldiers;
	std::vector<Transfer*> _transfers;
	std::vector<Vehicle*> _vehicles;

	/// Determines space taken up by ammo clips about to rearm craft.
//	double getIgnoredStores();

	/// Calculates the chance that aLiens have to detect this Base.
	int calcDetChance(
			int diff,
			int facQty,
			int shields) const;


	public:
		static const size_t
			MAX_BASES = 8,
			BASE_SIZE = 6;

		/// Creates a new Base.
		explicit Base(const Ruleset* const rules);
		/// Cleans up the Base.
		~Base();

		/// Loads a Base from YAML.
		using Target::load;
		void load(
				const YAML::Node& node,
				SavedGame* const gameSave,
				bool firstBase = false,
				bool skirmish = false);
		/// Saves this Base to YAML.
		YAML::Node save() const override;
		/// Saves this Base's ID to YAML.
		YAML::Node saveId() const override;

		/// Gets this Base's name.
		std::wstring getName(const Language* const lang) const override;
		/// Sets this Base's name.
		void setName(const std::wstring& wst);
		/// Gets this Base's marker.
		int getMarker() const override;

		/// Gets this Base's facilities.
		std::vector<BaseFacility*>* getFacilities();
		/// Gets this Base's soldiers.
		std::vector<Soldier*>* getSoldiers();
		/// Gets this Base's crafts.
		std::vector<Craft*>* getCrafts();
		/// Gets this Base's transfers.
		std::vector<Transfer*>* getTransfers();

		/// Gets this Base's items.
		ItemContainer* getStorageItems();

		/// Gets this Base's scientists.
		int getScientists() const;
		/// Sets this Base's scientists.
		void setScientists(int scientists);
		/// Gets this Base's engineers.
		int getEngineers() const;
		/// Sets this Base's engineers.
		void setEngineers(int engineers);

		/// Gets this Base's available soldiers.
		int getAvailableSoldiers(const bool combatReady = false) const;
		/// Gets this Base's total soldiers.
		int getTotalSoldiers() const;

		/// Gets this Base's total scientists.
		int getTotalScientists() const;
		/// Gets the number of scientists at work.
		int getAllocatedScientists() const;
		/// Gets this Base's total engineers.
		int getTotalEngineers() const;
		/// Gets the number of engineers at work.
		int getAllocatedEngineers() const;

		/// Gets this Base's used living quarters.
		int getUsedQuarters() const;
		/// Gets this Base's available living quarters.
		int getTotalQuarters() const;
		/// Gets this Base's total free personel space.
		int getFreeQuarters() const;

		/// Gets this Base's used storage space.
		double getUsedStores() const;
		/// Gets this Base's available storage space.
		int getTotalStores() const;
		/// Checks if this Base's stores are overfull.
		bool storesOverfull(double offset = 0.) const;

		/// Gets this Base's used laboratory space.
		int getUsedLaboratories() const;
		/// Gets this Base's available laboratory space.
		int getTotalLaboratories() const;
		/// Gets the number of available space lab (not used by a ResearchProject).
		int getFreeLaboratories() const;
		/// Checks if this Base has research facilities.
		bool hasResearch() const;

		/// Gets this Base's used workshop space.
		int getUsedWorkshops() const;
		/// Gets this Base's available workshop space.
		int getTotalWorkshops() const;
		/// Gets the number of available space lab (not used by a Production).
		int getFreeWorkshops() const;
		/// Checks if this Base has production facilities.
		bool hasProduction() const;

		/// Gets this Base's used psi lab space.
		int getUsedPsiLabs() const;
		/// Gets this Base's total available psi lab space.
		int getTotalPsiLabs() const;
		/// Gets this Base's total free psi lab space.
		int getFreePsiLabs() const;
		/// Checks if this Base has psi labs.
		bool hasPsiLabs() const;

		/// Gets the total amount of used containment space.
		int getUsedContainment() const;
		/// Gets the total amount of containment space.
		int getTotalContainment() const;
		/// Gets this Base's total free containment space.
		int getFreeContainment() const;
		/// Checks if this Base has alien containment.
		bool hasContainment() const;
		/// Gets the quantity of aLiens currently under interrogation.
		int getInterrogatedAliens() const;

		/// Gets this Base's used hangars.
		int getUsedHangars() const;
		/// Gets this Base's available hangars.
		int getTotalHangars() const;
		/// Gets this Base's total free hangar space.
		int getFreeHangars() const;

		/// Gets this Base's soldiers of a certain type.
		int getSoldierCount(const std::string &soldier) const;
		/// Gets this Base's crafts of a certain type.
		int getCraftCount(const std::string& craft) const;

		/// Gets the list of this Base's ResearchProject.
		const std::vector<ResearchProject*>& getResearch() const;
		/// Adds a new ResearchProject to this Base.
		void addResearch(ResearchProject* const project);
		/// Removes a ResearchProject from this Base.
		void removeResearch(
				ResearchProject* const project,
				bool grantHelp = true,
				bool goOffline = false);
		/// Research Help ala XcomUtil.
		void researchHelp(const std::string& aLien);
		/// Gets soldier factor for Research Help.
		double getSoldierHelp(const std::string& rp);
		/// Gets navigator factor for Research Help.
		double getNavigatorHelp(const std::string& rp);
		/// Gets medic factor for Research Help.
		double getMedicHelp(const std::string& rp);
		/// Gets engineer factor for Research Help.
		double getEngineerHelp(const std::string& rp);
		/// Gets leader factor for Research Help.
		double getLeaderHelp(const std::string& rp);
		/// Gets commander factor for Research Help.
		double getCommanderHelp(const std::string& rp);

		/// Adds a Production to this Base.
		void addProduction(Production* const prod);
		/// Removes a Production from this Base.
		void removeProduction(const Production* const prod);
		/// Gets a list of this Base's Production.
		const std::vector<Production*>& getProductions() const;

		/// Sets the Base's battlescape status.
		void setTactical(bool tactical = true);
		/// Gets if the Base is in the battlescape.
		bool getTactical() const;

		/// Sets this Base as eligible for alien retaliation.
		void setBaseExposed(bool exposed = true);
		/// Gets if this Base is eligible for alien retaliation.
		bool getBaseExposed() const;

		/// Sets this Base as placed and in operation.
		void setBasePlaced();
		/// Gets if this Base has been placed on the Globe.
		bool getBasePlaced() const;

		/// Checks if this Base is hyper-wave equipped.
		bool getHyperDetection() const;
		/// Gets this Base's short range detection.
//		int getShortRangeDetection() const;
		/// Gets this Base's short range detection value.
		int getShortRangeTotal() const;
		/// Gets this Base's long range detection.
//		int getLongRangeDetection() const;
		/// Gets this Base's long range detection.
		int getLongRangeTotal() const;

		/// Checks if a target is detected by this Base's radar.
		int detect(Target* const target) const;
		/// Checks if a target is inside this Base's radar range.
		double insideRadarRange(const Target* const target) const;

		/// Gets the detection chance for this Base.
		int getDetectionChance(
				int diff,
				int* facQty = nullptr,
				int* shields = nullptr) const;

		/// Gets how many Grav Shields this Base has.
		size_t getGravShields() const;

		/// Gets this Base's defense value.
		int getDefenseTotal() const;
		/// Sets up this Base defenses.
		void setupBaseDefense();
		/// Cleans up this Base's defenses vector after a Ufo attack and optionally reclaims the tanks and their ammo.
		void cleanupBaseDefense(bool hwpToStores = false);

		/// Sets the result of this Base's defense against aLien attacks.
		void setDefenseResult(int result);
		/// Gets the result of this Base's defense against aLien attacks.
		int getDefenseResult() const;

		/// Gets a list of defensive Facilities.
		std::vector<BaseFacility*>* getDefenses();

		/// Gets this Base's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Destroys all disconnected facilities in this Base.
		void destroyDisconnectedFacilities();
		/// Gets a sorted list of the facilities(=iterators) NOT connected to the Access Lift.
		std::list<std::vector<BaseFacility*>::const_iterator> getDisconnectedFacilities(const BaseFacility* const ignoreFac = nullptr);
		/// Destroys a facility and deals with the side effects.
		std::vector<BaseFacility*>::const_iterator destroyFacility(std::vector<BaseFacility*>::const_iterator pFac);

		/// Gets this Base's craft maintenance.
		int getCraftMaintenance() const;
		/// Gets this Base's personnel maintenance.
		int getPersonnelMaintenance() const;
		/// Gets this Base's facility maintenance.
		int getFacilityMaintenance() const;
		/// Gets this Base's total monthly maintenance.
		int getMonthlyMaintenace() const;

		/// Increases (or decreases) this Base's total income amount.
		void setCashIncome(int cash);
		/// Gets this Base's total income amount.
		int getCashIncome() const;
		/// Increases (or decreases) this Base's total spent amount.
		void setCashSpent(int cash);
		/// Gets this Base's total spent amount.
		int getCashSpent() const;

		/// Sets various recalls for this Base.
		void setRecallRow(
				RecallType recallType,
				size_t row);
		/// Gets various recalls for this Base.
		size_t getRecallRow(RecallType recallType) const;

		/// Calculates the bonus cost for soldiers by rank.
		int calcSoldierBonuses(const Craft* const craft = nullptr) const;
		/// Calculates a soldier's bonus pay for doing a tactical mission.
		int soldierExpense(
				const Soldier* const sol,
				const bool dead = false);
		/// Calculates the expense of sending HWPs/doggies on a tactical mission.
		int hwpExpense(
				const int hwpSize,
				const bool dead = false);
		/// Calculates the expense of sending a transport craft on a tactical mission.
		int craftExpense(const Craft* const craft);

		/// Sorts the soldiers according to a pre-determined algorithm.
		void sortSoldiers();
};

}

#endif
