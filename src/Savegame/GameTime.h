/*
 * Copyright 2010-2020 OpenXcom Developers.
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

#ifndef OPENXCOM_GAMETIME_H
#define OPENXCOM_GAMETIME_H

//#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Time periods.
 */
enum TimeTrigger
{
	TIME_1SEC,	// 0 Volutar's smooth_globe_terminator.
	TIME_5SEC,	// 1
	TIME_10MIN,	// 2
	TIME_30MIN,	// 3
	TIME_1HOUR,	// 4
	TIME_1DAY,	// 5
	TIME_1MONTH	// 6
};

class Language;


/**
 * Stores the current ingame time/date according to GMT.
 * @note Takes care of managing and representing each component as well as
 * common time operations.
 */
class GameTime
{

private:
	static constexpr int daysPerMonth[12u] { 32,29,32,31,32,31,32,32,31,32,31,32 };

	bool _endIsNear;
	int
		_day,
		_month,
		_year,
		_hour,
		_minute,
		_second;
//		_weekday,


	public:
		static const std::string GAME_MONTHS[12u];

		/// Creates an IG-time/date with specified values.
		GameTime(
//				int weekday,
				int day,
				int month,
				int year,
				int hour,
				int minute,
				int sec);
		/// Cleans up the IG-time/date.
		~GameTime();

		/// Loads the time from YAML.
		void load(const YAML::Node& node);
		/// Saves the time to YAML.
		YAML::Node save(bool memorial = false) const;

		/// Advances the IG time by 1 second.
		TimeTrigger advance();

		/// Gets the IG second.
		int getSecond() const;
		/// Gets the IG minute.
		int getMinute() const;
		/// Gets the IG hour.
		int getHour() const;

		/// Gets the IG day.
		int getDay() const;
		/// Gets a string version of the IG day.
		std::wstring getDayString(const Language* const lang) const;

		/// Gets the IG month.
		int getMonth() const;
		/// Gets a string version of the IG month.
		std::string getMonthString() const;

		/// Gets the IG year.
		int getYear() const;

		/// Gets the position of the daylight according to the IG time.
		double getDaylight() const;

		/// Checks if the end of the month is 1 day away.
		bool isEndNear() const
		{ return _endIsNear; }
		/// Resets the end of month notice/warning.
		void clearEndNear()
		{ _endIsNear = false; }

		/// Gets the IG weekday.
//		int getWeekday() const;
		/// Gets a string version of the IG weekday.
//		std::string getWeekdayString() const;
};

}

#endif
