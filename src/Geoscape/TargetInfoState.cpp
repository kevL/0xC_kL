/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"
#include "InterceptState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Craft.h"
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
		_geoState(geoState),
		_aLienBase(nullptr)
{
	_fullScreen = false;

	_window       = new Window(this, 192, 120, 32, 40, POPUP_BOTH);
	_txtTitle     = new Text(182, 17, 37, 54);

	_edtTarget    = new TextEdit(this, 50, 9, 39, 46);

	_txtTargeted  = new Text(182, 9, 37, 71);
	_lstTargeters = new TextList(168, 33, 37, 82);

	_btnIntercept = new TextButton(160, 16, 48, 119);
	_btnOk        = new TextButton(160, 16, 48, 137);

	setInterface("targetInfo");

	add(_window,       "window", "targetInfo");
	add(_txtTitle,     "text",   "targetInfo");
	add(_edtTarget,    "text",   "targetInfo");
	add(_txtTargeted,  "text",   "targetInfo");
	add(_lstTargeters, "list",   "targetInfo");
	add(_btnIntercept, "button", "targetInfo");
	add(_btnOk,        "button", "targetInfo");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnIntercept->setText(tr("STR_INTERCEPT"));
	_btnIntercept->onMouseClick(	static_cast<ActionHandler>(&TargetInfoState::btnInterceptClick));
	_btnIntercept->onKeyboardPress(	static_cast<ActionHandler>(&TargetInfoState::btnInterceptClick),
									Options::keyOk);
	_btnIntercept->onKeyboardPress(	static_cast<ActionHandler>(&TargetInfoState::btnInterceptClick),
									Options::keyOkKeypad);

	_btnOk->setText(tr("STR_CANCEL"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&TargetInfoState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&TargetInfoState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	std::wostringstream woststr;
	woststr << L'\x01' << target->getLabel(_game->getLanguage()); // <- color2
	_txtTitle->setText(woststr.str());

	for (std::vector<AlienBase*>::const_iterator
			i  = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
		  ++i)
	{
		if (target == dynamic_cast<Target*>(*i))
		{
			_aLienBase = *i;
			const std::wstring edit (Language::utf8ToWstr((*i)->getUserLabel()));
			_edtTarget->setText(edit);
			_edtTarget->onTextChange(static_cast<ActionHandler>(&TargetInfoState::edtTargetChange));
			break;
		}
	}
	if (_aLienBase == nullptr)
		_edtTarget->setVisible(false);


	if (target->getTargeters()->empty() == false)
	{
		_txtTargeted->setText(tr("STR_TARGETTED_BY"));
		_txtTargeted->setAlign(ALIGN_CENTER);

		_lstTargeters->setColumns(1, 182);
		_lstTargeters->setMargin();
		_lstTargeters->setBackground(_window);
		_lstTargeters->setSelectable();
		_lstTargeters->setAlign(ALIGN_CENTER);
		_lstTargeters->onMousePress(static_cast<ActionHandler>(&TargetInfoState::lstTargetersPress));

		Craft* craft;
		for (std::vector<Target*>::const_iterator
				i  = target->getTargeters()->begin();
				i != target->getTargeters()->end();
			  ++i)
		{
			if ((craft = dynamic_cast<Craft*>(*i)) != nullptr) // safety. These shall be Craft only.
			{
				_crafts.push_back(craft);
				_lstTargeters->addRow(1, (*i)->getLabel(_game->getLanguage()).c_str());
			}
		}
	}
	else
	{
		_txtTargeted->setVisible(false);
		_lstTargeters->setVisible(false);
	}
}

/**
 * dTor.
 */
TargetInfoState::~TargetInfoState()
{}

/**
 * LMB shows Craft info; RMB exits State and centers Craft.
 * @param action - pointer to an Action
 */
void TargetInfoState::lstTargetersPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			Craft* const craft (_crafts[_lstTargeters->getSelectedRow()]);
			_game->pushState(new GeoscapeCraftState(
												craft,
												_geoState,
												nullptr,
												true));
			break;
		}

		case SDL_BUTTON_RIGHT:
		{
			_game->popState();

			const Craft* const craft (_crafts[_lstTargeters->getSelectedRow()]);
			const double
				lon (craft->getLongitude()),
				lat (craft->getLatitude());
			_geoState->getGlobe()->center(lon,lat);
			_geoState->getGlobe()->setCrosshair(lon,lat);
		}
	}
}

/**
 * Edits an aLienBase's label.
 * @note For player to type in suspected race.
 * @param action - pointer to an Action
 */
void TargetInfoState::edtTargetChange(Action*)
{
	_aLienBase->setUserLabel(Language::wstrToUtf8(_edtTarget->getText()));
}

/**
 * Opens another State for player to pick a Craft to intercept the Target.
 * @param action - pointer to an Action
 */
void TargetInfoState::btnInterceptClick(Action*)
{
	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new InterceptState(_geoState));
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
