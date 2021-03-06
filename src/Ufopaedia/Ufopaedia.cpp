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

#include "Ufopaedia.h"

#include "ArticleState.h"
#include "ArticleStateArmor.h"
#include "ArticleStateAward.h"
#include "ArticleStateBaseFacility.h"
#include "ArticleStateCraft.h"
#include "ArticleStateCraftWeapon.h"
#include "ArticleStateItem.h"
#include "ArticleStateText.h"
#include "ArticleStateTextImage.h"
#include "ArticleStateUfo.h"
#include "ArticleStateVehicle.h"
#include "UfopaediaStartState.h"

#include "../Engine/Game.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

int Ufopaedia::_current_index = 0; // static.


/**
 * Gets the index of the selected article_id in the visible list.
 * @param playSave		- pointer to SavedGame
 * @param rules			- pointer to Ruleset
 * @param article_id	- reference to the article ID to find
 * @return, index of the given article ID in the internal list (-1 if not found)
 */
int Ufopaedia::getArticleIndex( // protected/static.
		const SavedGame* const playSave,
		const Ruleset* const rules,
		std::string& article_id)
{
	const ArticleDefinitionList articles (getAvailableArticles(playSave, rules));
	const std::string Id_UC (article_id + "_UC");

	for (size_t
			i = 0u;
			i != articles.size();
			++i)
	{
		for (std::vector<std::string>::const_iterator
				j = articles[i]->reqResearch.begin();
				j != articles[i]->reqResearch.end();
				++j)
		{
			if (*j == article_id)
			{
				article_id = articles[i]->id;
				return static_cast<int>(i);
			}
		}

		if (articles[i]->id == article_id)
			return static_cast<int>(i);

		if (articles[i]->id == Id_UC)
		{
			article_id = Id_UC;
			return static_cast<int>(i);
		}
	}
	return -1;
}

/**
 * Returns an ArticleDefinitionList with all the currently visible ArticleIds.
 * @param playSave	- pointer to SavedGame
 * @param rules		- pointer to Ruleset
 * @return, ArticleDefinitionList of visible articles
 */
ArticleDefinitionList Ufopaedia::getAvailableArticles( // protected/static.
		const SavedGame* const playSave,
		const Ruleset* const rules)
{
	const std::vector<std::string>& allUfopaedia (rules->getUfopaediaList());
	ArticleDefinitionList articles;

	for (std::vector<std::string>::const_iterator
			i = allUfopaedia.begin();
			i != allUfopaedia.end();
			++i)
	{
		ArticleDefinition* const article (rules->getUfopaediaArticle(*i));
		if (article->section != UFOPAEDIA_NOT_AVAILABLE
			&& isArticleAvailable(playSave, article) == true)
		{
			articles.push_back(article);
		}
	}
	return articles;
}

/**
 * Creates an Article state dependent on a specified ArticleDefinition.
 * @param article - pointer to an ArticleDefinition to create
 * @return, pointer to ArticleState object if created or nullptr otherwise
 */
ArticleState* Ufopaedia::createArticleState(ArticleDefinition* const article) // protected/static.
{
	switch (article->getType())
	{
		case UFOPAEDIA_TYPE_CRAFT:
			return new ArticleStateCraft(dynamic_cast<ArticleDefinitionCraft*>(article));

		case UFOPAEDIA_TYPE_CRAFT_WEAPON:
			return new ArticleStateCraftWeapon(dynamic_cast<ArticleDefinitionCraftWeapon*>(article));

		case UFOPAEDIA_TYPE_VEHICLE:
			return new ArticleStateVehicle(dynamic_cast<ArticleDefinitionVehicle*>(article));

		case UFOPAEDIA_TYPE_ITEM:
			return new ArticleStateItem(dynamic_cast<ArticleDefinitionItem*>(article));

		case UFOPAEDIA_TYPE_ARMOR:
			return new ArticleStateArmor(dynamic_cast<ArticleDefinitionArmor*>(article));

		case UFOPAEDIA_TYPE_BASE_FACILITY:
			return new ArticleStateBaseFacility(dynamic_cast<ArticleDefinitionBaseFacility*>(article));

		case UFOPAEDIA_TYPE_TEXTIMAGE:
			return new ArticleStateTextImage(dynamic_cast<ArticleDefinitionTextImage*>(article));

		case UFOPAEDIA_TYPE_TEXT:
			return new ArticleStateText(dynamic_cast<ArticleDefinitionText*>(article));

		case UFOPAEDIA_TYPE_UFO:
			return new ArticleStateUfo(dynamic_cast<ArticleDefinitionUfo*>(article));

		case UFOPAEDIA_TYPE_AWARD:
			return new ArticleStateAward(dynamic_cast<ArticleDefinitionAward*>(article));
	}
	return nullptr;
}

/**
 * Checks if a Ufopaedia article is accessible.
 * @param playSave	- pointer to SavedGame
 * @param article	- pointer to an ArticleDefinition to check
 * @return, true if the article is available
 */
bool Ufopaedia::isArticleAvailable( // static.
		const SavedGame* const playSave,
		const ArticleDefinition* const article)
{
	return playSave->isResearched(article->reqResearch);
}

/**
 * Sets the Ufopaedia index and opens a specified article by pointer.
 * @param game		- pointer to core Game
 * @param article	- pointer to an ArticleDefinition to open
 */
void Ufopaedia::openArticle( // static.
		Game* const game,
		ArticleDefinition* const article)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article->id);
	if (_current_index != -1)
		game->pushState(createArticleState(article));
}

/**
 * Sets the Ufopaedia index and opens a specified article by string.
 * @param game			- pointer to core Game
 * @param article_id	- reference to the article-type to find
 */
void Ufopaedia::openArticle( // static.
		Game* const game,
		std::string& article_id)
{
	_current_index = getArticleIndex(
								game->getSavedGame(),
								game->getRuleset(),
								article_id);
	if (_current_index != -1)
	{
		ArticleDefinition* const article (game->getRuleset()->getUfopaediaArticle(article_id));
		game->pushState(createArticleState(article));
	}
}

/**
 * Opens UfopaediaStartState presenting the topical selection-buttons.
 * @param game		- pointer to the Game
 * @param tactical	- true if opening Ufopaedia from battlescape (default false)
 */
void Ufopaedia::open( // static.
		Game* const game,
		bool tactical)
{
	game->pushState(new UfopaediaStartState(tactical));
}

/**
 * Opens the next article in the list or loops to the first.
 * @param game - pointer to actual Game
 */
void Ufopaedia::next(Game* const game) // static.
{
	ArticleDefinitionList articles (getAvailableArticles(
													game->getSavedGame(),
													game->getRuleset()));
	if (_current_index >= static_cast<int>(articles.size()) - 1)
		_current_index = 0; // goto first
	else
		++_current_index;

	game->popState();
	game->pushState(createArticleState(articles[static_cast<size_t>(_current_index)]));
}

/**
 * Opens the previous article in the list or loops to the last.
 * @param game - pointer to actual Game
 */
void Ufopaedia::prev(Game* const game) // static.
{
	ArticleDefinitionList articles (getAvailableArticles(
													game->getSavedGame(),
													game->getRuleset()));
	if (_current_index == 0)
		_current_index = static_cast<int>(articles.size()) - 1;
	else
		--_current_index;

	game->popState();
	game->pushState(createArticleState(articles[static_cast<size_t>(_current_index)]));
}

/**
 * Fills an ArticleDefinitionList with the currently visible ArticleIds of a
 * specified Ufopaedia section.
 * @param playSave	- pointer to SavedGame
 * @param rules		- pointer to Ruleset
 * @param section	- reference to the article section to find, e.g. "XCOM Crafts & Armaments", "Alien Lifeforms", etc.
 * @param data		- reference to the article definition list object to fill data in
 */
void Ufopaedia::list( // static.
		const SavedGame* const playSave,
		const Ruleset* const rules,
		const std::string& section,
		ArticleDefinitionList& data)
{
	ArticleDefinitionList articles (getAvailableArticles(playSave, rules));
	for (ArticleDefinitionList::const_iterator
			i = articles.begin();
			i != articles.end();
			++i)
	{
		if ((*i)->section == section)
			data.push_back(*i);
	}
}

}
