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

#include "ArticleStateArmor.h"

//#include <sstream>
#include "../fmath.h"

#include "../Engine/CrossPlatform.h"
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
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionArmor (ArticleDefinition.h)
 */
ArticleStateArmor::ArticleStateArmor(const ArticleDefinitionArmor* const defs)
	:
		ArticleState(defs->id),
		_row(0u)
{
	setPalette(PAL_BATTLEPEDIA);
	ArticleState::initLayout();

	_btnOk->setColor(uPed_ORANGE);
	_btnPrev->setColor(uPed_ORANGE);
	_btnNext->setColor(uPed_ORANGE);

	_txtTitle = new Text(300, 17, 5, 24);
	add(_txtTitle);
	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_BLUE_SLATE);
	_txtTitle->setBig();

	_image = new Surface();
	add(_image);

	const RuleArmor* const arRule (_game->getRuleset()->getArmor(defs->id));

	std::string look (arRule->getSpriteInventory() + "M0.SPK"); // See also: InventoryState::init().

	if (_game->getResourcePack()->getSurface(look) == nullptr)
//		&& CrossPlatform::fileExists(CrossPlatform::getDataFile("UFOGRAPH/" + look)) == false)
	{
		look = arRule->getSpriteInventory() + ".SPK";

		if (_game->getResourcePack()->getSurface(look) == nullptr)
			look = arRule->getSpriteInventory();
	}

	if (_game->getResourcePack()->getSurface(look) != nullptr)
		_game->getResourcePack()->getSurface(look)->blit(_image);


	_lstInfo = new TextList(155, 129, 145, 12);
	add(_lstInfo);
	_lstInfo->setColumns(2, 120,27);
	_lstInfo->setColor(uPed_BLUE_SLATE);
	_lstInfo->setDot();

	_txtInfo = new Text(300, 49, 8, 150);
	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();


	addStat("STR_FRONT_ARMOR",	arRule->getFrontArmor());
	addStat("STR_LEFT_ARMOR",	arRule->getSideArmor());
	addStat("STR_RIGHT_ARMOR",	arRule->getSideArmor());
	addStat("STR_REAR_ARMOR",	arRule->getRearArmor());
	addStat("STR_UNDER_ARMOR",	arRule->getUnderArmor());

	_lstInfo->addRow(0);
	++_row;


	for (size_t
			i = 0u;
			i != RuleArmor::DAMAGE_TYPES;
			++i)
	{
		const DamageType dType (static_cast<DamageType>(i));
		const std::string st (getDamageTypeText(dType));
		if (st != "STR_UNKNOWN")
		{
			const int vulnr (static_cast<int>(Round(static_cast<double>(arRule->getDamageModifier(dType)) * 100.)));
			addStat(st, Text::formatPercent(vulnr));
		}
	}

	_lstInfo->addRow(0);
	++_row;

	addStat("STR_TIME_UNITS",			arRule->getStats()->tu,				true);
	addStat("STR_STAMINA",				arRule->getStats()->stamina,		true);
	addStat("STR_HEALTH",				arRule->getStats()->health,			true);
	addStat("STR_BRAVERY",				arRule->getStats()->bravery,		true);
	addStat("STR_REACTIONS",			arRule->getStats()->reactions,		true);
	addStat("STR_FIRING_ACCURACY",		arRule->getStats()->firing,			true);
	addStat("STR_THROWING_ACCURACY",	arRule->getStats()->throwing,		true);
	addStat("STR_MELEE_ACCURACY",		arRule->getStats()->melee,			true);
	addStat("STR_STRENGTH",				arRule->getStats()->strength,		true);
	addStat("STR_PSIONIC_STRENGTH",		arRule->getStats()->psiStrength,	true);
	addStat("STR_PSIONIC_SKILL",		arRule->getStats()->psiSkill,		true);

	centerSurfaces();
}

/**
 * dTor.
 */
ArticleStateArmor::~ArticleStateArmor() // virtual.
{}

/**
 *
 * @param stLabel	-
 * @param iStat		-
 * @param addSign	- (default false)
 */
void ArticleStateArmor::addStat( // private.
		const std::string& stLabel,
		int iStat,
		bool addSign)
{
	if (iStat != 0)
	{
		std::wostringstream woststr;
		if (addSign == true && iStat > 0)
			woststr << L'+';
		woststr << iStat;

		_lstInfo->addRow(
						2,
						tr(stLabel).c_str(),
						woststr.str().c_str());
		_lstInfo->setCellColor(_row++, 1u, uPed_GREEN_SLATE);
	}
}

/**
 *
 * @param stLabel -
 * @param wstStat -
 */
void ArticleStateArmor::addStat( // private.
		const std::string& stLabel,
		const std::wstring& wstStat)
{
	_lstInfo->addRow(
					2,
					tr(stLabel).c_str(),
					wstStat.c_str());
	_lstInfo->setCellColor(_row++, 1u, uPed_GREEN_SLATE);
}

}
