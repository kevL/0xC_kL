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

#include "ActionMenuItem.h"

#include "../Engine/Game.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Sets up the ActionMenuItem.
 * @param id	- the unique identifier of the menu-item
 * @param game	- pointer to the core Game
 * @param x		- position on the x-axis
 * @param y		- Position on the y-axis
 */
ActionMenuItem::ActionMenuItem(
		int id,
		const Game* const game,
		int x,
		int y)
	:
		InteractiveSurface(
			272,
			40,
			x + 24,
			y - 40 * id),
		_highlightModifier(5u),
		_bat(BA_NONE),
		_tu(0)
{
	_frame		= new Frame(
						getWidth(),
						getHeight());
	_txtDesc	= new Text(160, 20,  10, 13);
	_txtAcc		= new Text( 63, 20, 151, 13);
	_txtTU		= new Text( 50, 20, 214, 13);

	Font
		* const big   (game->getResourcePack()->getFont("FONT_BIG")),
		* const small (game->getResourcePack()->getFont("FONT_SMALL"));
	Language* const lang (game->getLanguage());

	const Element* const el (game->getRuleset()->getInterface("battlescape")->getElement("actionMenu"));

	_frame->setColor(static_cast<Uint8>(el->border));
	_frame->setSecondaryColor(static_cast<Uint8>(el->color2));
	_frame->setHighContrast();
	_frame->setThickness(8);

	_txtDesc->initText(big, small, lang);
	_txtDesc->setColor(static_cast<Uint8>(el->color));
	_txtDesc->setHighContrast();
	_txtDesc->setBig();

	_txtAcc->initText(big, small, lang);
	_txtAcc->setColor(static_cast<Uint8>(el->color));
	_txtAcc->setHighContrast();
	_txtAcc->setBig();

	_txtTU->initText(big, small, lang);
	_txtTU->setColor(static_cast<Uint8>(el->color));
	_txtTU->setHighContrast();
	_txtTU->setBig();
}

/**
 * Deletes the ActionMenuItem.
 */
ActionMenuItem::~ActionMenuItem()
{
	delete _frame;
	delete _txtDesc;
	delete _txtTU;
	delete _txtAcc;
}

/**
 * Links with an action and fills in the text fields.
 * @param batType	- the battlescape action
 * @param desc		- reference to the action's description
 * @param acu		- reference to the action's accuracy including the 'acu' prefix
 * @param tu		- reference to the timeunits string including the 'tu' prefix
 * @param tuCost	- the timeunits value that will get expended
 */
void ActionMenuItem::setAction(
		BattleActionType baType,
		const std::wstring& desc,
		const std::wstring& acu,
		const std::wstring& tu,
		int tuCost)
{
	_txtDesc->setText(desc);
	_txtAcc->setText(acu);
	_txtTU->setText(tu);

	_bat = baType;
	_tu = tuCost;

	_redraw = true;
}

/**
 * Gets the BattleActionType that is linked to this MenuItem.
 * @return, the BattleActionType (BattlescapeGame.h)
 */
BattleActionType ActionMenuItem::getMenuActionType() const
{
	return _bat;
}

/**
 * Gets the action TUs that were linked to this MenuItem.
 * @return, the TU
 */
int ActionMenuItem::getMenuActionTu() const
{
	return _tu;
}

/**
 * Replaces a specified quantity of colors in this Surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void ActionMenuItem::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_frame->setPalette(colors, firstcolor, ncolors);
	_txtDesc->setPalette(colors, firstcolor, ncolors);
	_txtTU->setPalette(colors, firstcolor, ncolors);
	_txtAcc->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the bordered box.
 */
void ActionMenuItem::draw()
{
	_frame->blit(this);
	_txtDesc->blit(this);
	_txtTU->blit(this);
	_txtAcc->blit(this);
}

/**
 * Highlights the box when the cursor moves into its area.
 * @param action	- pointer to an Action
 * @param state		- pointer to a State
 */
void ActionMenuItem::mouseIn(Action* action, State* state)
{
	_frame->setSecondaryColor(static_cast<Uint8>(_frame->getSecondaryColor() - _highlightModifier)); // okay. c/G++ is neurotic.

	draw();
	InteractiveSurface::mouseIn(action, state);
}

/**
 * De-highlights the box when the cursor moves out of its area.
 * @param action	- pointer to an Action
 * @param state		- pointer to a State
 */
void ActionMenuItem::mouseOut(Action* action, State* state)
{
	_frame->setSecondaryColor(static_cast<Uint8>(_frame->getSecondaryColor() + _highlightModifier));

	draw();
	InteractiveSurface::mouseOut(action, state);
}

}
