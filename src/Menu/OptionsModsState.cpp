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

#include "OptionsModsState.h"

//#include <algorithm>

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Mod Options window.
 * @param origin Game section that originated this state.
 */
OptionsModsState::OptionsModsState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnMods);

	_lstMods = new TextList(200, 137, 94, 8);
	add(_lstMods, "optionLists", "modsMenu");

	centerSurfaces();


	Text text (Text(100, 9)); // how much room do you need for YES/NO
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());
	text.setText(tr("STR_YES"));
	text.setText(tr("STR_NO"));

	const int
		yes (text.getTextWidth()),
		no  (text.getTextWidth()),
		rightcol (std::max(yes, no) + 2),
		leftcol (_lstMods->getWidth() - rightcol);

	_lstMods->setAlign(ALIGN_RIGHT, 1);
	_lstMods->setColumns(2, leftcol, rightcol);
	_lstMods->setWordWrap();
	_lstMods->setSelectable();
	_lstMods->setBackground(_window);
	_lstMods->onMouseClick(	static_cast<ActionHandler>(&OptionsModsState::lstModsClick));
//	_lstMods->onMouseIn(	static_cast<ActionHandler>(&OptionsModsState::txtTooltipIn));
//	_lstMods->onMouseOut(	static_cast<ActionHandler>(&OptionsModsState::txtTooltipOut));
//	_lstMods->setTooltip("STR_MODS_DESC");

	std::string
		file,
		ruleset;
	std::wstring rulesetLabel;
	std::vector<std::string> rulesets (CrossPlatform::getDataContents("Ruleset/"));
	for (std::vector<std::string>::const_iterator
			i = rulesets.begin();
			i != rulesets.end();
			++i)
	{
		file = *i;
		std::transform(
					file.begin(),
					file.end(),
					file.begin(),
					tolower);

		if ((file.length() > 4u && file.substr(file.length() - 4u, 4u) == ".rul")
			|| CrossPlatform::getDataContents("Ruleset/" + *i, "rul").empty() == false)
		{
			ruleset = CrossPlatform::noExt(*i);
			rulesetLabel = Language::fsToWstr(ruleset);
			Language::replace(
							rulesetLabel,
							L"_",
							L" ");

//			if (ruleset != "Xcom1Ruleset") // ignore default ruleset
//			{
			const bool ruleEnabled ((std::find(
											Options::rulesets.begin(),
											Options::rulesets.end(),
											ruleset) != Options::rulesets.end()));
			_lstMods->addRow(
						2,
						rulesetLabel.c_str(),
						(ruleEnabled == true) ? tr("STR_YES").c_str() : tr("STR_NO").c_str());

			_mods.push_back(ruleset);
//			}
		}
	}
}

/**
 * dTor.
 */
OptionsModsState::~OptionsModsState()
{}

void OptionsModsState::lstModsClick(Action*)
{
	std::string selectedRuleset (_mods[_lstMods->getSelectedRow()]);
	std::vector<std::string>::const_iterator i (std::find(
														Options::rulesets.begin(),
														Options::rulesets.end(),
														selectedRuleset));
	if (i != Options::rulesets.end())
	{
		_lstMods->setCellText(
							_lstMods->getSelectedRow(),
							1u,
							tr("STR_NO"));
		Options::rulesets.erase(i);
	}
	else
	{
		_lstMods->setCellText(
							_lstMods->getSelectedRow(),
							1u,
							tr("STR_YES"));
		Options::rulesets.push_back(selectedRuleset);
	}
	Options::reload = true;
}

}
