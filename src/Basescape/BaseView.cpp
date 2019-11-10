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

#include "BaseView.h"

//#include <algorithm>
#include <cmath>
//#include <limits>
//#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"

#include "../Ruleset/RuleBaseFacility.h"
//#include "../Ruleset/RuleCraft.h"

#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"


namespace OpenXcom
{

/**
 * Sets up the BaseView with a specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
BaseView::BaseView(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_base(nullptr),
		_texture(nullptr),
		_srfDog(nullptr),
		_selFacility(nullptr),
		_big(nullptr),
		_small(nullptr),
		_lang(nullptr),
		_gridX(0),
		_gridY(0),
		_selSize(0),
		_selector(nullptr),
		_blink(true)
{
	for (size_t
			x1 = 0u;
			x1 != Base::BASE_SIZE;
			++x1)
	{
		for (size_t
				y1 = 0u;
				y1 != Base::BASE_SIZE;
				++y1)
		{
			_facilities[x1][y1] = nullptr;
		}
	}

	_timer = new Timer(125u);
	_timer->onTimer(static_cast<SurfaceHandler>(&BaseView::blink));
	_timer->start();
}

/**
 * Deletes contents.
 */
BaseView::~BaseView()
{
	delete _selector;
	delete _timer;
}

/**
 * Changes the various resources needed for text-rendering.
 * @note The different fonts need to be passed in advance since the text-size
 * can change mid-text and the language affects how the Text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void BaseView::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_big = big;
	_small = small;
	_lang = lang;
}

/**
 * Changes the Base to display and initializes the internal base-grid.
 * @param base - pointer to Base to display
 */
void BaseView::setBase(Base* const base)
{
	_base = base;
	_selFacility = nullptr;

	for (size_t
			x = 0u;
			x != Base::BASE_SIZE;
			++x)
	{
		for (size_t
				y = 0u;
				y != Base::BASE_SIZE;
				++y)
		{
			_facilities[x][y] = nullptr;
		}
	}

	size_t
		facX,
		facY,
		facSize;
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		facX = static_cast<size_t>((*i)->getX());
		facY = static_cast<size_t>((*i)->getY());
		facSize = (*i)->getRules()->getSize();

		for (size_t
				y = facY;
				y != facY + facSize;
				++y)
		{
			for (size_t
					x = facX;
					x != facX + facSize;
					++x)
			{
				_facilities[x][y] = *i;
			}
		}
	}
	_redraw = true;
}

/**
 * Sets the texture to use for drawing the Base's bits.
 * @param texture - pointer to SurfaceSet to use
 */
void BaseView::setTexture(SurfaceSet* const texture)
{
	_texture = texture;
}

/**
 * Sets the dog to draw at the Base.
 * @param texture - pointer to Surface to use
 */
void BaseView::setDog(Surface* const dog)
{
	_srfDog = dog;
}

/**
 * Gets the facility the mouse is currently over.
 * @return, pointer to base facility (0 if none)
 */
BaseFacility* BaseView::getSelectedFacility() const
{
	return _selFacility;
}

/**
 * Prevents any mouseover bugs on dismantling base facilities before setBase has
 * had time to update the base.
 */
void BaseView::resetSelectedFacility()
{
	_facilities[static_cast<size_t>(_selFacility->getX())]
			   [static_cast<size_t>(_selFacility->getY())] = nullptr;

	_selFacility = nullptr;
}

/**
 * Gets the x-position of the grid square the mouse is currently over.
 * @return, x-position on the grid
 */
int BaseView::getGridX() const
{
	return _gridX;
}

/**
 * Gets the y-position of the grid-square the mouse is currently over.
 * @return, y-position on the grid
 */
int BaseView::getGridY() const
{
	return _gridY;
}

/**
 * If enabled this BaseView will highlight the selected facility.
 * @param facSize - X/Y dimension in pixels (0 to disable)
 */
void BaseView::highlightFacility(size_t facSize)
{
	if ((_selSize = static_cast<int>(facSize)) != 0)
	{
		_selector = new Surface(
							_selSize * GRID_SIZE,
							_selSize * GRID_SIZE,
							_x,_y);
		_selector->setPalette(getPalette());

		SDL_Rect rect;
		rect.x =
		rect.y = 0;
		rect.w = static_cast<Uint16>(_selector->getWidth());
		rect.h = static_cast<Uint16>(_selector->getHeight());
		_selector->drawRect(&rect, _selectorColor);

		++rect.x;
		++rect.y;
		rect.w = static_cast<Uint16>(rect.w - 2u);
		rect.h = static_cast<Uint16>(rect.h - 2u);

		_selector->drawRect(&rect, 0u);
		_selector->setVisible(false);
	}
	else
		delete _selector;
}

/**
 * Returns if a facility can be placed on the currently selected square.
 * @param facRule - pointer to RuleBaseFacility
 * @return, true if allowed
 */
bool BaseView::isPlaceable(const RuleBaseFacility* const facRule) const
{
	if (   _gridX < 0
		|| _gridY < 0
		|| _gridX >= static_cast<int>(Base::BASE_SIZE)
		|| _gridY >= static_cast<int>(Base::BASE_SIZE))
	{
		return false;
	}

	const size_t
		gridX (static_cast<size_t>(_gridX)),
		gridY (static_cast<size_t>(_gridY)),
		facSize (facRule->getSize());

	for (size_t // check for overlaps
			x = 0u;
			x != facSize;
			++x)
	{
		for (size_t
				y = 0u;
				y != facSize;
				++y)
		{
			if (_facilities[gridX + x][gridY + y] != nullptr)
				return false;
		}
	}

	const bool allowQ (Options::allowBuildingQueue);
	for (size_t // check for a facility to connect to
			i = 0u;
			i != facSize;
			++i)
	{
		if ((	   gridX != 0u							// check left
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX - 1u]
							  [gridY + i] != nullptr
				&& (allowQ == true
					|| _facilities[gridX - 1u]
								  [gridY + i]
							->buildFinished() == true))

			|| (   gridX + i < Base::BASE_SIZE			// check top
				&& gridY != 0u
				&& _facilities[gridX + i]
							  [gridY - 1u] != nullptr
				&& (allowQ == true
					|| _facilities[gridX + i]
								  [gridY - 1u]
							->buildFinished() == true))

			|| (   gridX + facSize < Base::BASE_SIZE	// check right
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX + facSize]
							  [gridY + i] != nullptr
				&& (allowQ == true
					|| _facilities[gridX + facSize]
								  [gridY + i]
							->buildFinished() == true))

			|| (   gridX + i < Base::BASE_SIZE			// check bottom
				&& gridY + facSize < Base::BASE_SIZE
				&& _facilities[gridX + i]
							  [gridY + facSize] != nullptr
				&& (allowQ == true
					|| _facilities[gridX + i]
								  [gridY + facSize]
							->buildFinished() == true)))
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns if the placed facility is placed in queue or not.
 * @param facRule - pointer to RuleBaseFacility
 * @return, true if queued
 */
bool BaseView::isQueuedBuilding(const RuleBaseFacility* const facRule) const
{
	const size_t
		gridX (static_cast<size_t>(_gridX)),
		gridY (static_cast<size_t>(_gridY)),
		facSize (facRule->getSize());

	for (size_t
			i = 0u;
			i != facSize;
			++i)
	{
		if ((	   gridX != 0u							// check left
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX - 1u]
							  [gridY + i] != nullptr
				&& _facilities[gridX - 1u]
							  [gridY + i]
						->buildFinished() == true)

			|| (   gridX + i < Base::BASE_SIZE			// check top
				&& gridY != 0u
				&& _facilities[gridX + i]
							  [gridY - 1u] != nullptr
				&& _facilities[gridX + i]
							  [gridY - 1u]
						->buildFinished() == true)

			|| (   gridX + facSize < Base::BASE_SIZE	// check right
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX + facSize]
							  [gridY + i] != nullptr
				&& _facilities[gridX + facSize]
							  [gridY + i]
						->buildFinished() == true)

			|| (   gridX + i < Base::BASE_SIZE			// check bottom
				&& gridY + facSize < Base::BASE_SIZE
				&& _facilities[gridX + i]
							  [gridY + facSize] != nullptr
				&& _facilities[gridX + i]
							  [gridY + facSize]
						->buildFinished() == true))
		{
			return false;
		}
	}

	return true;
}

/**
 * ReCalculates the remaining build-time of all queued buildings.
 */
void BaseView::reCalcQueuedBuildings()
{
	setBase(_base);

	std::vector<BaseFacility*> facilities;
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->buildFinished() == false) // set all queued buildings to infinite.
		{
			if ((*i)->getBuildTime() > (*i)->getRules()->getBuildTime())
				(*i)->setBuildTime(std::numeric_limits<int>::max());

			facilities.push_back(*i);
		}
	}

	std::vector<BaseFacility*>::const_iterator pFac;
	const BaseFacility* fac;
	size_t
		x,y,
		facSize;

	while (facilities.empty() == false) // applying a simple Dijkstra Algorithm
	{
		pFac = facilities.begin();
		for (std::vector<BaseFacility*>::const_iterator
				i = facilities.begin();
				i != facilities.end();
				++i)
		{
			if ((*i)->getBuildTime() < (*pFac)->getBuildTime())
				pFac = i;
		}

		fac = *pFac;
		facilities.erase(pFac);

		x = static_cast<size_t>(fac->getX());
		y = static_cast<size_t>(fac->getY());
		facSize = fac->getRules()->getSize();

		for (size_t
				i = 0u;
				i != facSize;
				++i)
		{
			if (x != 0u)
				updateNeighborFacilityBuildTime(
											fac,
											_facilities[x - 1u]
													   [y + i]);

			if (y != 0u)
				updateNeighborFacilityBuildTime(
											fac,
											_facilities[x + i]
													   [y - 1u]);

			if (x + facSize < Base::BASE_SIZE)
				updateNeighborFacilityBuildTime(
											fac,
											_facilities[x + facSize]
													   [y + i]);

			if (y + facSize < Base::BASE_SIZE)
				updateNeighborFacilityBuildTime(
											fac,
											_facilities[x + i]
													   [y + facSize]);
		}
	}
}

/**
 * Updates a neighboring BaseFacility's build-time.
 * @note This is for internal use only by reCalcQueuedBuildings().
 * @param facility - pointer to a BaseFacility
 * @param neighbor - pointer to a neighboring BaseFacility
 */
void BaseView::updateNeighborFacilityBuildTime( // private.
		const BaseFacility* const facility,
		BaseFacility* const neighbor)
{
	if (facility != nullptr && neighbor != nullptr)
	{
		const int
			facBuild (facility->getBuildTime()),
			borBuild (neighbor->getBuildTime()),
			borTotal (neighbor->getRules()->getBuildTime());

		if (borBuild > borTotal
			&& facBuild + borTotal < borBuild)
		{
			neighbor->setBuildTime(facBuild + borTotal);
		}
	}
}

/**
 * Keeps the animation Timers running.
 */
void BaseView::think()
{
	_timer->think(nullptr, this);
}

/**
 * Makes the facility-selector blink.
 */
void BaseView::blink()
{
	if (_selSize != 0)
	{
		SDL_Rect rect;

		if ((_blink = !_blink) == true)
		{
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Uint16>(_selector->getWidth());
			rect.h = static_cast<Uint16>(_selector->getHeight());
			_selector->drawRect(&rect, _selectorColor);

			++rect.x;
			++rect.y;
			rect.w = static_cast<Uint16>(rect.w - 2u);
			rect.h = static_cast<Uint16>(rect.h - 2u);
			_selector->drawRect(&rect, 0);
		}
		else
		{
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Uint16>(_selector->getWidth());
			rect.h = static_cast<Uint16>(_selector->getHeight());
			_selector->drawRect(&rect, 0u);
		}
	}
}

/**
 * Draws the view of all the facilities in the base with connectors between
 * them and crafts currently based in hangars.
 * @note This does not draw large facilities that are under construction -
 * that is only the dotted building-outline is shown.
 */
void BaseView::draw()
{
	Surface::draw();

	Surface* srf;

	static const int baseSize (static_cast<int>(Base::BASE_SIZE));
	for (int // draw grid squares
			x = 0;
			x != baseSize;
			++x)
	{
		for (int
				y = 0;
				y != baseSize;
				++y)
		{
			srf = _texture->getFrame(0);
			srf->setX(x * GRID_SIZE);
			srf->setY(y * GRID_SIZE);
			srf->blit(this);
		}
	}

	int
		facSize,
		spriteId;

	for (std::vector<BaseFacility*>::const_iterator // draw facility shape
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		facSize = static_cast<int>((*i)->getRules()->getSize());
		for (int
				y = (*i)->getY(), j = 0;
				y != (*i)->getY() + facSize;
				++y)
		{
			for (int
					x = (*i)->getX();
					x != (*i)->getX() + facSize;
					++x, ++j)
			{
				spriteId = (*i)->getRules()->getSpriteShape() + j;
				if ((*i)->buildFinished() == false)
					spriteId += std::max(3, // outline
										 facSize * facSize);

				srf = _texture->getFrame(spriteId);
				srf->setX(x * GRID_SIZE);
				srf->setY(y * GRID_SIZE);
				srf->blit(this);
			}
		}
	}

	size_t
		facSize_t,
		facX,facY,
		x,y;

	for (std::vector<BaseFacility*>::const_iterator // draw connectors
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->buildFinished() == true)
		{
			facX = static_cast<size_t>((*i)->getX());
			facY = static_cast<size_t>((*i)->getY());
			facSize_t = (*i)->getRules()->getSize();
			x = facX + facSize_t; // facilities to the right
			y = facY + facSize_t; // facilities to the bottom

			if (x < Base::BASE_SIZE)
			{
				for (size_t
						y1 = facY;
						y1 != facY + facSize_t;
						++y1)
				{
					if (   _facilities[x][y1] != nullptr
						&& _facilities[x][y1]->buildFinished() == true)
					{
						srf = _texture->getFrame(7);
						srf->setX(static_cast<int>(x)  * GRID_SIZE - (GRID_SIZE >> 1u));
						srf->setY(static_cast<int>(y1) * GRID_SIZE);
						srf->blit(this);
					}
				}
			}

			if (y < Base::BASE_SIZE)
			{
				for (size_t
						x1 = facX;
						x1 != facX + facSize_t;
						++x1)
				{
					if (   _facilities[x1][y] != nullptr
						&& _facilities[x1][y]->buildFinished() == true)
					{
						srf = _texture->getFrame(8);
						srf->setX(static_cast<int>(x1) * GRID_SIZE);
						srf->setY(static_cast<int>(y)  * GRID_SIZE - (GRID_SIZE >> 1u));
						srf->blit(this);
					}
				}
			}
		}
	}

	for (std::vector<BaseFacility*>::const_iterator // draw facility graphic
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		(*i)->setCraft(); // nullptr these to prepare hangers for population by Craft.

		facSize = static_cast<int>((*i)->getRules()->getSize());
		for (int
				yOffset = (*i)->getY(), j = 0;
				yOffset != (*i)->getY() + facSize;
				++yOffset)
		{
			for (int
					xOffset = (*i)->getX();
					xOffset != (*i)->getX() + facSize;
					++xOffset, ++j)
			{
				if (facSize == 1)
				{
					srf = _texture->getFrame((*i)->getRules()->getSpriteFacility() + j);
					srf->setX(xOffset * GRID_SIZE);
					srf->setY(yOffset * GRID_SIZE);
					srf->blit(this);
				}
			}
		}

		if ((*i)->buildFinished() == false) // draw time remaining
		{
			Text* const text (new Text(
									GRID_SIZE * facSize,
									16,
									0,0));

			text->setPalette(getPalette());
			text->initText(
						_big,
						_small,
						_lang);
			text->setX(((*i)->getX() * GRID_SIZE) - 1);
			text->setY(((*i)->getY() * GRID_SIZE) + ((GRID_SIZE * facSize - 16) >> 1u));
			text->setBig();

			std::wostringstream woststr;
			woststr << (*i)->getBuildTime();
			text->setAlign(ALIGN_CENTER);
			text->setColor(_cellColor);
			text->setText(woststr.str());

			text->blit(this);

			delete text;
		}
	}

	std::vector<Craft*>::const_iterator pCraft (_base->getCrafts()->begin()); // draw Craft left to right, top row to bottom.
	BaseFacility* fac;
	bool hasDog (_base->getStorageItems()->getItemQuantity("STR_DOG") != 0);
	std::vector<std::pair<int,int>> dogPosition;
	int
		posDog_x,
		posDog_y;

	for (
			y = 0u;
			y != Base::BASE_SIZE;
			++y)
	{
		for (
				x = 0u;
				x != Base::BASE_SIZE;
				++x)
		{
			if ((fac = _facilities[x][y]) != nullptr)
			{
				facSize = static_cast<int>(fac->getRules()->getSize());

				if (pCraft != _base->getCrafts()->end()
					&& fac->buildFinished() == true
					&& fac->getRules()->getCrafts() != 0
					&& fac->getCraft() == nullptr)
				{
					if ((*pCraft)->getCraftStatus() != CS_OUT)
					{
						srf = _texture->getFrame((*pCraft)->getRules()->getSprite() + 33);
						srf->setX(fac->getX() * GRID_SIZE + (((facSize - 1) * GRID_SIZE) >> 1u) + 2);
						srf->setY(fac->getY() * GRID_SIZE + (((facSize - 1) * GRID_SIZE) >> 1u) - 4);
						srf->blit(this);
					}

					fac->setCraft(*pCraft);
					++pCraft;
				}

				if (hasDog == true
					&& (fac->getCraft() == nullptr
						|| fac->getCraft()->getCraftStatus() == CS_OUT))
				{
					posDog_x = fac->getX() * GRID_SIZE + facSize * RNG::seedless(2,11);
					posDog_y = fac->getY() * GRID_SIZE + facSize * RNG::seedless(2,17);
					std::pair<int,int> posDog (std::make_pair(posDog_x, posDog_y));
					dogPosition.push_back(posDog);
				}
			}
		}
	}

	if (dogPosition.empty() == false) // draw dog
	{
		const size_t i (RNG::pick(dogPosition.size(), true));
		_srfDog->setX(dogPosition[i].first);
		_srfDog->setY(dogPosition[i].second);
		_srfDog->blit(this);
	}
}

/**
 * Blits this BaseView and its selector.
 * @param srf - pointer to a Surface to blit to
 */
void BaseView::blit(const Surface* const srf)
{
	Surface::blit(srf);

	if (_selector != nullptr)
		_selector->blit(srf);
}

/**
 * Selects the facility the mouse is over.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void BaseView::mouseOver(Action* action, State* state)
{
	_gridX = static_cast<int>(std::floor(
			 action->getRelativeMouseX() / (static_cast<double>(GRID_SIZE) * action->getScaleX())));
	_gridY = static_cast<int>(std::floor(
			 action->getRelativeMouseY() / (static_cast<double>(GRID_SIZE) * action->getScaleY())));

	if (   _gridX > -1
		&& _gridX < static_cast<int>(Base::BASE_SIZE)
		&& _gridY > -1
		&& _gridY < static_cast<int>(Base::BASE_SIZE))
	{
		_selFacility = _facilities[static_cast<size_t>(_gridX)]
								  [static_cast<size_t>(_gridY)];
		if (_selSize != 0)
		{
			if (   _gridX + _selSize <= static_cast<int>(Base::BASE_SIZE)
				&& _gridY + _selSize <= static_cast<int>(Base::BASE_SIZE))
			{
				_selector->setX(_x + _gridX * GRID_SIZE);
				_selector->setY(_y + _gridY * GRID_SIZE);
				_selector->setVisible();
			}
			else
				_selector->setVisible(false);
		}
	}
	else
	{
		_selFacility = nullptr;
		if (_selSize != 0) _selector->setVisible(false);
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the facility.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void BaseView::mouseOut(Action* action, State* state)
{
	_selFacility = nullptr;
	if (_selSize != 0) _selector->setVisible(false);

	InteractiveSurface::mouseOut(action, state);
}

/**
 * Sets the primary color.
 * @param color - primary color
 */
void BaseView::setColor(Uint8 color)
{
	_cellColor = color;
}

/**
 * Sets the secondary color.
 * @param color - secondary color
 */
void BaseView::setSecondaryColor(Uint8 color)
{
	_selectorColor = color;
}

}
