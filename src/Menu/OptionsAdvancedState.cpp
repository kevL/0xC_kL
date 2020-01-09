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

#include "OptionsAdvancedState.h"

//#include <algorithm>
//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Advanced Options window.
 * @param origin - game section that originated this state
 */
OptionsAdvancedState::OptionsAdvancedState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnAdvanced);

	_lstOptions = new TextList(200, 137, 94, 8);

	if (origin != OPT_BATTLESCAPE)
		add(_lstOptions, "optionLists", "advancedMenu");
	else
		add(_lstOptions, "optionLists", "battlescape");

	centerSurfaces();

	Text text = Text(100, 9); // how much room do we need for YES/NO
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());
	text.setText(tr("STR_YES"));
	int yes = text.getTextWidth();
	text.setText(tr("STR_NO"));
	int no = text.getTextWidth();

	int rightcol = std::max(yes, no) + 2;
	int leftcol = _lstOptions->getWidth() - rightcol;

	_lstOptions->setAlign(ALIGN_RIGHT, 1);
	_lstOptions->setColumns(2, leftcol, rightcol);
//	_lstOptions->setColor(Palette::blockOffset(8)+10);
//	_lstOptions->setArrowColor(Palette::blockOffset(8)+5);
	_lstOptions->setWordWrap();
	_lstOptions->setSelectable();
	_lstOptions->setBackground(_window);
	_lstOptions->onMouseClick(	static_cast<ActionHandler>(&OptionsAdvancedState::lstOptionsClick),
								0u);
	_lstOptions->onMouseOver(	static_cast<ActionHandler>(&OptionsAdvancedState::lstOptionsMouseOver));
	_lstOptions->onMouseOut(	static_cast<ActionHandler>(&OptionsAdvancedState::lstOptionsMouseOut));

	_colorGroup = _lstOptions->getSecondaryColor();

//	_settingBoolSet.push_back(std::pair<std::string, bool*>("battleRangeBasedAccuracy", &Options::battleRangeBasedAccuracy)); // kL


	const std::vector<OptionInfo>& options (Options::getOptionInfo());
	for (std::vector<OptionInfo>::const_iterator
			i = options.begin();
			i != options.end();
			++i)
	{
		if (i->type() != OPTION_KEY && i->description().empty() == false)
		{
			if		(i->category() == "STR_GENERAL")		_settingsGeneral.push_back(*i);
			else if	(i->category() == "STR_GEOSCAPE")		_settingsGeo.push_back(*i);
			else if	(i->category() == "STR_BATTLESCAPE")	_settingsBattle.push_back(*i);
		}
	}
}

/**
 * dTor.
 */
OptionsAdvancedState::~OptionsAdvancedState()
{}

/**
 * Fills the settings list based on category.
 */
void OptionsAdvancedState::init()
{
	OptionsBaseState::init();
	_lstOptions->clearList();

	_lstOptions->addRow(2, tr("STR_GENERAL").c_str(), L"");
	_lstOptions->setCellColor(
							0u,0u,
							_colorGroup);

	addSettings(_settingsGeneral);

	_lstOptions->addRow(2, L"", L"");
	_lstOptions->addRow(2, tr("STR_GEOSCAPE").c_str(), L"");
	_lstOptions->setCellColor(
							_settingsGeneral.size() + 2u,
							0u,
							_colorGroup);

	addSettings(_settingsGeo);

	_lstOptions->addRow(2, L"", L"");
	_lstOptions->addRow(2, tr("STR_BATTLESCAPE").c_str(), L"");
	_lstOptions->setCellColor(
							_settingsGeneral.size() + 2u + _settingsGeo.size() + 2u,
							0u,
							_colorGroup);

	addSettings(_settingsBattle);
}

/**
 * Adds a bunch of settings to the list.
 * @param settings List of settings.
 */
void OptionsAdvancedState::addSettings(const std::vector<OptionInfo>& settings) // private.
{
	for (std::vector<OptionInfo>::const_iterator
			i = settings.begin();
			i != settings.end();
			++i)
	{
		std::wstring value;

		if (i->type() == OPTION_BOOL)
			value = *i->asBool() ? tr("STR_YES") : tr("STR_NO");
		else if (i->type() == OPTION_INT)
		{
			std::wostringstream woststr;
			woststr << *i->asInt();
			value = woststr.str();
		}

		_lstOptions->addRow(
						2,
						tr(i->description()).c_str(),
						value.c_str());
	}
}

/**
 * Gets the currently selected setting.
 * @param sel Selected row.
 * @return, Pointer to option, nullptr if none selected.
 */
OptionInfo* OptionsAdvancedState::getSetting(size_t sel) // private.
{
	if (sel > 0u
		&& sel <= _settingsGeneral.size())
	{
		return &_settingsGeneral[sel - 1u];
	}

	if (sel > _settingsGeneral.size() + 2u
		&& sel <= _settingsGeneral.size() + 2u + _settingsGeo.size())
	{
		return &_settingsGeo[sel - 1u - _settingsGeneral.size() - 2u];
	}

	if (sel > _settingsGeneral.size() + 2u + _settingsGeo.size() + 2u
		&& sel <= _settingsGeneral.size() + 2u + _settingsGeo.size() + 2u + _settingsBattle.size())
	{
		return &_settingsBattle[sel - 1u - _settingsGeneral.size() - 2u - _settingsGeo.size() - 2u];
	}

	return nullptr;
}

/**
 * Changes the clicked setting.
 * @param action - pointer to an Action
 */
void OptionsAdvancedState::lstOptionsClick(Action* action)
{
	Uint8 button (action->getDetails()->button.button);
	switch (button)
	{
		case SDL_BUTTON_LEFT:
		case SDL_BUTTON_RIGHT:
		{
			size_t sel (_lstOptions->getSelectedRow());
			OptionInfo* setting (getSetting(sel));
			if (setting != nullptr)
			{
				std::wstring settingText;
				switch (setting->type())
				{
					case OPTION_BOOL:
					{
						bool* b (setting->asBool());
						*b = !*b;
						settingText = *b ? tr("STR_YES") : tr("STR_NO");
						break;
					}

					case OPTION_INT: // integer variables will need special handling
					{
						int* i (setting->asInt());

						int increment ((button == SDL_BUTTON_LEFT) ? 1 : -1); // left-click increases, right-click decreases
						if (   i == &Options::FPS)
//							|| i == &Options::FPSUnfocused)
						{
							increment *= 10;
						}
						*i += increment;

						int
							minVal,
							maxVal;
						if (i == &Options::battleExplosionHeight)
						{
							minVal = 0;
							maxVal = 3;
						}
						else if (i == &Options::FPS)
						{
							minVal = 0;
							maxVal = 120;
						}
//						else if (i == &Options::FPSUnfocused)
//						{
//							minVal = 10;
//							maxVal = 120;
//						}
						else if (i == &Options::mousewheelSpeed)
						{
							minVal = 1;
							maxVal = 7;
						}
						else if (i == &Options::autosaveFrequency)
						{
							minVal = 1;
							maxVal = 5;
						}
						else
						{
							minVal =
							maxVal = 0;
						}


						if		(*i < minVal) *i = maxVal;
						else if	(*i > maxVal) *i = minVal;

						std::wostringstream woststr;
						woststr << *i;
						settingText = woststr.str();
					}
				}

				_lstOptions->setCellText(
									sel,
									1u,
									settingText);
			}
		}
	}
}

/**
 *
 * @param action - pointer to an Action
 */
void OptionsAdvancedState::lstOptionsMouseOver(Action*)
{
	std::wstring desc;
	OptionInfo* setting (getSetting(_lstOptions->getSelectedRow()));
	if (setting != nullptr)
		desc = tr(setting->description() + "_DESC");

	_txtTooltip->setText(desc);
}

/**
 *
 * @param action - pointer to an Action
 */
void OptionsAdvancedState::lstOptionsMouseOut(Action*)
{
	_txtTooltip->setText(L"");
}

}
