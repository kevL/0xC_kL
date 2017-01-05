/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#ifndef OPENXCOM_MONTHLYREPORTSTATE_H
#define OPENXCOM_MONTHLYREPORTSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class SavedGame;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Report screen shown monthly to display the player's performance and funding.
 */
class MonthlyReportState
	:
		public State
{

private:
	bool _defeated;
	int
		_deltaFunds,
		_ratingPrior,
		_ratingTotal;

	SavedGame* _playSave;
	Text
		* _txtChange,
		* _txtDefeat,
		* _txtMonth,
		* _txtRating,
		* _txtTitle;
	TextButton
		* _btnOk,
		* _btnDefeat;
	TextList* _lstCouncil;
	Window* _window;

	std::vector<std::string>
		_listHappy,
		_listSad,
		_listPacts;

	std::vector<Soldier*> _soldiersFeted;

	/// Strings together a grammatically correct listing of Countries.
	std::wstring countryList(
			const std::vector<std::string>& countries,
			const std::string& singular,
			const std::string& plural) const;
	/// Calculates monthly stuff.
	void calculateReport();
	/// Deals with monthly SoldierAwards.
	void awards();


	public:
		/// Creates a MonthlyReport state.
		MonthlyReportState();
		/// Cleans up the MonthlyReport state.
		~MonthlyReportState();

		/// Updates palettes.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
