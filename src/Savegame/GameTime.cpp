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

#include "GameTime.h"

#include "../Engine/Language.h"


namespace OpenXcom
{

constexpr int GameTime::daysPerMonth[12u]; // static.

const std::string GameTime::GAME_MONTHS[12u] // static.
{
	"STR_JAN",
	"STR_FEB",
	"STR_MAR",
	"STR_APR",
	"STR_MAY",
	"STR_JUN",
	"STR_JUL",
	"STR_AUG",
	"STR_SEP",
	"STR_OCT",
	"STR_NOV",
	"STR_DEC"
};


/**
 * Creates an IG-time/date with specified values.
// * @param weekday	- starting weekday
 * @param day		- starting day
 * @param month		- starting month
 * @param year		- starting year
 * @param hour		- starting hour
 * @param minute	- starting minute
 * @param sec		- starting second
 */
GameTime::GameTime(
		int day,
		int month,
		int year,
		int hour,
		int minute,
		int sec)
//		int weekday
	:
		_day(day),
		_month(month),
		_year(year),
		_hour(hour),
		_minute(minute),
		_second(sec),
		_endIsNear(false)
//		_weekday(weekday)
{}

/**
 * dTor.
 */
GameTime::~GameTime()
{}

/**
 * Loads the time from a YAML file.
 * @param node - reference a YAML node
 */
void GameTime::load(const YAML::Node& node)
{
	_day		= node["day"]		.as<int>(_day);
	_month		= node["month"]		.as<int>(_month);
	_year		= node["year"]		.as<int>(_year);
	_hour		= node["hour"]		.as<int>(_hour);
	_minute		= node["minute"]	.as<int>(_minute);
	_second		= node["second"]	.as<int>(_second);
//	_weekday	= node["weekday"]	.as<int>(_weekday);
}

/**
 * Saves the time to a YAML file.
 * @param memorial - true if setting time of soldier death for the memorial (default false)
 * @return, YAML node
 */
YAML::Node GameTime::save(bool memorial) const
{
	YAML::Node node;

	node["day"]		= _day;
	node["month"]	= _month;
	node["year"]	= _year;

	if (memorial == false)
	{
		node["hour"]	= _hour;
		node["minute"]	= _minute;
		node["second"]	= _second;
//		node["weekday"]	= _weekday;
	}

	return node;
}

/**
 * Advances IG time by 1 second.
 * @note This automatically increments the larger time-components when necessary
 * and sends out an appropriate trigger if any of those specific time-markers
 * get reached - for time-dependent events on the Geoscape.
 * @return, time-span trigger
 */
TimeTrigger GameTime::advance()
{
	if (++_second % 5 != 0) // Volutar smooth globe terminator.
		return TIME_1SEC;

	if (_second != 60)
		return TIME_5SEC;

	_second = 0;

	if (++_minute % 10 != 0)
		return TIME_5SEC;

	if (_minute % 30 != 0)
		return TIME_10MIN;

	if (_minute != 60)
		return TIME_30MIN;

	_minute = 0;

	if (++_hour != 24)
		return TIME_1HOUR;

	_hour = 0;

//	if (++_weekday == 8) _weekday = 1;

	++_day;

	if (_month == 2)
	{
		int daysFebruary (daysPerMonth[1u]);

		if (    _year %   4 == 0 // leap year
			&& (_year % 100 != 0 || _year % 400 == 0))
		{
			++daysFebruary;
		}

		if (_day != daysFebruary)
		{
			if (_day == daysFebruary - 1) _endIsNear = true;
			return TIME_1DAY;
		}
	}
	else
	{
		const int days (daysPerMonth[static_cast<size_t>(_month - 1)]);
		if (_day != days)
		{
			if (_day == days - 1) _endIsNear = true;
			return TIME_1DAY;
		}
	}

	_day = 1;

	if (++_month == 13)
	{
		_month = 1;
		++_year;
	}

	return TIME_1MONTH;
}

/**
 * Gets the current IG second.
 * @return, second (0-59)
 */
int GameTime::getSecond() const
{
	return _second;
}

/**
 * Gets the current IG minute.
 * @return, minute (0-59)
 */
int GameTime::getMinute() const
{
	return _minute;
}

/**
 * Gets the current IG hour.
 * @return, hour (0-23)
 */
int GameTime::getHour() const
{
	return _hour;
}

/**
 * Gets the current IG day.
 * @return, day (1-31)
 */
int GameTime::getDay() const
{
	return _day;
}

/**
 * Gets the localized representation of the current IG day with its cardinal
 * suffix attached.
 * @param lang - pointer to current language
 * @return, localized day string
 */
std::wstring GameTime::getDayString(const Language* const lang) const
{
	std::string st;
	switch (_day)
	{
		case 1:
		case 21:
		case 31:
			st = "STR_DATE_FIRST";
			break;
		case 2:
		case 22:
			st = "STR_DATE_SECOND";
			break;
		case 3:
		case 23:
			st = "STR_DATE_THIRD";
			break;

		default:
			st = "STR_DATE_FOURTH";
	}
	return lang->getString(st).arg(_day);
}

/**
 * Gets the current IG month.
 * @return, month (1-12)
 */
int GameTime::getMonth() const
{
	return _month;
}

/**
 * Returns a localizable-string representation of the current IG month.
 * @return, month string ID
 */
std::string GameTime::getMonthString() const
{
	return GAME_MONTHS[static_cast<size_t>(_month - 1)];
}

/**
 * Gets the current IG year.
 * @return, year
 */
int GameTime::getYear() const
{
	return _year;
}

/**
 * Gets the current position of the daylight emitted on the globe according
 * to the current IG-time - so the value is 0 when the light starts at 0º
 * longitude (6h) and 1 when the light ends at 0º longitude (18h).
 * @return, daylight position (0-1)
 */
double GameTime::getDaylight() const
{
	return static_cast<double>(
					(((((_hour + 18) % 24) * 60) + _minute) * 60) + _second)
						/ (60. * 60. * 24.); // kL: Take Two!!!
}

/**
 * Gets the current IG weekday.
 * @return, weekday (1-7) starts on Sunday
 *
int GameTime::getWeekday() const
{
	return _weekday;
} */
/**
 * Returns a localizable-string representation of the current IG weekday.
 * @return, weekday string ID
 *
std::string GameTime::getWeekdayString() const
{
	static const std::string weekdays[7] =
	{
		"STR_SUNDAY",
		"STR_MONDAY",
		"STR_TUESDAY",
		"STR_WEDNESDAY",
		"STR_THURSDAY",
		"STR_FRIDAY",
		"STR_SATURDAY"
	};
	return weekdays[static_cast<size_t>(_weekday) - 1];
} */

}
