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

#ifndef OPENXCOM_EXTRAALIENINFOSTATE_H
#define OPENXCOM_EXTRAALIENINFOSTATE_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionTextImage;
class TextButton;
class TextList;
class Window;


/**
 * Displays alien properties such as vulnerabilities and fixed-weapon damage.
 */
class ExtraAlienInfoState
	:
		public ArticleState
{

private:
	TextButton* _btnExit;
	TextList
		* _lstInfo,
		* _lstWeapon;
	Window* _window;

	/// Closes state.
	void btnExit(Action* action);

	public:
		/// cTor.
		explicit ExtraAlienInfoState(const ArticleDefinitionTextImage* const defs);
		/// dTor.
		virtual ~ExtraAlienInfoState();
};

}

#endif
