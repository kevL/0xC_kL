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

#ifndef OPENXCOM_MEDIKITVIEW_H
#define OPENXCOM_MEDIKITVIEW_H

#include "../Engine/InteractiveSurface.h"

#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

class BattleUnit;
class Text;


/**
 * Display a view of unit wounds
 */
class MedikitView
	:
		public InteractiveSurface
{

private:
	static const std::string BODY_PARTS[];

	UnitBodyPart _selectedPart;

	const BattleUnit* _unit;
	const Game* _game;
	Text
		* _txtPart,
		* _txtWound;

	/// Handles clicking on the MedikitView.
	void mouseClick(Action* action, State* state) override;


	public:
		/// Creates a MedikitView.
		MedikitView(
				int w,
				int h,
				int x,
				int y,
				const Game* const game,
				const BattleUnit* const unit,
				Text* const part,
				Text* const wound);

		/// Draws the MedikitView.
		void draw() override;

		/// Gets any selected body-part.
		UnitBodyPart getSelectedPart() const;
		/// Selects a wounded body-part if any automatically.
		void autoSelectPart();
};

}

#endif
