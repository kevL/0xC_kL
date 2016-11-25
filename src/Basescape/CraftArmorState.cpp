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

#include "CraftArmorState.h"

//#include <climits>
//#include <string>

#include "SoldierArmorState.h"
#include "SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Armor screen.
 * @param base		- pointer to the base to get info from
 * @param craftId	- ID of the selected craft (default 0)
 */
CraftArmorState::CraftArmorState(
		Base* const base,
		size_t craftId)
	:
		_base(base),
		_craftId(craftId)
{
	_window			= new Window(this);

	_txtTitle		= new Text(288, 17,  16, 10);
	_txtBaseLabel	= new Text( 80,  9, 224, 10);

	_txtName		= new Text(90, 9,  16, 31);
	_txtArmor		= new Text(50, 9, 106, 31);
	_txtCraft		= new Text(50, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("craftArmor");

	add(_window,		"window",	"craftArmor");
	add(_txtTitle,		"text",		"craftArmor");
	add(_txtBaseLabel,	"text",		"craftArmor");
	add(_txtName,		"text",		"craftArmor");
	add(_txtArmor,		"text",		"craftArmor");
	add(_txtCraft,		"text",		"craftArmor");
	add(_lstSoldiers,	"list",		"craftArmor");
	add(_btnOk,			"button",	"craftArmor");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&CraftArmorState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftArmorState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftArmorState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&CraftArmorState::btnOkClick),
							Options::keyCancel);


	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getLabel());

	_txtName->setText(tr("STR_NAME_UC"));
	_txtCraft->setText(tr("STR_CRAFT"));
	_txtArmor->setText(tr("STR_ARMOR"));

	_lstSoldiers->setArrow(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 90,120,73);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress(		static_cast<ActionHandler>(&CraftArmorState::lstSoldiersPress));
	_lstSoldiers->onLeftArrowClick(	static_cast<ActionHandler>(&CraftArmorState::lstLeftArrowClick));
	_lstSoldiers->onRightArrowClick(static_cast<ActionHandler>(&CraftArmorState::lstRightArrowClick));
}

/**
 * dTor.
 */
CraftArmorState::~CraftArmorState()
{}

/**
 * The armors can change after going into other screens.
 */
void CraftArmorState::init()
{
	State::init();

	_lstSoldiers->clearList();

	// in case this is invoked from SoldiersState at a Base without any Craft:
	const Craft* craft;
	if (_base->getCrafts()->empty() == false)
		craft = _base->getCrafts()->at(_craftId);
	else
		craft = nullptr;

	Uint8 color;

	size_t r (0u);
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i, ++r)
	{
		_lstSoldiers->addRow(
						3,
						(*i)->getLabel().c_str(),
						tr((*i)->getArmor()->getType()).c_str(),
						(*i)->getCraftLabel(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == nullptr)
			color = _lstSoldiers->getColor();
		else if ((*i)->getCraft() == craft)
			color = _lstSoldiers->getSecondaryColor();
		else
			color = static_cast<Uint8>(_game->getRuleset()->getInterface("craftArmor")->getElement("otherCraft")->color);

		_lstSoldiers->setRowColor(r, color);

		if ((*i)->getSickbay() != 0)
			_lstSoldiers->setCellColor(r, 2u, (*i)->getSickbayColor(), true);
	}

	_lstSoldiers->scrollTo(_base->getRecallRow(RCL_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void CraftArmorState::btnOkClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * LMB shows the Select Armor window.
 * RMB shows Soldier info.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstSoldiersPress(Action* action)
{
	const double mX (action->getAbsoluteMouseX());
	if (   mX >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mX <  static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			size_t r (_lstSoldiers->getSelectedRow());
			const Soldier* const sol (_base->getSoldiers()->at(r));
			if (sol->getCraft() == nullptr
				|| sol->getCraft()->getCraftStatus() != CS_OUT)
			{
				_game->pushState(new SoldierArmorState(_base, r));
			}
			else
				_game->pushState(new ErrorMessageState(
													tr("STR_SOLDIER_NOT_AT_BASE"),
													_palette,
													Palette::blockOffset(4)+10,
													"BACK12.SCR",
													6));
			break;
		}

		case SDL_BUTTON_RIGHT:
			_game->pushState(new SoldierInfoState(
											_base,
											_lstSoldiers->getSelectedRow()));
			kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Reorders a Soldier up.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t r (_lstSoldiers->getSelectedRow());
	if (r != 0u)
	{
		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
			{
				Soldier* const sol (_base->getSoldiers()->at(r));

				_base->getSoldiers()->at(r) = _base->getSoldiers()->at(r - 1u);
				_base->getSoldiers()->at(r - 1u) = sol;

				if (r != _lstSoldiers->getScroll())
					SDL_WarpMouse(
							static_cast<Uint16>(action->getBorderLeft() + action->getMouseX()),
							static_cast<Uint16>(action->getBorderTop()  + action->getMouseY()
								- static_cast<int>(8. * action->getScaleY())));
				else
				{
					_base->setRecallRow(
									RCL_SOLDIER,
									_lstSoldiers->getScroll() - 1u);
					_lstSoldiers->scrollUp();
				}

				init();
				break;
			}

			case SDL_BUTTON_RIGHT:
			{
				_base->setRecallRow(
								RCL_SOLDIER,
								_lstSoldiers->getScroll() + 1u);

				Soldier* const sol (_base->getSoldiers()->at(r));

				_base->getSoldiers()->erase(_base->getSoldiers()->begin() + static_cast<std::ptrdiff_t>(r));
				_base->getSoldiers()->insert(
										_base->getSoldiers()->begin(),
										sol);
				init();
			}
		}
	}
}

/**
 * Reorders a Soldier down.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t qtySoldiers (_base->getSoldiers()->size());
	if (qtySoldiers != 0u)
	{
		const size_t r (_lstSoldiers->getSelectedRow());
		if (r < qtySoldiers - 1u)
		{
			switch (action->getDetails()->button.button)
			{
				case SDL_BUTTON_LEFT:
				{
					Soldier* const sol (_base->getSoldiers()->at(r));

					_base->getSoldiers()->at(r) = _base->getSoldiers()->at(r + 1u);
					_base->getSoldiers()->at(r + 1u) = sol;

					if (r != _lstSoldiers->getVisibleRows() + _lstSoldiers->getScroll() - 1u)
						SDL_WarpMouse(
								static_cast<Uint16>(action->getBorderLeft() + action->getMouseX()),
								static_cast<Uint16>(action->getBorderTop()  + action->getMouseY()
									+ static_cast<int>(8. * action->getScaleY())));
					else
					{
						_base->setRecallRow(
										RCL_SOLDIER,
										_lstSoldiers->getScroll() + 1u);
						_lstSoldiers->scrollDown();
					}

					init();
					break;
				}

				case SDL_BUTTON_RIGHT:
				{
					Soldier* const sol (_base->getSoldiers()->at(r));

					_base->getSoldiers()->erase(_base->getSoldiers()->begin() + static_cast<std::ptrdiff_t>(r));
					_base->getSoldiers()->insert(
											_base->getSoldiers()->end(),
											sol);
					init();
				}
			}
		}
	}
}

}
