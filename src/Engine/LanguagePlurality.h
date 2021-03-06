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

#ifndef OPENXCOM_LANGUAGEPLURALITY_H
#define OPENXCOM_LANGUAGEPLURALITY_H

#include <map>
#include <string>


namespace OpenXcom
{

/**
 * This class is the interface used to find plural forms for the different languages.
 * Derived classes implement getSuffix() according to the specific language's rules.
 */
class LanguagePlurality
{

private:
	typedef LanguagePlurality* (*PFCreate)();

	static std::map<std::string, PFCreate> s_factoryFunctions;


	protected:
		///
		LanguagePlurality()
		{}


		public:
			/// Allow proper destruction through base pointer.
			virtual ~LanguagePlurality()
			{}

			/// Get dictionary key suffix for value of @a n.
			/**
			 * @param n - The number controlling the plurality.
			 * @return, Pointer to the zero-terminated suffix string
			 */
			virtual const char* getSuffix(unsigned n) const = 0;

			/// Create a concrete instance for a given language.
			/**
			 * @param language - Reference to the locale of a language.
			 * @return, Pointer to plurality rules for that language.
			 */
			static LanguagePlurality* create(const std::string& language);
};

}

#endif
