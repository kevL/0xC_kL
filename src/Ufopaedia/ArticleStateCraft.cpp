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

#include "ArticleStateCraft.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionCraft (ArticleDefinition.h)
 */
ArticleStateCraft::ArticleStateCraft(const ArticleDefinitionCraft* const defs)
	:
		ArticleState(defs->id)
{
	_txtTitle = new Text(210, 32, 5, 24);

	setPalette(PAL_UFOPAEDIA);

	ArticleState::initLayout(false);

	add(_txtTitle);

	_game->getResourcePack()->getSurface(defs->image_id)->blit(_bg);

	_btnOk->setColor(uPed_BLUE_SLATE);
	_btnPrev->setColor(uPed_BLUE_SLATE);
	_btnNext->setColor(uPed_BLUE_SLATE);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_BLUE_SLATE);
	_txtTitle->setBig();

	_txtInfo = new Text(
					defs->rect_text.width,
					defs->rect_text.height,
					defs->rect_text.x,
					defs->rect_text.y);
	add(_txtInfo);

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();

	_txtStats = new Text(
						defs->rect_stats.width,
						defs->rect_stats.height,
						defs->rect_stats.x,
						defs->rect_stats.y);
	add(_txtStats);

	_txtStats->setColor(uPed_BLUE_SLATE);
	_txtStats->setSecondaryColor(uPed_GREEN_SLATE);

	const RuleCraft* const crRule (_game->getRuleset()->getCraft(defs->id));
	int range (crRule->getFuelCapacity());
	if (crRule->getRefuelItem().empty() == false)
		range *= crRule->getTopSpeed();

	range /= 6; // six doses per hour on Geoscape.

	std::wostringstream woststr;
	woststr << tr("STR_MAXIMUM_SPEED_")		.arg(crRule->getTopSpeed()) << L'\n'
			<< tr("STR_ACCELERATION_")		.arg(crRule->getAcceleration()) << L'\n'
			<< tr("STR_FUEL_CAPACITY_")		.arg(range) << L'\n'
			<< tr("STR_DAMAGE_CAPACITY_")	.arg(crRule->getCraftHullCap());

	if (crRule->getWeaponCapacity() != 0u)
		woststr << L'\n' << tr("STR_WEAPON_PODS_")	.arg(crRule->getWeaponCapacity());
	if (crRule->getSoldierCapacity() != 0)
		woststr << L'\n' << tr("STR_CARGO_SPACE_")	.arg(crRule->getSoldierCapacity());
	if (crRule->getVehicleCapacity() != 0)
		woststr << L'\n' << tr("STR_HWP_CAPACITY_")	.arg(crRule->getVehicleCapacity());
	_txtStats->setText(woststr.str());

	centerSurfaces();
}

/**
 * dTor.
 */
ArticleStateCraft::~ArticleStateCraft() // virtual.
{}

}
