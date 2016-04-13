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

#include "ArticleStateItem.h"

//#include <algorithm>
//#include <sstream>

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/InteractiveSurface.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Palette.h"
//#include "../Engine/Surface.h"

//#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

FirearmInfo ArticleStateItem::_infoMode = MODE_SHOT; // static.


/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionItem (ArticleDefinition.h)
 */
ArticleStateItem::ArticleStateItem(const ArticleDefinitionItem* const defs)
	:
		ArticleState(defs->id)
{
	setPalette(PAL_BATTLEPEDIA);
	ArticleState::initLayout();

	_game->getResourcePack()->getSurface("BACK08.SCR")->blit(_bg);

	_txtTitle = new Text(148, 32, 5, 24);
	add(_txtTitle);
	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_BLUE_SLATE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	_btnOk->setColor(tac_YELLOW);
	_btnPrev->setColor(tac_YELLOW);
	_btnNext->setColor(tac_YELLOW);

	_image = new Surface(32, 48, 157, 5);
	add(_image);


	const RuleItem* const itRule (_game->getRuleset()->getItemRule(defs->id));

	itRule->drawHandSprite(
					_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
					_image);

	if (itRule->isTwoHanded() == true)
	{
		_txtTwoHand = new Text(13, 9, 138, 52);
		add(_txtTwoHand);
		_txtTwoHand->setText(tr("STR_2HAND"));
		_txtTwoHand->setColor(uPed_BLUE_SLATE);
	}

	const std::vector<std::string>* const ammo_data (itRule->getCompatibleAmmo());

	// SHOT-MODE STATS-TABLE (for firearms + melee only)
	switch (itRule->getBattleType())
	{
		case BT_FIREARM:
		{
			_txtShotType = new Text(100, 9, 8, 66);
			add(_txtShotType);
			_txtShotType->setColor(uPed_BLUE_SLATE);
			if (itRule->getMeleeTu() != 0)
			{
				_txtShotType->setText(tr("STR_SHOT_TYPE_").arg(L"*"));

				_isfMode = new InteractiveSurface(204, 9, 8, 66);
				add(_isfMode);
				_isfMode->onMouseClick((ActionHandler)& ArticleStateItem::toggleTable);

				_lstInfoMelee = new TextList(204, 64, 8, 78);
				add(_lstInfoMelee);
				_lstInfoMelee->setColor(uPed_GREEN_SLATE); // color for %-data!
				_lstInfoMelee->setColumns(3, 100,52,52);
				_lstInfoMelee->setMargin();
				_lstInfoMelee->setBig();
				_lstInfoMelee->setVisible(_infoMode == MODE_MELEE);
			}
			else
				_txtShotType->setText(tr("STR_SHOT_TYPE_").arg(L""));

			_txtAccuracy = new Text(52, 9, 108, 66);
			add(_txtAccuracy);
			_txtAccuracy->setText(tr("STR_ACCURACY_UC"));
			_txtAccuracy->setColor(uPed_BLUE_SLATE);

			_txtTuCost = new Text(52, 9, 160, 66);
			add(_txtTuCost);
			_txtTuCost->setText(tr("STR_TIME_UNIT_COST"));
			_txtTuCost->setColor(uPed_BLUE_SLATE);

			_lstInfo = new TextList(204, 64, 8, 78);
			add(_lstInfo);
			_lstInfo->setColor(uPed_GREEN_SLATE); // color for %-data!
			_lstInfo->setColumns(3, 100,52,52);
			_lstInfo->setMargin();
			_lstInfo->setBig();
			_lstInfo->setVisible(itRule->getMeleeTu() == 0 || _infoMode == MODE_SHOT);


			const bool flatRate (itRule->isFlatRate() == true);
			std::wstring tu;

			size_t current_row (0u);
			if (itRule->getAutoTu() != 0)
			{
				if (flatRate == true)
					tu = Text::intWide(itRule->getAutoTu());
				else
					tu = Text::formatPercent(itRule->getAutoTu());

				_lstInfo->addRow(
								3,
								tr("STR_SHOT_TYPE_AUTO").c_str(),
								Text::intWide(itRule->getAccuracyAuto()).c_str(),
								tu.c_str());
				_lstInfo->setCellColor(current_row++, 0u, uPed_BLUE_SLATE);
			}

			if (itRule->getSnapTu() != 0)
			{
				if (flatRate == true)
					tu = Text::intWide(itRule->getSnapTu());
				else
					tu = Text::formatPercent(itRule->getSnapTu());

				_lstInfo->addRow(
								3,
								tr("STR_SHOT_TYPE_SNAP").c_str(),
								Text::intWide(itRule->getAccuracySnap()).c_str(),
								tu.c_str());
				_lstInfo->setCellColor(current_row++, 0u, uPed_BLUE_SLATE);
			}

			if (itRule->getAimedTu() != 0)
			{
				if (flatRate == true)
					tu = Text::intWide(itRule->getAimedTu());
				else
					tu = Text::formatPercent(itRule->getAimedTu());

				_lstInfo->addRow(
								3,
								tr("STR_SHOT_TYPE_AIMED").c_str(),
								Text::intWide(itRule->getAccuracyAimed()).c_str(),
								tu.c_str());
				_lstInfo->setCellColor(current_row++, 0u, uPed_BLUE_SLATE);
			}

			if (itRule->getLaunchTu() != 0)
			{
				if (flatRate == true)
					tu = Text::intWide(itRule->getLaunchTu());
				else
					tu = Text::formatPercent(itRule->getLaunchTu());

				_lstInfo->addRow(
								3,
								tr("STR_SHOT_TYPE_LAUNCH").c_str(),
								L"",
								tu.c_str());
				_lstInfo->setCellColor(current_row, 0u, uPed_BLUE_SLATE);
			}

			if (itRule->getMeleeTu() != 0)
			{
				if (flatRate == true)
					tu = Text::intWide(itRule->getMeleeTu());
				else
					tu = Text::formatPercent(itRule->getMeleeTu());

				_lstInfoMelee->addRow(
									3,
									tr("STR_HIT_MELEE").c_str(),
									Text::intWide(itRule->getAccuracyMelee()).c_str(),
									tu.c_str());
				_lstInfoMelee->setCellColor(0u,0u, uPed_BLUE_SLATE);
			}

			// text_info goes BELOW the info-table
			_txtInfo = new Text((ammo_data->size() < 3u ? 304 : 200), 57, 8, 136);
			break;
		}

		case BT_MELEE:
		{
			_txtShotType = new Text(100, 9, 8, 66);
			add(_txtShotType);
			_txtShotType->setText(tr("STR_SHOT_TYPE_").arg(L""));
			_txtShotType->setColor(uPed_BLUE_SLATE);

			_txtAccuracy = new Text(52, 9, 108, 66);
			add(_txtAccuracy);
			_txtAccuracy->setText(tr("STR_ACCURACY_UC"));
			_txtAccuracy->setColor(uPed_BLUE_SLATE);

			_txtTuCost = new Text(52, 9, 160, 66);
			add(_txtTuCost);
			_txtTuCost->setText(tr("STR_TIME_UNIT_COST"));
			_txtTuCost->setColor(uPed_BLUE_SLATE);

			_lstInfo = new TextList(204, 57, 8, 78);
			add(_lstInfo);
			_lstInfo->setColor(uPed_GREEN_SLATE); // color for %-data!
			_lstInfo->setColumns(3, 100,52,52);
			_lstInfo->setMargin();
			_lstInfo->setBig();


			std::wstring tu;
			if (itRule->getMeleeTu() != 0)
			{
				if (itRule->isFlatRate() == true)
					tu = Text::intWide(itRule->getMeleeTu());
				else
					tu = Text::formatPercent(itRule->getMeleeTu());

				_lstInfo->addRow(
								3,
								tr("STR_HIT_MELEE").c_str(),
								Text::intWide(itRule->getAccuracyMelee()).c_str(),
								tu.c_str());
				_lstInfo->setCellColor(0u, 0u, uPed_BLUE_SLATE);
			}

			// text_info goes BELOW the info-table
			_txtInfo = new Text(304, 57, 8, 136);
			break;
		}

		default: // text_info is larger and starts on top
			_txtInfo = new Text(304, 121, 8, 69);
	}

	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();


	// Deal with AMMO sidebar.
	size_t ammo_types;
	switch (itRule->getBattleType())
	{
		case BT_FIREARM:
			if (ammo_data->empty() == true)
				ammo_types = 1u;
			else
				ammo_types = std::min(ammo_data->size(),
									  3u); // yeh right.
			break;

		case BT_MELEE:
		case BT_AMMO:
		case BT_GRENADE:
		case BT_PROXYGRENADE:
			ammo_types = 1u;
			break;

		default:
			ammo_types = 0u;
	}

	for (size_t
			i = 0u;
			i != ammo_types;
			++i)
	{
		_txtAmmoType[i] = new Text(90, 9, 189, 24 + (static_cast<int>(i) * 49));
		add(_txtAmmoType[i]);
		_txtAmmoType[i]->setColor(uPed_BLUE_SLATE);
		_txtAmmoType[i]->setAlign(ALIGN_CENTER);
		_txtAmmoType[i]->setVerticalAlign(ALIGN_MIDDLE);
		_txtAmmoType[i]->setWordWrap();

		_txtAmmoDamage[i] = new Text(90, 16, 190, 40 + (static_cast<int>(i) * 49));
		add(_txtAmmoDamage[i]);
		_txtAmmoDamage[i]->setColor(tac_RED);
		_txtAmmoDamage[i]->setAlign(ALIGN_CENTER);
		_txtAmmoDamage[i]->setBig();

		_imageAmmo[i] = new Surface(32, 48, 280, 16 + static_cast<int>(i) * 49);
		add(_imageAmmo[i]);
	}

	switch (itRule->getBattleType())
	{
		case BT_FIREARM:
		{
//			_txtDamage = new Text(50, 9, 210, 7);
//			add(_txtDamage);
//			_txtDamage->setText(tr("STR_DAMAGE_UC"));
//			_txtDamage->setColor(uPed_BLUE_SLATE);
//			_txtDamage->setAlign(ALIGN_CENTER);

//			_txtAmmo = new Text(40, 9, 275, 7);
//			add(_txtAmmo);
//			_txtAmmo->setText(tr("STR_AMMO"));
//			_txtAmmo->setColor(uPed_BLUE_SLATE);
//			_txtAmmo->setAlign(ALIGN_CENTER);

			std::wostringstream woststr;
			if (ammo_data->empty() == true)
			{
				_txtAmmoType[0u]->setText(tr(getDamageTypeText(itRule->getDamageType())));

				woststr << itRule->getPower();
				if (itRule->getShotgunPellets() != 0)
					woststr << L"x" << itRule->getShotgunPellets();

				_txtAmmoDamage[0u]->setText(woststr.str());
			}
			else
			{
				for (size_t
						i = 0u;
						i != ammo_types;
						++i)
				{
					const ArticleDefinition* const ammo_article (_game->getRuleset()->getUfopaediaArticle((*ammo_data)[i]));
					if (Ufopaedia::isArticleAvailable(
												_game->getSavedGame(),
												ammo_article) == true)
					{
						const RuleItem* const ammo_rule (_game->getRuleset()->getItemRule((*ammo_data)[i]));
						_txtAmmoType[i]->setText(tr(getDamageTypeText(ammo_rule->getDamageType())));

						if (i != 0u) woststr.str(L"");
						woststr << ammo_rule->getPower();
						if (ammo_rule->getShotgunPellets() != 0)
							woststr << L"x" << ammo_rule->getShotgunPellets();

						_txtAmmoDamage[i]->setText(woststr.str());

						ammo_rule->drawHandSprite(
											_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
											_imageAmmo[i]);
					}
				}
			}
			break;
		}

		case BT_MELEE:
		case BT_AMMO:
		case BT_GRENADE:
		case BT_PROXYGRENADE:
//			_txtDamage = new Text(50, 9, 210, 7);
//			add(_txtDamage);
//			_txtDamage->setText(tr("STR_DAMAGE_UC"));
//			_txtDamage->setColor(uPed_BLUE_SLATE);
//			_txtDamage->setAlign(ALIGN_CENTER);

			_txtAmmoType[0u]->setText(tr(getDamageTypeText(itRule->getDamageType())));
			_txtAmmoDamage[0u]->setText(Text::intWide(itRule->getPower()));
	}

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateItem::~ArticleStateItem() // virtual.
{}

/**
 * Switches the info-table for Firearms between displaying shot-or-melee data.
 * @param action - pointer to an Action
 */
void ArticleStateItem::toggleTable(Action*) // private.
{
	switch (_infoMode)
	{
		case MODE_SHOT:
			_infoMode = MODE_MELEE;
			_lstInfo->setVisible(false);
			_lstInfoMelee->setVisible();
			break;

		case MODE_MELEE:
			_infoMode = MODE_SHOT;
			_lstInfo->setVisible();
			_lstInfoMelee->setVisible(false);
			break;
	}
}

/**
 * Resets the FirearmInfo enumerator.
 */
void ArticleStateItem::resetFirearmInfo() // static.
{
	_infoMode = MODE_SHOT;
}

}
