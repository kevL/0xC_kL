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

#ifndef OPENXCOM_RESEARCHINFOSTATE
#define OPENXCOM_RESEARCHINFOSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class ArrowButton;
class Base;
class ResearchProject;
class RuleResearch;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * Window that allows changing of the quantity of assigned scientists to a project.
 */
class ResearchInfoState
	:
		public State
{

private:
	ArrowButton
		* _btnMore,
		* _btnLess;
	Base* _base;
	ResearchProject* _project;
	const RuleResearch* _resRule;
	Text
		* _txtAssigned,
		* _txtFreeSci,
		* _txtFreeSpace,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnStartStop;
	Timer
		* _timerLess,
		* _timerMore;
	Window* _window;

	/// Builds the UI.
	void buildUi();
	/// Updates counts of assigned/free scientists and available lab-space.
	void updateInfo();

	/// Handler for clicking the OK button.
	void btnStartStopClick(Action* action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action* action);

	/// Handler for pressing the More button.
	void morePress(Action* action);
	/// Handler for releasing the More button.
	void moreRelease(Action* action);
	/// Handler for pressing the Less button.
	void lessPress(Action* action);
	/// Handler for releasing the Less button.
	void lessRelease(Action* action);

	/// Runs state functionality every Timer tick.
	void think() override;

	/// Function called every time the _timerMore timer is triggered.
	void onMore();
	/// Adds a given number of scientists to the project if possible
	void moreByValue(int delta);
	/// Function called every time the _timerLess timer is triggered.
	void onLess();
	/// Removes a the given number of scientists from the project if possible
	void lessByValue(int delta);


	public:
		/// Creates a ResearchInfo state.
		ResearchInfoState(
				Base* const base,
				const RuleResearch* const resRule);
		/// Creates a ResearchInfo state.
		ResearchInfoState(
				Base* const base,
				ResearchProject* const project);
		/// Cleans up the ResearchInfo state.
		~ResearchInfoState();
};

}

#endif
