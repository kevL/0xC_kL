/*
 * Copyright 2010-2017 OpenXcom Developers.
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

#include "BriefingState.h"

#include "AliensCrashState.h"
#include "BattlescapeState.h"
#include "InventoryState.h"
#include "NextTurnState.h"

#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Geoscape/GeoscapeState.h" // kL_geoMusicPlaying

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleAlienDeployment.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUfo.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Briefing screen.
 * @param craft	- pointer to the xCom Craft in the mission (default nullptr)
 * @param base	- pointer to the xCom Base in the mission (default nullptr)
 */
BriefingState::BriefingState(
		const Craft* const craft,
		const Base* const base)
{
	_window			= new Window(this);
	_txtTitle		= new Text(288, 17, 16, 22);

	_txtTarget		= new Text(288, 17, 16, 39);
	_txtCraft		= new Text(288, 17, 16, 56);

	_txtBriefing	= new Text(288, 97, 16, 75);

	_btnOk			= new TextButton(288, 16, 16, 177);


	std::string type (_game->getSavedGame()->getBattleSave()->getTacticalType());
	const RuleAlienDeployment* ruleDeploy (_game->getRuleset()->getDeployment(type)); // check, Xcom1Ruleset->alienDeployments for a missionType

	if (ruleDeploy == nullptr // landing site or crash site -> define BG & Music by ufoType instead
		&& craft != nullptr)
	{
		const Ufo* const ufo (dynamic_cast<Ufo*>(craft->getTarget()));
		if (ufo != nullptr) // landing site or crash site.
			ruleDeploy = _game->getRuleset()->getDeployment(ufo->getRules()->getType()); // check, Xcom1Ruleset->alienDeployments for a ufoType
	}

	std::string
		description (type + "_BRIEFING"),
		track,	// default defined in Ruleset/RuleAlienDeployment.h: OpenXcom::res_MUSIC_GEO_BRIEFING,
		bg;		// default defined in Ruleset/RuleAlienDeployment.h: "BACK16.SCR",
	BackPals backpal;

	if (ruleDeploy == nullptr) // should never happen
	{
		Log(LOG_WARNING) << "No deployment rule for Briefing: " << type;
		bg = "BACK16.SCR";
		backpal = static_cast<BackPals>(_game->getRuleset()->getInterface("briefing")->getElement("backpal")->color);
		track = OpenXcom::res_MUSIC_GEO_BRIEFING;
	}
	else
	{
		const BriefingData dataBrief (ruleDeploy->getBriefingData());

		bg = dataBrief.background;
		backpal = static_cast<BackPals>(dataBrief.palette);

		switch (_game->getSavedGame()->getBattleSave()->getTacType())
		{
			case TCT_UFOCRASHED:
				track = OpenXcom::res_MUSIC_GEO_BRIEF_UFOCRASHED;
				break;

			case TCT_UFOLANDED:
				track = OpenXcom::res_MUSIC_GEO_BRIEF_UFOLANDED;
				break;

			default:
				track = dataBrief.music;	// note This currently conflicts w/ UFO Recovery/Assault.
											// that is, the music assigned to a UFO will be overridden ...
		}
//		_txtBriefing->setY(_txtBriefing->getY() + dataBrief.textOffset);
//		_txtCraft->setY(_txtCraft->getY() + dataBrief.textOffset);
//		_txtTarget->setVisible(dataBrief.showTargetText);
//		_txtCraft->setVisible(dataBrief.showCraftText);

		if (dataBrief.title.empty() == false)
			type = dataBrief.title;

		if (dataBrief.desc.empty() == false)
			description = dataBrief.desc;
	}
	_game->getResourcePack()->playMusic(track);
	kL_geoMusicPlaying = false;	// otherwise the Briefing music switches back to Geoscape
								// music when on high time-compression (eg, BaseDefense);
								// although Geoscape::init() *should not even run* after this ......
	setPalette(PAL_GEOSCAPE, backpal);
	_window->setBackground(_game->getResourcePack()->getSurface(bg));

	add(_window,		"window",	"briefing");
	add(_txtTitle,		"text",		"briefing");
	add(_txtTarget,		"text",		"briefing");
	add(_txtCraft,		"text",		"briefing");
	add(_txtBriefing,	"text",		"briefing");
	add(_btnOk,			"button",	"briefing");

	centerSurfaces();


	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BriefingState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BriefingState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BriefingState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BriefingState::btnOkClick),
							Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setText(tr(type));

	_txtBriefing->setText(tr(description));
	_txtBriefing->setWordWrap();

	const bool isQuickBattle (_game->getSavedGame()->getMonthsElapsed() == -1);

	std::wstring label;
	if (craft != nullptr)
	{
		label = tr("STR_CRAFT_UC_").arg(craft->getLabel(
													_game->getLanguage(),
													isQuickBattle == false));

		if (craft->getTarget() != nullptr)
		{
			_txtTarget->setBig();
			_txtTarget->setText(craft->getTarget()->getLabel(
														_game->getLanguage(),
														isQuickBattle == false));
		}
		else
			_txtTarget->setVisible(false);
	}
	else if (base != nullptr)
	{
		label = tr("STR_BASE_LC_").arg(base->getLabel());
		_txtTarget->setVisible(false);
	}

	if (label.empty() == false)
	{
		_txtCraft->setText(label);
		_txtCraft->setBig();
	}
	else
		_txtCraft->setVisible(false);

	if (_txtTarget->getVisible() == false)
	{
		_txtCraft->setY(_txtCraft->getY() - 16);
		_txtBriefing->setY(_txtBriefing->getY() - 16);
	}

	if (_txtCraft->getVisible() == false)
		_txtBriefing->setY(_txtBriefing->getY() - 16);

//	if (tacType == TCT_BASEDEFENSE)
//		base->setBaseExposed(false);
}

/**
 * dTor.
 */
BriefingState::~BriefingState()
{}

/**
 * Closes the window.
 * @param action - pointer to an Action
 */
void BriefingState::btnOkClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 335);
	_game->popState();

	BattlescapeState* const battleState (new BattlescapeState()); // <- ah there it is!
	int
		liveHostile,
		livePlayer;
	battleState->getBattleGame()->tallyUnits(
										liveHostile,
										livePlayer);

	switch (liveHostile)
	{
		case 0:
			Options::baseXResolution = Options::baseXGeoscape;
			Options::baseYResolution = Options::baseYGeoscape;

			delete battleState;
			_game->pushState(new AliensCrashState());
			break;

		default:
			Options::baseXResolution = Options::baseXBattlescape;
			Options::baseYResolution = Options::baseYBattlescape;

			_game->pushState(battleState);
			_game->getSavedGame()->getBattleSave()->setBattleState(battleState);
			_game->pushState(new NextTurnState(
											_game->getSavedGame()->getBattleSave(),
											battleState));
			_game->pushState(new InventoryState(
											false,
											battleState));
	}

	_game->getScreen()->resetDisplay(false);
}

}
