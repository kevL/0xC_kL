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

#ifndef OPENXCOM_TECHTREEVIEWERSTATE_H
#define OPENXCOM_TECHTREEVIEWERSTATE_H

#include "../Engine/State.h"

#include <unordered_set> // std::unordered_set


namespace OpenXcom
{

enum TechTreeType
{
	TECH_NONE,			// 0
	TECH_RESEARCH,		// 1
	TECH_MANUFACTURE	// 2
};

class RuleManufacture;
class RuleResearch;
class Ruleset;

class Text;
class TextButton;
class TextList;
class Window;


/**
 * TechTreeViewer screen where you can browse the Tech Tree.
 */
class TechTreeViewerState
	:
		public State
{

private:
	static const Uint8
		TOPIC_GREEN		=  48u,
		TOPIC_BLUE		= 128u,
		TOPIC_YELLOW	= 144u,
		WHITE			= 208u,
		GOLD			= 213u,
		BLUE			= 218u,
		PINK			= 241u;

	static const std::string START_PLAY;

	TechTreeType _selFlag;

	std::string _selTopic;

	std::vector<std::string>
		_topicsLeft,
		_topicsRight;
	std::vector<TechTreeType>
		_flagsLeft,
		_flagsRight;

	std::unordered_set<std::string> _discovered;

	const Ruleset* _rules;

	Text
		* _txtTitle,
		* _txtSelTopic,
		* _txtProgress;
	TextButton
		* _btnOk,
		* _btnSelect;
	TextList
		* _lstLeft,
		* _lstRight;
	Window* _window;

	/// Populates the topics.
	void fillTechTreeLists();
	/// Selects a topic on the left.
	void lstLeftTopicClick(Action* action);
	/// Selects a topic on the right.
	void lstRightTopicClick(Action* action);


	public:
		/// Creates a TechTreeViewer state.
		TechTreeViewerState();
		/// Creates a TechTreeViewer state with a specified topic of Research.
		TechTreeViewerState(const RuleResearch* const topicResearch);
		/// Creates a TechTreeViewer state with a specified topic of Manufacture.
		TechTreeViewerState(const RuleManufacture* const topicManufacture);
		/// Cleans up the TechTreeViewer state.
		~TechTreeViewerState();

		/// Builds the State.
		void build();

		/// Initializes the State.
		void init() override;

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Select Topic button.
		void btnSelectClick(Action* action);

		/// Sets the selected topic.
		void setSelectedTopic(
				const std::string& selTopic,
				bool isResearch);
		/// Checks if a specified topic is discovered.
		bool isDiscovered(const std::string& topic) const;
};

}

#endif
