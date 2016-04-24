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

#include "State.h"

//#include <climits>
#include <cstring>

#include "Game.h"
#include "InteractiveSurface.h"
#include "Language.h"
//#include "LocalizedText.h"
#include "Screen.h"
#include "Surface.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Slider.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

Game* State::_game = nullptr; // static.


/**
 * Initializes the State with no child-elements.
 * @note States are full-screen by default.
 */
State::State()
	:
		_fullScreen(true),
		_modal(nullptr),
		_uiRule(nullptr),
		_uiRuleParent(nullptr)
{
	std::memset( // initialize palette to all-black
			_palette,
			0,
			sizeof(_palette));

	_cursorColor = _game->getCursor()->getColor();
}

/**
 * Deletes the child-elements contained in this State.
 */
State::~State()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
		delete *i;
}

/**
 * Sets the User Interface data from a ruleset.
 * @note Also sets the Palette for the State.
 * @param category		- reference the category of the interface from an Interfaces ruleset
 * @param altBackpal	- true to swap out the backpal-colors (default false)
 * @param tactical		- true to use Battlescape Palette (applies only to options screens) (default false)
 */
void State::setInterface(
		const std::string& category,
		bool altBackpal,
		bool tactical)
{
	PaletteType palType (PAL_NONE);
	BackPals backpal (BACKPAL_NONE);

	if ((_uiRule = _game->getRuleset()->getInterface(category)) != nullptr)
	{
		palType = _uiRule->getPalette();
		const Element* elBackpal (_uiRule->getElement("backpal"));

		_uiRuleParent = _game->getRuleset()->getInterface(_uiRule->getParent());
		if (_uiRuleParent != nullptr)
		{
			if (palType == PAL_NONE)
				palType = _uiRuleParent->getPalette();

			if (elBackpal == nullptr)
				elBackpal = _uiRuleParent->getElement("backpal");
		}

		if (elBackpal != nullptr && tactical == false)
		{
			int color;
			if (altBackpal == true)	color = elBackpal->color2;
			else					color = elBackpal->color;

			if (color != std::numeric_limits<int>::max())
				backpal = static_cast<BackPals>(color);
		}
	}

	if		(tactical == true)		palType = PAL_BATTLESCAPE;
	else if	(palType == PAL_NONE)	palType = PAL_GEOSCAPE;

	setPalette(palType, backpal);
}

/**
 * Adds a child-surface for this State to take care of, giving it the Game's
 * display palette.
 * @note Once associated the State handles all of the Surface's behavior and
 * management automatically. Since visible elements can overlap they have to be
 * added in ascending Z-Order to be blitted correctly onto the screen.
 * @param surface - child surface
 */
void State::add(Surface* surface)
{
	surface->setPalette(_palette);

	if (_game->getLanguage() != nullptr && _game->getResourcePack() != nullptr)
		surface->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());

	_surfaces.push_back(surface);
}

/**
 * As above except this adds a Surface based on an interface-element defined in
 * the ruleset.
 * @note that this function REQUIRES the ruleset to have been loaded prior to
 * use. If no parent is defined the element will not be moved.
 * @param surface	- pointer to child Surface
 * @param id		- reference the ID of the element defined in the ruleset if any
 * @param category	- reference the category of elements the Interface is associated with
 * @param parent	- pointer to the Surface to base the coordinates of this element off (default nullptr)
 */
void State::add(
		Surface* const surface,
		const std::string& id,
		const std::string& category,
		Surface* const parent)
{
	surface->setPalette(_palette);

	BattlescapeButton* const tacBtn (dynamic_cast<BattlescapeButton*>(surface));

	if (_game->getRuleset()->getInterface(category) != nullptr)
	{
		const Element* const element (_game->getRuleset()->getInterface(category)->getElement(id));
		if (element != nullptr)
		{
			if (parent != nullptr)
			{
				if (element->x != std::numeric_limits<int>::max()
					&& element->y != std::numeric_limits<int>::max())
				{
					surface->setX(parent->getX() + element->x);
					surface->setY(parent->getY() + element->y);
				}

				if (element->w != -1
					&& element->h != -1)
				{
					surface->setWidth(element->w);
					surface->setHeight(element->h);
				}
			}

			if (element->color != -1)
				surface->setColor(static_cast<Uint8>(element->color));

			if (element->color2 != -1)
				surface->setSecondaryColor(static_cast<Uint8>(element->color2));

			if (element->border != -1)
				surface->setBorderColor(static_cast<Uint8>(element->border));
		}
	}

	if (tacBtn != nullptr)
	{
		tacBtn->copy(parent);
		tacBtn->altSurface();
	}

	if (_game->getLanguage() != nullptr
		&& _game->getResourcePack() != nullptr)
	{
		surface->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	}
	_surfaces.push_back(surface);
}

/**
 * Returns whether this is a full-screen State.
 * @note This is used to optimize the state-machine since full-screen states
 * automatically cover the whole screen (whether they actually use it all or
 * not) so states behind them can be safely ignored since they'd be covered up.
 * @return, true if full-screen
 */
bool State::isFullScreen() const
{
	return _fullScreen;
}

/**
 * Toggles the full-screen flag.
 * @note Used by windows to keep the previous screen in display while the window
 * is still "popping up".
 */
void State::toggleScreen()
{
	_fullScreen = !_fullScreen;
}

/**
 * Initializes this State and its child-elements.
 * @note This is used for settings that have to be reset every time the state is
 * returned to focus (eg. palettes) so can't just be put in the constructor.
 * There's a stack of states so they can be created once but then repeatedly
 * switched in and out of focus.
 */
void State::init() // virtual
{
	_game->getScreen()->setPalette(_palette);

	_game->getCursor()->setPalette(_palette);
	_game->getCursor()->setColor(_cursorColor);
	_game->getCursor()->draw();

	_game->getFpsCounter()->setPalette(_palette);
	_game->getFpsCounter()->setColor(_cursorColor + 2u);
	_game->getFpsCounter()->draw();

	if (_game->getResourcePack() != nullptr)
		_game->getResourcePack()->setPalette(_palette);

	Window* window;
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if ((window = dynamic_cast<Window*>(*i)) != nullptr)
			window->invalidate();
	}
}

/**
 * Runs any code this State needs to keep updating every game-cycle like Timers
 * and other real-time elements.
 */
void State::think() // virtual
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->think();
	}
}

/**
 * Blits all the visible Surface child-elements onto the display-screen by order
 * of addition.
 */
void State::blit() // virtual
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->blit(_game->getScreen()->getSurface());
	}
}

/**
 * Takes care of any events from the core game-engine and passes them on to any
 * InteractiveSurface child-elements.
 * @param action - pointer to an Action
 */
void State::handle(Action* action) // virtual
{
	if (_modal == nullptr)
	{
		InteractiveSurface* srf;
		for (std::vector<Surface*>::const_reverse_iterator
				rit = _surfaces.rbegin();
				rit != _surfaces.rend();
				++rit)
		{
			if ((srf = dynamic_cast<InteractiveSurface*>(*rit)) != nullptr)
				srf->handle(action, this);
		}
	}
	else
		_modal->handle(action, this);
}

/**
 * Hides all the Surface child-elements from displaying.
 */
void State::hideAll()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setHidden(true);
	}
}

/**
 * Shows all the hidden Surface child-elements.
 */
void State::showAll()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setHidden(false);
	}
}

/**
 * Resets the status of all the Surface child-elements like unpressing buttons.
 */
void State::resetAll()
{
	InteractiveSurface* srf;
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if ((srf = dynamic_cast<InteractiveSurface*>(*i)) != nullptr)
		{
			srf->unpress(this);
//			srf->setFocus(false);
		}
	}
}

/**
 * Gets the LocalizedText for dictionary key @a id.
 * @note This function forwards the call to Language::getString(const std::string&).
 * @param id - reference the dictionary key to search for
 * @return, reference to the LocalizedText
 */
const LocalizedText& State::tr(const std::string& id) const
{
	return _game->getLanguage()->getString(id);
}

/**
 * Gets a modifiable copy of the LocalizedText for dictionary key @a id.
 * @note This function forwards the call to Language::getString(const std::string&, unsigned).
 * @param id	- reference the dictionary key to search for
 * @param qty	- the number to use to find the correct plurality
 * @return, copy of the LocalizedLext
 */
LocalizedText State::tr(
		const std::string& id,
		unsigned qty) const
{
	return _game->getLanguage()->getString(id, qty);
}

/**
 * Centers all the Surfaces on the screen.
 */
void State::centerAllSurfaces()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + _game->getScreen()->getDX());
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY());
	}
}

/**
 * Drops all the Surfaces by half the screen-height.
 */
void State::lowerAllSurfaces()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY() / 2);
	}
}

/**
 * Switches all the colors to something a little more Battlescape appropriate.
 */
void State::applyBattlescapeTheme()
{
	const Element* const el (_game->getRuleset()->getInterface("mainMenu")->getElement("battlescapeTheme"));

	Window* window;
	TextList* textList;
	ComboBox* combo;

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setColor(static_cast<Uint8>(el->color));
		(*i)->setHighContrast();

		if ((window = dynamic_cast<Window*>(*i)) != nullptr)
			window->setBackground(_game->getResourcePack()->getSurface("Diehard"));
		else if ((textList = dynamic_cast<TextList*>(*i)) != nullptr)
			textList->setArrowColor(static_cast<Uint8>(el->border));
		else if ((combo = dynamic_cast<ComboBox*>(*i)) != nullptr)
			combo->setArrowColor(static_cast<Uint8>(el->border));
	}
}

/**
 * Redraws all the text-like Surfaces.
 */
void State::redrawText()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		if (   dynamic_cast<Text*>(*i)			!= nullptr
			|| dynamic_cast<TextButton*>(*i)	!= nullptr
			|| dynamic_cast<TextList*>(*i)		!= nullptr
			|| dynamic_cast<TextEdit*>(*i)		!= nullptr)
		{
			(*i)->draw();
		}
	}
}

/**
 * Changes the currently responsive Surface.
 * @note If a Surface is modal then only that Surface can receive events. This
 * is used when an element needs to take priority over everything else, eg. focus.
 * @param srf - pointer to modal Surface; nullptr for no modal in this State
 */
void State::setModal(InteractiveSurface* const srf)
{
	_modal = srf;
}

/**
 * Replaces a certain amount of colors in this State's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 * @param apply			- true to apply changes immediately, false to wait in
 *						  case of multiple setPalettes (default true)
 */
void State::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors,
		bool apply)
{
	if (colors != nullptr)
		std::memcpy(
				_palette + firstcolor,
				colors,
				sizeof(SDL_Color) * static_cast<size_t>(ncolors));

	if (apply == true)
	{
		_game->getCursor()->setPalette(_palette);
		_game->getCursor()->draw();

		_game->getFpsCounter()->setPalette(_palette);
		_game->getFpsCounter()->draw();

		if (_game->getResourcePack() != nullptr)
			_game->getResourcePack()->setPalette(_palette);
	}
}

/**
 * Loads palettes from the ResourcePack into the state.
 * @param pal		- reference the string-ID of the palette to load
 * @param backpal	- BACKPALS.DAT offset to use (Palette.h) (default BACKPAL_NONE)
 */
void State::setPalette(
		const PaletteType pal,
		BackPals backpal)
{
	setPalette(
			_game->getResourcePack()->getPalette(pal)->getColors(),
			0,
			256,
			false);

	switch (pal)
	{
		case PAL_BASESCAPE:
			_cursorColor = static_cast<Uint8>(ResourcePack::BASESCAPE_CURSOR);
			break;
		case PAL_GEOSCAPE:
			_cursorColor = static_cast<Uint8>(ResourcePack::GEOSCAPE_CURSOR);
			break;
		case PAL_GRAPHS:
			_cursorColor = static_cast<Uint8>(ResourcePack::GRAPHS_CURSOR);
			break;
		case PAL_UFOPAEDIA:
			_cursorColor = static_cast<Uint8>(ResourcePack::UFOPAEDIA_CURSOR);
			break;

		default:
			_cursorColor = static_cast<Uint8>(ResourcePack::BATTLESCAPE_CURSOR);
	}


	if (backpal != BACKPAL_NONE)
		setPalette(
				_game->getResourcePack()->getPalette(PAL_BACKPALS)
						->getColors(static_cast<int>(Palette::blockOffset(static_cast<Uint8>(backpal)))),
				Palette::PAL_bgID,
				16,
				false);

	setPalette(nullptr); // delay actual update to the end
}

/**
 * Returns this State's 8-bpp palette.
 * @return, pointer to the palette's colors
 */
SDL_Color* State::getPalette()
{
	return _palette;
}

/**
 * Each State will probably need its own resize handling so this space is
 * intentionally left blank.
 * @param dX - reference to the x-delta
 * @param dY - reference to the y-delta
 */
void State::resize(
		int& dX,
		int& dY)
{
	recenter(dX, dY);
}

/**
 * Re-orients all the Surfaces in the State.
 * @param dX - x-delta
 * @param dY - y-delta
 */
void State::recenter(
		int dX,
		int dY)
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);
		(*i)->setY((*i)->getY() + dY / 2);
	}
}

/**
 * Sets a pointer to the Game-object.
 * @note This pointer can be used universally by all child-states.
 * @param game - THE pointer to Game
 */
void State::setGamePtr(Game* game)
{
	_game = game;
}

}
