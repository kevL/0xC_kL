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

#include "ManufactureState.h"

//#include <sstream>

#include "BasescapeState.h"
#include "ManufactureInfoState.h"
#include "ManufactureListState.h"
#include "MiniBaseView.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Manufacture.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Manufacture screen.
 * @param base	- pointer to the Base to get info from
 * @param state	- pointer to the BasescapeState (default nullptr when geoscape-invoked)
 */
ManufactureState::ManufactureState(
		Base* const base,
		BasescapeState* const state)
	:
		_base(base),
		_state(state),
		_baseList(_game->getSavedGame()->getBases())
{
	_window			= new Window(this, 320, 200);

	_mini			= new MiniBaseView(128, 16, 180, 27, MBV_PRODUCTION);

	_txtTitle		= new Text(320, 17,   0, 10);
	_txtBaseLabel	= new Text( 80,  9,  16, 10);
	_txtHoverBase	= new Text( 80,  9, 224, 10);

	_txtAllocated	= new Text(60, 9, 16, 26);
	_txtAvailable	= new Text(60, 9, 16, 35);

	_txtSpace		= new Text(100, 9, 80, 26);
	_txtFunds		= new Text(100, 9, 80, 35);

	_txtItem		= new Text(120,  9,  16, 53);
	_txtEngineers	= new Text( 45,  9, 145, 53);
	_txtProduced	= new Text( 40,  9, 174, 45);
	_txtCost		= new Text( 50, 17, 215, 45);
	_txtDuration	= new Text( 25, 17, 271, 45);

	_lstManufacture	= new TextList(285, 81, 16,  71);
	_lstResources	= new TextList(285, 17, 16, 156);

	_btnNew			= new TextButton(134, 16,  16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setInterface("manufactureMenu");

	add(_window,			"window",	"manufactureMenu");
	add(_mini,				"miniBase",	"basescape");		// <-
	add(_txtTitle,			"text1",	"manufactureMenu");
	add(_txtBaseLabel,		"text1",	"manufactureMenu");
	add(_txtHoverBase,		"numbers",	"baseInfo");		// <-
	add(_txtAvailable,		"text1",	"manufactureMenu");
	add(_txtAllocated,		"text1",	"manufactureMenu");
	add(_txtSpace,			"text1",	"manufactureMenu");
	add(_txtFunds,			"text1",	"manufactureMenu");
	add(_txtItem,			"text2",	"manufactureMenu");
	add(_txtEngineers,		"text2",	"manufactureMenu");
	add(_txtProduced,		"text2",	"manufactureMenu");
	add(_txtCost,			"text2",	"manufactureMenu");
	add(_txtDuration,		"text2",	"manufactureMenu");
	add(_lstManufacture,	"list",		"manufactureMenu");
	add(_lstResources,		"list",		"manufactureMenu");
	add(_btnNew,			"button",	"manufactureMenu");
	add(_btnOk,				"button",	"manufactureMenu");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_baseList);
	for (size_t
			i = 0u;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == _base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(static_cast<ActionHandler>(&ManufactureState::miniClick));
	_mini->onMouseOver(	static_cast<ActionHandler>(&ManufactureState::miniMouseOver));
	_mini->onMouseOut(	static_cast<ActionHandler>(&ManufactureState::miniMouseOut));
	// TODO: onKeyboardPress(). See BaseInfoState.

	_txtHoverBase->setAlign(ALIGN_RIGHT);

	_btnNew->setText(tr("STR_NEW_PRODUCTION"));
	_btnNew->onMouseClick(static_cast<ActionHandler>(&ManufactureState::btnNewProductionClick));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&ManufactureState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&ManufactureState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_PRODUCTION"));

	_txtFunds->setText(tr("STR_CURRENT_FUNDS_")
						.arg(Text::formatCurrency(_game->getSavedGame()->getFunds())));

	_txtItem->setText(tr("STR_ITEM"));
	_txtEngineers->setText(tr("STR_ENGINEERS_ALLOCATED"));
	_txtProduced->setText(tr("STR_UNITS_PRODUCED"));
	_txtCost->setText(tr("STR_COST_PER_UNIT"));
	_txtDuration->setText(tr("STR_DAYS_HOURS_LEFT"));

	_lstManufacture->setColumns(5, 121,29,41,56,30);
	_lstManufacture->setSelectable();
	_lstManufacture->setBackground(_window);
	_lstManufacture->onMouseClick(static_cast<ActionHandler>(&ManufactureState::lstManufactureClick));

	_lstResources->setColumns(4, 151,50,40,40);
	_lstResources->setMargin(4);
	_lstResources->setDot();

	const Uint8 color (static_cast<Uint8>(_game->getRuleset()->getInterface("manufactureMenu")->getElement("text1")->color2));
	std::string st;
	for (size_t
			i = 0u;
			i != 2u;
			++i)
	{
		switch (i)
		{
			case 0u: st = "STR_ALIEN_ALLOYS"; break;
			default: st = "STR_ELERIUM_115";
		}

		_lstResources->addRow(4,
							tr(st).c_str(),
							L"",
							tr("STR_TOTAL").c_str(),
							L"");
		_lstResources->setCellColor(i, 1u, color);
		_lstResources->setCellColor(i, 3u, color);
	}
}

/**
 * dTor.
 */
ManufactureState::~ManufactureState()
{}

/**
 * Updates the Manufacture list after going to other screens.
 */
void ManufactureState::init()
{
	State::init();

	_txtBaseLabel->setText(_base->getLabel());

	_lstManufacture->clearList();

	const std::vector<Manufacture*> baseProds (_base->getManufacture());
	for (std::vector<Manufacture*>::const_iterator
			i = baseProds.begin();
			i != baseProds.end();
			++i)
	{
		std::wostringstream
			woststr1,
			woststr2,
			woststr3;

		if ((*i)->getAutoSales() == true)
		{
//			std::streamsize strSize = woststr1.tellp();
//			woststr1.str(L"$" + woststr1.str());
//			woststr1.seekp(strSize + 1); // lolc++

			woststr1 << L"$ ";
		}
		woststr1 << tr((*i)->getRules()->getType());

		woststr2 << (*i)->getProducedQuantity() << L"/";
		if ((*i)->getInfinite() == true)
			woststr2 << L"oo";
		else
			woststr2 << (*i)->getProductionTotal();

		int
			days,
			hours;
		if ((*i)->tillFinish(days, hours) == true)
			woststr3 << days << L"/" << hours;
		else
			woststr3 << L"oo";

		_lstManufacture->addRow
							(5,
							woststr1.str().c_str(),
							Text::intWide((*i)->getAssignedEngineers()).c_str(),
							woststr2.str().c_str(),
							Text::formatCurrency((*i)->getRules()->getManufactureCost()).c_str(),
							woststr3.str().c_str());
	}

	_txtAvailable->setText(tr("STR_ENGINEERS_AVAILABLE_")
							.arg(_base->getEngineers()));
	_txtAllocated->setText(tr("STR_ENGINEERS_ALLOCATED_")
							.arg(_base->getAllocatedEngineers()));
	_txtSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE_")
							.arg(_base->getFreeWorkshops()));

	int
		qtyA (0),
		qtyE (0),
		totalA (0),
		totalE (0),
		ally,
		eler;
	for (std::vector<Base*>::const_iterator
			i = _baseList->begin();
			i != _baseList->end();
			++i)
	{
		ally = (*i)->getStorageItems()->getItemQuantity("STR_ALIEN_ALLOYS");
		eler = (*i)->getStorageItems()->getItemQuantity("STR_ELERIUM_115");

		totalA += ally;
		totalE += eler;

		if (*i == _base)
		{
			qtyA += ally;
			qtyE += eler;
		}

		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			if ((*j)->getTransferItems() == "STR_ALIEN_ALLOYS")
			{
				ally = (*j)->getQuantity();
				totalA += ally;
				if (*i == _base) qtyA += ally;
			}
			else if ((*j)->getTransferItems() == "STR_ELERIUM_115")
			{
				eler = (*j)->getQuantity();
				totalA += eler;
				if (*i == _base) qtyE += eler;
			}
		}
	}
	_lstResources->setCellText(0u,1u, Text::intWide(qtyA));
	_lstResources->setCellText(1u,1u, Text::intWide(qtyE));
	_lstResources->setCellText(0u,3u, Text::intWide(totalA));
	_lstResources->setCellText(1u,3u, Text::intWide(totalE));
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ManufactureState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the screen with a list of available Productions.
 * @param action - pointer to an Action
 */
void ManufactureState::btnNewProductionClick(Action*)
{
	_game->pushState(new ManufactureListState(_base));
}

/**
 * Opens the screen displaying production settings.
 * @param action - pointer to an Action
 */
void ManufactureState::lstManufactureClick(Action*)
{
	const std::vector<Manufacture*> baseProds(_base->getManufacture());
	_game->pushState(new ManufactureInfoState(
											_base,
											baseProds[_lstManufacture->getSelectedRow()]));
}

/**
 * Selects a different Base to display.
 * TODO: Implement key-presses to switch bases.
 * @param action - pointer to an Action
 */
void ManufactureState::miniClick(Action*)
{
	if (_state != nullptr) // cannot switch bases if origin is Geoscape.
	{
		const size_t baseId (_mini->getHoveredBase());
		if (baseId < _baseList->size())
		{
			Base* const base (_baseList->at(baseId));
			if (base != _base && base->hasProduction() == true)
			{
				_txtHoverBase->setText(L"");

				_mini->setSelectedBase(baseId);
				_state->setBase(_base = base);

				_state->resetStoresWarning();
				init();
			}
		}
	}
}

/**
 * Displays the label of the Base the mouse is over.
 * @param action - pointer to an Action
 */
void ManufactureState::miniMouseOver(Action*)
{
	const size_t baseId (_mini->getHoveredBase());
	const Base* const base (_baseList->at(baseId));

	if (baseId < _baseList->size()
		&& base != _base
		&& base->hasProduction() == true)
	{
		_txtHoverBase->setText(base->getLabel().c_str());
	}
	else
		_txtHoverBase->setText(L"");
}

/**
 * Clears the hovered Base label.
 * @param action - pointer to an Action
 */
void ManufactureState::miniMouseOut(Action*)
{
	_txtHoverBase->setText(L"");
}

}
