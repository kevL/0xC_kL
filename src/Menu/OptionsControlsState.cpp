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

#include "OptionsControlsState.h"

//#include <SDL.h>

#include "../Engine/Language.h"
#include "../Engine/Options.h"

#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Engine/Action.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Controls screen.
 * @param origin Game section that originated this state.
 */
OptionsControlsState::OptionsControlsState(OptionsOrigin origin)
	:
		OptionsBaseState(origin),
		_selected(-1),
		_selKey(nullptr)
{
	setCategory(_btnControls);

	_lstControls = new TextList(200, 137, 94, 8);

	if (origin != OPT_BATTLESCAPE)
		add(_lstControls, "optionLists", "controlsMenu");
	else
		add(_lstControls, "optionLists", "battlescape");

	centerAllSurfaces();


	_lstControls->setColumns(2, 152, 48);
	_lstControls->setWordWrap();
	_lstControls->setSelectable();
	_lstControls->setBackground(_window);
	_lstControls->onMouseClick(		static_cast<ActionHandler>(&OptionsControlsState::lstControlsClick),
									0u);
	_lstControls->onKeyboardPress(	static_cast<ActionHandler>(&OptionsControlsState::lstControlsKeyPress));
	_lstControls->setFocus(true);
//	_lstControls->onMouseIn(static_cast<ActionHandler>(&OptionsControlsState::txtTooltipIn));
//	_lstControls->onMouseOut(static_cast<ActionHandler>(&OptionsControlsState::txtTooltipOut));
//	_lstControls->setTooltip("STR_CONTROLS_DESC");

	_colorGroup = _lstControls->getSecondaryColor();
	_colorSel = _lstControls->getScrollbarColor();
	_colorNormal = _lstControls->getColor();

	const std::vector<OptionInfo>& options = Options::getOptionInfo();
	for (std::vector<OptionInfo>::const_iterator
			i = options.begin();
			i != options.end();
			++i)
	{
		if (i->type() == OPTION_KEY && i->description().empty() == false)
		{
			if		(i->category() == "STR_GENERAL")		_controlsGeneral.push_back(*i);
			else if	(i->category() == "STR_GEOSCAPE")		_controlsGeo.push_back(*i);
			else if	(i->category() == "STR_BATTLESCAPE")	_controlsBattle.push_back(*i);
		}
	}
}

/**
 * dTor.
 */
OptionsControlsState::~OptionsControlsState()
{}

/**
 * Fills the controls list based on category.
 */
void OptionsControlsState::init()
{
	OptionsBaseState::init();
	_lstControls->clearList();

	_lstControls->addRow(2, tr("STR_GENERAL").c_str(), L"");
	_lstControls->setCellColor(
							0u,0u,
							_colorGroup);

	addControls(_controlsGeneral);

	_lstControls->addRow(2, L"", L"");
	_lstControls->addRow(2, tr("STR_GEOSCAPE").c_str(), L"");
	_lstControls->setCellColor(
							_controlsGeneral.size() + 2u,
							0u,
							_colorGroup);

	addControls(_controlsGeo);

	_lstControls->addRow(2, L"", L"");
	_lstControls->addRow(2, tr("STR_BATTLESCAPE").c_str(), L"");
	_lstControls->setCellColor(
							_controlsGeneral.size() + 2u + _controlsGeo.size() + 2u,
							0u,
							_colorGroup);

	addControls(_controlsBattle);
}

/**
 * Uppercases all the words in a string.
 * @param st - source string
 * @return, destination string
 */
std::string OptionsControlsState::ucWords(std::string st)
{
	if (st.empty() == false)
		st[0u] = static_cast<char>(std::toupper(st[0u]));

	for (size_t
			i = st.find_first_of(' ');
			i != std::string::npos;
			i = st.find_first_of(' ', i + 1u))
	{
		if (st.length() > i + 1u)
			st[i + 1u] = static_cast<char>(std::toupper(st[i + 1u]));
		else
			break;
	}
	return st;
}

/**
 * Adds a bunch of controls to the list.
 * @param keys List of controls.
 */
void OptionsControlsState::addControls(const std::vector<OptionInfo>& keys)
{
	SDLKey* key;
	std::wstring keyLabel;

	for (std::vector<OptionInfo>::const_iterator
			i = keys.begin();
			i != keys.end();
			++i)
	{
		key = i->asKey();
		keyLabel = Language::utf8ToWstr(ucWords(SDL_GetKeyName(*key)));

		if (*key == SDLK_UNKNOWN) keyLabel = L"";
		_lstControls->addRow(
							2,
							tr(i->description()).c_str(),
							keyLabel.c_str());
	}
}

/**
 * Gets the currently selected control.
 * @param sel Selected row.
 * @return, Pointer to option, nullptr if none selected.
 */
OptionInfo* OptionsControlsState::getControl(size_t sel)
{
	if (sel > 0u
		&& sel <= _controlsGeneral.size())
	{
		return &_controlsGeneral[sel - 1u];
	}

	if (sel > _controlsGeneral.size() + 2u
		&& sel <= _controlsGeneral.size() + 2u + _controlsGeo.size())
	{
		return &_controlsGeo[sel - 1u - _controlsGeneral.size() - 2u];
	}

	if (sel > _controlsGeneral.size() + 2u + _controlsGeo.size() + 2u
		&& sel <= _controlsGeneral.size() + 2u + _controlsGeo.size() + 2u + _controlsBattle.size())
	{
		return &_controlsBattle[sel - 1u - _controlsGeneral.size() - 2u - _controlsGeo.size() - 2u];
	}

	return nullptr;
}

/**
 * Select a control for changing.
 * @param action - pointer to an Action
 */
void OptionsControlsState::lstControlsClick(Action* action)
{
	const Uint8 btn (action->getDetails()->button.button);
	switch (btn)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
			if (_selected != -1)
			{
				size_t selected (static_cast<size_t>(_selected));

				_lstControls->setCellColor(
										static_cast<size_t>(_selected),
										0u,
										_colorNormal);
				_lstControls->setCellColor(
										static_cast<size_t>(_selected),
										1u,
										_colorNormal);
				_selected = -1;
				_selKey = nullptr;

				if (selected == _lstControls->getSelectedRow())
					return;
			}

			_selected = static_cast<int>(_lstControls->getSelectedRow());
			_selKey = getControl(static_cast<size_t>(_selected));

			if (_selKey == nullptr)
			{
				_selected = -1;
				return;
			}

			switch (btn)
			{
				case SDL_BUTTON_LEFT:
					_lstControls->setCellColor(
											static_cast<size_t>(_selected),
											0u,
											_colorSel);
					_lstControls->setCellColor(
											static_cast<size_t>(_selected),
											1u,
											_colorSel);
					break;

				case SDL_BUTTON_RIGHT:
					_lstControls->setCellText(
											static_cast<size_t>(_selected),
											1u, L"");
					*_selKey->asKey() = SDLK_UNKNOWN;
					_selected = -1;
					_selKey = nullptr;
			}
	}
}

/**
 * Change selected control.
 * @param action - pointer to an Action
 */
void OptionsControlsState::lstControlsKeyPress(Action* action)
{
	if (_selected != -1)
	{
		SDLKey key (action->getDetails()->key.keysym.sym);
		if (key != SDLK_UNKNOWN)
		{
			*_selKey->asKey() = key;
			_lstControls->setCellText(
									static_cast<size_t>(_selected),
									1u,
									Language::utf8ToWstr(ucWords(SDL_GetKeyName(*_selKey->asKey()))));
		}

		_lstControls->setCellColor(
								static_cast<size_t>(_selected),
								0u,
								_colorNormal);
		_lstControls->setCellColor(
								static_cast<size_t>(_selected),
								1u,
								_colorNormal);
		_selected = -1;
		_selKey = nullptr;
	}
}

}
