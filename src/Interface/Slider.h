/*
 * Copyright 2010-2015 OpenXcom Developers.
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

#ifndef OPENXCOM_SLIDER_H
#define OPENXCOM_SLIDER_H

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Font;
class Frame;
class Language;
class TextButton;
class Text;


/**
 * Horizontal slider control to select from a range of values.
 */
class Slider final
	:
		public InteractiveSurface
{

private:
	bool _pressed;
	int
		_textness,
		_thickness,
		_min,
		_minX,
		_max,
		_maxX,
		_offsetX,
		_value;
	double _pos;

	ActionHandler _change;
	Frame *_frame;
	Text
		* _txtMinus,
		* _txtPlus;
	TextButton *_button;

	/// Sets the slider's position.
	void setPosition(double pos);


	public:
		/// Creates a new slider with the specified size and position.
		Slider(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the slider.
		~Slider();

		/// Sets the X position of the surface.
		void setX(int x) override;
		/// Sets the Y position of the surface.
		void setY(int y) override;
		/// Initializes the slider's resources.
		void initText(
				Font* big,
				Font* small,
				Language* lang) override;
		/// Sets the slider's high contrast color setting.
		void setHighContrast(bool contrast = true);
		/// Sets the slider's color.
		void setColor(Uint8 color) override;
		/// Gets the slider's color.
		Uint8 getColor() const;
		/// Sets the slider's palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256) override;
		/// Sets the slider's range.
		void setRange(
				int min,
				int max);
		/// Sets the slider's value.
		void setValue(int value);
		/// Gets the slider's value.
		int getValue() const;
		/// Blits the slider onto another surface.
		void blit(Surface* surface) override;
		/// Moves the slider.
		void handle(Action* action, State* state) override;
		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state) override;
		/// Hooks an action handler to when the slider changes.
		void onSliderChange(ActionHandler handler);
};

}

#endif
