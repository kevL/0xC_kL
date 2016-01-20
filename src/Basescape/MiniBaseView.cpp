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

#include "MiniBaseView.h"

//#include <cmath>

#include "../Engine/Action.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Ruleset/RuleBaseFacility.h"
//#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Sets up a MiniBaseView with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 * @param mode		- MiniBaseViewType (default MBV_STANDARD)
 */
MiniBaseView::MiniBaseView(
		int width,
		int height,
		int x,
		int y,
		MiniBaseViewType mode)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_mode(mode),
		_texture(nullptr),
		_baseId(0),
		_hoverBase(0),
		_blink(false)
{
	if (_mode == MBV_STANDARD)
	{
		_timer = new Timer(250);
		_timer->onTimer((SurfaceHandler)& MiniBaseView::blink);
		_timer->start();
	}
}

/**
 * dTor.
 */
MiniBaseView::~MiniBaseView()
{
	if (_mode == MBV_STANDARD)
		delete _timer;
}

/**
 * Changes the current list of bases to display.
 * @param bases - pointer to a vector of pointers to Base
 */
void MiniBaseView::setBases(std::vector<Base*>* bases)
{
	_baseList = bases;
	_redraw = true;
}

/**
 * Changes the texture to use for drawing the Base's various elements.
 * @param texture - pointer to a SurfaceSet to use
 */
void MiniBaseView::setTexture(SurfaceSet* texture)
{
	_texture = texture;
}

/**
 * Returns the Base the mouse cursor is currently over.
 * @return, ID of the base
 */
size_t MiniBaseView::getHoveredBase() const
{
	return _hoverBase;
}

/**
 * Changes the Base that is currently selected on the MiniBaseView.
 * @param baseId - ID of the base
 */
void MiniBaseView::setSelectedBase(size_t baseId)
{
	kL_curBase =
	_baseId = baseId;

	_redraw = true;
}

/**
 * Draws the view of all the Base's with facilities in varying colors.
 */
void MiniBaseView::draw()
{
	Surface::draw();

	Base* base;
	int
		x,y;
	Uint8 color;

	for (size_t
			i = 0;
			i != Base::MAX_BASES;
			++i)
	{
		if (i == _baseId) // Draw white border.
		{
			SDL_Rect rect;
			rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2));
			rect.y = 0;
			rect.w =
			rect.h = static_cast<Uint16>(MINI_SIZE + 2);

			color = WHITE;
			if (i < _baseList->size())
			{
				base = _baseList->at(i);
				if (base->getBaseExposed() == true)
					color = RED_D;
			}

			drawRect(&rect, color);
		}

		_texture->getFrame(41)->setX(static_cast<int>(i) * (MINI_SIZE + 2)); // Draw base squares.
		_texture->getFrame(41)->setY(0);
		_texture->getFrame(41)->blit(this);


		if (i < _baseList->size()) // Draw facilities.
		{
			base = _baseList->at(i);

			if ((_mode == MBV_RESEARCH
					&& base->hasResearch() == false)
				|| (_mode == MBV_PRODUCTION
					&& base->hasProduction() == false))
			{
				continue;
			}

			lock();
			SDL_Rect rect;

			for (std::vector<BaseFacility*>::const_iterator
					j = base->getFacilities()->begin();
					j != base->getFacilities()->end();
					++j)
			{
				if ((*j)->buildFinished() == true)
					color = GREEN;
				else
					color = RED_D;

				rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2) + 2 + (*j)->getX() * 2);
				rect.y = static_cast<Sint16>(2 + (*j)->getY() * 2);
				rect.w =
				rect.h = static_cast<Uint16>((*j)->getRules()->getSize() * 2);
				drawRect(&rect, color + 3);

				++rect.x;
				++rect.y;
				--rect.w;
				--rect.h;
				drawRect(&rect, color + 5);

				--rect.x;
				--rect.y;
				drawRect(&rect, color + 2);

				++rect.x;
				++rect.y;
				--rect.w;
				--rect.h;
				drawRect(&rect, color + 3);

				--rect.x;
				--rect.y;
				setPixelColor(
							rect.x,
							rect.y,
							color + 1);
			}
			unlock();


			// Dot Marks for various base-status indicators.
			if (_mode == MBV_STANDARD)
			{
				x = static_cast<int>(i) * (MINI_SIZE + 2);

				if (base->getTransfers()->empty() == false) // incoming Transfers
					setPixelColor(
								x + 2,
								21,
								LAVENDER_L);

				if (base->getCrafts()->empty() == false)
				{
					const RuleCraft* craftRule;
					y = 17;

					for (std::vector<Craft*>::const_iterator
							j = base->getCrafts()->begin();
							j != base->getCrafts()->end();
							++j)
					{
						if ((*j)->getWarning() != CW_NONE)
						{
							if ((*j)->getWarning() == CW_CANTREFUEL)
								color = ORANGE_L;
							else if ((*j)->getWarning() == CW_CANTREARM)
								color = ORANGE_D;
							else //if ((*j)->getWarning() == CW_CANTREPAIR) // should not happen without a repair mechanic!
								color = RED_D;

							setPixelColor( // Craft needs materiels
										x + 2,
										19,
										color);
						}

						if ((*j)->getCraftStatus() == CS_READY)
							setPixelColor(
										x + 14,
										y,
										GREEN);


						craftRule = (*j)->getRules();

						if (craftRule->getRefuelItem().empty() == false)
							color = YELLOW_L;
						else
							color = YELLOW_D;

						setPixelColor(
									x + 12,
									y,
									color);

						if (craftRule->getWeapons() > 0
							&& craftRule->getWeapons() == (*j)->getNumWeapons())
						{
							setPixelColor(
										x + 10,
										y,
										BLUE);
						}

						if (craftRule->getSoldiers() > 0)
							setPixelColor(
										x + 8,
										y,
										BROWN);

						if (craftRule->getVehicles() > 0)
							setPixelColor(
										x + 6,
										y,
										LAVENDER_D);

						y += 2;
					}
				}
			}
		}
	}
}

/**
 * Selects the base the mouse is hovered over.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void MiniBaseView::mouseOver(Action* action, State* state)
{
	_hoverBase = static_cast<size_t>(std::floor(
				 action->getRelativeXMouse()) / (static_cast<double>(MINI_SIZE + 2) * action->getXScale()));

	if (_hoverBase > Base::MAX_BASES - 1)
		_hoverBase = Base::MAX_BASES - 1;

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the base the mouse was hovered over.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void MiniBaseView::mouseOut(Action* action, State* state)
{
	_hoverBase = Base::MAX_BASES;
	InteractiveSurface::mouseOut(action, state);
}

/**
 * Handles the blink() timer.
 */
void MiniBaseView::think()
{
	if (_mode == MBV_STANDARD)
		_timer->think(nullptr, this);
}

/**
 * Blinks the craft status indicators.
 */
void MiniBaseView::blink()
{
	_blink = !_blink;

	Base* base;
	CraftStatus stat;
	int
		x,y;
	Uint8 color;

	for (size_t
			i = 0;
			i != _baseList->size();
			++i)
	{
		base = _baseList->at(i);

		x = i * (MINI_SIZE + 2);

		if (base->getScientists() != 0 // unused Scientists &/or Engineers &/or PsiLab space
			|| base->getEngineers() != 0
			|| (base->getUsedPsiLabs() != base->getTotalPsiLabs()
				&& base->getUsedPsiLabs() < static_cast<int>(base->getSoldiers()->size())))
		{
			if (_blink == true)
				color = RED_L;
			else
				color = 0;

			setPixelColor(
						x + 2,
						17,
						color);
		}

		if (base->getCrafts()->empty() == false)
		{
			const RuleCraft* craftRule;
			y = 17;

			for (std::vector<Craft*>::const_iterator
					j = base->getCrafts()->begin();
					j != base->getCrafts()->end();
					++j)
			{
				stat = (*j)->getCraftStatus();
				if (stat != CS_READY)
				{
					if (_blink == true)
					{
						switch (stat)
						{
							default:
							case CS_OUT:
								color = GREEN;
								break;
							case CS_REFUELLING:
								color = ORANGE_L;
								break;
							case CS_REARMING:
								color = ORANGE_D;
								break;
							case CS_REPAIRS:
								color = RED_D;
						}
					}
					else
						color = 0;

					setPixelColor(
								x + 14,
								y,
								color);
				}

				craftRule = (*j)->getRules();
				if (craftRule->getWeapons() > 0 // craft needs Weapons mounted.
					&& craftRule->getWeapons() != (*j)->getNumWeapons())
				{
					if (_blink == true)
						color = BLUE;
					else
						color = 0;

					setPixelColor(
								x + 10,
								y,
								color);
				}

				y += 2;
			}
		}
	}
}

}
