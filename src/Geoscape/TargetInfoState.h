/*
 * Copyright 2010-2018 OpenXcom Developers.
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

#ifndef OPENXCOM_TARGETINFOSTATE_H
#define OPENXCOM_TARGETINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class AlienBase;
class Craft;
class GeoscapeState;
class Target;
class Text;
class TextButton;
class TextEdit;
class TextList;
class Window;


/**
 * A State that displays all the Craft targeting a Target on the Globe.
 */
class TargetInfoState
	:
		public State
{

private:
	AlienBase* _aLienBase;
	GeoscapeState* _geoState;
	Text
		* _txtTitle,
		* _txtTargeted;
	TextButton
		* _btnIntercept,
		* _btnOk;
	TextEdit* _edtTarget;
	TextList* _lstTargeters;
	Window* _window;

	std::vector<Craft*> _crafts;


	public:
		/// Creates a TargetInfo state.
		TargetInfoState(
				Target* const target,
				GeoscapeState* const geoState);
		/// Cleans up the TargetInfo state.
		~TargetInfoState();

		/// Handler for pressing the Targeters list.
		void lstTargetersPress(Action* action);

		/// Edits an aLienBase's label.
		void edtTargetChange(Action* action);

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
};

}

#endif
