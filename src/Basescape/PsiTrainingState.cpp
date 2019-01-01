/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#include "PsiTrainingState.h"

//#include <climits>
//#include <sstream>

#include "../Basescape/SoldierInfoState.h"

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

#include "../Resource/XcomResourcePack.h"

//#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the PsiTraining screen.
 * @param base - pointer to the Base to get info from
 */
PsiTrainingState::PsiTrainingState(Base* const base)
	:
		_base(base)
{
	_window			= new Window(this);

	_txtTitle		= new Text(300, 17,  10,  8);
	_txtBaseLabel	= new Text( 80,  9, 230,  8);
	_txtSpaceFree	= new Text(100,  9,  16, 20);
	_txtName		= new Text(114,  9,  16, 31);
	_txtPsiStrength	= new Text( 48,  9, 134, 31);
	_txtPsiSkill	= new Text( 48,  9, 182, 31);
	_txtTraining	= new Text( 34,  9, 260, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("allocatePsi");

	add(_window,			"window",	"allocatePsi");
	add(_txtTitle,			"text",		"allocatePsi");
	add(_txtBaseLabel,		"text",		"allocatePsi");
	add(_txtSpaceFree,		"text",		"allocatePsi");
	add(_txtName,			"text",		"allocatePsi");
	add(_txtPsiStrength,	"text",		"allocatePsi");
	add(_txtPsiSkill,		"text",		"allocatePsi");
	add(_txtTraining,		"text",		"allocatePsi");
	add(_lstSoldiers,		"list",		"allocatePsi");
	add(_btnOk,				"button",	"allocatePsi");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&PsiTrainingState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PsiTrainingState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PsiTrainingState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&PsiTrainingState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(_base->getLabel());
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	_labSpace = _base->getFreePsiLabs();
	_txtSpaceFree->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
	_txtSpaceFree->setSecondaryColor(Palette::blockOffset(13u));

	_txtName->setText(tr("STR_NAME"));
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_HEADER"));
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_HEADER"));
	_txtTraining->setText(tr("STR_IN_TRAINING"));

	_lstSoldiers->setColumns(4, 118,48,78,34);
	_lstSoldiers->setArrow(193, ARROW_VERTICAL);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress(		static_cast<ActionHandler>(&PsiTrainingState::lstSoldiersPress));
	_lstSoldiers->onLeftArrowClick(	static_cast<ActionHandler>(&PsiTrainingState::lstLeftArrowClick));
	_lstSoldiers->onRightArrowClick(static_cast<ActionHandler>(&PsiTrainingState::lstRightArrowClick));
}

/**
 * dTor.
 */
PsiTrainingState::~PsiTrainingState()
{}

/**
 * Resets the palette and soldier-list.
 */
void PsiTrainingState::init()
{
	State::init();

	_lstSoldiers->clearList();

	size_t r (0u);
	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i, ++r)
	{
		std::wostringstream
			woststr1, // strength
			woststr2; // skill

		if ((*i)->getCurrentStats()->psiSkill != 0)
		{
			woststr1 << (*i)->getCurrentStats()->psiStrength;
			woststr2 << (*i)->getCurrentStats()->psiSkill;
		}
		else
		{
			woststr1 << tr("STR_UNKNOWN");
			woststr2 << tr("STR_UNKNOWN");
		}

		std::wstring wst;
		Uint8 color;
		bool contrast;
		if ((*i)->inPsiTraining() == true)
		{
			wst = tr("STR_YES");
			color = _lstSoldiers->getSecondaryColor();
			contrast = false;
		}
		else
		{
			wst = tr("STR_NO");
			if ((*i)->getSickbay() != 0)
			{
				color = 80u; // light brown
				contrast = true;
			}
			else
			{
				color = _lstSoldiers->getColor();
				contrast = false;
			}
		}

		_lstSoldiers->addRow(
						4,
						(*i)->getLabel().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						wst.c_str());
		_lstSoldiers->setRowColor(r, color, contrast);
	}

	_lstSoldiers->scrollTo(_base->getRecallRow(RCL_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void PsiTrainingState::btnOkClick(Action*)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * LMB assigns & removes a Soldier from psi-training.
 * RMB shows Soldier info.
 * @param action - pointer to an Action
 */
void PsiTrainingState::lstSoldiersPress(Action* action)
{
	const double mX (action->getAbsoluteMouseX());
	if (   mX >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mX <  static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
		{
			const size_t r (_lstSoldiers->getSelectedRow());

			Soldier* const sol (_base->getSoldiers()->at(r));
			if (sol->inPsiTraining() == false)
			{
				if (sol->getSickbay() == 0
					&& _base->getUsedPsiLabs() < _base->getTotalPsiLabs())
				{
					sol->togglePsiTraining();
					_txtSpaceFree->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(--_labSpace));
					_lstSoldiers->setCellText(
											r, 3u,
											tr("STR_YES"));
					_lstSoldiers->setRowColor(r, _lstSoldiers->getSecondaryColor());
				}
			}
			else
			{
				sol->togglePsiTraining();
				_txtSpaceFree->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(++_labSpace));
				_lstSoldiers->setCellText(
										r, 3u,
										tr("STR_NO"));
				_lstSoldiers->setRowColor(r, _lstSoldiers->getColor());
			}
			break;
		}

		case SDL_BUTTON_RIGHT:
			_base->setRecallRow(
							RCL_SOLDIER,
							_lstSoldiers->getScroll());

			_game->pushState(new SoldierInfoState(
											_base,
											_lstSoldiers->getSelectedRow()));
			kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Re-orders a Soldier up.
 * @param action - pointer to an Action
 */
void PsiTrainingState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t r (_lstSoldiers->getSelectedRow());
	if (r > 0u)
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
 * Re-orders a Soldier down.
 * @param action - pointer to an Action
 */
void PsiTrainingState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					RCL_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t qtySoldiers (_base->getSoldiers()->size());
	if (qtySoldiers > 0u)
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
