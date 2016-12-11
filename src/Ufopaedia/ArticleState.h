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

#ifndef OPENXCOM_ARTICLESTATE_H
#define OPENXCOM_ARTICLESTATE_H

#include <string>

#include "../Engine/State.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

class Action;
class Surface;
class TextButton;
class Timer;


/**
 * ArticleState is the base class for all articles of various types.
 * @note It encapsulates the common characteristics.
 */

class ArticleState
	:
		public State
{

protected:
	static const Uint8
		uPed_ORANGE			=  16u,
		uPed_VIOLET			=  83u,
		uPed_PINK			= 162u, // -> 192+ lt.brown for ExtraAlienInfo
		uPed_BLUE_SLATE		= 239u,
		uPed_GREEN_SLATE	= 244u,

		GEOSCAPE_CYAN		= 133u,

		BASESCAPE_VIOLET	=  64u,
		BASESCAPE_BLUE		= 218u,
		BASESCAPE_WHITE		= 208u,

		tac_RED				=  37u,
		tac_YELLOW			= 144u;


	std::string _id;

	InteractiveSurface* _bg;
	TextButton
		* _btnNext,
		* _btnOk,
		* _btnPrev;
	Timer* _timer;

	/// Constructor (can be instantiated by derived classes only).
	explicit ArticleState(const std::string& article_id);
	/// Destructor.
	virtual ~ArticleState();

	/// Advances to the next/previous Article when right/left key is depressed.
	void keyRepeat();

	/// Screen-layout helpers.
	void initLayout(bool contrast = true);

	/// Callback for Ok button.
	void btnOkClick(Action* action);
	/// Callback for PREV button.
	void btnPrevClick(Action* action);
	/// Callback for NEXT button.
	void btnNextClick(Action* action);


	public:
		/// Runs the Timer.
		void think() override;

		/// Converts damage-type to string.
		static std::string getDamageTypeText(DamageType dType);

		/// Gets the article-ID.
		std::string getId() const
		{ return _id; }
};

}

#endif
