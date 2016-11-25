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

#include "ResearchCompleteState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ResearchComplete screen.
 * @param resRulePedia	- pointer to the discovered RuleResearch (will be converted to Ufopaedia article)
 *						  or nullptr if the Ufopaedia article shouldn't popup
 * @param gofRule		- pointer to bonus discovered RuleResearch
 * @param resRule		- pointer to the discovered RuleResearch
 */
ResearchCompleteState::ResearchCompleteState(
		const RuleResearch* const resRulePedia,
		const RuleResearch* const gofRule,
		const RuleResearch* const resRule)
	:
		_resRulePedia(resRulePedia),
		_gofRule(gofRule)
{
	_fullScreen = false;

	_window			= new Window(this, 230, 140, 45, 30, POPUP_BOTH);

	_txtTitle		= new Text(230, 17, 45, 70);
	_txtResearch	= new Text(230, 17, 45, 96);

	_btnReport		= new TextButton(80, 16,  64, 146);
	_btnOk			= new TextButton(80, 16, 176, 146);

	setInterface("geoResearch");

	add(_window,		"window",	"geoResearch");
	add(_txtTitle,		"text1",	"geoResearch");
	add(_txtResearch,	"text2",	"geoResearch");
	add(_btnReport,		"button",	"geoResearch");
	add(_btnOk,			"button",	"geoResearch");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_RESEARCH_COMPLETED"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ResearchCompleteState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchCompleteState::btnOkClick),
							Options::keyCancel);

	if (_resRulePedia != nullptr || _gofRule != nullptr)
	{
		_btnReport->setText(tr("STR_VIEW_REPORTS"));
		_btnReport->onMouseClick(	static_cast<ActionHandler>(&ResearchCompleteState::btnReportClick));
		_btnReport->onKeyboardPress(static_cast<ActionHandler>(&ResearchCompleteState::btnReportClick),
									Options::keyOk);
		_btnReport->onKeyboardPress(static_cast<ActionHandler>(&ResearchCompleteState::btnReportClick),
									Options::keyOkKeypad);
	}
	else
	{
		_btnReport->setVisible(false);

		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchCompleteState::btnOkClick),
								Options::keyOk);
		_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ResearchCompleteState::btnOkClick),
								Options::keyOkKeypad);
	}

	_txtResearch->setText(tr(resRule->getType()));
	_txtResearch->setAlign(ALIGN_CENTER);
	_txtResearch->setBig();
}

/**
 * dTor.
 */
ResearchCompleteState::~ResearchCompleteState()
{}

/**
 * Exits to the previous screen
 * @param action - pointer to an Action
 */
void ResearchCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Ufopaedia to the entry about the Research.
 * @param action - pointer to an Action
 */
void ResearchCompleteState::btnReportClick(Action*)
{
	_game->popState();

	if (_resRulePedia != nullptr)
	{
		std::string resType (_resRulePedia->getUfopaediaEntry()); // strip const.
		Ufopaedia::openArticle(_game, resType);
	}

	if (_gofRule != nullptr)
	{
		std::string gofType (_gofRule->getUfopaediaEntry()); // strip const.
		Ufopaedia::openArticle(_game, gofType);
	}
}

}
