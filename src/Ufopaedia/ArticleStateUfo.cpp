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

#include "ArticleStateUfo.h"

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
//#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionUfo (ArticleDefinition.h)
 */
ArticleStateUfo::ArticleStateUfo(const ArticleDefinitionUfo* const defs)
	:
		ArticleState(defs->id)
{
	_txtTitle = new Text(155, 32, 5, 24);

	setPalette(PAL_GEOSCAPE);

	ArticleState::initLayout(false);

	add(_txtTitle);

	_game->getResourcePack()->getSurface("BACK11.SCR")->blit(_bg);

	_btnOk->setColor(GEOSCAPE_CYAN);
	_btnPrev->setColor(GEOSCAPE_CYAN);
	_btnNext->setColor(GEOSCAPE_CYAN);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(GEOSCAPE_CYAN);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	_image = new Surface(160, 52, 160, 6);
	add(_image);

/*	Surface* graphic = _game->getResourcePack()->getSurface("INTERWIN.DAT");
	graphic->setX(0);
	graphic->setY(0);
	graphic->getCrop()->x = 0;
	graphic->getCrop()->y = 0;
	graphic->getCrop()->w = 160;
	graphic->getCrop()->h = 52;
	_image->drawRect(graphic->getCrop(), 15); */

/*old
	graphic->getCrop()->y = 96;
	graphic->getCrop()->h = 15;
	graphic->blit(_image);
	graphic->setY(67);
	graphic->getCrop()->y = 111;
	graphic->getCrop()->h = 29;
	graphic->blit(_image); */

/*	if (ufo->getSpriteString().empty())
	{
		graphic->getCrop()->y = 140 + 52 * static_cast<Sint16>(ufo->getSprite());
		graphic->getCrop()->h = 52;
	}
	else
	{
		graphic = _game->getResourcePack()->getSurface(ufo->getSpriteString());
		graphic->setX(0);
		graphic->setY(0);
	}
	graphic->blit(_image); */

	const RuleUfo* const ufo (_game->getRuleset()->getUfo(defs->id));
	const int sprite (ufo->getSprite());
	std::ostringstream oststr;
	oststr << "INTERWIN_" << sprite;
	Surface* const srfPreview (_game->getResourcePack()->getSurface(oststr.str()));
	if (srfPreview != nullptr)
		srfPreview->blit(_image);

	_txtInfo = new Text(300, 50, 10, 140);
	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(GEOSCAPE_CYAN);
	_txtInfo->setWordWrap();

	_lstInfo = new TextList(300, 65,10,68);
	add(_lstInfo);

	centerSurfaces();


	_lstInfo->setColor(GEOSCAPE_CYAN);
	_lstInfo->setColumns(2, 200, 100);
	_lstInfo->setBig();
	_lstInfo->setDot();

	_lstInfo->addRow(
				2,
				tr("STR_DAMAGE_CAPACITY").c_str(),
				Text::intWide(ufo->getUfoHullCap()).c_str());
	_lstInfo->addRow(
				2,
				tr("STR_WEAPON_POWER").c_str(),
				Text::intWide(ufo->getWeaponPower()).c_str());
	_lstInfo->addRow(
				2,
				tr("STR_WEAPON_RANGE").c_str(),
				tr("STR_KILOMETERS").arg(ufo->getWeaponRange()).c_str());
	_lstInfo->addRow(
				2,
				tr("STR_MAXIMUM_SPEED").c_str(),
				tr("STR_KNOTS").arg(ufo->getTopSpeed()).c_str());
}

/**
 * dTor.
 */
ArticleStateUfo::~ArticleStateUfo() // virtual.
{}

}
