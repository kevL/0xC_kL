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

#include "MedikitView.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"


namespace OpenXcom
{

/**
 * A array of strings that define body parts.
 */
const std::string MedikitView::BODY_PARTS[BattleUnit::PARTS_BODY] // static.
{
	"STR_HEAD",			// 0
	"STR_TORSO",		// 1
	"STR_RIGHT_ARM",	// 2
	"STR_LEFT_ARM",		// 3
	"STR_RIGHT_LEG",	// 4
	"STR_LEFT_LEG"		// 5
};


/**
 * Initializes the MedikitView.
 * @param w		- the MedikitView width
 * @param h		- the MedikitView height
 * @param x		- the MedikitView x origin
 * @param y		- the MedikitView y origin
 * @param game	- pointer to the core Game
 * @param unit	- pointer to the wounded BattleUnit
 * @param part	- pointer to Text of the selected body part
 * @param wound	- pointer to Text of fatal woundage
 */
MedikitView::MedikitView(
		int w,
		int h,
		int x,
		int y,
		const Game* const game,
		const BattleUnit* const unit,
		Text* const part,
		Text* const wound)
	:
		InteractiveSurface(
			w,h,
			x,y),
		_game(game),
		_unit(unit),
		_txtPart(part),
		_txtWound(wound),
		_selectedPart(BODYPART_NONE)
{
	autoSelectPart();
	_redraw = true;
}

/**
 * Draws this MedikitView.
 */
void MedikitView::draw()
{
	SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT"));
	Surface* srf;
	int color;

	this->lock();
	const Element* const el (_game->getRuleset()->getInterface("medikit")->getElement("body"));
	for (size_t
			i = 0u;
			i != BattleUnit::PARTS_BODY;
			++i)
	{
		if (_unit->getFatals(static_cast<UnitBodyPart>(i)) != 0)
			color = el->color2;
		else
			color = el->color;

		srf = srt->getFrame(static_cast<int>(i));
		srf->blitNShade(
					this,
					Surface::getX(),
					Surface::getY(),
					0, false,
					color);
	}
	this->unlock();

	_redraw = false;


	if (_selectedPart != BODYPART_NONE)
	{
		_txtPart->setText(_game->getLanguage()->getString(BODY_PARTS[_selectedPart]));
		_txtWound->setText(Text::intWide(_unit->getFatals(_selectedPart)));
	}
}

/**
 * Handles mouse-clicks on this MedikitView.
 * @param action - pointer to an Action
 * @param state - state that the ActionHandlers belong to
 */
void MedikitView::mouseClick(Action* action, State*)
{
	SurfaceSet* const srt (_game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT"));
	const Surface* srf;

	const int
		x (static_cast<int>(action->getRelativeMouseX() / action->getScaleX())),
		y (static_cast<int>(action->getRelativeMouseY() / action->getScaleY()));

	for (size_t
			i = 0u;
			i != BattleUnit::PARTS_BODY;
			++i)
	{
		srf = srt->getFrame(static_cast<int>(i));
		if (srf->getPixelColor(x,y) != 0)
		{
			_selectedPart = static_cast<UnitBodyPart>(i);
			_redraw = true;
			break;
		}
	}
}

/**
 * Gets the currently selected body-part.
 * @return, selected body-part (BattleUnit.h)
 */
UnitBodyPart MedikitView::getSelectedPart() const
{
	return _selectedPart;
}

/**
 * Automatically selects a wounded body-part.
 */
void MedikitView::autoSelectPart()
{
	for (size_t
			i = 0u;
			i != BattleUnit::PARTS_BODY;
			++i)
	{
		if (_unit->getFatals(static_cast<UnitBodyPart>(i)) != 0)
		{
			_selectedPart = static_cast<UnitBodyPart>(i);
			return;
		}
	}
	_txtPart->setVisible(false);
	_txtWound->setVisible(false);
}

}
