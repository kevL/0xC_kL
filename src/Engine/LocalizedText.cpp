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

#include "LocalizedText.h"

#include "Language.h"


namespace OpenXcom
{

/**
 * Replaces the next argument placeholder with @a val.
 * @param val - reference to the value to place in the next placeholder's position
 * @return, a translated string with all occurrences of the marker replaced by @a val
 */
LocalizedText LocalizedText::arg(const std::wstring& val) const
{
	std::wostringstream woststr;
	woststr << '{' << _nextArg << '}';
	std::wstring marker (woststr.str());

	size_t pos (_text.find(marker));
	if (std::wstring::npos == pos)
		return *this;

	std::wstring ntext(_text);
	for (
			;
			std::wstring::npos != pos;
			pos = ntext.find(
							marker,
							pos + val.length()))
	{
		ntext.replace(
					pos,
					marker.length(),
					val);
	}

	return LocalizedText(ntext, _nextArg);
}

/**
 * Replaces the next argument placeholder with @a val.
 * @param val - reference to the value to place in the next placeholder's position
 * @return, a reference to the translated string with all occurrences of the marker replaced by @a val
 */
LocalizedText& LocalizedText::arg(const std::wstring& val)
{
	std::wostringstream woststr;
	woststr << '{' << _nextArg << '}';
	std::wstring marker (woststr.str());

	size_t pos (_text.find(marker));
	if (std::wstring::npos != pos)
	{
		for (
				;
				std::wstring::npos != pos;
				pos = _text.find(
								marker,
								pos + val.length()))
		{
			_text.replace(
						pos,
						marker.length(),
						val);
		}

		++_nextArg;
	}

	return *this;
}

/**
 * Returns the UTF-8 representation of this string.
 * @return, a UTF-8 string
 */
std::string LocalizedText::asUTF8() const
{
	return Language::wstrToUtf8(_text);
}

}
