/*
 * Copyright 2010-2019 OpenXcom Developers.
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

#ifndef OPENXCOM_BAR_H
#define OPENXCOM_BAR_H

#include "../Engine/Surface.h"


namespace OpenXcom
{

/**
 * Bar graphic that represents a certain value.
 * @note Drawn with a colored border and partly filled content to contrast two
 * values, typically used for showing base and soldier stats.
 */
class Bar final
	:
		public Surface
{

private:
	bool
		_invert,
		_secondOnTop;
	int _offSecond_y;
	Uint8
		_color,
		_color2,
		_borderColor;
	double
		_scale,
		_maxVal,
		_value,
		_value2;


	public:
		/// Creates a Bar with the specified size and position.
		Bar(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the Bar.
		~Bar();

		/// Sets the Bar's color.
		void setColor(Uint8 color) override;
		/// Gets the Bar's color.
		Uint8 getColor() const;
		/// Sets the Bar's second color.
		void setSecondaryColor(Uint8 color) override;
		/// Gets the Bar's second color.
		Uint8 getSecondaryColor() const;

		/// Sets the Bar's scale.
		void setScale(double scale = 1.);
		/// Gets the Bar's scale.
		double getScale() const;

		/// Sets the Bar's maximum value.
		void setMaxValue(double maxVal = 100.);
		/// Gets the Bar's maximum value.
//		double getMax() const;

		/// Sets the Bar's current value.
		void setValue(double value);
		/// Gets the Bar's current value.
		double getValue() const;
		/// Sets the Bar's second current value.
		void setValue2(double value);
		/// Gets the Bar's second current value.
		double getValue2() const;

		/// Defines whether the second value should be drawn on top.
		void setSecondValueOnTop(bool onTop = true);
		/// Offsets y-value of second Bar.
		void offsetSecond(int y);

		/// Sets the Bar's color-invert setting.
		void setInvert(bool invert = true);

		/// Draws the Bar.
		void draw() override;

		/// Sets the outline color for the Bar.
		void setBorderColor(Uint8 color) override;
};

}

#endif
