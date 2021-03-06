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

#include "ConfirmLandingState.h"

//#include <sstream>

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/BriefingState.h"

#include "../Engine/Exception.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleTerrain.h"
#include "../Ruleset/RuleTexture.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ConfirmLanding window.
 * @param craft			- pointer to the Craft to confirm
 * @param texRule		- pointer to the RuleTexture of the landing site (default nullptr)
 * @param shade			- shade of the landing site (default -1)
 * @param allowTactical	- true to show the Yes btn (default true)
 */
ConfirmLandingState::ConfirmLandingState(
		Craft* const craft,
		const RuleTexture* const texRule,
		const int shade,
		const bool allowTactical)
	:
		_craft(craft),
		_shade(shade),
		_terrainRule(nullptr)
{
	//Log(LOG_INFO) << "Create ConfirmLandingState()";
	// TODO: show Country & Region
	// TODO: should do buttons: Patrol (isCancel already) or GeoscapeCraftState or Return to Base.
	_fullScreen = false;

	_window			= new Window(this, 230, 160, 13, 20, POPUP_BOTH);

	_txtBase		= new Text( 80, 9,  23, 29);
	_txtTexture		= new Text(150, 9,  83, 29);
	_txtShade		= new Text( 60, 9, 173, 39);

	_txtMessage		= new Text(206, 40, 25, 47);
	_txtMessage2	= new Text(206, 43, 25, 87);

	_txtBegin		= new Text(206, 17, 25, 130);

	_btnPatrol		= new TextButton(80, 18,  40, 152);
	_btnYes			= new TextButton(80, 18, 136, 152);

	setInterface("confirmLanding");

	add(_window,		"window",	"confirmLanding");
	add(_txtBase,		"text",		"confirmLanding");
	add(_txtTexture,	"text",		"confirmLanding");
	add(_txtShade,		"text",		"confirmLanding");
	add(_txtMessage,	"text",		"confirmLanding");
	add(_txtMessage2,	"text",		"confirmLanding");
	add(_txtBegin,		"text",		"confirmLanding");
	add(_btnPatrol,		"button",	"confirmLanding");
	add(_btnYes,		"button",	"confirmLanding");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK15.SCR"));

	_txtBase->setText(craft->getBase()->getLabel());

	_txtShade->setText(tr("STR_SHADE_").arg(shade));
	_txtShade->setAlign(ALIGN_RIGHT);

	// NOTE: the following terrain determination will fall through to
	// BattlescapeGenerator for Base assault/defense and Cydonia missions;
	// concerned only with UFOs and TerrorSites here.
	Ufo* const ufo (dynamic_cast<Ufo*>(_craft->getTarget()));
	TerrorSite* const site (dynamic_cast<TerrorSite*>(_craft->getTarget()));

	if (ufo != nullptr || site != nullptr) // ... else it's an aLienBase assault (NOT defense nor Cydonia).
	{
		std::string terrainType;
		if (ufo != nullptr) // UFO crashed/landed
			terrainType = ufo->getUfoTerrainType();		// the Ufo stores the terrainType value.
		else
			terrainType = site->getSiteTerrainType();	// the TerrorSite stores the terrainType value.

		if (terrainType.empty() == true) // determine terrainType/RuleTerrain and store it ->
		{
			//Log(LOG_INFO) << ". determine Terrain";
			if (site != nullptr) // terrorSite
			{
				//Log(LOG_INFO) << ". . terrorSite";
				std::vector<std::string> terrainList;
				// terrains for tacticals can be/are defined in both RuleAlienDeployment AND through RuleGlobe (Textures)
				// Options:
				// 1. choose from both aspects
				// 2. choose Deployment preferentially
				// 3. choose Globe-texture preferentially
				// ...
				// PROFIT!!
				// conclusion: choose among RuleGlobe-Texture's def'd. deployments first;
				// if none found choose among RuleAlienDeployment def'd. terrains ....
				// Note: cf. QuickBattleState::cbxMissionChange()

				// BZZZT. Do it the opposite way; check RuleAlienDeployment first, then RuleGlobe's texture-terrains.

				// TODO: tie all this into WeightedOptions

				// get a Terrain from RuleAlienDeployment first ->
				//Log(LOG_INFO) << ". . . finding eligible terrain-types in RuleAlienDeployment";
				terrainList = site->getTerrorDeployed()->getDeployTerrains();

				// second check for Terrains in RuleGlobe's textures ->
				if (terrainList.empty() == true)
				{
					//Log(LOG_INFO) << ". . . finding eligible terrain-types in RuleGlobe";
					terrainList = _game->getRuleset()->getGlobe()->getGlobeTerrains();
				}

				if (terrainList.empty() == false) // safety.
				{
					const size_t pick (RNG::pick(terrainList.size()));
					//Log(LOG_INFO) << ". . . . size = " << terrainList.size() << " pick = " << pick;
					//Log(LOG_INFO) << ". . . . terrain = " << terrainList.at(pick) << " - Not Weighted";
					_terrainRule = _game->getRuleset()->getTerrain(terrainList.at(pick));
				}
//				else fuck off. Thanks!
				//else Log(LOG_INFO) << ". . . . eligibleTerrain NOT Found. Must be Cydonia, Base assault/defense.";

				terrainType = _terrainRule->getType();
				site->setSiteTerrainType(terrainType);
				//Log(LOG_INFO) << ". . . using terrainType: " << terrainType;
			}
			else // is UFO
			{
				double // determine if Craft is landing at a City ->
					lon (_craft->getLongitude()),
					lat (_craft->getLatitude());
				RuleRegion* regionRule;

				bool city (false);
				for (std::vector<Region*>::const_iterator
						i = _game->getSavedGame()->getRegions()->begin();
						i != _game->getSavedGame()->getRegions()->end() && city == false;
						++i)
				{
					regionRule = const_cast<RuleRegion*>((*i)->getRules()); // strip const for iteration below_
					if (regionRule->insideRegion(lon,lat) == true)
					{
						for (std::vector<RuleCity*>::const_iterator
								j = regionRule->getCities().begin();
								j != regionRule->getCities().end();
								++j)
						{
							if (AreSameTwo(
										(*j)->getLongitude(), lon,
										(*j)->getLatitude(),  lat) == true)
							{
								//Log(LOG_INFO) << ". . . city found: " << (*j)->getLabel();
								city = true;
								break;
							}
						}
					}
				}

				if (city == true)
				{
					//Log(LOG_INFO) << ". . UFO at City";
					const RuleGlobe* const globeRule (_game->getRuleset()->getGlobe());
					const RuleTexture* const cityTextureRule (globeRule->getTextureRule(OpenXcom::TT_URBAN));

					terrainType = cityTextureRule->getTextureTerrain(ufo);
				}
				else // UFO not at City
				{
					//Log(LOG_INFO) << ". . UFO not at City";
					terrainType = texRule->getTextureTerrain(_craft->getTarget());
				}

				ufo->setUfoTerrainType(terrainType);
			}
		}
		//Log(LOG_INFO) << ". chosen terrainType = " << terrainType;

		if (_terrainRule == nullptr) // '_terrainRule' can be set above^ if terrorSite <-
			_terrainRule = _game->getRuleset()->getTerrain(terrainType);

		_txtTexture->setText(tr("STR_TEXTURE_").arg(tr(terrainType)));
		_txtTexture->setAlign(ALIGN_RIGHT);
	}
	else // aLienBase assault (NOT defense nor Cydonia)
	{
		//Log(LOG_INFO) << ". ufo/terrorsite NOT valid";
		_txtTexture->setVisible(false);
		_txtShade->setVisible(false);
	}

	_txtMessage->setText(tr("STR_CRAFT_READY_TO_LAND_AT_")
						 .arg(_craft->getLabel(_game->getLanguage())));
	_txtMessage->setAlign(ALIGN_CENTER);
	_txtMessage->setBig();

	std::wostringstream woststr;
	if (ufo != nullptr)
	{
		woststr << tr(ufo->getRules()->getType());

		if (ufo->getHyperdecoded() == true) // only ufoType shows if not hyperdetected.
			woststr << L" : " << tr(ufo->getAlienRace());
	}
	_txtMessage2->setText(tr("STR_CRAFT_DESTINATION_")
						 .arg(_craft->getTarget()->getLabel(_game->getLanguage()))
						 .arg(woststr.str()));
	_txtMessage2->setAlign(ALIGN_CENTER);
	_txtMessage2->setBig();

	if (allowTactical == true)
	{
		_txtBegin->setText(tr("STR_BEGIN_MISSION"));
		_txtBegin->setAlign(ALIGN_CENTER);
		_txtBegin->setBig();

		_btnYes->setText(tr("STR_YES"));
		_btnYes->onMouseClick(		static_cast<ActionHandler>(&ConfirmLandingState::btnYesClick));
		_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnYesClick),
									Options::keyOk);
		_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnYesClick),
									Options::keyOkKeypad);
	}
	else if (ufo != nullptr)
	{
		_txtBegin->setVisible(false);

		switch (ufo->getUfoStatus())
		{
			case Ufo::LANDED:
				_btnYes->setText(tr("STR_INTERCEPT"));
				_btnYes->onMouseClick(		static_cast<ActionHandler>(&ConfirmLandingState::btnInterceptClick));
				_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnInterceptClick),
											Options::keyOk);
				_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnInterceptClick),
											Options::keyOkKeypad);
				break;

			case Ufo::CRASHED:
			default:
				_btnYes->setText(tr("STR_RETURN_TO_BASE"));
				_btnYes->onMouseClick(		static_cast<ActionHandler>(&ConfirmLandingState::btnBaseClick));
				_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnBaseClick),
											Options::keyOk);
				_btnYes->onKeyboardPress(	static_cast<ActionHandler>(&ConfirmLandingState::btnBaseClick),
											Options::keyOkKeypad);
		}
	}
	else
	{
		_btnYes->setVisible(false);
		_txtBegin->setVisible(false);
	}

	_btnPatrol->setText(tr("STR_PATROL"));
	_btnPatrol->onMouseClick(	static_cast<ActionHandler>(&ConfirmLandingState::btnPatrolClick));
	_btnPatrol->onKeyboardPress(static_cast<ActionHandler>(&ConfirmLandingState::btnPatrolClick),
								Options::keyCancel);
}

/**
 * dTor
 */
ConfirmLandingState::~ConfirmLandingState()
{}

/**
 * Initializes state.
 */
void ConfirmLandingState::init()
{
	State::init();
}

/**
 * Enters tactical battle.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnYesClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 335);
	_game->popState();

	Ufo* const ufo (dynamic_cast<Ufo*>(_craft->getTarget()));
	TerrorSite* const terrorSite (dynamic_cast<TerrorSite*>(_craft->getTarget()));
	AlienBase* const alienBase (dynamic_cast<AlienBase*>(_craft->getTarget()));

	SavedBattleGame* const battleSave (new SavedBattleGame(
													_game->getSavedGame(),
													&_game->getRuleset()->getOperations(),
													_game->getRuleset()));
	_game->getSavedGame()->setBattleSave(battleSave);

	BattlescapeGenerator bGen (_game);
	bGen.setCraft(_craft);

	if (ufo != nullptr)
	{
		std::string type;
		switch (ufo->getUfoStatus())
		{
			case Ufo::CRASHED: type = "STR_UFO_CRASH_RECOVERY"; break;
			case Ufo::LANDED:  type = "STR_UFO_GROUND_ASSAULT";
		}
		battleSave->setTacticalType(type);

		bGen.setUfo(ufo);
		bGen.setAlienRace(ufo->getAlienRace());
		bGen.setTerrain(_terrainRule);
		bGen.setShade(_shade);
	}
	else if (terrorSite != nullptr)
	{
		battleSave->setTacticalType(terrorSite->getTerrorDeployed()->getType()); // "STR_TERROR_MISSION" / "STR_PORT_ATTACK"

		bGen.setTerrorSite(terrorSite);
		bGen.setAlienRace(terrorSite->getAlienRace());
		bGen.setTerrain(_terrainRule);
		bGen.setShade(_shade);
	}
	else if (alienBase != nullptr)
	{
		battleSave->setTacticalType(alienBase->getAlienBaseDeployed()->getType()); // "STR_ALIEN_BASE_ASSAULT"

		bGen.setAlienBase(alienBase);
		bGen.setAlienRace(alienBase->getAlienRace());
	}
	else
	{
		throw Exception("ConfirmLandingState::btnYesClick() No mission available!");
	}

	bGen.stage(); // <- DETERMINE ALL TACTICAL DATA. |<--|||

	_game->pushState(new BriefingState(_craft));
}

/**
 * Sets the Craft to patrol and exits to the previous state.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnPatrolClick(Action*)
{
	_craft->setTarget();
	_game->popState();
}

/**
 * The Craft stays targeted on a UFO if there are no Soldiers onboard.
 * @note See the 'allowTactical' par in the cTor.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnInterceptClick(Action*)
{
	_craft->interceptGroundTarget(true);
	_game->popState();
}

/**
 * Sends the Craft back to its Base.
 * @param action - pointer to an Action
 */
void ConfirmLandingState::btnBaseClick(Action*)
{
	_craft->returnToBase();
	_game->popState();
}

/**
 * Selects a terrain-type for crashed or landed UFOs.
 * @param lat - latitude of the UFO
 *
RuleTerrain* ConfirmLandingState::selectTerrain(const double lat)
{
	std::vector<RuleTerrain*> terrains;

	const std::vector<std::string>& terrainList = _game->getRuleset()->getTerrainList();
	for (std::vector<std::string>::const_iterator
			i = terrainList.begin();
			i != terrainList.end();
			++i)
	{
		Log(LOG_INFO) << ". . . terrain = " << *i;
		RuleTerrain* const terrainRule = _game->getRuleset()->getTerrain(*i);
		for (std::vector<int>::const_iterator
				j = terrainRule->getTextures()->begin();
				j != terrainRule->getTextures()->end();
				++j)
		{
			Log(LOG_INFO) << ". . . . texture = " << *j;
			if (*j == _texture
				&& (terrainRule->getHemisphere() == 0
					|| (terrainRule->getHemisphere() < 0
						&& lat < 0.)
					|| (terrainRule->getHemisphere() > 0
						&& lat >= 0.)))
			{
				Log(LOG_INFO) << ". . . . . terrainRule = " << *i;
				terrains.push_back(terrainRule);
			}
		}
	}

	if (terrains.empty() == false)
	{
		const size_t pick = static_cast<size_t>(RNG::generate(0,
															  static_cast<int>(terrains.size()) - 1));
		Log(LOG_INFO) << ". . selected terrain = " << terrains.at(pick)->getLabel();
		return terrains.at(pick);
	}

	// else, mission is on water ... pick a city terrain
	// This should actually never happen if AlienMission zone3 globe data is correct.
	// But do this as a safety:
	Log(LOG_INFO) << ". WARNING: terrain NOT Valid - selecting City terrain";
	// note that the URBAN RuleMapScript, spec'd for all city terrains, will not add the UFO-dropship.
	// ... could be cool. Postnote: yeh, was cool!!!!
	_city = true;
	return selectCityTerrain(lat);
} */

/**
 * Selects a terrain-type for missions at cities.
 * @param lat - latitude of the city
 *
RuleTerrain* ConfirmLandingState::selectCityTerrain(const double lat)
{
	const RuleAlienDeployment* const ruleDeploy = _game->getRuleset()->getDeployment("STR_TERROR_MISSION");
	const size_t pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(ruleDeploy->getDeployTerrains().size()) - 1));
	RuleTerrain* const terrainRule = _game->getRuleset()->getTerrain(ruleDeploy->getDeployTerrains().at(pick));
	Log(LOG_INFO) << "cityTerrain = " << ruleDeploy->getDeployTerrains().at(pick);

	return terrainRule;
} */
/*	if (lat < 0. // northern hemisphere
		&& terrainRule->getLabel() == "NATIVEURBAN")
	{
		Log(LOG_INFO) << ". north: switching from Native to Dawn A";
		terrainRule = _game->getRuleset()->getTerrain("DAWNURBANA");
	}
	else if (lat > 0. // southern hemisphere
		&& terrainRule->getLabel() == "DAWNURBANA")
	{
		Log(LOG_INFO) << ". south: switching from Dawn A to Native";
		terrainRule = _game->getRuleset()->getTerrain("NATIVEURBAN");
	} */

}
