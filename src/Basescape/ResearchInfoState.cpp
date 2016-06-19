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

#include "ResearchInfoState.h"

//#include <limits>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Timer.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/ResearchProject.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base		- pointer to the Base to get info from
 * @param resRule	- pointer to a RuleResearch which will be used to create a fresh ResearchProject
 */
ResearchInfoState::ResearchInfoState(
		Base* const base,
		const RuleResearch* const resRule)
	:
		_base(base),
		_resRule(resRule),
		_project(new ResearchProject(resRule))
{
	//Log(LOG_INFO) << "ResearchInfoState cTor w/ resRule " << resRule->getType();
	buildUi();
}

/**
 * Initializes all the elements in the ResearchProject screen.
 * @param base		- pointer to the Base to get info from
 * @param project	- pointer to a ResearchProject to modify
 */
ResearchInfoState::ResearchInfoState(
		Base* const base,
		ResearchProject* const project)
	:
		_base(base),
		_resRule(nullptr),
		_project(project)
{
	//Log(LOG_INFO) << "ResearchInfoState cTor w/ project " << project->getRules()->getType();
	buildUi();
}

/**
 * Frees up memory that's not automatically cleaned on exit.
 */
ResearchInfoState::~ResearchInfoState()
{
	delete _timerMore;
	delete _timerLess;
}

/**
 * Builds dialog.
 */
void ResearchInfoState::buildUi()
{
	_fullScreen = false;

	_window			= new Window(this, 240, 140, 40, 30);

	_txtTitle		= new Text(198, 16, 61, 40);

	_txtFreeSci		= new Text(198,  9, 61, 60);
	_txtFreeSpace	= new Text(198,  9, 61, 70);
	_txtAssigned	= new Text(198, 16, 61, 80);

	_btnMore		= new ArrowButton(ARROW_BIG_UP,   120, 16, 100, 100);
	_btnLess		= new ArrowButton(ARROW_BIG_DOWN, 120, 16, 100, 120);

	_btnCancel		= new TextButton(95, 16,  61, 144);
	_btnStartStop	= new TextButton(95, 16, 164, 144);

	setInterface("allocateResearch");

	add(_window,		"window",	"allocateResearch");
	add(_txtTitle,		"text",		"allocateResearch");
	add(_txtFreeSci,	"text",		"allocateResearch");
	add(_txtFreeSpace,	"text",		"allocateResearch");
	add(_txtAssigned,	"text",		"allocateResearch");
	add(_btnMore,		"button1",	"allocateResearch");
	add(_btnLess,		"button1",	"allocateResearch");
	add(_btnCancel,		"button2",	"allocateResearch");
	add(_btnStartStop,	"button2",	"allocateResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	std::wstring wst;
	if (_resRule != nullptr)
		wst = tr(_resRule->getType());
	else
		wst = tr(_project->getRules()->getType());
	_txtTitle->setText(wst);
	_txtTitle->setBig();

	_txtAssigned->setBig();

	updateInfo();

	_btnMore->onMousePress(		(ActionHandler)& ResearchInfoState::morePress);
	_btnMore->onMouseRelease(	(ActionHandler)& ResearchInfoState::moreRelease);

	_btnLess->onMousePress(		(ActionHandler)& ResearchInfoState::lessPress);
	_btnLess->onMouseRelease(	(ActionHandler)& ResearchInfoState::lessRelease);

	_timerMore = new Timer(Timer::SCROLL_SLOW);
	_timerMore->onTimer((StateHandler)& ResearchInfoState::onMore);

	_timerLess = new Timer(Timer::SCROLL_SLOW);
	_timerLess->onTimer((StateHandler)& ResearchInfoState::onLess);

	std::string
		st1,
		st2;

	if (_resRule != nullptr || _project->getOffline() == true)
	{
		st1 = "STR_CANCEL_UC";
		st2 = "STR_START_PROJECT";
	}
	else
	{
		st1 = "STR_OK"; // NOTE: This is activated by a Cancel [Esc] click [key-press]. It means "modifications are done, get rid of popup".
		st2 = "STR_CANCEL_PROJECT";
	}

	_btnCancel->setText(tr(st1));
	_btnCancel->onMouseClick((ActionHandler)& ResearchInfoState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& ResearchInfoState::btnCancelClick,
					Options::keyCancel);

	_btnStartStop->setText(tr(st2));
	_btnStartStop->onMouseClick((ActionHandler)& ResearchInfoState::btnStartStopClick);
	_btnStartStop->onKeyboardPress(
					(ActionHandler)& ResearchInfoState::btnStartStopClick,
					Options::keyOk);
	_btnStartStop->onKeyboardPress(
					(ActionHandler)& ResearchInfoState::btnStartStopClick,
					Options::keyOkKeypad);
}

/**
 * Updates counts of assigned/free scientists and available lab-space.
 */
void ResearchInfoState::updateInfo()
{
	_txtFreeSci->setText(tr("STR_SCIENTISTS_AVAILABLE_UC_")
								.arg(_base->getScientists()));
	_txtFreeSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE_UC_")
								.arg(_base->getFreeLaboratories()));
	_txtAssigned->setText(tr("STR_SCIENTISTS_ALLOCATED_")
								.arg(_project->getAssignedScientists()));
}

/**
 * Exits to the previous screen and either starts or stops a project.
 * @param action - pointer to an Action
 */
void ResearchInfoState::btnStartStopClick(Action*)
{
	if (_resRule != nullptr)					// start a new project
	{
		_project->setCost(_resRule->getCost() * RNG::generate(65,130) / 100);

		_base->addResearch(_project);
		if (_resRule->needsItem() == true)
			_base->getStorageItems()->removeItem(_resRule->getType());
	}
	else if (_project->getOffline() == true)	// re-activate an offline project
		_project->setOffline(false);
	else										// de-activate an active project
	{											// - live alien projects are cancelled
		_resRule = _project->getRules();		// - other projects go offline.

		const bool isLiveAlien (_game->getRuleset()->getUnitRule(_resRule->getType()) != nullptr);
		_base->removeResearch(
						_project,
						false,
						isLiveAlien == false);

		if (isLiveAlien == true && _resRule->needsItem() == true)
			_base->getStorageItems()->addItem(_resRule->getType());
	}

	_game->popState();
}

/**
 * Exits to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchInfoState::btnCancelClick(Action*)
{
	if (_resRule != nullptr) delete _project;

	_game->popState();
}

/**
 * Starts the more Timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::morePress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			moreByValue(stepDelta());
			_timerMore->setInterval(Timer::SCROLL_SLOW);
			_timerMore->start();
			break;

		case SDL_BUTTON_RIGHT:
			moreByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops the more Timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::moreRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerMore->stop();
}

/**
 * Starts the less Timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::lessPress(Action* action)
{
	switch (action->getDetails()->button.button)
	{
		case SDL_BUTTON_LEFT:
			lessByValue(stepDelta());
			_timerLess->setInterval(Timer::SCROLL_SLOW);
			_timerLess->start();
			break;

		case SDL_BUTTON_RIGHT:
			lessByValue(std::numeric_limits<int>::max());
	}
}

/**
 * Stops the less Timer.
 * @param action - pointer to an Action
 */
void ResearchInfoState::lessRelease(Action* action)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		_timerLess->stop();
}

/**
 * Runs state functionality every cycle (used to update the timer).
 */
void ResearchInfoState::think()
{
	State::think();

	_timerLess->think(this, nullptr);
	_timerMore->think(this, nullptr);
}

/**
 * Adds one scientist to the project if possible.
 */
void ResearchInfoState::onMore()
{
	_timerMore->setInterval(Timer::SCROLL_FAST);
	moreByValue(stepDelta());
}

/**
 * Adds the given number of scientists to the project if possible.
 * @param delta - quantity of scientists to add
 */
void ResearchInfoState::moreByValue(int delta)
{
	const int
		freeScientists (_base->getScientists()),
		freeSpaceLab (_base->getFreeLaboratories());

	if (freeScientists != 0 && freeSpaceLab != 0)
	{
		delta = std::min(delta,
						 std::min(freeScientists,
								  freeSpaceLab));
		_project->setAssignedScientists(_project->getAssignedScientists() + delta);
		_base->setScientists(_base->getScientists() - delta);

		updateInfo();
	}
}

/**
 * Removes one scientist from the project if possible.
 */
void ResearchInfoState::onLess()
{
	_timerLess->setInterval(Timer::SCROLL_FAST);
	lessByValue(stepDelta());
}

/**
 * Removes the given number of scientists from the project if possible.
 * @param delta - quantity of scientists to subtract
 */
void ResearchInfoState::lessByValue(int delta)
{
	const int assigned (_project->getAssignedScientists());
	if (assigned != 0)
	{
		delta = std::min(delta, assigned);
		_project->setAssignedScientists(assigned - delta);
		_base->setScientists(_base->getScientists() + delta);

		updateInfo();
	}
}

/**
 * Gets quantity to change by.
 * @note what were these guys smokin'
 * @return, 10 if CTRL is pressed else 1
 */
int ResearchInfoState::stepDelta() const
{
	if ((SDL_GetModState() & KMOD_CTRL) == 0)
		return 1;

	return 10;
}

}
