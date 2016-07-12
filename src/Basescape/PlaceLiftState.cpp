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

#include "PlaceLiftState.h"

#include "BasescapeState.h"
#include "BaseView.h"
#include "SelectStartFacilityState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the PlaceLift screen.
 * @param base		- pointer to the Base to get info from
 * @param globe		- pointer to the geoscape Globe
 * @param firstBase	- true if this is a custom start base
 */
PlaceLiftState::PlaceLiftState(
		Base* const base,
		Globe* const globe,
		bool firstBase)
	:
		_base(base),
		_globe(globe),
		_firstBase(firstBase)
{
	_baseLayout	= new BaseView(192, 192, 0, 8);
	_txtTitle	= new Text(320, 9);

	setInterface("placeFacility");

	add(_baseLayout,	"baseView",	"basescape");
	add(_txtTitle,		"text",		"placeFacility");

	centerAllSurfaces();


	_baseLayout->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_baseLayout->setBase(_base);

	const RuleBaseFacility* facRule;
	const std::vector<std::string> allFacs (_game->getRuleset()->getBaseFacilitiesList());
	for (std::vector<std::string>::const_iterator
			i = allFacs.begin();
			i != allFacs.end();
			++i)
	{
		facRule = _game->getRuleset()->getBaseFacility(*i);
		if (facRule->isLift() == true)
		{
			_lift = facRule;
			break;
		}
	}
	_baseLayout->highlightFacility(_lift->getSize());
	_baseLayout->onMouseClick(static_cast<ActionHandler>(&PlaceLiftState::baseLayoutClick));

	_txtTitle->setText(tr("STR_SELECT_POSITION_FOR_ACCESS_LIFT"));
}

/**
 * dTor.
 */
PlaceLiftState::~PlaceLiftState()
{}

/**
 * Processes clicking on Facilities.
 * @param action - pointer to an Action
 */
void PlaceLiftState::baseLayoutClick(Action*)
{
	BaseFacility* const fac (new BaseFacility(_lift, _base));
	fac->setX(_baseLayout->getGridX());
	fac->setY(_baseLayout->getGridY());

	_base->getFacilities()->push_back(fac);
	_base->placeBase();

	_game->popState();

	BasescapeState* const baseState (new BasescapeState(_base, _globe));

	_game->pushState(baseState);

	if (_firstBase == true)
		_game->pushState(new SelectStartFacilityState(_base, baseState, _globe));
}

}
