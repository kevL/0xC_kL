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

#ifndef OPENXCOM_TEXTBUTTON_H
#define OPENXCOM_TEXTBUTTON_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class ComboBox;
class Font;
class Language;
class Sound;
class Text;


/**
 * Colored button with a Text label.
 * @note Drawn to look like a 3D-shaped box with text on top and responds to
 * mouse clicks. Can be attached to a group of buttons to make it behave like a
 * radio-button with only one button depressed at a time.
 */
class TextButton
	:
		public InteractiveSurface
{

private:
	bool _silent;
//		_geoscapeButton,

	Uint8
		_color,
		_contrast;

	ComboBox* _comboBox;
	Text* _text;
	TextButton** _group;

	/// For use by RuleInterface.
	/// kL_note: Using this instead of the regular setSecondaryColor() will
	/// cause the Text to be rendered in the secondary-color; however, I'm using
	/// the secondary-color for the depressed-color (inverted-color) instead.
//	void setSecondaryColor(Uint8 color) override
//	{ setTextColor(color); }


protected:
	/// Checks if a specified mouse-button is handled.
	bool isButtonHandled(Uint8 btn = 0u) override final;


	public:
		static const Sound* soundPress;

		/// Creates a TextButton with the specified size and position.
		TextButton(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the TextButton.
		~TextButton();

		/// Sets the TextButton's color.
		void setColor(Uint8 color) override;
		/// Gets the TextButton's color.
		Uint8 getColor() const;
		/// Sets the secondary color of this TextButton.
		void setSecondaryColor(Uint8 color) override final;
		/// Sets the TextButton's Text color.
		void setTextColor(Uint8 color);

		/// Sets the Text size to big.
		void setBig();
		/// Sets the Text size to small.
		void setSmall();
		/// Gets the TextButton's current Font.
		Font* getFont() const;
		/// Initializes the TextButton's resources.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override final;

		/// Sets the TextButton's high-contrast color setting.
		void setHighContrast(bool contrast = true) override final;

		/// Sets the TextButton's Text.
		void setText(const std::wstring& text);
		/// Gets the TextButton's Text.
//		std::wstring getText() const;
		/// Gets a pointer to the Text.
		Text* getTextPtr() const;

		/// Sets the TextButton's group.
		void setGroup(TextButton** const group);

		/// Sets the TextButton's palette.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override final;

		/// Draws the TextButton.
		void draw() override;

		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state) override final;

		/// Attaches this button to a combobox.
		void setComboBox(ComboBox* comboBox);

		/// Sets the width of this TextButton.
		void setWidth(int width) override final;
		/// Sets the height of this TextButton.
		void setHeight(int height) override final;

		/// Sets this TextButton as silent.
	   void setSilent();

		/// Sets whether this TextButton is a Geoscape button.
//		void setGeoscapeButton(bool geo);
};

}

#endif
