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

#include "Language.h"

#include <algorithm>	// std::find()
#include <cassert>		// assert
//#include <limits>		// std::numeric_limits
//#include <map>		// std::map
//#include <set>		// std::set
//#include <sstream>	// std::wostringstream

#ifdef _WIN32
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif

#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif

#	include <windows.h>
#endif

#include "CrossPlatform.h"
#include "LanguagePlurality.h"
#include "Logger.h"
#include "Options.h"

#include "../Ruleset/ExtraStrings.h"


namespace OpenXcom
{

std::map<std::string, std::wstring> Language::_langList;
std::vector<std::string>
	Language::_rtl,
	Language::_cjk;


/**
 * Initializes a Language.
 */
Language::Language()
	:
		_handler(nullptr),
		_direction(DIRECTION_LTR),
		_wrap(WRAP_WORDS)
{
	if (_langList.empty() == true) // maps don't have initializers :(
	{
		_langList["en-US"]	= utf8ToWstr("English (US)");
		_langList["en-GB"]	= utf8ToWstr("English (UK)");
		_langList["bg"]		= utf8ToWstr("Български");
		_langList["cs"]		= utf8ToWstr("Česky");
		_langList["cy"]		= utf8ToWstr("Cymraeg");
		_langList["da"]		= utf8ToWstr("Dansk");
		_langList["de"]		= utf8ToWstr("Deutsch");
		_langList["el"]		= utf8ToWstr("Ελληνικά");
		_langList["et"]		= utf8ToWstr("Eesti");
		_langList["es-ES"]	= utf8ToWstr("Español (ES)");
		_langList["es-419"]	= utf8ToWstr("Español (AL)");
		_langList["fr"]		= utf8ToWstr("Français (FR)");
		_langList["fr-CA"]	= utf8ToWstr("Français (CA)");
		_langList["fi"]		= utf8ToWstr("Suomi");
		_langList["hr"]		= utf8ToWstr("Hrvatski");
		_langList["hu"]		= utf8ToWstr("Magyar");
		_langList["it"]		= utf8ToWstr("Italiano");
		_langList["ja"]		= utf8ToWstr("日本語");
		_langList["ko"]		= utf8ToWstr("한국어");
		_langList["lb"]		= utf8ToWstr("Lëtzebuergesch");
		_langList["lv"]		= utf8ToWstr("Latviešu");
		_langList["nl"]		= utf8ToWstr("Nederlands");
		_langList["no"]		= utf8ToWstr("Norsk");
		_langList["pl"]		= utf8ToWstr("Polski");
		_langList["pt-BR"]	= utf8ToWstr("Português (BR)");
		_langList["pt-PT"]	= utf8ToWstr("Português (PT)");
		_langList["ro"]		= utf8ToWstr("Română");
		_langList["ru"]		= utf8ToWstr("Русский");
		_langList["sk"]		= utf8ToWstr("Slovenčina");
		_langList["sl"]		= utf8ToWstr("Slovenščina");
		_langList["sv"]		= utf8ToWstr("Svenska");
		_langList["th"]		= utf8ToWstr("ไทย");
		_langList["tr"]		= utf8ToWstr("Türkçe");
		_langList["uk"]		= utf8ToWstr("Українська");
		_langList["zh-CN"]	= utf8ToWstr("中文");
		_langList["zh-TW"]	= utf8ToWstr("文言");
	}

	if (_rtl.empty() == true)
		_rtl.push_back("he");

	if (_cjk.empty() == true)
	{
		_cjk.push_back("ja");
//		_cjk.push_back("ko"); // has spacing between words
		_cjk.push_back("zh-CN");
		_cjk.push_back("zh-TW");
	}
}

/**
 * dTor.
 */
Language::~Language()
{
	delete _handler;
}

/**
 * Takes a wide-character string and converts it to a 8-bit string encoded in
 * UTF-8.
 * @note Adapted from
 * http://stackoverflow.com/questions/148403/utf8-to-from-wide-char-conversion-in-stl
 * @param src - reference to a wide-character string
 * @return, UTF-8 string
 */
std::string Language::wstrToUtf8(const std::wstring& src) // static.
{
	if (src.empty() == true)
		return "";

#ifdef _WIN32
	int bytes (WideCharToMultiByte(
								CP_UTF8,
								0,
								&src[0u],
								static_cast<int>(src.size()),
								nullptr,
								0,
								nullptr,
								nullptr));
	std::string st (static_cast<size_t>(bytes), 0);
	WideCharToMultiByte(
					CP_UTF8,
					0,
					&src[0u],
					static_cast<int>(src.size()),
					&st[0u],
					bytes,
					nullptr,
					nullptr);
	return st;
#else
	std::string out;
	unsigned codepoint (0);

	for (std::wstring::const_iterator
			i = src.begin();
			i != src.end();
			++i)
	{
		wchar_t ch (*i);
		if (ch >= 0xd800 && ch <= 0xdbff)
			codepoint = ((ch - 0xd800) << 10) + 0x10000;
		else
		{
			if (ch >= 0xdc00 && ch <= 0xdfff)
				codepoint |= ch - 0xdc00;
			else
				codepoint = ch;

			if (codepoint <= 0x7f)
				out.append(1, static_cast<char>(codepoint));
			else if (codepoint <= 0x7ff)
			{
				out.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
				out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
			else if (codepoint <= 0xffff)
			{
				out.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
				out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
				out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
			else
			{
				out.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
				out.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
				out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
				out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
			codepoint = 0;
		}
	}
	return out;
#endif
}

/**
 * Takes a wide-character string and converts it to an 8-bit string encoded in
 * the current system codepage.
 * @param src - reference to a wide-character string
 * @return, codepage string
 */
std::string Language::wstrToCp(const std::wstring& src) // static.
{
	if (src.empty() == true)
		return "";

#ifdef _WIN32
	int bytes (WideCharToMultiByte(
								CP_ACP,
								0,
								&src[0u],
								static_cast<int>(src.size()),
								nullptr,
								0,
								nullptr,
								nullptr));
	std::string st (static_cast<size_t>(bytes), 0);
	WideCharToMultiByte(
					CP_ACP,
					0,
					&src[0u],
					static_cast<int>(src.size()),
					&st[0u],
					bytes,
					nullptr,
					nullptr);
	return st;
#else
	const int MAX (500);
	char buffer[MAX];
	setlocale(LC_ALL, "");
	wcstombs(buffer, src.c_str(), MAX);
	setlocale(LC_ALL, "C");
	std::string st(buffer);
	return st;
#endif
}

/**
 * Takes a wide-character string and converts it to an 8-bit string with the
 * filesystem encoding.
 * @param src - reference to a wide-character string
 * @return, filesystem string
 */
std::string Language::wstrToFs(const std::wstring& src) // static.
{
#ifdef _WIN32
	return Language::wstrToCp(src);
#else
	return Language::wstrToUtf8(src);
#endif
}

/**
 * Takes an 8-bit string encoded in UTF-8 and converts it to a wide-character
 * string.
 * @note Adapted from
 * http://stackoverflow.com/questions/148403/utf8-to-from-wide-char-conversion-in-stl
 * @param src - reference to a UTF-8 string
 * @return, wide-character string
 */
std::wstring Language::utf8ToWstr(const std::string& src) // static.
{
	if (src.empty() == true)
		return L"";

#ifdef _WIN32
	int bytes (MultiByteToWideChar(
								CP_UTF8,
								0,
								&src[0u],
								static_cast<int>(src.size()),
								nullptr,
								0));
	std::wstring wst (static_cast<size_t>(bytes), 0);
	MultiByteToWideChar(
					CP_UTF8,
					0,
					&src[0u],
					static_cast<int>(src.size()),
					&wst[0u],
					bytes);
	return wst;
#else
	std::wstring out;
	unsigned codepoint (0);
	int following (0);

	for (std::string::const_iterator
			i = src.begin();
			i != src.end();
			++i)
	{
		unsigned char ch (*i);
		if (ch <= 0x7f)
		{
			codepoint = ch;
			following = 0;
		}
		else if (ch <= 0xbf)
		{
			if (following > 0)
			{
				codepoint = (codepoint << 6) | (ch & 0x3f);
				--following;
			}
		}
		else if (ch <= 0xdf)
		{
			codepoint = ch & 0x1f;
			following = 1;
		}
		else if (ch <= 0xef)
		{
			codepoint = ch & 0x0f;
			following = 2;
		}
		else
		{
			codepoint = ch & 0x07;
			following = 3;
		}
		if (following == 0)
		{
			if (codepoint > 0xffff)
			{
				out.append(1, static_cast<wchar_t>(0xd800 + (codepoint >> 10)));
				out.append(1, static_cast<wchar_t>(0xdc00 + (codepoint & 0x03ff)));
			}
			else
				out.append(1, static_cast<wchar_t>(codepoint));

			codepoint = 0;
		}
	}
	return out;
#endif
}

/**
 * Takes an 8-bit string encoded in the current system codepage and converts it
 * to a wide-character string.
 * @param src - reference to a codepage string
 * @return, wide-character string
 */
std::wstring Language::cpToWstr(const std::string& src) // static.
{
	if (src.empty() == true)
		return L"";

#ifdef _WIN32
	int bytes (MultiByteToWideChar(
								CP_ACP,
								0,
								&src[0u],
								static_cast<int>(src.size()),
								nullptr,
								0));
	std::wstring wst (static_cast<size_t>(bytes), 0);
	MultiByteToWideChar(
					CP_ACP,
					0,
					&src[0u],
					static_cast<int>(src.size()),
					&wst[0u],
					bytes);

	return wst;
#else
	const int MAX = 500;
	wchar_t buffer[MAX + 1];
	setlocale(
			LC_ALL,
			"");
	size_t len = mbstowcs(
						buffer,
						src.c_str(),
						MAX);
	setlocale(
			LC_ALL,
			"C");

	if (len == (size_t)-1)
		return L"?";

	return std::wstring(buffer, len);
#endif
}

/**
 * Takes an 8-bit string with the filesystem encoding and converts it to a
 * wide-character string.
 * @param src - reference to a filesystem string
 * @return, wide-character string
 */
std::wstring Language::fsToWstr(const std::string& src) // static.
{
#ifdef _WIN32
	return Language::cpToWstr(src);
#else
	return Language::utf8ToWstr(src);
#endif
}

/**
 * Replaces every instance of a substring.
 * @param st		- reference to the string to modify
 * @param stPre		- reference to the substring to find
 * @param stPost	- reference to the substring to replace it with
 */
void Language::replace( // static.
		std::string& st,
		const std::string& stPre,
		const std::string& stPost)
{
	for (size_t
			i = st.find(stPre);
			i != std::string::npos;
			i = st.find(
					stPre,
					i + stPost.length()))
	{
		st.replace(
				i,
				stPre.length(),
				stPost);
	}
}

/**
 * Replaces every instance of a substring.
 * @param wst		- reference to the string to modify
 * @param wstPre	- reference to the substring to find
 * @param wstPost	- reference to the substring to replace it with
 */
void Language::replace( // static.
		std::wstring& wst,
		const std::wstring& wstPre,
		const std::wstring& wstPost)
{
	for (size_t
			i = wst.find(wstPre);
			i != std::wstring::npos;
			i = wst.find(
					wstPre,
					i + wstPost.length()))
	{
		wst.replace(
				i,
				wstPre.length(),
				wstPost);
	}
}

/**
 * Gets all the languages found in the Data folder and returns their properties.
 * @note Used in OptionsVideoState cTor.
 * @param files		- reference to a vector of language files
 * @param languages	- reference to a vector of languages (translated)
 */
void Language::getList( // static.
		std::vector<std::string>& files,
		std::vector<std::wstring>& languages)
{
	files = CrossPlatform::getFolderContents(CrossPlatform::getDataFolder("Language/"), "yml");
	languages.clear();

	std::wstring wst;

	for (std::vector<std::string>::iterator
			i = files.begin();
			i != files.end();
			++i)
	{
		*i = CrossPlatform::noExt(*i);
		std::map<std::string, std::wstring>::const_iterator pLang (_langList.find(*i));

		if (pLang != _langList.end())
			wst = pLang->second;
		else
			wst = Language::fsToWstr(*i);

		languages.push_back(wst);
	}
}

/**
 * Loads a language file in Ruby-on-Rails YAML format.
 * @note Not that this has anything to do with Ruby but since it's a
 * widely-supported format and we already have YAML it was convenient.
 * @param file		- reference a YAML file
 * @param extras	- pointer to extra-strings from that rule
 */
void Language::load(
		const std::string& file,
		ExtraStrings* const extras)
{
	_strings.clear();

	YAML::Node doc = YAML::LoadFile(file);
	YAML::Node lang;

	if (doc.begin()->second.IsMap()) // well-formed language rule
	{
		_id = doc.begin()->first.as<std::string>();
		lang = doc.begin()->second;
	}
	else // fallback when file is missing its language specifier
	{
		_id = CrossPlatform::noExt(CrossPlatform::baseFilename(file));
		lang = doc;
	}

	std::string val;

	for (YAML::const_iterator
			i = lang.begin();
			i != lang.end();
			++i)
	{
		if (i->second.IsMap() == true) // strings with plurality
		{
			for (YAML::const_iterator
					j = i->second.begin();
					j != i->second.end();
					++j)
			{
				if ((val = j->second.as<std::string>()).empty() == false)
					_strings[i->first.as<std::string>() + "_" + j->first.as<std::string>()] = loadString(val);
			}
		}
		else if (i->second.IsScalar() == true // regular strings
			&& (val = i->second.as<std::string>()).empty() == false)
		{
			_strings[i->first.as<std::string>()] = loadString(val);
		}
	}

	if (extras != nullptr)
	{
		for (std::map<std::string, std::string>::const_iterator
				i = extras->getStrings()->begin();
				i != extras->getStrings()->end();
				++i)
		{
			_strings[i->first] = loadString(i->second);
		}
	}

	delete _handler;
	_handler = LanguagePlurality::create(_id);

	if (std::find(
				_rtl.begin(),
				_rtl.end(),
				_id) == _rtl.end())
	{
		_direction = DIRECTION_LTR;
	}
	else
		_direction = DIRECTION_RTL;

	if (std::find(
				_cjk.begin(),
				_cjk.end(),
				_id) == _cjk.end())
	{
		_wrap = WRAP_WORDS;
	}
	else
		_wrap = WRAP_LETTERS;
}

/**
 * Replaces all special string markers with the appropriate characters and
 * converts the string encoding.
 * @param stIn - reference to the original UTF-8 string
 * @return, new widechar-string
 */
std::wstring Language::loadString(const std::string& stIn) const // private.
{
	std::string stOut (stIn);

	replace(stOut, "{NEWLINE}",	  "\n");
	replace(stOut, "{SMALLLINE}", "\x02");
	replace(stOut, "{ALT}",		  "\x01");

	return utf8ToWstr(stOut);
}

/**
 * Gets the Language's locale.
 * @return, IANA language tag
 */
std::string Language::getId() const
{
	return _id;
}

/**
 * Returns this Language's label in its native language.
 * @return, translation as a wide-string
 */
std::wstring Language::getLabel() const
{
	return _langList[_id];
}

/**
 * Gets the LocalizedText of the specified ID.
 * @note If not found return the ID itself.
 * @param id - reference to the string-ID
 * @return, reference to LocalizedText (wide-string) of the requested ID
 */
const LocalizedText& Language::getString(const std::string& id) const
{
	static LocalizedText hack (L""); // why. Because (without it) this funct returns a ref to a temporary var.

	if (id.empty() == true)
	{
		hack = LocalizedText(L"");
		return hack;
	}

	const std::map<std::string, LocalizedText>::const_iterator pSt (_strings.find(id));
	if (pSt == _strings.end())
	{
		hack = getString(id, std::numeric_limits<unsigned>::max());
		return hack;
	}
	return pSt->second;
}

/**
 * Gets the LocalizedText with the specified ID in the proper form for @a qty.
 * @note The substitution of @a qty has already happened in the returned
 * LocalizedText. If not found return the ID itself.
 * @param id	- reference to the string-ID
 * @param qty	- number to use to decide the proper form
 * @return, LocalizedText (widestring) of the requested ID
 */
LocalizedText Language::getString(
		const std::string& id,
		unsigned qty) const
{
	assert(id.empty() == false);

	static std::set<std::string> notFounds; // container to check errors against so log won't Hormel.

	std::map<std::string, LocalizedText>::const_iterator pSt (_strings.end());
	if (qty == 0)								// Try specialized form
		pSt = _strings.find(id + "_zero");

	if (pSt == _strings.end())					// Try proper form by language
		pSt = _strings.find(id + _handler->getSuffix(qty));

	if (pSt == _strings.end())					// Try default form
		pSt = _strings.find(id + "_other");

	if (pSt == _strings.end())					// Give up
	{
		if (notFounds.find(id) == notFounds.end())
		{
			notFounds.insert(id);
			Log(LOG_WARNING) << id << " not found in " << Options::language;
		}
		return LocalizedText(utf8ToWstr(id));
	}

	if (qty == std::numeric_limits<unsigned>::max()) // Special case, passed by getString(id) above^
	{
		if (notFounds.find(id) == notFounds.end())
		{
			notFounds.insert(id);
			Log(LOG_WARNING) << id << " has plural format in [" << Options::language << "]. Code assumes singular format.";
//			Hint: Change "getstring(ID).arg(value)" to "getString(ID, value)" in appropriate files.
		}
		return pSt->second;
	}

	std::wostringstream woststr;
	woststr << qty;

	std::wstring
		wst (pSt->second),
		marker (L"{N}"),
		val (woststr.str());
	replace(
		wst,
		marker,
		val);

	return wst;
}

/**
 * Gets the LocalizedText with the specified ID in the proper form for the
 * gender.
 * @note If not found return the ID itself.
 * @param id		- reference to the string-ID
 * @param gender	- current soldier gender
 * @return, reference to LocalizedText (widestring)
 */
const LocalizedText& Language::getString(
		const std::string& id,
		SoldierGender gender) const
{
	std::string genderId;
	switch (gender)
	{
		default:
		case GENDER_MALE:
			genderId = id + "_MALE";
			break;

		case GENDER_FEMALE:
			genderId = id + "_FEMALE";
	}
	return getString(genderId);
}

/**
 * Outputs all the language IDs and strings to an HTML table.
 * @param file - reference to HTML file
 *
void Language::toHtml(const std::string& file) const
{
	std::ofstream fileHtml (file.c_str(), std::ios::out);
	fileHtml << "<table border=\"1\" width=\"100%\">" << std::endl;
	fileHtml << "<tr><th>ID String</th><th>English String</th></tr>" << std::endl;

	for (std::map<std::string, LocalizedText>::const_iterator
			i = _strings.begin();
			i != _strings.end();
			++i)
	{
		fileHtml << "<tr><td>" << i->first << "</td><td>";
		const std::string st = wstrToUtf8(i->second);
		for (std::string::const_iterator
				j = st.begin();
				j != st.end();
				++j)
		{
			if (*j == 2 || *j == '\n')
				fileHtml << "<br />";
			else
				fileHtml << *j;
		}
		fileHtml << "</td></tr>" << std::endl;
	}
	fileHtml << "</table>" << std::endl;
	fileHtml.close();
} */

/**
 * Gets the direction to use for rendering text in this Language.
 * @return, text-direction
 */
TextDirection Language::getTextDirection() const
{
	return _direction;
}

/**
 * Gets the wrapping rules to use for rendering text in this Language.
 * @return, text-wrapping
 */
TextWrapping Language::getTextWrapping() const
{
	return _wrap;
}

}

/**
@page LanguageFiles Format of the language files.

Language files are formatted as YAML (.yml) containing UTF-8 (no BOM) text.
The first line in a language file is the language's identifier.
The rest of the file are key-value pairs. The key of each pair
contains the ID string (dictionary key), and the value contains the localized
text for the given key in quotes.

The LocalizedText may contain the following special markers:
<table>
<tr>
 <td><tt>{</tt><i>0, 1, 2, ...</i> <tt>}</tt></td>
 <td>These markers will be replaced by programmer-supplied values before the
 message is displayed.</td></tr>
<tr>
 <td><tt>{ALT}</tt></td>
 <td>The rest of the text will be in an alternate color. Using this again will
 switch back to the primary color.</td></tr>
<tr>
 <td><tt>{NEWLINE}</tt></td>
 <td>It will be replaced with a line break in the game.</td></tr>
<tr>
 <td><tt>{SMALLLINE}</tt></td>
 <td>The rest of the text will be in a small font.</td></tr>
</table>

There is an additional marker sequence, that should only appear in texts that
depend on a number. This marker <tt>{N}</tt> will be replaced by the actual
number used. The keys for texts that depend on numbers also have special
suffixes, that depend on the language. For all languages, a suffix of
<tt>_zero</tt> is tried if the number is zero, before trying the actual key
according to the language rules. The rest of the suffixes depend on the language,
as described <a href="http://unicode.org/repos/cldr-tmp/trunk/diff/supplemental/language_plural_rules.html">here</a>.

So, you would write (for English):
<pre>
STR_ENEMIES:
  zero:  "There are no enemies left."
  one:   "There is a single enemy left."
  other: "There are {N} enemies left."
</pre>

*/
