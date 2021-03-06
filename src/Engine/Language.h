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
 *e
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_LANGUAGE_H
#define OPENXCOM_LANGUAGE_H

#include <map>		// std::map
#include <string>	// std::string, std::wstring
#include <vector>	// std::vector

#include "LocalizedText.h"

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

enum TextDirection
{
	DIRECTION_LTR,	// 0
	DIRECTION_RTL	// 1
};


enum TextWrapping
{
	WRAP_WORDS,		// 0
	WRAP_LETTERS	// 1
};


class ExtraStrings;
class LanguagePlurality;
class TextList;


/**
 * Contains strings used throughout the game for Language localization.
 * @note Languages are a set of strings identified by an ID string.
 */
class Language
{

private:
	std::string _id;

	LanguagePlurality* _handler;
	TextDirection _direction;
	TextWrapping _wrap;

	std::map<std::string, LocalizedText> _strings;

	static std::map<std::string, std::wstring> _langList;
	static std::vector<std::string>
		_rtl,
		_cjk;

	/// Parses a text string loaded from an external file.
	std::wstring loadString(const std::string& stIn) const;


	public:
		/// Creates a blank language.
		Language();
		/// Cleans up the language.
		~Language();

		/// Converts a wide-string to UTF-8.
		static std::string wstrToUtf8(const std::wstring& src);
		/// Converts a wide-string to local-codepage string.
		static std::string wstrToCp(const std::wstring& src);
		/// Converts a wide-string to filesystem string.
		static std::string wstrToFs(const std::wstring &src);
		/// Converts a UTF-8 string to wide-string.
		static std::wstring utf8ToWstr(const std::string& src);
		/// Converts a local-codepage string to wide-string.
		static std::wstring cpToWstr(const std::string& src);
		/// Converts a filesystem string to wide-string.
		static std::wstring fsToWstr(const std::string &src);

		/// Replaces a substring.
		static void replace(
				std::string& st,
				const std::string& stPre,
				const std::string& stPost);
		/// Replaces a substring.
		static void replace(
				std::wstring& wst,
				const std::wstring& wstPre,
				const std::wstring& wstPost);

		/// Gets list of languages in the data directory.
		static void getList(
				std::vector<std::string>& files,
				std::vector<std::wstring>& languages);

		/// Loads the language from a YAML file.
		void load(
				const std::string& file,
				ExtraStrings* const extras);

		/// Gets the language's ID.
		std::string getId() const;
		/// Gets the language's label.
		std::wstring getLabel() const;

		/// Outputs the language to a HTML file.
//		void toHtml(const std::string& file) const;

		/// Gets a LocalizedText.
		const LocalizedText& getString(const std::string& id) const;
		/// Gets a quantity-dependent LocalizedText.
		LocalizedText getString(
				const std::string& id,
				unsigned qty) const;
		/// Gets a gender-dependent LocalizedText.
		const LocalizedText& getString(
				const std::string& id,
				SoldierGender gender) const;

		/// Gets the direction of text in the language.
		TextDirection getTextDirection() const;
		/// Gets the wrapping of text in the language.
		TextWrapping getTextWrapping() const;
};

}

#endif
