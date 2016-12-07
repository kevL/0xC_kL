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

#ifndef OPENXCOM_TEXT_H
#define OPENXCOM_TEXT_H

//#include <string>	// std::wstring
#include <vector>	// std::vector

#include "../Engine/Surface.h"


namespace OpenXcom
{

class Font;
class Language;


enum TextHAlign
{
	ALIGN_LEFT,		// 0
	ALIGN_CENTER,	// 1
	ALIGN_RIGHT		// 2
};


enum TextVAlign
{
	ALIGN_TOP,		// 0
	ALIGN_MIDDLE,	// 1
	ALIGN_BOTTOM	// 2
};


/**
 * Text displayed on-screen.
 * @note Takes the characters from a Font and puts them together on-screen to
 * display a string of text taking care of any required aligning or wrapping.
 */
class Text
	:
		public Surface
{

private:
	static const int
		CONTRAST_LOW  = 1,
		CONTRAST_HIGH = 3;

	bool
		_indent,
		_invert,
		_wrap;
	int
		_contrast;
	Uint8
		_color,
		_color2;
	std::wstring
		_text,
		_wrappedText;

	Font
		* _big,
		* _small,
		* _font;
	const Language* _lang;

	TextHAlign _align;
	TextVAlign _valign;

	std::vector<int>
		_lineHeight,
		_lineWidth;

	/// Processes the contained Text.
	void processText();


	public:
		/// Creates a Text with the specified size and position.
		Text(
				int width = 320,
				int height = 200,
				int x = 0,
				int y = 0);
		/// Cleans up the Text.
		~Text();

		/// Quickly converts an integer to a wide-string.
		static std::wstring intWide(int val);
		/// Formats an integer value as number with separators.
		static std::wstring formatInt(
				int64_t val,
				const bool spaced = true,
				const std::wstring& glyf = L"");
		/// Formats an integer value as currency.
		static std::wstring formatCurrency(int64_t val);
		/// Formats an integer value as percentage.
		static std::wstring formatPercent(int val);

		/// Sets the Text to its big Font.
		void setBig();
		/// Sets the Text to its small Font.
		void setSmall();
		/// Gets the Text's current Font.
		Font* getFont() const;

		/// Initializes the resources for the Text.
		void initText(
				Font* const big,
				Font* const small,
				const Language* const lang) override final;

		/// Sets the Text's string.
		void setText(const std::wstring& text);
		/// Gets the Text's string.
		std::wstring getText() const;

		/// Sets the Text's wordwrap setting.
		void setWordWrap(
				bool wrap = true,
				bool indent = false);

		/// Sets the Text's color-invert setting.
		void setInvert(bool invert = true);

		/// Sets the Text's high-contrast setting.
		void setHighContrast(bool contrast = true) override final;
		/// Gets if the Text is using high-contrast color.
		bool getHighContrast() const;

		/// Sets the Text's horizontal alignment.
		void setAlign(TextHAlign align);
		/// Gets the Text's horizontal alignment.
		TextHAlign getAlign() const;
		/// Sets the Text's vertical alignment.
		void setVerticalAlign(TextVAlign valign);
		/// Gets the Text's vertical alignment.
		TextVAlign getVerticalAlign() const;

		/// Sets the Text's color.
		void setColor(Uint8 color) override final;
		/// Gets the Text's color.
		Uint8 getColor() const;
		/// Sets the Text's secondary color.
		void setSecondaryColor(Uint8 color) override final;
		/// Gets the Text's secondary color.
		Uint8 getSecondaryColor() const;

		/// Gets the number of lines in the wrapped Text if wrapping is enabled.
		int getQtyLines() const;

		/// Gets the rendered Text's width.
		int getTextWidth(int line = -1) const;
		/// Gets the rendered Text's height.
		int getTextHeight(int line = -1) const;
		/// Adds to the Text's height.
		void addTextHeight(int pad = 1);

		/// Gets the x-position of a Text line.
		int getLineX(int line) const;

		/// Draws the Text.
		void draw() override final;
};

}

#endif
