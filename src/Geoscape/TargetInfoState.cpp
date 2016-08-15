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

#include "TargetInfoState.h"

#include "GeoscapeState.h"
#include "InterceptState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the TargetInfo window.
 * @param target	- pointer to a Target to show info about
 * @param geoState	- pointer to GeoscapeState
 */
TargetInfoState::TargetInfoState(
		Target* const target,
		GeoscapeState* const geoState)
	:
		_target(target),
		_geoState(geoState),
		_aBase(nullptr)
{
	_fullScreen = false;

	_window			= new Window(this, 192, 120, 32, 40, POPUP_BOTH);
	_txtTitle		= new Text(182, 17, 37, 54);

	_edtTarget		= new TextEdit(this, 50, 9, 38, 46);

	_txtTargeted	= new Text(182,  9, 37, 71);
	_txtTargeters	= new Text(182, 40, 37, 82);

	_btnIntercept	= new TextButton(160, 16, 48, 119);
	_btnOk			= new TextButton(160, 16, 48, 137);

	setInterface("targetInfo");

	add(_window,		"window",	"targetInfo");
	add(_txtTitle,		"text",		"targetInfo");
	add(_edtTarget,		"text",		"targetInfo");
	add(_txtTargeted,	"text",		"targetInfo");
	add(_txtTargeters,	"text",		"targetInfo");
	add(_btnIntercept,	"button",	"targetInfo");
	add(_btnOk,			"button",	"targetInfo");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick(static_cast<ActionHandler>(&TargetInfoState::btnInterceptClick));

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TargetInfoState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TargetInfoState::btnOkClick),
							Options::keyCancel);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TargetInfoState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TargetInfoState::btnOkClick),
							Options::keyOkKeypad);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	std::wostringstream woststr;
	woststr << L'\x01' << _target->getLabel(_game->getLanguage());
	_txtTitle->setText(woststr.str());

	for (std::vector<AlienBase*>::const_iterator
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
	{
		if (_target == dynamic_cast<Target*>(*i))
		{
			_aBase = *i;
			const std::wstring edit (Language::utf8ToWstr((*i)->getUserLabel()));
			_edtTarget->setText(edit);
			_edtTarget->onTextChange(static_cast<ActionHandler>(&TargetInfoState::edtTargetChange));
			break;
		}
	}
	if (_aBase == nullptr)
		_edtTarget->setVisible(false);

	bool targeted (false);

	_txtTargeters->setAlign(ALIGN_CENTER);
	woststr.str(L"");
	for (std::vector<Target*>::const_iterator
			i = _target->getTargeters()->begin();
			i != _target->getTargeters()->end();
			++i)
	{
		woststr << (*i)->getLabel(_game->getLanguage()) << L'\n';

		if (targeted == false)
		{
			targeted = true;
			_txtTargeted->setText(tr("STR_TARGETTED_BY"));
			_txtTargeted->setAlign(ALIGN_CENTER);
		}
	}
	_txtTargeters->setText(woststr.str());

	if (targeted == false)
		_txtTargeted->setVisible(false);
}

/**
 * dTor.
 */
TargetInfoState::~TargetInfoState()
{}

/**
 * Edits an aLienBase's label.
 * @note For player to type in suspected race.
 * @param action - pointer to an Action
 */
void TargetInfoState::edtTargetChange(Action*)
{
	_aBase->setUserLabel(Language::wstrToUtf8(_edtTarget->getText()));
}

/**
 * Picks a craft to intercept the UFO.
 * @param action - pointer to an Action
 */
void TargetInfoState::btnInterceptClick(Action*)
{
	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new InterceptState(nullptr, _geoState));
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void TargetInfoState::btnOkClick(Action*)
{
	_game->popState();
}

}
