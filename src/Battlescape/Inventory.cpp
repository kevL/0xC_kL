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

#include "Inventory.h"

//#include <cmath>

#include "BattlescapeState.h"
#include "PrimeGrenadeState.h"
#include "TileEngine.h"
#include "WarningMessage.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game		- pointer to core Game
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 * @param atBase	- true if inventory is accessed from Basescape (default false)
 */
Inventory::Inventory(
		Game* game,
		int width,
		int height,
		int x,
		int y,
		bool atBase)
	:
		InteractiveSurface(
			Options::baseXResolution,
			Options::baseYResolution,
			x - (Options::baseXResolution - 320) / 2,
			y - (Options::baseYResolution - 200) / 2),
		_game(game),
		_selUnit(nullptr),
		_selItem(nullptr),
		_mouseOverItem(nullptr),
		_tuMode(true),
		_atBase(atBase),
		_groundOffset(0),
		_fuseFrame(0),
		_prime(-1),
		_tuCost(-1)
{
	_srfGrid		= new Surface(
								Options::baseXResolution,
								Options::baseYResolution,
								x,y);
	_srfItems		= new Surface(
								Options::baseXResolution,
								Options::baseYResolution,
								x,y);
	_srfGrab		= new Surface(
								RuleInventory::HAND_W * RuleInventory::SLOT_W,
								RuleInventory::HAND_H * RuleInventory::SLOT_H);
	_warning		= new WarningMessage(
								224,24,
								(Options::baseXResolution - 320) / 2 + 48,
								(Options::baseYResolution - 200) / 2 + 176);
	_stackNumber	= new NumberText(15,15);
	_animTimer		= new Timer(80);

	_warning->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	_warning->setTextColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color));
	_warning->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color2));

	_stackNumber->setBordered();

	_animTimer->onTimer((SurfaceHandler)& Inventory::drawPrimers);
	_animTimer->start();
}

/**
 * Deletes inventory surfaces.
 */
Inventory::~Inventory()
{
	delete _srfGrid;
	delete _srfItems;
	delete _srfGrab;
	delete _warning;
	delete _stackNumber;
	delete _animTimer;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void Inventory::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_srfGrid->setPalette(colors, firstcolor, ncolors);
	_srfItems->setPalette(colors, firstcolor, ncolors);
	_srfGrab->setPalette(colors, firstcolor, ncolors);
	_warning->setPalette(colors, firstcolor, ncolors);

	_stackNumber->setPalette(getPalette());
}

/**
 * Changes the inventory's Time Units mode.
 * - when True inventory actions cost soldier time units (for battle);
 * - when False inventory actions don't cost anything (for pre-equip).
 * @param tu - true for Time Units mode; false for pre-battle equip
 */
void Inventory::setTuMode(bool tu)
{
	_tuMode = tu;
}

/**
 * Changes the unit to display the inventory of.
 * @param unit - pointer to a BattleUnit
 */
void Inventory::setSelectedUnitInventory(BattleUnit* const unit)
{
	_selUnit = unit;
	_groundOffset = 999;
	arrangeGround();
}

/**
 * Draws the inventory elements.
 */
void Inventory::draw()
{
	drawGrids();
	drawItems();
}

/**
 * Draws the inventory grids for item placement.
 */
void Inventory::drawGrids() // private.
{
	_srfGrid->clear();

	Text text = Text(16,9);
	text.setPalette(_srfGrid->getPalette());
	text.setHighContrast();
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());

	const RuleInterface* const rule = _game->getRuleset()->getInterface("inventory");
	text.setColor(static_cast<Uint8>(rule->getElement("textSlots")->color));

	const Uint8 color = static_cast<Uint8>(rule->getElement("grid")->color);
	bool doLabel;

	SDL_Rect rect;

	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		doLabel = true;

		if (i->second->getCategory() == IC_SLOT) // draw grid
		{
			for (std::vector<RuleSlot>::const_iterator
					j = i->second->getSlots()->begin();
					j != i->second->getSlots()->end();
					++j)
			{
				rect.x = static_cast<Sint16>(i->second->getX() + RuleInventory::SLOT_W * j->x);
				rect.y = static_cast<Sint16>(i->second->getY() + RuleInventory::SLOT_H * j->y);
				rect.w = static_cast<Uint16>(RuleInventory::SLOT_W + 1);
				rect.h = static_cast<Uint16>(RuleInventory::SLOT_H + 1);
				_srfGrid->drawRect(&rect, color);

				++rect.x;
				++rect.y;
				rect.w -= 2;
				rect.h -= 2;
				_srfGrid->drawRect(&rect, 0);
			}
		}
		else if (i->second->getCategory() == IC_HAND) // draw grid
		{
			rect.x = static_cast<Sint16>(i->second->getX());
			rect.y = static_cast<Sint16>(i->second->getY());
			rect.w = static_cast<Uint16>(RuleInventory::HAND_W * RuleInventory::SLOT_W);
			rect.h = static_cast<Uint16>(RuleInventory::HAND_H * RuleInventory::SLOT_H);
			_srfGrid->drawRect(&rect, color);

			++rect.x;
			++rect.y;
			rect.w -= 2;
			rect.h -= 2;
			_srfGrid->drawRect(&rect, 0);
		}
		else if (i->second->getCategory() == IC_GROUND) // draw grid
		{
			doLabel = false;

			const int
				width = i->second->getX() + RuleInventory::SLOT_W * RuleInventory::GROUND_W,
				height = i->second->getY() + RuleInventory::SLOT_H * RuleInventory::GROUND_H;

			for (int
					x = i->second->getX();
					x < width;
					x += RuleInventory::SLOT_W)
			{
				for (int
						y = i->second->getY();
						y < height;
						y += RuleInventory::SLOT_H)
				{
					rect.x = static_cast<Sint16>(x);
					rect.y = static_cast<Sint16>(y);
					rect.w = static_cast<Uint16>(RuleInventory::SLOT_W + 1);
					rect.h = static_cast<Uint16>(RuleInventory::SLOT_H + 1);
					_srfGrid->drawRect(&rect, color);

					++rect.x;
					++rect.y;
					rect.w -= 2;
					rect.h -= 2;
					_srfGrid->drawRect(&rect, 0);
				}
			}
		}

		if (doLabel == true)
		{
			text.setX(i->second->getX()); // draw label
			text.setY(i->second->getY() - text.getFont()->getHeight()
										- text.getFont()->getSpacing());
			text.setText(_game->getLanguage()->getString(i->second->getInventoryType()));
			text.blit(_srfGrid);
		}
	}
}

/**
 * Draws the items contained in the unit's inventory.
 */
void Inventory::drawItems() // private.
{
	_srfItems->clear();
	_grenadeFuses.clear();

	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("BIGOBS.PCK");
	Surface* srf;

	for (std::vector<BattleItem*>::const_iterator // Soldier items
			i = _selUnit->getInventory()->begin();
			i != _selUnit->getInventory()->end();
			++i)
	{
		if (*i != _selItem)
		{
			srf = srt->getFrame((*i)->getRules()->getBigSprite());
			if (srf != nullptr) // safety.
			{
				if ((*i)->getInventorySection()->getCategory() == IC_SLOT)
				{
					srf->setX((*i)->getInventorySection()->getX() + (*i)->getSlotX() * RuleInventory::SLOT_W);
					srf->setY((*i)->getInventorySection()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
				}
				else if ((*i)->getInventorySection()->getCategory() == IC_HAND)
				{
					srf->setX((*i)->getInventorySection()->getX()
							+ (RuleInventory::HAND_W - (*i)->getRules()->getInventoryWidth())
								* RuleInventory::SLOT_W / 2);
					srf->setY((*i)->getInventorySection()->getY()
							+ (RuleInventory::HAND_H - (*i)->getRules()->getInventoryHeight())
								* RuleInventory::SLOT_H / 2);
				}

				srf->blit(_srfItems);

				if ((*i)->getFuse() > -1) // grenade primer indicators
					_grenadeFuses.push_back(std::make_pair(srf->getX(), srf->getY()));
			}
			else Log(LOG_INFO) << "ERROR: Inventory::drawItems() bigob not found[1] #" << (*i)->getRules()->getBigSprite(); // see also RuleItem::drawHandSprite()
		}
	}

	Surface* const stackLayer = new Surface(getWidth(), getHeight());
	stackLayer->setPalette(getPalette());

	static const Uint8
		colorQty (static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("numStack")->color)),
		RED (37);

	for (std::vector<BattleItem*>::const_iterator // Ground items
			i = _selUnit->getTile()->getInventory()->begin();
			i != _selUnit->getTile()->getInventory()->end();
			++i)
	{
		if (*i != _selItem									// Note that items can be made invisible by setting their
			&& (*i)->getSlotX() >= _groundOffset			// width or height to 0 - eg. used with tank corpse items.
			&& (*i)->getRules()->getInventoryHeight() != 0
			&& (*i)->getRules()->getInventoryWidth() != 0)
		{
			srf = srt->getFrame((*i)->getRules()->getBigSprite());
			if (srf != nullptr) // safety.
			{
				srf->setX((*i)->getInventorySection()->getX()
						+ ((*i)->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
				srf->setY((*i)->getInventorySection()->getY()
						+ ((*i)->getSlotY() * RuleInventory::SLOT_H));

				srf->blit(_srfItems);

				if ((*i)->getFuse() > -1) // grenade primer indicators
					_grenadeFuses.push_back(std::make_pair(srf->getX(), srf->getY()));
			}
			else Log(LOG_INFO) << "ERROR: Inventory::drawItems() bigob not found[2] #" << (*i)->getRules()->getBigSprite(); // see also RuleItem::drawHandSprite()

			const int qty (_stackLevel[(*i)->getSlotX()] // item stacking
									  [(*i)->getSlotY()]);
			int fatals;
			if ((*i)->getUnit() != nullptr)
				fatals = (*i)->getUnit()->getFatalWounds();
			else
				fatals = 0;

			if (qty > 1 || fatals != 0)
			{
				_stackNumber->setX(((*i)->getInventorySection()->getX()
									+ (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset)
										* RuleInventory::SLOT_W) - 4);

				if (qty > 9 || fatals > 9)
					_stackNumber->setX(_stackNumber->getX() - 4);

				_stackNumber->setY(((*i)->getInventorySection()->getY()
									+ ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight())
										* RuleInventory::SLOT_H) - 6);
				_stackNumber->setValue(fatals ? fatals : qty);
				_stackNumber->draw();
				_stackNumber->setColor(fatals ? RED : colorQty);
				_stackNumber->blit(stackLayer);
			}
		}
	}

	stackLayer->blit(_srfItems);
	delete stackLayer;
}

/**
 * Moves an item to a specified section in the selected unit's inventory.
 * @param item		- pointer to a BattleItem
 * @param inRule	- pointer to RuleInventory
 * @param x			- X position (default 0)
 * @param y			- Y position (default 0)
 */
void Inventory::moveItem( // private.
		BattleItem* const item,
		const RuleInventory* const inRule,
		int x,
		int y)
{
/*	if (inRule == nullptr) // Make items vanish (ie. load ammo into weapon)
	{
		if (item->getInventorySection()->getCategory() == IC_GROUND)
			_selUnit->getTile()->removeItem(item);
		else
			item->changeOwner();
	}
	else
	{ */
	if (inRule != item->getInventorySection()) // Handle dropping from/to ground.
	{
		if (inRule->getCategory() == IC_GROUND) // set to Ground
		{
			item->changeOwner();
			_selUnit->getTile()->addItem(item, item->getInventorySection());

			if (item->getUnit() != nullptr
				&& item->getUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
			{
				item->getUnit()->setPosition(_selUnit->getPosition());
			}
		}
		else if (item->getInventorySection() == nullptr					// unload a weapon clip to left hand
			|| item->getInventorySection()->getCategory() == IC_GROUND)	// or pick up item.
		{
			if (_tuMode == true												// To prevent units from picking up large objects and running around with
				&& item->getInventorySection()->getCategory() == IC_GROUND)	// nearly full TU on the same turn its weight becomes an extra tu-burden
			{
				_selUnit->setTimeUnits(std::max(0,
											_selUnit->getTimeUnits() - item->getRules()->getWeight()));
			}

			item->changeOwner(_selUnit);
			_selUnit->getTile()->removeItem(item);

			if (item->getUnit() != nullptr
				&& item->getUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
			{
				item->getUnit()->setPosition(Position(-1,-1,-1));
			}
		}
	}

	item->setInventorySection(inRule);
	item->setSlotX(x);
	item->setSlotY(y);
//	}
}

/**
 * Checks if an item to be placed in a certain section-slot would overlap
 * another inventory item.
 * @param unit		- pointer to BattleUnit w/ inventory
 * @param item		- pointer to a BattleItem to be placed
 * @param inRule	- pointer to a RuleInventory section
 * @param x			- X position in section (default 0)
 * @param y			- Y position in section (default 0)
 * @return, true if overlap
 */
bool Inventory::isOverlap( // static.
		BattleUnit* const unit,
		const BattleItem* const item,
		const RuleInventory* const inRule,
		int x,
		int y)
{
	if (inRule->getCategory() != IC_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			if ((*i)->getInventorySection() == inRule
				&& (*i)->occupiesSlot(x,y, item) == true)
			{
				return true;
			}
		}
	}
	else if (unit->getTile() != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getTile()->getInventory()->begin();
				i != unit->getTile()->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x,y, item) == true)
				return true;
		}
	}

	return false;
}

/**
 * Gets the inventory section located in the specified mouse position.
 * @param x - pointer to mouse X position; returns the slot's X position
 * @param y - pointer to mouse Y position; returns the slot's Y position
 * @return, pointer to section rules or nullptr if none
 */
RuleInventory* Inventory::getSlotInPosition( // private.
		int* x,
		int* y) const
{
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		if (i->second->checkSlotAtPosition(x,y) == true)
			return i->second;
	}

	return nullptr;
}

/**
 * Returns the item currently grabbed by the player.
 * @return, pointer to selected BattleItem or nullptr if none
 */
BattleItem* Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently grabbed by the player.
 * @param item - pointer to selected BattleItem (default nullptr)
 */
void Inventory::setSelectedItem(BattleItem* const item)
{
	if (item != nullptr && item->getRules()->isFixed() == false)
	{
		_selItem = item;

		if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
			_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
					   [static_cast<size_t>(_selItem->getSlotY())] -= 1;

		_selItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_srfGrab);
	}
	else
	{
		_selItem = nullptr;
		_srfGrab->clear();
	}

	drawItems();
}

/**
 * Returns the item currently under mouse cursor.
 * @return, pointer to a BattleItem or nullptr if none
 */
BattleItem* Inventory::getMouseOverItem() const
{
	return _mouseOverItem;
}

/**
 * Changes the item currently under mouse cursor.
 * @param item - pointer to a BattleItem or nullptr if none
 */
void Inventory::setMouseOverItem(BattleItem* const item)
{
	if (item != nullptr && item->getRules()->isFixed() == false)
		_mouseOverItem = item;
	else
		_mouseOverItem = nullptr;
}

/**
 * Handles WarningMessage timer.
 */
void Inventory::think()
{
	if (_prime > -1)
	{
		std::wstring activated;

		if (_prime > 0)
			activated = Text::intWide(_prime) + L" ";

		activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");

		if (_prime > 0)
			activated += L" " + Text::intWide(_prime);

		_warning->showMessage(activated);
		_prime = -1;
	}

	_warning->think();
	_animTimer->think(nullptr, this);
}

/**
 * Blits the inventory elements.
 * @param surface - pointer to Surface to blit onto
 */
void Inventory::blit(Surface* surface)
{
	clear();

	_srfGrid->blit(this);
	_srfItems->blit(this);
	_srfGrab->blit(this);
	_warning->blit(this);

	Surface::blit(surface);
}

/**
 * Handles the cursor over.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Inventory::mouseOver(Action* action, State* state)
{
	int
		x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
		y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

	RuleInventory* const inRule = getSlotInPosition(&x,&y);
	if (inRule != nullptr)
	{
		if (_tuMode == true
			&& _selItem != nullptr
			&& fitItem(inRule, _selItem, true) == true)
		{
			_tuCost = _selItem->getInventorySection()->getCost(inRule);
		}
		else
		{
			_tuCost = -1;

			if (inRule->getCategory() == IC_GROUND)
				x += _groundOffset;

			BattleItem* const item = _selUnit->getItem(inRule, x,y);
			setMouseOverItem(item);
		}
	}
	else
	{
		_tuCost = -1;
		setMouseOverItem(nullptr);
	}

	_srfGrab->setX(static_cast<int>(std::floor(
					action->getAbsoluteXMouse())) - getX() - _srfGrab->getWidth() / 2);
	_srfGrab->setY(static_cast<int>(std::floor(
					action->getAbsoluteYMouse())) - getY() - _srfGrab->getHeight() / 2);

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Handles the cursor click. Picks up / drops an item.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Inventory::mouseClick(Action* action, State* state)
{
	int soundId = -1;

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selItem == nullptr) // Pickup or Move item.
		{
			int
				x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
				y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

			RuleInventory* const inRule = getSlotInPosition(&x,&y);
			if (inRule != nullptr)
			{
				if (inRule->getCategory() == IC_GROUND)
					x += _groundOffset;

				BattleItem* const item = _selUnit->getItem(inRule, x,y);
				if (item != nullptr)
				{
					if ((SDL_GetModState() & KMOD_CTRL) != 0) // Move item.
					{
						bool
							placed = false,
							toGround = true;

						RuleInventory* targetSection = nullptr;

						if (inRule->getCategory() == IC_HAND
							|| (inRule->getCategory() != IC_GROUND
								&& (_tuMode == false
									|| _selUnit->getOriginalFaction() != FACTION_PLAYER))) // aLien units drop-to-ground on Ctrl+LMB
						{
							targetSection = _game->getRuleset()->getInventory("STR_GROUND");
						}
						else
						{
							if (_selUnit->getItem("STR_RIGHT_HAND") == nullptr)
							{
								toGround = false;
								targetSection = _game->getRuleset()->getInventory("STR_RIGHT_HAND");
							}
							else if (_selUnit->getItem("STR_LEFT_HAND") == nullptr)
							{
								toGround = false;
								targetSection = _game->getRuleset()->getInventory("STR_LEFT_HAND");
							}
							else if (inRule->getCategory() != IC_GROUND)
								targetSection = _game->getRuleset()->getInventory("STR_GROUND");
						}

						if (targetSection != nullptr)
						{
							if (toGround == true)
							{
								if (_tuMode == false
									|| _selUnit->spendTimeUnits(item->getInventorySection()->getCost(targetSection)) == true)
								{
									placed = true;
									moveItem(item, targetSection);
									arrangeGround(false);

									soundId = ResourcePack::ITEM_DROP;
								}
								else
									_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
							}
							else
							{
								_stackLevel[static_cast<size_t>(item->getSlotX())]
										   [static_cast<size_t>(item->getSlotY())] -= 1;

								if (fitItem(targetSection, item) == true)
									placed = true;
								else
									_stackLevel[static_cast<size_t>(item->getSlotX())]
											   [static_cast<size_t>(item->getSlotY())] += 1;
							}

							if (placed == true)
							{
								_mouseOverItem = nullptr; // remove cursor info 'cause item is no longer under the cursor
								mouseOver(action, state);
							}
						}
					}
					else // Pickup item.
					{
						setSelectedItem(item);

						const int explTurn = item->getFuse();
						if (explTurn > -1)
						{
							std::wstring activated;

							if (explTurn > 0)
								activated = Text::intWide(explTurn) + L" ";

							activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");

							if (explTurn > 0)
								activated += L" " + Text::intWide(explTurn);

							_warning->showMessage(activated);
						}
					}
				}
			}
		}
		else // Drop item or Load weapon.
		{
			int
				x = _srfGrab->getX()
						+ (RuleInventory::HAND_W - _selItem->getRules()->getInventoryWidth())
							* RuleInventory::SLOT_W / 2
						+ RuleInventory::SLOT_W / 2,
				y = _srfGrab->getY()
						+ (RuleInventory::HAND_H - _selItem->getRules()->getInventoryHeight())
							* RuleInventory::SLOT_H / 2
						+ RuleInventory::SLOT_H / 2;

			RuleInventory* inRule = getSlotInPosition(&x,&y);

			if (inRule != nullptr)
			{
				if (inRule->getCategory() == IC_GROUND)
					x += _groundOffset;

				BattleItem* const item = _selUnit->getItem(inRule, x,y);

				const bool canStack = inRule->getCategory() == IC_GROUND
								   && canBeStacked(item, _selItem) == true;

				if (item == nullptr // Put item in empty slot or stack it if possible.
					|| item == _selItem
					|| canStack == true)
				{
					if (isOverlap(
								_selUnit,
								_selItem,
								inRule,
								x,y) == false
						&& inRule->fitItemInSlot(_selItem->getRules(), x,y) == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getInventorySection()->getCost(inRule)) == true)
						{
							_tuCost = -1;

							moveItem(_selItem, inRule, x,y);

							if (inRule->getCategory() == IC_GROUND)
								_stackLevel[static_cast<size_t>(x)]
										   [static_cast<size_t>(y)] += 1;
							setSelectedItem();

							soundId = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
					else if (canStack == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getInventorySection()->getCost(inRule)) == true)
						{
							_tuCost = -1;

							moveItem(
									_selItem,
									inRule,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[static_cast<size_t>(item->getSlotX())]
									   [static_cast<size_t>(item->getSlotY())] += 1;
							setSelectedItem();

							soundId = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
				else if (item->getRules()->getCompatibleAmmo()->empty() == false) // Put item in weapon.
				{
					if (item->getAmmoItem() != nullptr)
						_warning->showMessage(_game->getLanguage()->getString("STR_WEAPON_IS_ALREADY_LOADED"));
					else
					{
						bool fail = true;
						for (std::vector<std::string>::const_iterator
								i = item->getRules()->getCompatibleAmmo()->begin();
								i != item->getRules()->getCompatibleAmmo()->end();
								++i)
						{
							if (*i == _selItem->getRules()->getType())
							{
								fail = false;
								break;
							}
						}

						if (fail == true)
							_warning->showMessage(_game->getLanguage()->getString("STR_WRONG_AMMUNITION_FOR_THIS_WEAPON"));
						else if (_tuMode == false
							|| _selUnit->spendTimeUnits(item->getRules()->getReloadTu()) == true)
						{
							_tuCost = -1;

							if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
							{
								_selUnit->getTile()->removeItem(item);
								arrangeGround(false);
							}
//							moveItem(_selItem);

							item->setAmmoItem(_selItem);
//							_selItem->changeOwner();
							setSelectedItem();

							soundId = ResourcePack::ITEM_RELOAD;
//							if (item->getInventorySection()->getCategory() == IC_GROUND)
//								arrangeGround(false);
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
				// else swap the item positions ...
			}
			else
			{
				// try again using the position of the mouse cursor not the item (slightly more intuitive for stacking)
				x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX();
				y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

				inRule = getSlotInPosition(&x,&y);
				if (inRule != nullptr
					&& inRule->getCategory() == IC_GROUND)
				{
					x += _groundOffset;

					BattleItem* const item = _selUnit->getItem(inRule, x,y);
					if (canBeStacked(item, _selItem) == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getInventorySection()->getCost(inRule)) == true)
						{
							moveItem(
									_selItem,
									inRule,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[static_cast<size_t>(item->getSlotX())]
									   [static_cast<size_t>(item->getSlotY())] += 1;
							setSelectedItem();

							soundId = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_tuCost = -1;

		if (_selItem == nullptr)
		{
			if ((SDL_GetModState() & KMOD_CTRL) == 0)
			{
				if (Options::includePrimeStateInSavedLayout == true
					|| _atBase == false) // Priming is allowed only on the field or in preBattle, or if fuse-state can save to Layouts.
				{
					if (_tuMode == false)
					{
						int
							x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
							y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

						const RuleInventory* const inRule = getSlotInPosition(&x,&y);
						if (inRule != nullptr)
						{
							if (inRule->getCategory() == IC_GROUND)
								x += _groundOffset;

							BattleItem* const item = _selUnit->getItem(inRule, x,y);
							if (item != nullptr)
							{
								const RuleItem* const itRule = item->getRules();
								if (itRule->isGrenade() == true)
								{
									if (item->getFuse() == -1) // Prime that grenade!
									{
										if (itRule->getBattleType() == BT_PROXYGRENADE)
										{
											item->setFuse(0);
											arrangeGround(false);

											const std::wstring activated = _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");
											_warning->showMessage(activated);
										}
										else // This is where activation warning for nonProxy preBattle grenades goes.
											_game->pushState(new PrimeGrenadeState(nullptr, true, item, this));
									}
									else // deFuse grenade
									{
										_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_DEACTIVATED"));
										item->setFuse(-1);
										arrangeGround(false);
									}
								}
								else if (inRule->getCategory() != IC_GROUND) // move item to Ground
								{
									moveItem(
											item,
											_game->getRuleset()->getInventory("STR_GROUND"));

									arrangeGround(false);
									soundId = ResourcePack::ITEM_DROP;

									_mouseOverItem = nullptr; // remove cursor info 'cause item is no longer under the cursor
									mouseOver(action, state);
								}
							}
						}
					}
					else
						_game->popState(); // Close the inventory window on right-click if not in preBattle equip screen!
				}
			}
			else // Open Ufopaedia article.
			{
				int
					x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
					y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

				RuleInventory* const inRule = getSlotInPosition(&x,&y);
				if (inRule != nullptr)
				{
					if (inRule->getCategory() == IC_GROUND)
						x += _groundOffset;

					BattleItem* const item = _selUnit->getItem(inRule, x,y);
					if (item != nullptr)
					{
						std::string article = item->getRules()->getType(); // strip const. yay,
						Ufopaedia::openArticle(_game, article);
					}
				}
			}
		}
		else // RMB w/ item on cursor
		{
			if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
				_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
						   [static_cast<size_t>(_selItem->getSlotY())] += 1;

			setSelectedItem(); // Return item to original position.
			soundId = ResourcePack::ITEM_DROP;
		}
	}

	if (soundId != -1)
		_game->getResourcePack()->getSound("BATTLE.CAT", soundId)->play();

	_game->getSavedGame()->getBattleSave()->getBattleState()->refreshMousePosition();
	InteractiveSurface::mouseClick(action, state);
}

/**
 * Unloads the selected weapon placing the gun on the right hand and the ammo
 * on the left hand - unless tuMode is false then drop its ammo to the ground.
 * @return, true if a weapon is successfully unloaded
 */
bool Inventory::unload()
{
	if (_selItem == nullptr
		|| _selItem->selfPowered() == true
		|| _game->getSavedGame()->isResearched(_selItem->getRules()->getRequirements()) == false)
	{
		return false;
	}

	BattleItem* const ammo = _selItem->getAmmoItem();
	if (ammo == nullptr)
	{
		if (_selItem->getRules()->getCompatibleAmmo()->empty() == false)
			_warning->showMessage(_game->getLanguage()->getString("STR_NO_AMMUNITION_LOADED"));

		return false;
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _selUnit->getInventory()->begin();
			i != _selUnit->getInventory()->end();
			++i)
	{
		if ((*i)->getInventorySection()->getCategory() == IC_HAND
			&& *i != _selItem)
		{
			_warning->showMessage(_game->getLanguage()->getString("STR_BOTH_HANDS_MUST_BE_EMPTY"));
			return false;
		}
	}


	if (_tuMode == false
		|| _selUnit->spendTimeUnits(_selItem->getRules()->getUnloadTu()) == true)
	{
		RuleInventory* inRule;
		BattleUnit* owner;

		if (_tuMode == false)
		{
			inRule = _game->getRuleset()->getInventory("STR_GROUND");
			owner = nullptr;
		}
		else
		{
			inRule = _game->getRuleset()->getInventory("STR_LEFT_HAND");
			owner = _selUnit;;
		}

		moveItem(
				_selItem,
				_game->getRuleset()->getInventory("STR_RIGHT_HAND"));
		_selItem->changeOwner(_selUnit);

		_selItem->setAmmoItem();
		setSelectedItem();

		moveItem(ammo, inRule);
		ammo->changeOwner(owner);

		if (owner == nullptr)
			arrangeGround(false);
		else
			drawItems();
	}
	else
	{
		_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
		return false;
	}

	return true;
}

/**
 * Arranges items on the ground for the inventory display.
 * @note Since items on the ground aren't assigned to anyone they don't actually
 * have permanent slot positions.
 * @param alterOffset - true to alter the ground offset (default true)
 */
void Inventory::arrangeGround(bool alterOffset)
{
	_stackLevel.clear();

	RuleInventory* const grdRule = _game->getRuleset()->getInventory("STR_GROUND");

	// first move all items out of the way -> a big number in X direction to right
	for (std::vector<BattleItem*>::const_iterator
			i = _selUnit->getTile()->getInventory()->begin();
			i != _selUnit->getTile()->getInventory()->end();
			++i)
	{
		(*i)->setInventorySection(grdRule);
		(*i)->setSlotX(100000);
		(*i)->setSlotY(0);
	}

	bool fit;
	int
		x,y,
		xMax = 0;

	// for each item find the most topleft position that is not occupied and will fit
	for (std::vector<BattleItem*>::const_iterator
			i = _selUnit->getTile()->getInventory()->begin();
			i != _selUnit->getTile()->getInventory()->end();
			++i)
	{
		x =
		y = 0;

		fit = false;
		while (fit == false)
		{
			fit = true; // assume the item can be put here, if one of the following checks fails it can't
			for (int
					xd = 0;
					xd < (*i)->getRules()->getInventoryWidth() && fit == true;
					++xd)
			{
				if ((x + xd) % RuleInventory::GROUND_W < x % RuleInventory::GROUND_W)
					fit = false;
				else
				{
					for (int
							yd = 0;
							yd < (*i)->getRules()->getInventoryHeight() && fit == true;
							++yd)
					{
						BattleItem* const item = _selUnit->getItem(
																grdRule,
																x + xd,
																y + yd);
						fit = (item == nullptr);

						if (canBeStacked(item, *i) == true)
							fit = true;
					}
				}
			}

			if (fit == true)
			{
				(*i)->setSlotX(x);
				(*i)->setSlotY(y);
				// only increase the stack level if the item is actually visible.
				if ((*i)->getRules()->getInventoryWidth())
					_stackLevel[x][y] += 1;

				xMax = std::max(
							xMax,
							x + (*i)->getRules()->getInventoryWidth());
			}
			else
			{
				++y;
				if (y > RuleInventory::GROUND_H - (*i)->getRules()->getInventoryHeight())
				{
					y = 0;
					++x;
				}
			}
		}
	}

	if (alterOffset == true)
	{
		int itemWidth;
		if (_selItem != nullptr)
			itemWidth = _selItem->getRules()->getInventoryWidth();
		else
			itemWidth = 0;

		if (xMax > _groundOffset + RuleInventory::GROUND_W - itemWidth)
			_groundOffset += RuleInventory::GROUND_W;
		else
			_groundOffset = 0;
	}

	drawItems();
}

/**
 * Attempts to place the item in an inventory section.
 * @param inRule	- pointer to where to place the item
 * @param item		- pointer to item to be placed
 * @param test		- true if only doing a test (no move happens) (default false)
 * @return, true if the item was successfully placed in the inventory
 */
bool Inventory::fitItem(
		const RuleInventory* const inRule,
		BattleItem* const item,
		bool test)
{
	bool placed = false;

	for (int
			y = 0;
			y <= inRule->getY() / RuleInventory::SLOT_H && placed == false;
			++y)
	{
		for (int
				x = 0;
				x <= inRule->getX() / RuleInventory::SLOT_W && placed == false;
				++x)
		{
			if (inRule->fitItemInSlot(item->getRules(), x,y))
			{
				if (_tuMode == true && test == true)
					placed = true;
				else if (isOverlap(
								_selUnit,
								item,
								inRule,
								x,y) == false)
				{
					if (_tuMode == false
						|| _selUnit->spendTimeUnits(item->getInventorySection()->getCost(inRule)) == true)
					{
						placed = true;
						moveItem(item, inRule, x,y);

						_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();
						drawItems();
					}
					else if (test == false)
						_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
				}
			}
		}
	}

	return placed;
}

/**
 * Checks if two items can be stacked on one another.
 * @param itemA - first item
 * @param itemB - second item
 * @return, true if the items can be stacked on one another
 */
bool Inventory::canBeStacked(
		const BattleItem* const itemA,
		const BattleItem* const itemB)
{
	return (itemA != nullptr && itemB != nullptr														// both items actually exist
		&& itemA->getRules() == itemB->getRules()														// both items have the same ruleset
		&& ((itemA->getAmmoItem() == nullptr && itemB->getAmmoItem() == nullptr)						// either they both have no ammo
			|| (itemA->getAmmoItem() && itemB->getAmmoItem()												// or they both have ammo
				&& itemA->getAmmoItem()->getRules() == itemB->getAmmoItem()->getRules()						// and the same ammo type
				&& itemA->getAmmoItem()->getAmmoQuantity() == itemB->getAmmoItem()->getAmmoQuantity()))		// and the same ammo quantity
		&& itemA->getFuse() == -1 && itemB->getFuse() == -1												// and neither is set to explode
		&& itemA->getUnit() == nullptr && itemB->getUnit() == nullptr									// and neither is a corpse or unconscious unit
		&& itemA->getPainKillerQuantity() == itemB->getPainKillerQuantity()								// and if it's a medkit, it has the same number of charges
		&& itemA->getHealQuantity() == itemB->getHealQuantity()
		&& itemA->getStimulantQuantity() == itemB->getStimulantQuantity());
}

/**
 * Shows a warning message.
 * @param msg - reference a message to show
 */
void Inventory::showWarning(const std::wstring& msg)
{
	_warning->showMessage(msg);
}

/**
 * Shows primer warnings on all live grenades.
 */
void Inventory::drawPrimers()
{
	static const int pulse[22] = { 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
								  13,12,11,10, 9, 8, 7, 6, 5, 4, 3};

	if (_fuseFrame == 22) _fuseFrame = 0;

	static Surface* const srf = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9);
	for (std::vector<std::pair<int,int>>::const_iterator
			i = _grenadeFuses.begin();
			i != _grenadeFuses.end();
			++i)
	{
		srf->blitNShade(
					_srfItems,
					(*i).first,
					(*i).second,
					pulse[_fuseFrame],
					false, 3); // red
	}

	++_fuseFrame;
}

/**
 * Sets grenade to show a warning in Inventory.
 * @param turn - turns until grenade is to explode
 */
void Inventory::setPrimeGrenade(int turn)
{
	_prime = turn;
}

/**
 * Gets the TU cost for moving items around.
 * @return, time unit cost
 */
int Inventory::getTuCostInventory() const
{
/*	int wt;
	if (_tuCost > 0 && _selItem->getInventorySection()->getId() == "STR_GROUND")
		wt = _selItem->getRules()->getWeight();
	else wt = 0;
	return (_tuCost + wt); */
	return _tuCost;
}

}
