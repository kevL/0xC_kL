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

#include "ExtraAlienInfoState.h"

#include "../fmath.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Displays alien properties such as vulnerabilities and fixed-weapon damage.
 * @param defs - pointer to an ArticleDefinitionTextImage
 */
ExtraAlienInfoState::ExtraAlienInfoState(const ArticleDefinitionTextImage* const defs)
	:
		ArticleState(defs->id)
{
	_fullScreen = false;

	_window		= new Window(this, 224, 168, 48, 16, POPUP_VERTICAL);
	_btnExit	= new TextButton(62, 16, 129, 159);
	_lstInfo	= new TextList(150, 73, 84,  28);
	_lstWeapon	= new TextList(150,  9, 84, 110);

	setPalette(PAL_UFOPAEDIA);

	add(_window);
	add(_btnExit);
	add(_lstInfo);
	add(_lstWeapon);

	centerAllSurfaces();


	const ResourcePack* const rp (_game->getResourcePack());
	_window->setBackground(rp->getSurface(rp->getRandomBackground()));
	_window->setColor(uPed_PINK);

	_btnExit->setText(tr("STR_OK"));
	_btnExit->setColor(uPed_GREEN_SLATE);
	_btnExit->onMouseClick((ActionHandler)& ExtraAlienInfoState::btnExit);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyOk);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyOkKeypad);
	_btnExit->onKeyboardPress(
					(ActionHandler)& ExtraAlienInfoState::btnExit,
					Options::keyCancel);

	_lstInfo->setColumns(2, 125,25);
	_lstInfo->setColor(uPed_BLUE_SLATE);
	_lstInfo->setDot();
	_lstInfo->setMargin();

	_lstWeapon->setColumns(3, 100,25,25);
	_lstWeapon->setColor(uPed_BLUE_SLATE);
	_lstWeapon->setDot();
	_lstWeapon->setMargin();


	std::string type;
	if (defs->id.find("_AUTOPSY") != std::string::npos)
		type = defs->id.substr(0, defs->id.length() - 8);
	else
		type = defs->id;

	const RuleUnit* unitRule;
	if (_game->getRuleset()->getUnitRule(type + "_SOLDIER") != nullptr)
		unitRule = _game->getRuleset()->getUnitRule(type + "_SOLDIER");
	else if (_game->getRuleset()->getUnitRule(type + "_TERRORIST") != nullptr)
		unitRule = _game->getRuleset()->getUnitRule(type + "_TERRORIST");
	else
	{
		unitRule = nullptr;
		Log(LOG_INFO) << "ERROR: rules not found for unit - " << type;
	}

	if (unitRule != nullptr)
	{
		const RuleArmor* const armorRule (_game->getRuleset()->getArmor(unitRule->getArmorType()));
		size_t row (0);
		DamageType dType;
		int vulnr;

		for (size_t
				i = 0;
				i != RuleArmor::DAMAGE_TYPES;
				++i)
		{
			dType = static_cast<DamageType>(i);
			type = ArticleState::getDamageTypeText(dType);
			if (type != "STR_UNKNOWN")
			{
				vulnr = static_cast<int>(Round(armorRule->getDamageModifier(dType) * 100.f));
				_lstInfo->addRow(
							2,
							tr(type).c_str(),
							Text::formatPercent(vulnr).c_str());
				_lstInfo->setCellColor(row++, 1, uPed_GREEN_SLATE);
			}
		}

		if (unitRule->isLivingWeapon() == true)
		{
			type = unitRule->getRace().substr(4) + "_WEAPON";
			const RuleItem* const itRule (_game->getRuleset()->getItemRule(type));
			if (itRule != nullptr)
			{
				dType = itRule->getDamageType();
				type = ArticleState::getDamageTypeText(dType);
				if (type != "STR_UNKNOWN")
				{
					std::wstring wstPower;
					if (itRule->isStrengthApplied() == true)
						wstPower = tr("STR_VARIABLE");
					else
						wstPower = Text::intWide(itRule->getPower());

					_lstWeapon->addRow(
									3,
									tr("STR_WEAPON").c_str(),
									wstPower.c_str(),
									tr(type).c_str());
					_lstWeapon->setCellColor(0,1, uPed_GREEN_SLATE);
					_lstWeapon->setCellColor(0,2, uPed_GREEN_SLATE);
				}
			}
		}
	}
}

/**
 * dTor.
 */
ExtraAlienInfoState::~ExtraAlienInfoState() // virtual.
{}

/**
 * Closes state.
 * @param action - pointer to an Action
 */
void ExtraAlienInfoState::btnExit(Action*) // private.
{
	_game->popState();
}

}
