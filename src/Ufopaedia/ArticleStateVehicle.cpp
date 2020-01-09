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

#include "ArticleStateVehicle.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

/**
 * ArticleStateVehicle has a caption, text, and a stats block.
 * @param defs - pointer to ArticleDefinitionVehicle (ArticleDefinition.h)
 */
ArticleStateVehicle::ArticleStateVehicle(const ArticleDefinitionVehicle* const defs)
	:
		ArticleState(defs->id)
{
	RuleUnit* const unitRule (_game->getRuleset()->getUnitRule(defs->id));
	const RuleArmor* const arRule (_game->getRuleset()->getArmor(unitRule->getArmorType()));
	const RuleItem* const itRule (_game->getRuleset()->getItemRule(defs->id));

	_txtTitle	= new Text(310,  17,  5,  23);
	_txtInfo	= new Text(300, 150, 10, 122);

	_lstStats	= new TextList(300, 89, 10, 48);

	setPalette(PAL_UFOPAEDIA);

	ArticleState::initLayout();

	add(_txtTitle);
	add(_txtInfo);
	add(_lstStats);

	_game->getResourcePack()->getSurface("BACK10.SCR")->blit(_bg);

	_btnOk->setColor(uPed_VIOLET);
	_btnPrev->setColor(uPed_VIOLET);
	_btnNext->setColor(uPed_VIOLET);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_GREEN_SLATE);
	_txtTitle->setBig();

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();

	_lstStats->setColumns(2, 175,145);
	_lstStats->setColor(uPed_GREEN_SLATE);
	_lstStats->setDot();

	_lstStats->addRow(
				2,
				tr("STR_TIME_UNITS").c_str(),
				Text::intWide(unitRule->getStats()->tu).c_str());

	_lstStats->addRow(
				2,
				tr("STR_HEALTH").c_str(),
				Text::intWide(unitRule->getStats()->health).c_str());

	_lstStats->addRow(
				2,
				tr("STR_FRONT_ARMOR").c_str(),
				Text::intWide(arRule->getFrontArmor()).c_str());

	_lstStats->addRow(
				2,
				tr("STR_LEFT_ARMOR").c_str(),
				Text::intWide(arRule->getSideArmor()).c_str());
	_lstStats->addRow(
				2,
				tr("STR_RIGHT_ARMOR").c_str(),
				Text::intWide(arRule->getSideArmor()).c_str());

	_lstStats->addRow(
				2,
				tr("STR_REAR_ARMOR").c_str(),
				Text::intWide(arRule->getRearArmor()).c_str());

	_lstStats->addRow(
				2,
				tr("STR_UNDER_ARMOR").c_str(),
				Text::intWide(arRule->getUnderArmor()).c_str());

	_lstStats->addRow(
				2,
				tr("STR_WEAPON_LC").c_str(),
				tr(defs->weapon).c_str());

	if (itRule->getClipTypes()->empty() == false)
	{
		const RuleItem* const aRule (_game->getRuleset()->getItemRule(itRule->getClipTypes()->front()));

		_lstStats->addRow(
					2,
					tr("STR_WEAPON_POWER").c_str(),
					Text::intWide(aRule->getPower()).c_str());

		_lstStats->addRow(
					2,
					tr("STR_ORDNANCE_LC").c_str(),
					tr(aRule->getType()).c_str());

		int clipSize;
		if (itRule->getFullClip() > 0)
			clipSize = itRule->getFullClip();
		else
			clipSize = aRule->getFullClip();
		_lstStats->addRow(
					2,
					tr("STR_ROUNDS").c_str(),
					Text::intWide(clipSize).c_str());

		_txtInfo->setY(138);
	}
	else
		_lstStats->addRow(
					2,
					tr("STR_WEAPON_POWER").c_str(),
					Text::intWide(itRule->getPower()).c_str());

	centerSurfaces();
}

/**
 * dTor.
 */
 ArticleStateVehicle::~ArticleStateVehicle() // virtual.
{}

}
