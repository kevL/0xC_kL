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

#include "OptionInfo.h"

#include <algorithm>

#include "Exception.h"


namespace OpenXcom
{

/**
 * Creates info for a boolean option.
 * @param id		- reference to the string ID used in serializing
 * @param option	- pointer to the Option
 * @param def		- default option value (bool)
 * @param desc		- reference to the Language ID for the option description (if any)
 * @param cat		- reference to the Language ID for the option category (if any)
 */
OptionInfo::OptionInfo(
		const std::string& id,
		bool* option,
		bool def,
		const std::string& desc,
		const std::string& cat)
	:
		_id(id),
		_desc(desc),
		_cat(cat),
		_type(OPTION_BOOL)
{
	_ref.b = option;
	_def.b = def;
}

/**
 * Creates info for an integer option.
 * @param id		- reference to the string ID used in serializing
 * @param option	- pointer to the Option
 * @param def		- default option value (int)
 * @param desc		- reference to the Language ID for the option description (if any)
 * @param cat		- reference to the Language ID for the option category (if any)
 */
OptionInfo::OptionInfo(
		const std::string& id,
		int* option,
		int def,
		const std::string& desc,
		const std::string& cat)
	:
		_id(id),
		_desc(desc),
		_cat(cat),
		_type(OPTION_INT)
{
	_ref.i = option;
	_def.i = def;
}

/**
 * Creates info for a keyboard shortcut option.
 * @param id		- reference to the string ID used in serializing
 * @param option	- pointer to the Option
 * @param def		- default option value (SDLKey)
 * @param desc		- reference to the Language ID for the option description (if any)
 * @param cat		- reference to the Language ID for the option category (if any)
 */
OptionInfo::OptionInfo(
		const std::string& id,
		SDLKey* option,
		SDLKey def,
		const std::string& desc,
		const std::string& cat)
	:
		_id(id),
		_desc(desc),
		_cat(cat),
		_type(OPTION_KEY)
{
	_ref.k = option;
	_def.k = def;
}

/**
 * Creates info for a string option.
 * @param id		- reference to the string ID used in serializing
 * @param option	- pointer to the Option
 * @param def		- default option value (string)
 * @param desc		- reference to the Language ID for the option description (if any)
 * @param cat		- reference to the Language ID for the option category (if any)
 */
OptionInfo::OptionInfo(
		const std::string& id,
		std::string* option,
		const char* def,
		const std::string& desc,
		const std::string& cat)
	:
		_id(id),
		_desc(desc),
		_cat(cat),
		_type(OPTION_STRING)
{
	_ref.s = option;
	_def.s = def;
}

/**
 * Loads an option value from the corresponding YAML.
 * @param node - reference to the Options YAML node
 */
void OptionInfo::load(const YAML::Node& node) const
{
	switch (_type)
	{
		case OPTION_BOOL:
			*_ref.b = node[_id].as<bool>(_def.b);
			break;
		case OPTION_INT:
			*_ref.i = node[_id].as<int>(_def.i);
			break;
		case OPTION_KEY:
			*_ref.k = static_cast<SDLKey>(node[_id].as<int>(_def.k));
			break;
		case OPTION_STRING:
			*_ref.s = node[_id].as<std::string>(_def.s);
	}
}

/**
 * Loads an option value from the corresponding map (eg. for command-line options).
 * @param optionsMap - reference to the Options map (string,string)
 */
void OptionInfo::load(const std::map<std::string, std::string>& optionsMap) const
{
	std::string id (_id);
	std::transform(
				id.begin(),
				id.end(),
				id.begin(),
				::tolower);
	std::map<std::string, std::string>::const_iterator it (optionsMap.find(id));
	if (it != optionsMap.end())
	{
		std::string value (it->second);
		std::stringstream ststr;
		bool b;
		int i;
		switch (_type)
		{
			case OPTION_BOOL:
				ststr << std::boolalpha << value;
				ststr >> std::boolalpha >> b;
				*_ref.b = b;
				break;
			case OPTION_INT:
				ststr << std::dec << value;
				ststr >> std::dec >> i;
				*_ref.i = i;
				break;
			case OPTION_KEY:
				ststr << std::dec << value;
				ststr >> std::dec >> i;
				*_ref.k = static_cast<SDLKey>(i);
				break;
			case OPTION_STRING:
				*_ref.s = value;
		}
	}
}

/**
 * Saves an option value to the corresponding YAML.
 * @param node - reference an Options YAML node
 */
void OptionInfo::save(YAML::Node& node) const
{
	switch (_type)
	{
		case OPTION_BOOL:
			node[_id] = *_ref.b;
			break;
		case OPTION_INT:
			node[_id] = *_ref.i;
			break;
		case OPTION_KEY:
			node[_id] = static_cast<int>(*_ref.k);
			break;
		case OPTION_STRING:
			node[_id] = *_ref.s;
	}
}

/**
 * Resets an option back to its default value.
 */
void OptionInfo::reset() const
{
	switch (_type)
	{
		case OPTION_BOOL:
			*_ref.b = _def.b;
			break;
		case OPTION_INT:
			*_ref.i = _def.i;
			break;
		case OPTION_KEY:
			*_ref.k = _def.k;
			break;
		case OPTION_STRING:
			*_ref.s = _def.s;
	}
}

/**
 * Returns the variable type of the option.
 * @return, Option type
 */
OptionType OptionInfo::type() const
{
	return _type;
}

/**
 * Returns the description of the option.
 * @note Options with descriptions show up in the Options screens.
 * @return, Language string ID for the description
 */
std::string OptionInfo::description() const
{
	return _desc;
}

/**
 * Returns the category of the option.
 * @note Options with categories show up in the Options screens.
 * @return, Language string ID for the category
 */
std::string OptionInfo::category() const
{
	return _cat;
}

/**
 * Returns the pointer to the boolean option.
 * @note Throws an exception if it's not a boolean.
 * @return, pointer to the Option
 */
bool* OptionInfo::asBool() const
{
	if (_type != OPTION_BOOL)
		throw Exception(_id + " is not a boolean!");

	return _ref.b;
}

/**
 * Returns the pointer to the integer option.
 * @note Throws an exception if it's not a integer.
 * @return, pointer to the Option
 */
int* OptionInfo::asInt() const
{
	if (_type != OPTION_INT)
		throw Exception(_id + " is not an integer!");

	return _ref.i;
}

/**
 * Returns the pointer to the key option.
 * @note Throws an exception if it's not a key.
 * @return, pointer to the Option
 */
SDLKey* OptionInfo::asKey() const
{
	if (_type != OPTION_KEY)
		throw Exception(_id + " is not a key!");

	return _ref.k;
}

/**
 * Returns the pointer to the string option.
 * @note Throws an exception if it's not a string.
 * @return, pointer to the Option
 */
std::string* OptionInfo::asString() const
{
	if (_type != OPTION_STRING)
		throw Exception(_id + " is not a string!");

	return _ref.s;
}

}
