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

#ifndef OPENXCOM_TEXTEDIT_H
#define OPENXCOM_TEXTEDIT_H

#include "Text.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Timer;


/**
 * Editable version of Text.
 * @note Receives keyboard-input to allow the player to change the text.
 */
class TextEdit final
	:
		public InteractiveSurface
{

private:
	bool
		_blink,
		_modal,
		_numerical;
	size_t _caretPlace;
	wchar_t _ascii;

	std::wstring
		_edit,
		_editCache;

	ActionHandler _change;
	State* _state;
	Text
		* _caret,
		* _text;
	Timer* _timer;

	/// Checks if a character will exceed the maximum width.
	bool exceedsMaxWidth(wchar_t fontChar);


	public:
		/// Creates a TextEdit with the specified size and position.
		TextEdit(
				State* state,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the TextEdit.
		~TextEdit();

		/// Handle focus.
		void handle(Action* action, State* state) override;

		/// Sets focus on the TextEdit.
		using InteractiveSurface::setFocus;
		void setFocus(
				bool focus,
				bool pokeModal);

		/// Sets the text size to big.
		void setBig();
		/// Sets the text size to small.
		void setSmall();

		/// Initializes the TextEdit's resources.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override;

		/// Sets the text's string.
		void setText(const std::wstring& text);
		/// Gets the text's string.
		std::wstring getText() const;
		/// Stores the original string.
		void storeText(const std::wstring& text);
		/// Gets the original string.
		std::wstring getStoredText() const;

		/// Sets the wordwrap setting.
		void setWordWrap(bool wrap = true);

		/// Sets the color-invert setting.
		void setInvert(bool invert = true);
		/// Sets the high-contrast setting.
		void setHighContrast(bool contrast = true) override;

		/// Sets the horizontal alignment.
		void setAlign(TextHAlign align);
		/// Sets the vertical alignment.
		void setVerticalAlign(TextVAlign valign);

		/// Sets the edit to numerical input.
		void setNumerical(bool numerical = true);

		/// Sets the color.
		void setColor(Uint8 color) override;
		/// Gets the color.
		Uint8 getColor() const;
		/// Sets the secondary color.
		void setSecondaryColor(Uint8 color) override;
		/// Gets the secondary color.
		Uint8 getSecondaryColor() const;
		/// Sets the palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Handles the timers.
		void think() override;
		/// Plays the blinking caret.
		void blink();
		/// Draws the text.
		void draw() override;

		/// Handling for mouse-presses.
		void mousePress(Action* action, State* state) override;
		/// Handling for keyboard-presses.
		void keyboardPress(Action* action, State* state) override;
		/// Hooks an ActionHandler to the edit-field.
		void onTextChange(ActionHandler handler);
};

}

#endif
