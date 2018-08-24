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

#include "Inventory.h"

//#include <algorithm>
//#include <cmath>

#include "BattlescapeState.h"
#include "PrimeGrenadeState.h"
#include "TileEngine.h"
#include "WarningMessage.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
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
 * Sets up the Inventory.
 * @param game		- pointer to core Game
 * @param atBase	- true if inventory is accessed from Basescape (default false)
 */
Inventory::Inventory(
		Game* const game,
		bool atBase)
	:
		InteractiveSurface(
				Options::baseXResolution, // create this Surface the entire size of Screen so RMB-exit works anywhere.
				Options::baseYResolution,
				(320 - Options::baseXResolution) >> 1u,
				(200 - Options::baseYResolution) >> 1u),
		_game(game),
		_atBase(atBase),
		_selUnit(nullptr),
		_selItem(nullptr),
		_overItem(nullptr),
		_tuMode(true),
		_grdOffset(0),
		_fusePulse(0u),
		_primed(-1),
		_tuCost(-1),
		_xOff((Options::baseXResolution - 640) >> 1u), // go fucking figure.
		_yOff((Options::baseYResolution - 360) >> 1u)
{
	_srfGrid	= new Surface(
							Options::baseXResolution,
							Options::baseYResolution,
							_xOff, _yOff); // why I don't know trial and error only.
	_srfItems	= new Surface(
							Options::baseXResolution,
							Options::baseYResolution,
							_xOff, _yOff);
	_srfGrab	= new Surface(
							RuleInventory::HAND_W * RuleInventory::SLOT_W,
							RuleInventory::HAND_H * RuleInventory::SLOT_H);
	_warning	= new WarningMessage(
							225,24,
							((Options::baseXResolution - 320) >> 1u) + 48,
							((Options::baseYResolution - 200) >> 1u) + 177);
	_numStack	= new NumberText(15,15);
	_aniTimer	= new Timer(80u);

	_srtBigobs = _game->getResourcePack()->getSurfaceSet("BIGOBS.PCK");

	_warning->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	const RuleInterface* const uiRule (_game->getRuleset()->getInterface("battlescape"));
	_warning->setTextColor(static_cast<Uint8>(uiRule->getElement("warning")->color));
	_warning->setColor(static_cast<Uint8>(uiRule->getElement("warning")->color2));

	_numStack->setBordered();

	_aniTimer->onTimer(static_cast<SurfaceHandler>(&Inventory::drawPrimers));
	_aniTimer->start();
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
	delete _numStack;
	delete _aniTimer;
}

/**
 * Replaces a specified quantity of colors in the inventory's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void Inventory::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_srfGrid->setPalette(colors, firstcolor, ncolors);
	_srfItems->setPalette(colors, firstcolor, ncolors);
	_srfGrab->setPalette(colors, firstcolor, ncolors);
	_warning->setPalette(colors, firstcolor, ncolors);

	_numStack->setPalette(getPalette());
}

/**
 * Draws the inventory grids for item placement.
 */
void Inventory::drawGrids()
{
	Text text (Text(16,9));
	text.setPalette(_srfGrid->getPalette());
	text.setHighContrast();
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());

	const RuleInterface* const uiRule (_game->getRuleset()->getInterface("inventory"));
	text.setColor(static_cast<Uint8>(uiRule->getElement("textSlots")->color));

	const Uint8 color (static_cast<Uint8>(uiRule->getElement("grid")->color));

	SDL_Rect rect;
	bool doLabel;
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		switch (i->second->getCategory())
		{
			case IC_SLOT: // draw grids for unit-sections
				doLabel = true;
				for (std::vector<InSlot>::const_iterator
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
					rect.w = static_cast<Uint16>(rect.w - 2u);
					rect.h = static_cast<Uint16>(rect.h - 2u);
					_srfGrid->drawRect(&rect, 0u);
				}
				break;

			case IC_HAND: // draw grids for hand-sections
				doLabel = true;
				rect.x = static_cast<Sint16>(i->second->getX());
				rect.y = static_cast<Sint16>(i->second->getY());
				rect.w = static_cast<Uint16>(RuleInventory::HAND_W * RuleInventory::SLOT_W);
				rect.h = static_cast<Uint16>(RuleInventory::HAND_H * RuleInventory::SLOT_H);
				_srfGrid->drawRect(&rect, color);

				++rect.x;
				++rect.y;
				rect.w = static_cast<Uint16>(rect.w - 2u);
				rect.h = static_cast<Uint16>(rect.h - 2u);
				_srfGrid->drawRect(&rect, 0u);
				break;

			default:
			case IC_GROUND: // draw grid for ground-section
				{
					doLabel = false;
					const int
						width  (i->second->getX() + RuleInventory::SLOT_W * RuleInventory::GROUND_W),
						height (i->second->getY() + RuleInventory::SLOT_H * RuleInventory::GROUND_H);

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
							rect.w = static_cast<Uint16>(rect.w - 2u);
							rect.h = static_cast<Uint16>(rect.h - 2u);
							_srfGrid->drawRect(&rect, 0u);
						}
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
 * Draws the items contained in the currently selected unit's inventory and on
 * its ground-tile.
 */
void Inventory::drawItems()
{
	if (_selUnit != nullptr)
	{
		_srfItems->clear();
		_fusePairs.clear();

		Surface* sprite;

		const RuleInventory* inRule;

		std::vector<BattleItem*>* list (_selUnit->getInventory());
		for (std::vector<BattleItem*>::const_iterator // draw items in Unit sections.
				i = list->begin();
				i != list->end();
				++i)
		{
			if (*i != _selItem)
			{
				if ((sprite = _srtBigobs->getFrame((*i)->getRules()->getBigSprite())) != nullptr) // safety.
				{
					inRule = (*i)->getInventorySection();
					switch (inRule->getCategory())
					{
						case IC_SLOT:
							sprite->setX(inRule->getX() + (*i)->getSlotX() * RuleInventory::SLOT_W);
							sprite->setY(inRule->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
							break;

						case IC_HAND:
							sprite->setX(inRule->getX()
									+ (RuleInventory::HAND_W - (*i)->getRules()->getInventoryWidth())
										* RuleInventory::SLOT_W_2);
							sprite->setY(inRule->getY()
									+ (RuleInventory::HAND_H - (*i)->getRules()->getInventoryHeight())
										* RuleInventory::SLOT_H_2);
					}
					sprite->blit(_srfItems);

					if ((*i)->getFuse() > -1) // grenade-primed indicators
						_fusePairs.push_back(std::make_pair(
														sprite->getX(),
														sprite->getY()));
				}
//				else Log(LOG_WARNING) << "Inventory::drawItems() bigob not found[1] #" << (*i)->getRules()->getBigSprite(); // see also RuleItem::drawHandSprite()
			}
		}


		Surface* const stackLayer (new Surface(getWidth(), getHeight()));
		stackLayer->setPalette(getPalette());

		static const Uint8
			color (static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("numStack")->color)),
			RED (37u);

		inRule = _game->getRuleset()->getInventoryRule(ST_GROUND);
		list = _selUnit->getUnitTile()->getInventory();
		for (std::vector<BattleItem*>::const_iterator // draw items in Ground section.
				i = list->begin();
				i != list->end();
				++i)
		{
			if (*i != _selItem									// Items can be made invisible by setting their width to 0;
				&& (*i)->getRules()->getInventoryWidth() != 0)	// eg, used for large-unit corpses.
//				&& (*i)->getRules()->getInventoryHeight() != 0
//				&& (*i)->getSlotX() >= _grdOffset				// && < _grdOffset + RuleInventory::GROUND_W ... or so.
			{
				if ((sprite = _srtBigobs->getFrame((*i)->getRules()->getBigSprite())) != nullptr) // safety.
				{
					sprite->setX(inRule->getX()
							  + ((*i)->getSlotX() - _grdOffset) * RuleInventory::SLOT_W);
					sprite->setY(inRule->getY()
							  + ((*i)->getSlotY() * RuleInventory::SLOT_H));

					sprite->blit(_srfItems);

					if ((*i)->getFuse() > -1) // grenade primer indicators
						_fusePairs.push_back(std::make_pair(
														sprite->getX(),
														sprite->getY()));

					const int qty (_stackLevel[(*i)->getSlotX()] // item stacking
											  [(*i)->getSlotY()]);
					int fatals;
					if ((*i)->getBodyUnit() != nullptr)
						fatals = (*i)->getBodyUnit()->getFatalsTotal();
					else
						fatals = 0;

					if (qty > 1 || fatals != 0)
					{
						_numStack->setX((inRule->getX()
											+ (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _grdOffset)
												* RuleInventory::SLOT_W) - 4);

						if (qty > 9 || fatals > 9)
							_numStack->setX(_numStack->getX() - 4);

						_numStack->setY((inRule->getY()
											+ ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight())
												* RuleInventory::SLOT_H) - 6);
						_numStack->setValue(fatals ? static_cast<unsigned>(fatals) : static_cast<unsigned>(qty));
						_numStack->draw();
						_numStack->setColor(fatals ? RED : color);
						_numStack->blit(stackLayer);
					}
				}
//				else Log(LOG_WARNING) << "Inventory::drawItems() bigob not found[2] #" << (*i)->getRules()->getBigSprite(); // see also RuleItem::drawHandSprite()
			}
		}

		stackLayer->blit(_srfItems);
		delete stackLayer;
	}
}

/**
 * Handles WarningMessage timer.
 */
void Inventory::think()
{
	if (_primed != -1)
	{
		std::wstring activated (_game->getLanguage()->getString("STR_GRENADE_ACTIVATED"));
		if (_primed > 0)
		{
			const std::wstring wst (Text::intWide(_primed));
			activated.insert(0u, wst + L" ");
			activated += L" " + wst;
		}
		_warning->showMessage(activated);
		_primed = -1;
	}
	_warning->think();
	_aniTimer->think(nullptr, this);
}

/**
 * Shows primer warnings on all live grenades.
 */
void Inventory::drawPrimers() // private.
{
	static const int
		CB_RED = 3,
		pulse[22u] { 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
					13,12,11,10, 9, 8, 7, 6, 5, 4, 3};

	if (_fusePulse == 22u) _fusePulse = 0u;

	static Surface* const srf (_game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9));
	for (std::vector<std::pair<int,int>>::const_iterator
			i = _fusePairs.begin();
			i != _fusePairs.end();
			++i)
	{
		srf->blitNShade(
					_srfItems,
					i->first  + _xOff,
					i->second + _yOff,
					pulse[_fusePulse],
					false, CB_RED);
	}
	++_fusePulse;
}

/**
 * Blits the inventory elements.
 * @param srf - pointer to Surface to blit to
 */
void Inventory::blit(const Surface* const srf)
{
	clear();

	_srfGrid->blit(this);
	_srfItems->blit(this);
	_srfGrab->blit(this);
	_warning->blit(this);

	Surface::blit(srf);
}

/**
 * Handles mouse-over.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Inventory::mouseOver(Action* action, State* state)
{
	if (_selUnit != nullptr)
	{
		int
			x (static_cast<int>(std::floor(action->getAbsoluteMouseX())) - getX() - _xOff),
			y (static_cast<int>(std::floor(action->getAbsoluteMouseY())) - getY() - _yOff);

		RuleInventory* const inRule (getSlotAtCursor(&x,&y));
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
					x += _grdOffset;

				BattleItem* const it (_selUnit->getItem(inRule, x,y));
				setMouseOverItem(it);
			}
		}
		else
		{
			_tuCost = -1;
			setMouseOverItem();
		}

		_srfGrab->setX(static_cast<int>(std::floor(
						action->getAbsoluteMouseX())) - getX() - (_srfGrab->getWidth()  >> 1u));
		_srfGrab->setY(static_cast<int>(std::floor(
						action->getAbsoluteMouseY())) - getY() - (_srfGrab->getHeight() >> 1u));

		InteractiveSurface::mouseOver(action, state);
	}
}

/**
 * Handles mouse-click.
 * @note Picks up or drops an item.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Inventory::mouseClick(Action* action, State* state)
{
	if (_selUnit != nullptr)
	{
		unsigned soundId (std::numeric_limits<unsigned>::max());

		switch (action->getDetails()->button.button)
		{
			case SDL_BUTTON_LEFT:
				if (_selItem == nullptr) // Pickup or Move item.
				{
					int
						x (static_cast<int>(std::floor(action->getAbsoluteMouseX())) - getX() - _xOff),
						y (static_cast<int>(std::floor(action->getAbsoluteMouseY())) - getY() - _yOff);

					const RuleInventory* const inRule (getSlotAtCursor(&x,&y));
					if (inRule != nullptr)
					{
						if (inRule->getCategory() == IC_GROUND)
							x += _grdOffset;

						BattleItem* const overItem (_selUnit->getItem(inRule, x,y));
						if (overItem != nullptr)
						{
							if ((SDL_GetModState() & KMOD_CTRL) != 0) // auto-Move item.
							{
								InventorySection toType;

								if (inRule->getCategory() == IC_HAND
									|| (inRule->getCategory() != IC_GROUND
										&& (_tuMode == false
											|| _selUnit->getOriginalFaction() != FACTION_PLAYER))) // Mc'd aLien units drop-to-ground on Ctrl+LMB
								{
									toType = ST_GROUND;
								}
								else if (_selUnit->getItem(ST_RIGHTHAND) == nullptr)
									toType = ST_RIGHTHAND;
								else if (_selUnit->getItem(ST_LEFTHAND) == nullptr)
									toType = ST_LEFTHAND;
								else if (inRule->getCategory() != IC_GROUND)
									toType = ST_GROUND;
								else
									toType = ST_NONE;

								bool placed;
								switch (toType)
								{
									case ST_NONE:
										placed = false;
										break;

									case ST_GROUND:
									{
										const RuleInventory* const toSection (_game->getRuleset()->getInventoryRule(ST_GROUND));
										if (_tuMode == false
											|| _selUnit->expendTu(overItem->getInventorySection()->getCost(toSection)) == true)
										{
											placed = true;
											moveItem(overItem, toSection);
											arrangeGround();
											drawItems();

											soundId = ResourcePack::ITEM_DROP;
										}
										else
										{
											placed = false;
											_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
										}
										break;
									}

									default: // from Ground
									{
										const RuleInventory* const toSection (_game->getRuleset()->getInventoryRule(toType));
										placed = fitItem(toSection, overItem);
									}
								}

								if (placed == true)
								{
									_overItem = nullptr; // remove item-info 'cause item is no longer under the cursor
									mouseOver(action, state);
								}
							}
							else // Pickup item.
							{
								setSelectedItem(overItem);

								drawItems();

								const int explTurn (overItem->getFuse());
								if (explTurn > -1)
								{
									std::wstring activated;
									if (explTurn > 0) activated = Text::intWide(explTurn) + L" ";
									activated += _game->getLanguage()->getString("STR_GRENADE_ACTIVATED");
									if (explTurn > 0) activated += L" " + Text::intWide(explTurn);
									_warning->showMessage(activated);
								}
							}
						}
					}
				}
				else // item on cursor, Drop item or Load weapon with it.
				{
					int
						x (_srfGrab->getX() - _xOff
								+ (RuleInventory::HAND_W - _selItem->getRules()->getInventoryWidth())
									* RuleInventory::SLOT_W_2
								+ RuleInventory::SLOT_W_2),
						y (_srfGrab->getY() - _yOff
								+ (RuleInventory::HAND_H - _selItem->getRules()->getInventoryHeight())
									* RuleInventory::SLOT_H_2
								+ RuleInventory::SLOT_H_2);

					RuleInventory* inRule (getSlotAtCursor(&x,&y));

					if (inRule != nullptr)
					{
						if (inRule->getCategory() == IC_GROUND)
							x += _grdOffset;

						BattleItem* const overItem (_selUnit->getItem(inRule, x,y));

						const bool stack (inRule->getCategory() == IC_GROUND
									   && canStack(overItem, _selItem) == true);

						if (overItem == _selItem	// put item back where it came from
							|| overItem == nullptr	// put item in empty slot
							|| stack == true)		// stack item
						{
							if (isOverlap(
										_selUnit,
										_selItem,
										inRule,
										x,y) == false
								&& inRule->fitItemInSlot(_selItem->getRules(), x,y) == true)
							{
								if (_tuMode == false
									|| _selUnit->expendTu(_selItem->getInventorySection()->getCost(inRule)) == true)
								{
									_tuCost = -1;

									moveItem(_selItem, inRule, x,y);
									setSelectedItem();
									if (inRule->getCategory() == IC_GROUND)
									{
//										_stackLevel[static_cast<size_t>(x)]
//												   [static_cast<size_t>(y)] += 1;
										arrangeGround();
									}
									drawItems();

									soundId = ResourcePack::ITEM_DROP;
								}
								else
									_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
							}
							else if (stack == true)
							{
								if (_tuMode == false
									|| _selUnit->expendTu(_selItem->getInventorySection()->getCost(inRule)) == true)
								{
									_tuCost = -1;

									moveItem(
											_selItem,
											inRule,
											overItem->getSlotX(),
											overItem->getSlotY());
//									_stackLevel[static_cast<size_t>(overItem->getSlotX())]
//											   [static_cast<size_t>(overItem->getSlotY())] += 1;
									setSelectedItem();
									arrangeGround();
									drawItems();

									soundId = ResourcePack::ITEM_DROP;
								}
								else
									_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
							}
						}
						else if (overItem->getRules()->getAcceptedLoadTypes()->empty() == false // Put item in weapon.
							&& (_tuMode == false || overItem->getInventorySection()->getCategory() == IC_HAND))
						{
							if (overItem->getAmmoItem() != nullptr)
								_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[8u]));
							else
							{
								bool fail (true);
								for (std::vector<std::string>::const_iterator
										i = overItem->getRules()->getAcceptedLoadTypes()->begin();
										i != overItem->getRules()->getAcceptedLoadTypes()->end();
										++i)
								{
									if (*i == _selItem->getRules()->getType())
									{
										fail = false;
										break;
									}
								}

								if (fail == true)
									_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[9u]));
								else if (_tuMode == true
									&& _selUnit->getItem(ST_RIGHTHAND) != nullptr
									&& _selUnit->getItem(ST_RIGHTHAND) != _selItem
									&& _selUnit->getItem(ST_LEFTHAND) != nullptr
									&& _selUnit->getItem(ST_LEFTHAND) != _selItem)
								{
									_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[10u])); // TODO: "one hand must be empty"
								}
								else
								{
									int tuReload;
									if (_tuMode == true)
									{
										tuReload = overItem->getRules()->getReloadTu();
										if (_selItem->getInventorySection()->getCategory() != IC_HAND)
										{
											InventorySection toHand;
											if (_selUnit->getItem(ST_RIGHTHAND) == nullptr)
												toHand = ST_RIGHTHAND;
											else
												toHand = ST_LEFTHAND;
											const RuleInventory* const handRule (_game->getRuleset()->getInventoryRule(toHand));
											tuReload += _selItem->getInventorySection()->getCost(handRule);
										}
									}
									else
										tuReload = 0; // safety, not used.

									if (_tuMode == false
										|| _selUnit->expendTu(tuReload) == true)
									{
										_tuCost = -1;

										bool doGround;
										if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
										{
											doGround = true;
											_selUnit->getUnitTile()->removeItem(_selItem);

											// TODO: set stackLevel
										}
										else
											doGround = false;

										overItem->setAmmoItem(_selItem);

										setSelectedItem();
										if (doGround == true) arrangeGround();
										drawItems();

										soundId = ResourcePack::ITEM_RELOAD;
									}
									else
										_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
								}
							}
						}
						// else swap the item positions ...
					}
					else
					{
						// try again using the position of the mouse cursor not the item (slightly more intuitive for stacking)
						x = static_cast<int>(std::floor(action->getAbsoluteMouseX())) - getX() - _xOff;
						y = static_cast<int>(std::floor(action->getAbsoluteMouseY())) - getY() - _yOff;

						if ((inRule = getSlotAtCursor(&x,&y)) != nullptr
							&& inRule->getCategory() == IC_GROUND)
						{
							x += _grdOffset;

							BattleItem* const overItem (_selUnit->getItem(inRule, x,y));
							if (canStack(overItem, _selItem) == true)
							{
								if (_tuMode == false
									|| _selUnit->expendTu(_selItem->getInventorySection()->getCost(inRule)) == true)
								{
									moveItem(
											_selItem,
											inRule,
											overItem->getSlotX(),
											overItem->getSlotY());
//									_stackLevel[static_cast<size_t>(overItem->getSlotX())]
//											   [static_cast<size_t>(overItem->getSlotY())] += 1;
									setSelectedItem();
									arrangeGround();
									drawItems();

									soundId = ResourcePack::ITEM_DROP;
								}
								else
									_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
							}
						}
					}
				}
				break;

			case SDL_BUTTON_RIGHT:
				_tuCost = -1;

				if (_selItem == nullptr)
				{
					if ((SDL_GetModState() & KMOD_CTRL) == 0)
					{
						if (_atBase == false // Priming is allowed only on the field or in pre-battle, or if fuse-state can save to Layouts.
							|| Options::includePrimeStateInSavedLayout == true)
						{
							if (_tuMode == false)
							{
								int
									x (static_cast<int>(std::floor(action->getAbsoluteMouseX())) - getX() - _xOff),
									y (static_cast<int>(std::floor(action->getAbsoluteMouseY())) - getY() - _yOff);

								const RuleInventory* const inRule (getSlotAtCursor(&x,&y));
								if (inRule != nullptr)
								{
									if (inRule->getCategory() == IC_GROUND)
										x += _grdOffset;

									BattleItem* const overItem (_selUnit->getItem(inRule, x,y));
									if (overItem != nullptr)
									{
										const RuleItem* const itRule (overItem->getRules());
										switch (itRule->getBattleType())
										{
											case BT_GRENADE:
											case BT_PROXYGRENADE:
											case BT_FLARE:
												if (overItem->getFuse() == -1) // Prime that grenade!
												{
													switch (itRule->getBattleType())
													{
														case BT_PROXYGRENADE:
														case BT_FLARE:
															overItem->setFuse(0);
															arrangeGround();
															drawItems();
															_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_ACTIVATED"));
															break;

														default: // This is where activation warning for nonProxy preBattle grenades goes.
															_game->pushState(new PrimeGrenadeState(nullptr, true, overItem, this));
													}
												}
												else // deFuse grenade
												{
													_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_DEACTIVATED"));
													overItem->setFuse(-1);
													arrangeGround();
													drawItems();
												}
												break;

											default:
												if (inRule->getCategory() != IC_GROUND) // move item to Ground
												{
													moveItem(
															overItem,
															_game->getRuleset()->getInventoryRule(ST_GROUND));
													arrangeGround();
													drawItems();
													soundId = ResourcePack::ITEM_DROP;

													_overItem = nullptr; // remove cursor info 'cause item is no longer under the cursor
													mouseOver(action, state);
												}
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
							x (static_cast<int>(std::floor(action->getAbsoluteMouseX())) - getX() - _xOff),
							y (static_cast<int>(std::floor(action->getAbsoluteMouseY())) - getY() - _yOff);

						RuleInventory* const inRule (getSlotAtCursor(&x,&y));
						if (inRule != nullptr)
						{
							if (inRule->getCategory() == IC_GROUND)
								x += _grdOffset;

							BattleItem* const overItem (_selUnit->getItem(inRule, x,y));
							if (overItem != nullptr)
							{
								std::string article (overItem->getRules()->getType()); // strip const. yay,
								if (_game->getSavedGame()->isResearched(article) == true)
									Ufopaedia::openArticle(_game, article);
							}
						}
					}
				}
				else // RMB w/ item on cursor
				{
					bool doGround;
					if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
						doGround = true;
					else
						doGround = false;

					setSelectedItem(); // Return item to original position.
					if (doGround == true)
					{
//						_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
//								   [static_cast<size_t>(_selItem->getSlotY())] += 1;
						arrangeGround();
					}
					drawItems();

					soundId = ResourcePack::ITEM_DROP;
				}
		}

		if (soundId != std::numeric_limits<unsigned>::max())
			_game->getResourcePack()->getSound("BATTLE.CAT", soundId)->play();

		_game->getSavedGame()->getBattleSave()->getBattleState()->refreshMousePosition();
		InteractiveSurface::mouseClick(action, state);
	}
}

/**
 * Gets the inventory section located in the specified mouse-position.
 * @param x - pointer to mouse x-position; returns the slot's x-position
 * @param y - pointer to mouse y-position; returns the slot's y-position
 * @return, pointer to section rules or nullptr if none
 */
RuleInventory* Inventory::getSlotAtCursor( // private.
		int* x,
		int* y) const
{
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		if (i->second->detSlotAtCursor(x,y) == true)
			return i->second;
	}
	return nullptr;
}

/**
 * Moves a specified BattleItem to any given x/y-slot in the selected unit's
 * Inventory while respecting inventory-sections.
 * @note Any required TU-checks and/or expenditures need to be done before
 * calling this function (although this adds the TU-weight-penalty for picking
 * an item up from the ground).
 * @param it		- pointer to a BattleItem (item to move)
 * @param inRule	- pointer to RuleInventory (section to move @a it to)
 * @param x			- x-position (default 0)
 * @param y			- y-position (default 0)
 */
void Inventory::moveItem( // private.
		BattleItem* const it,
		const RuleInventory* const inRule,
		int x,
		int y)
{
	if (it->getInventorySection() != inRule) // handle item from/to ground, or unload a weapon
	{
		if (inRule->getCategory() == IC_GROUND) // set down onto Ground, or unload a weapon-clip to Ground in pre-battle
		{
			it->changeOwner(); // NOTE: Already done in case of unload() during pre-battle equip.
			_selUnit->getUnitTile()->addItem(it);

			if (it->getBodyUnit() != nullptr) //&& it->getBodyUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
				it->getBodyUnit()->setPosition(_selUnit->getPosition());
		}
		else if (it->getInventorySection() == nullptr) // unload a weapon-clip to left hand
		{
			it->changeOwner(_selUnit);
		}
		else if (it->getInventorySection()->getCategory() == IC_GROUND) // pick up an item
		{
			it->changeOwner(_selUnit);

			_selUnit->getUnitTile()->removeItem(it);

			if (it->getBodyUnit() != nullptr) //&& it->getBodyUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
				it->getBodyUnit()->setPosition(Position::POS_BOGUS);

			if (_tuMode == true) // To prevent units from picking up large objects and running around on the same turn with nearly full TU
				_selUnit->setTu(_selUnit->getTu() - it->getRules()->getWeight()); // its weight becomes an extra tu-burden.
		}

		it->setInventorySection(inRule); // above, or simply moving item from one section to another
	}
	it->setSlotX(x);
	it->setSlotY(y);
}

/**
 * Attempts to place the item in an inventory section.
 * @param inRule	- pointer to where to place the item
 * @param it		- pointer to item to be placed
 * @param test		- true if only doing a test for tuCost, no move happens and
 *					  no check for overlaps is done (default false)
 * @return, true if the item can be successfully placed in the inventory
 */
bool Inventory::fitItem( // private.
		const RuleInventory* const inRule,
		BattleItem* const it,
		bool test)
{
	for (int
			y = 0;
			y <= inRule->getY() / RuleInventory::SLOT_H;
			++y)
	{
		for (int
				x = 0;
				x <= inRule->getX() / RuleInventory::SLOT_W;
				++x)
		{
			if (inRule->fitItemInSlot(it->getRules(), x,y))
			{
				if (test == true) return true; // NOTE: Called by mouseOver() to get/set tuCost.

				if (isOverlap(
							_selUnit,
							it,
							inRule,
							x,y) == false)
				{
					if (_tuMode == false
						|| _selUnit->expendTu(it->getInventorySection()->getCost(inRule)) == true)
					{
						// NOTE: Called only in mouseClick() and item will be from Ground.
//						_stackLevel[static_cast<size_t>(it->getSlotX())]
//								   [static_cast<size_t>(it->getSlotY())] -= 1;

						moveItem(it, inRule, x,y);
						arrangeGround();
						drawItems();

						_game->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)->play();
						return true;
					}
					else
					{
						_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u]));
						return false;
					}
				}
			}
		}
	}
	return false;
}

/**
 * Checks if two items can be stacked with one another.
 * @param itA - first item
 * @param itB - second item
 * @return, true if the items can be stacked on one another
 */
bool Inventory::canStack( // private.
		const BattleItem* const itA,
		const BattleItem* const itB)
{
	return (itA != nullptr && itB != nullptr														// both items actually exist
		&& itA->getRules() == itB->getRules()														// both items have the same ruleset
		&& ((itA->getAmmoItem() == nullptr && itB->getAmmoItem() == nullptr)						// either they both have no ammo
			|| (itA->getAmmoItem() && itB->getAmmoItem()												// or they both have ammo
				&& itA->getAmmoItem()->getRules() == itB->getAmmoItem()->getRules()						// and the same ammo type
				&& itA->getAmmoItem()->getAmmoQuantity() == itB->getAmmoItem()->getAmmoQuantity()))		// and the same ammo quantity
		&& itA->getFuse() == itB->getFuse()															// and both have the same fuse-setting
		&& itA->getBodyUnit() == nullptr && itB->getBodyUnit() == nullptr							// and neither is a corpse or unconscious unit
		&& itA->getPainKillerQuantity() == itB->getPainKillerQuantity()								// and if it's a medkit, it has the same number of charges
		&& itA->getHealQuantity() == itB->getHealQuantity()
		&& itA->getStimulantQuantity() == itB->getStimulantQuantity());
}

/**
 * Arranges items on the ground for the inventory display.
 * @note Since items on the ground aren't assigned to anyone they don't actually
 * have permanent slot positions.
 * @param dir - direction to shift ground-items: +1 right, -1 left (default 0)
 */
void Inventory::arrangeGround(int dir)
{
	_stackLevel.clear();

	const RuleInventory* const grdRule (_game->getRuleset()->getInventoryRule(ST_GROUND));

	std::vector<BattleItem*>* list (_selUnit->getUnitTile()->getInventory());

	// first move all items out of the way -> a big number in x-direction to right
	for (std::vector<BattleItem*>::const_iterator
			i = list->begin();
			i != list->end();
			++i)
	{
		(*i)->setInventorySection(grdRule);
		(*i)->setSlotX(100000);
		(*i)->setSlotY(0);
	}

	bool placed;
	int
		x,y,
		width,
		height,
		tallySlotX (0);

	const BattleItem* other;

	// for each item find the most top-left position that is not occupied and will fit
	for (std::vector<BattleItem*>::const_iterator
			i = list->begin();
			i != list->end();
			++i)
	{
		if ((*i)->getRules()->getBigSprite() != BIGSPRITE_NONE
			&& (width = (*i)->getRules()->getInventoryWidth()) != 0)
		{
			height = (*i)->getRules()->getInventoryHeight();

			x =
			y = 0;

			do
			{
				placed = true; // assume the item can be put here, if one of the following checks fails it can't
				for (int
						xd = 0;
						xd < width && placed == true;
						++xd)
				{
					if ((x + xd) % RuleInventory::GROUND_W < x % RuleInventory::GROUND_W)
					{
						placed = false; // don't let wider items overlap right-side ground boundary.
						break;
					}

					for (int
							yd = 0;
							yd < height && placed == true;
							++yd)
					{
						other = _selUnit->getItem(
												grdRule,
												x + xd,
												y + yd);
						placed = other == nullptr
							 || canStack(other, *i) == true;
					}
				}

				if (placed == true)
				{
					(*i)->setSlotX(x);
					(*i)->setSlotY(y);

					if (*i != _selItem)// && width != 0) // only increase the stack-level if the item is actually visible.
						_stackLevel[x][y] += 1;

					tallySlotX = std::max(x + width, tallySlotX);
				}
				else if (++y > RuleInventory::GROUND_H - height)
				{
					y = 0;
					++x;
				}
			}
			while (placed == false);
		}
	}

	if (dir != 0)
	{
		if (_selItem != nullptr)
			width = _selItem->getRules()->getInventoryWidth();
		else
			width = 0;

		if (tallySlotX > _grdOffset + (RuleInventory::GROUND_W * dir) - width
			&& (dir > 0 || _grdOffset > 0)) // don't let a shift-left go less than 0.
		{
			_grdOffset += (RuleInventory::GROUND_W * dir);
		}
		else
			_grdOffset = 0;
	}
}

/**
 * Checks if an item to be placed in a certain section-slot would overlap
 * another inventory item.
 * @param unit		- pointer to BattleUnit w/ inventory
 * @param it		- pointer to a BattleItem to be placed
 * @param inRule	- pointer to a RuleInventory section
 * @param x			- x-position in section (default 0)
 * @param y			- y-position in section (default 0)
 * @return, true if overlap
 */
bool Inventory::isOverlap( // static.
		BattleUnit* const unit,
		const BattleItem* const it,
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
				&& (*i)->occupiesSlot(x,y, it) == true)
			{
				return true;
			}
		}
	}
	else if (unit->getUnitTile() != nullptr)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getUnitTile()->getInventory()->begin();
				i != unit->getUnitTile()->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x,y, it) == true)
				return true;
		}
	}
	return false;
}

/**
 * Changes the inventory's Time Units mode.
 * - when TRUE inventory actions cost soldier time units (for battle);
 * - when FALSE inventory actions don't cost anything (for pre-equip).
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
	_grdOffset = 999;
	arrangeGround(+1);
	drawItems();
}

/**
 * Changes the item currently grabbed by the player.
 * @param it - pointer to selected BattleItem (default nullptr)
 */
void Inventory::setSelectedItem(BattleItem* const it)
{
	if (it != nullptr && it->getRules()->isFixed() == false)
	{
		_selItem = it;

		if (_selItem->getInventorySection()->getCategory() == IC_GROUND)
		{
//			_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
//					   [static_cast<size_t>(_selItem->getSlotY())] -= 1;
			arrangeGround();
		}

		_selItem->getRules()->drawHandSprite(
										_srtBigobs,
										_srfGrab);
	}
	else
	{
		_selItem = nullptr;
		_srfGrab->clear();
	}
}

/**
 * Gets the item currently grabbed by the player.
 * @return, pointer to selected BattleItem or nullptr if none
 */
BattleItem* Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently under mouse-cursor.
 * @param it - pointer to a BattleItem (default nullptr)
 */
void Inventory::setMouseOverItem(BattleItem* const it)
{
	if (it != nullptr && it->getRules()->isFixed() == false)
		_overItem = it;
	else
		_overItem = nullptr;
}

/**
 * Gets the item currently under mouse-cursor.
 * @return, pointer to a BattleItem or nullptr if none
 */
BattleItem* Inventory::getMouseOverItem() const
{
	return _overItem;
}

/**
 * Unloads the selected weapon placing the gun on the right hand and the ammo
 * on the left hand. If '_tuMode' is false then the load will drop to the ground
 * instead.
 * @return, true if a weapon is successfully unloaded
 */
bool Inventory::unload()
{
	if (_selItem == nullptr
		|| _selItem->selfPowered() == true
		|| _game->getSavedGame()->isResearched(_selItem->getRules()->getRequiredResearch()) == false)
	{
		return false;
	}

	BattleItem* const load (_selItem->getAmmoItem());
	if (load == nullptr)
	{
		if (_selItem->getRules()->getAcceptedLoadTypes()->empty() == false)
			_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[2u])); // weapon is not loaded

		return false;
	}

	InventorySection
		sectionLoad,
		sectionWeap;

	if (_tuMode == true)
	{
		for (std::vector<BattleItem*>::const_iterator
				i  = _selUnit->getInventory()->begin();
				i != _selUnit->getInventory()->end();
				++i)
		{
			if (*i != _selItem
				&& (*i)->getInventorySection() != nullptr
				&& (*i)->getInventorySection()->getCategory() == IC_HAND)
			{
				_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[10u])); // hands
				return false;
			}
		}

		if (_selUnit->expendTu(_selItem->getRules()->getUnloadTu()) == false)
		{
			_warning->showMessage(_game->getLanguage()->getString(BattlescapeGame::PLAYER_ERROR[0u])); // not enough TU
			return false;
		}

		sectionLoad = ST_LEFTHAND;
		sectionWeap = ST_RIGHTHAND;
	}
	else
	{
		sectionLoad = ST_GROUND;

		const BattleItem* const itRight (_selUnit->getItem(ST_RIGHTHAND));
		if (itRight == nullptr || itRight == _selItem)
			sectionWeap = ST_RIGHTHAND;
		else
			sectionWeap = ST_GROUND;
	}


	moveItem(
			_selItem,
			_game->getRuleset()->getInventoryRule(sectionWeap));
	_selItem->setAmmoItem();

	moveItem(
			load,
			_game->getRuleset()->getInventoryRule(sectionLoad));

	setSelectedItem();
	if (_tuMode == false) arrangeGround();
	drawItems();

	return true;
}

/**
 * Sets grenade to show a warning in Inventory.
 * @param turn - turns until grenade is to explode
 */
void Inventory::setPrimeGrenade(int turn)
{
	_primed = turn;
}

/**
 * Shows a warning message.
 * @param msg - reference a message to show
 */
void Inventory::showWarning(const std::wstring& wst)
{
	_warning->showMessage(wst);
}

/**
 * Gets the TU-cost for moving items around.
 * @return, time unit cost
 */
int Inventory::getTuCostInventory() const
{
	return _tuCost;

//	int wt;
//	if (_tuCost > 0 && _selItem->getInventorySection()->getId() == "STR_GROUND")
//		wt = _selItem->getRules()->getWeight();
//	else wt = 0;
//	return (_tuCost + wt);
}

}
