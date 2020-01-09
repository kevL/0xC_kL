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

#include "InterceptState.h"

//#include <sstream>

#include "ConfirmDestinationState.h"
#include "GeoscapeCraftState.h"
#include "GeoscapeState.h"
#include "Globe.h"

#include "../Basescape/BasescapeState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Intercept window.
 * @param geoState	- pointer to GeoscapeState
 * @param base		- pointer to Base to show crafts at a particular base (default nullptr to show all Craft)
 * @param ufo		- pointer to Ufo to intercept a freshly detected UFO (default nullptr)
 */
InterceptState::InterceptState(
		GeoscapeState* const geoState,
		Base* const base,
		Ufo* const ufo)
	:
		_geoState(geoState),
		_base(base),
		_ufo(ufo)
{
	_fullScreen = false;

	int dx; // x - 32 to center on Globe
	if (Options::baseXResolution > 320 + 32)
		dx = -32;
	else
		dx = 0;

	_window     = new Window(
						this,
						320,176,
						0 + dx, 14,
						POPUP_HORIZONTAL);
	_txtBase    = new Text(288, 16, 16 + dx, 24); // might do getRegion in here also.

	_txtCraft   = new Text(86,  9,  16 + dx, 40);
	_txtStatus  = new Text(53,  9, 115 + dx, 40);
	_txtWeapons = new Text(50, 27, 241 + dx, 24);

	_lstCrafts  = new TextList(285, 113, 16 + dx, 50);

	_btnBase    = new TextButton(142, 16,  16 + dx, 167);
	_btnCancel  = new TextButton(142, 16, 162 + dx, 167);

	setInterface("geoCraftScreens");

	add(_window,     "window", "geoCraftScreens");
	add(_txtBase,    "text2",  "geoCraftScreens");
	add(_txtCraft,   "text2",  "geoCraftScreens");
	add(_txtStatus,  "text2",  "geoCraftScreens");
	add(_txtWeapons, "text2",  "geoCraftScreens");
	add(_lstCrafts,  "list",   "geoCraftScreens");
	add(_btnBase,    "button", "geoCraftScreens");
	add(_btnCancel,  "button", "geoCraftScreens");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK12.SCR"), dx);

	if (_base != nullptr)
	{
		_btnBase->setText(_base->getLabel()); //tr("STR_GO_TO_BASE")
		_btnBase->onMouseClick(static_cast<ActionHandler>(&InterceptState::btnGotoBaseClick));
	}
	else
		_btnBase->setVisible(false);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick(	static_cast<ActionHandler>(&InterceptState::btnCancelClick));
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&InterceptState::btnCancelClick),
								Options::keyCancel);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&InterceptState::btnCancelClick),
								Options::keyOk);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&InterceptState::btnCancelClick),
								Options::keyOkKeypad);
	_btnCancel->onKeyboardPress(static_cast<ActionHandler>(&InterceptState::btnCancelClick),
								Options::keyGeoIntercept);

	_txtCraft->setText(tr("STR_CRAFT"));

	_txtStatus->setText(tr("STR_STATUS"));

	_txtBase->setText(tr("STR_INTERCEPT"));
	_txtBase->setBig();

	_txtWeapons->setText(tr("STR_WEAPONS_CREW_HWPS"));

	_lstCrafts->setColumns(5, 91,126,25,15,15);
	_lstCrafts->setBackground(_window);
	_lstCrafts->setSelectable();
	_lstCrafts->onMouseClick(	static_cast<ActionHandler>(&InterceptState::lstCraftsClickLeft));
	_lstCrafts->onMouseClick(	static_cast<ActionHandler>(&InterceptState::lstCraftsClickRight),
								SDL_BUTTON_RIGHT);
	_lstCrafts->onMouseOver(	static_cast<ActionHandler>(&InterceptState::lstCraftsMouseOver));
	_lstCrafts->onMouseOut(		static_cast<ActionHandler>(&InterceptState::lstCraftsMouseOut));


	const RuleCraft* crRule;

	size_t r (0u);
	for (std::vector<Base*>::const_iterator
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
	{
		if (*i == _base || _base == nullptr)
		{
			for (std::vector<Craft*>::const_iterator
					j = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end();
					++j)
			{
				_bases.push_back((*i)->getLabel());
				_crafts.push_back(*j);

				std::wostringstream
					woststr1,
					woststr2,
					woststr3;

				crRule = (*j)->getRules();

				if (crRule->getWeaponCapacity() != 0u)
					woststr1 << (*j)->getQtyWeapons() << L"/" << crRule->getWeaponCapacity();
				else
					woststr1 << L"-";

				if (crRule->getSoldierCapacity() != 0)
					woststr2 << (*j)->getQtySoldiers();
				else
					woststr2 << L"-";

				if (crRule->getVehicleCapacity() != 0)
					woststr3 << (*j)->getQtyVehicles();
				else
					woststr3 << L"-";

				_lstCrafts->addRow(
								5,
								(*j)->getLabel(_game->getLanguage()).c_str(),
								getAltStatus(*j).c_str(),
								woststr1.str().c_str(),
								woststr2.str().c_str(),
								woststr3.str().c_str());
				_lstCrafts->setCellColor(r++, 1u, _cellColor, true);
			}
		}
	}

	if (_base != nullptr)
		_txtBase->setText(_base->getLabel());
}

/**
 * dTor.
 */
InterceptState::~InterceptState()
{}

/**
 * A more descriptive state of the Crafts.
 * @note See also CraftsState::getAltStatus() & GeoscapeCraftState::cTor.
 * @param craft - pointer to Craft in question
 * @return, status string
 */
std::wstring InterceptState::getAltStatus(Craft* const craft) // private.
{
	const CraftStatus stat (craft->getCraftStatus());
	if (stat != CS_OUT)
	{
		if (stat == CS_READY)
		{
			_cellColor = GREEN;
			return tr("STR_READY");
		}

		_cellColor = SLATE;

		std::string st (craft->getCraftStatusString());
		st.push_back('_');

		bool isDelayed;
		const int hrs (craft->getDowntime(isDelayed, _game->getRuleset()));
		return tr(st).arg(_game->getSavedGame()->formatCraftDowntime(
																hrs, isDelayed,
																_game->getLanguage()));
	}

	std::wstring status;
	if (craft->isLowFuel() == true)
	{
		status = tr("STR_LOW_FUEL_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->isTacticalReturn() == true)
	{
		status = tr("STR_MISSION_COMPLETE_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->getTarget() == dynamic_cast<Target*>(craft->getBase()))
	{
		status = tr("STR_RETURNING_TO_BASE");
		_cellColor = BROWN;
	}
	else if (craft->getTarget() == nullptr)
	{
		status = tr("STR_PATROLLING");
		_cellColor = OLIVE;
	}
	else
	{
		const Ufo* const ufo (dynamic_cast<Ufo*>(craft->getTarget()));
		if (ufo != nullptr)
		{
			if (craft->inDogfight() == true)
				status = tr("STR_TAILING_UFO").arg(ufo->getId());
			else if (ufo->getUfoStatus() == Ufo::FLYING)
				status = tr("STR_INTERCEPTING_UFO").arg(ufo->getId());
			else
				status = tr("STR_DESTINATION_UC_")
							.arg(ufo->getLabel(_game->getLanguage()));
		}
		else
			status = tr("STR_DESTINATION_UC_")
						.arg(craft->getTarget()->getLabel(_game->getLanguage()));

		_cellColor = PURPLE;
	}
	return status;
}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void InterceptState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Goes to the Base of the respective Craft.
 * @param action - pointer to an Action
 */
void InterceptState::btnGotoBaseClick(Action*)
{
	_game->getScreen()->fadeScreen();

	_geoState->resetTimer();

	_game->popState();
	_game->pushState(new BasescapeState(_base, _geoState->getGlobe()));
}

/**
 * Opens a craft-info window.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsClickLeft(Action*)
{
	_game->pushState(new GeoscapeCraftState(
										_crafts[_lstCrafts->getSelectedRow()],
										_geoState,
										nullptr,
										true,
										false,
										_ufo));
}

/**
 * Centers on the selected Craft.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsClickRight(Action*)
{
	_game->popState();

	const Craft* const craft (_crafts[_lstCrafts->getSelectedRow()]);
	const double
		lon (craft->getLongitude()),
		lat (craft->getLatitude());
	_geoState->getGlobe()->center(lon,lat);
	_geoState->getGlobe()->setCrosshair(lon,lat);
}

/**
 * Shows Base label.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsMouseOver(Action*)
{
	if (_base == nullptr)
	{
		std::wstring wst;

		const size_t sel (_lstCrafts->getSelectedRow());
		if (sel < _bases.size())
			wst = _bases[sel];
		else
			wst = tr("STR_INTERCEPT");

		_txtBase->setText(wst);
	}
}

/**
 * Hides Base label.
 * @param action - pointer to an Action
 */
void InterceptState::lstCraftsMouseOut(Action*)
{
	if (_base == nullptr)
		_txtBase->setText(tr("STR_INTERCEPT"));
}

}
