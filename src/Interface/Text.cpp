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

#include "Text.h"

#include <cmath>		// std::ceil()
//#include <sstream>	// std::wostringstream

#include "../Engine/Font.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
//#include "../Engine/ShaderDraw.h"
#include "../Engine/ShaderMove.h"


namespace OpenXcom
{
/**
 * Sets up the blank Text with a specified size and position.
 * @param width		- width in pixels (default 320)
 * @param height	- height in pixels (default 200)
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
Text::Text(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
				width,
				height,
				x,y),
		_big(nullptr),
		_small(nullptr),
		_font(nullptr),
		_lang(nullptr),
		_wrap(false),
		_invert(false),
		_contrast(CONTRAST_LOW),
		_indent(false),
		_align(ALIGN_LEFT),
		_valign(ALIGN_TOP),
		_color(0u),
		_color2(0u)
{}

/**
 * dTor.
 */
Text::~Text()
{}

/**
 * Quickly converts an integer to a wide-string.
 * @note See c++11 [std::wstring to_wstring()].
 * @param val - the value to convert
 * @return, value as a wide-string
 */
std::wstring Text::intWide(int val) // static.
{
	std::wostringstream woststr;
	woststr << val;
	return woststr.str();
}

/**
 * Takes an integer value and formats it as number with separators spacing the
 * thousands.
 * @param val		- a value
 * @param spaced	- true to insert a space every 3 digits (default true)
 * @param glyf		- reference a currency symbol (default L"")
 * @return, formatted string
 */
std::wstring Text::formatInt( // static.
		int64_t val,
		const bool spaced,
		const std::wstring& glyf)
{
	std::wostringstream woststr;

	const bool neg (val < 0);
	if (neg == true)
		woststr << -val;
	else
		woststr << val;

	std::wstring ret (woststr.str());

	if (spaced == true)
	{
		const std::wstring thousands (L"'");

		size_t place (ret.size() - 3u);
		while (place > 0u && place < ret.size())
		{
			ret.insert(place, thousands);
			place -= 3u;
		}
	}

	if (glyf.empty() == false)
		ret.insert(0u, glyf);

	if (neg == true)
		ret.insert(0u, L"-");

	return ret;
}

/**
 * Takes an integer value and formats it as currency by spacing the thousands
 * and adding a $ sign to the front.
 * @param val - the funds value
 * @return, the formatted string
 */
std::wstring Text::formatCurrency(int64_t val) // static.
{
	return formatInt(val, true, L"$");
}

/**
 * Takes an integer value and formats it as percentage by adding a % sign.
 * @param val - the percentage value
 * @return, the formatted string
 */
std::wstring Text::formatPercent(int val) // static.
{
	std::wostringstream woststr;
	woststr << val << L"%";
	return woststr.str();
}

/**
 * Sets this Text to use the big-size Font.
 */
void Text::setBig()
{
	_font = _big;
	processText();
}

/**
 * Sets this Text to use the small-size Font.
 */
void Text::setSmall()
{
	_font = _small;
	processText();
}

/**
 * Gets the Font currently used by this Text.
 * @return, pointer to Font
 */
Font* Text::getFont() const
{
	return _font;
}

/**
 * Sets the various resources needed for text rendering.
 * @note The different fonts need to be passed in advance since the text-size
 * can change mid-text and the language affects how this Text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void Text::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_big = big;
	_small = _font = small;
	_lang = lang;

	processText();
}

/**
 * Sets the string displayed on screen.
 * @param text - reference a text-string
 */
void Text::setText(const std::wstring& text)
{
	_text = text;
	processText();

//	if (_font == _big // if big Font won't fit the space try small Font
//		&& (getTextWidth() > getWidth() || getTextHeight() > getHeight())
//		&& _text[_text.size() - 1u] != L'.')
//	{
//		setSmall();
//	} // screw font.
}

/**
 * Gets the string displayed on screen.
 * @return, text string
 */
std::wstring Text::getText() const
{
	return _text;
}

/**
 * Enables/disables text wordwrapping.
 * @note If enabled lines of text are automatically split to ensure they stay
 * within the drawing area - otherwise they simply go off the edge.
 * @param wrap		- true to wrap text (default true)
 * @param indent	- true to indent wrapped text (default false)
 */
void Text::setWordWrap(
		bool wrap,
		bool indent)
{
	if (wrap != _wrap || indent != _indent)
	{
		_wrap = wrap;
		_indent = indent;

		processText();
	}
}

/**
 * Enables/disables color inverting.
 * @note Mostly used to make button text look pressed along with the button.
 * @param invert - true to invert text (default true)
 */
void Text::setInvert(bool invert)
{
	_invert = invert;
	_redraw = true;
}

/**
 * Enables/disables high-contrast color.
 * @note Mostly used for Battlescape UI.
 * @param contrast - true for high-contrast (default true)
 */
void Text::setHighContrast(bool contrast)
{
	if (contrast == true)	_contrast = CONTRAST_HIGH;
	else					_contrast = CONTRAST_LOW;

	_redraw = true;
}

/**
 * Gets if this Text is using high-contrast color.
 * @return, true if high-contrast
 */
bool Text::getHighContrast() const
{
	return (_contrast == CONTRAST_HIGH);
}

/**
 * Sets the way this Text is aligned horizontally relative to the drawing area.
 * @param align - horizontal alignment (Text.h)
 */
void Text::setAlign(TextHAlign align)
{
	_align = align;
	_redraw = true;
}

/**
 * Gets the way this Text is aligned horizontally relative to the drawing area.
 * @return, horizontal alignment (Text.h)
 */
TextHAlign Text::getAlign() const
{
	return _align;
}

/**
 * Sets the way this Text is aligned vertically relative to the drawing area.
 * @param valign - vertical alignment (Text.h)
 */
void Text::setVerticalAlign(TextVAlign valign)
{
	_valign = valign;
	_redraw = true;
}

/**
 * Gets the way this Text is aligned vertically relative to the drawing area.
 * @return, vertical alignment (Text.h)
 */
TextVAlign Text::getVerticalAlign() const
{
	return _valign;
}

/**
 * Sets the color used to render this Text.
 * @note Unlike regular graphics Fonts are greyscale so they need to be
 * assigned a specific position in the palette to be displayed.
 * @param color - color value
 */
void Text::setColor(Uint8 color)
{
	_color  =
	_color2 = color;
	_redraw = true;
}

/**
 * Gets the color used to render this Text.
 * @return, color value
 */
Uint8 Text::getColor() const
{
	return _color;
}

/**
 * Sets the secondary color used to render this Text.
 * @note The text switches between the primary and secondary color whenever
 * there's a '0x01' in the string.
 * @param color - color value
 */
void Text::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Gets the secondary color used to render this Text.
 * @return, color value
 */
Uint8 Text::getSecondaryColor() const
{
	return _color2;
}

/**
 * Gets the quantity of lines in this Text including wrapping.
 * @return, quantity of lines
 */
int Text::getQtyLines() const
{
	if (_wrap == true)
		return static_cast<int>(_lineHeight.size());

	return 1;
}

/**
 * Gets this Text's rendered width.
 * @param line - line to get the width of or -1 to get whole text width (default -1)
 * @return, width in pixels
 */
int Text::getTextWidth(int line) const
{
	switch (line)
	{
		case -1:
		{
			int width (0);
			for (std::vector<int>::const_iterator
					i = _lineWidth.begin();
					i != _lineWidth.end();
					++i)
			{
				if (*i > width)
					width = *i;
			}
			return width;
		}

		default:
			return _lineWidth[static_cast<size_t>(line)];
	}
}

/**
 * Gets this Text's rendered height.
 * @note Used to check if word-wrap applies.
 * @param line - line to get the height of or -1 to get whole text height (default -1)
 * @return, height in pixels
 */
int Text::getTextHeight(int line) const
{
	switch (line)
	{
		case -1:
		{
			int height (0);
			for (std::vector<int>::const_iterator
					i = _lineHeight.begin();
					i != _lineHeight.end();
					++i)
			{
				height += *i;
			}
			return height;
		}

		default:
			return _lineHeight[static_cast<size_t>(line)];
	}
}

/**
 * Adds to this Text's height.
 * @param pad - pixels to add to height of a textline (default 1)
 */
void Text::addTextHeight(int pad)
{
	if (_lineHeight.empty() == false)
		_lineHeight[_lineHeight.size() - 1u] += pad;
}

/**
 * Takes care of any text post-processing like calculating line metrics for
 * alignment and word-wrapping if necessary.
 */
void Text::processText() // private.
{
	if (_font != nullptr && _lang != nullptr)
	{
		std::wstring* wst;
		if (_wrap == true)
		{
			_wrappedText = _text; // use a separate string for wordwrapping text
			wst = &_wrappedText;
		}
		else
			wst = &_text;

		_lineWidth.clear();
		_lineHeight.clear();

		bool start (true);
		int
			width (0),
			word  (0);
		size_t
			posSpace     (0u),
			lengthIndent (0u);

		Font* font (_font);

		bool isWrapLetters (_lang->getTextWrapping() == WRAP_LETTERS);

		for (size_t // go through the text character by character
				i = 0u;
				i <= wst->size();
				++i)
		{
			if (i == wst->size() // end of the line
				|| Font::isLinebreak((*wst)[i]) == true)
			{
				// add line measurements for alignment later
				_lineWidth.push_back(width);
				_lineHeight.push_back(static_cast<int>(font->getCharSize(L'\n').h));

				if (i == wst->size())
					break;

				width =
				word  = 0;
				start = true;

				if ((*wst)[i] == Font::TOKEN_BREAK_SMALLLINE) // \x02 marks start of small text - [handled by draw() below_]
					font = _small;
			}
			else if (Font::isSpace((*wst)[i]) == true // keep track of spaces for word-wrapping
				|| Font::isSeparator((*wst)[i]) == true)
			{
				if (i == lengthIndent) // store existing indentation
					++lengthIndent;

				posSpace = i;
				width += static_cast<int>(font->getCharSize((*wst)[i]).w);
				word = 0;
				start = false;
			}
			else if ((*wst)[i] != Font::TOKEN_FLIP_COLORS) // \x01 marks a change of color [handled by draw() below_]
			{
				if (font->getChar((*wst)[i]) == nullptr)
					(*wst)[i] = L'.';

				const int charWidth (static_cast<int>(font->getCharSize((*wst)[i]).w)); // keep track of the width of the last line and word
				width += charWidth;
				word += charWidth;

				if (_wrap == true // word-wrap if the last word doesn't fit the line
					&& width >= getWidth()
					&& (start == false || isWrapLetters == true))
				{
					size_t posIndent (i);

					if (isWrapLetters == false // ie. isWrapWords
						|| Font::isSpace((*wst)[i]) == true)
					{
						width -= word; // go back to the last space and put a linebreak there

						posIndent = posSpace;
						if (Font::isSpace((*wst)[posSpace]) == true)
						{
							width -= static_cast<int>(font->getCharSize((*wst)[posSpace]).w);
							(*wst)[posSpace] = L'\n';
						}
						else
						{
							wst->insert(posSpace + 1u, L"\n");
							++posIndent;
						}
					}
					else if (isWrapLetters == true) // go back to the last letter and put a linebreak there
					{
						wst->insert(i, L"\n");
						width -= charWidth;
					}

					if (lengthIndent != 0u) // keep initial indentation of text
					{
						wst->insert(posIndent + 1u, L" \xA0", lengthIndent);
						posIndent += lengthIndent;
					}

					if (_indent == true) // indent due to word wrap
					{
						wst->insert(posIndent + 1u, L" \xA0");
						width += static_cast<int>(font->getCharSize(L' ').w + font->getCharSize(Font::TOKEN_NBSP).w);
					}

					_lineWidth.push_back(width);
					_lineHeight.push_back(static_cast<int>(font->getCharSize(L'\n').h));

					switch (_lang->getTextWrapping())
					{
						case WRAP_WORDS:
							width = word;
							break;
						case WRAP_LETTERS:
							width = 0;
					}

					start = true;
				}
			}
		}

		_redraw = true;
	}
}

/**
 * Calculates the starting x-position for a line of text.
 * @param line - the line number (0 = first, etc)
 * @return, the x-position in pixels
 */
int Text::getLineX(int line) const
{
	int x (0);
	switch (_lang->getTextDirection())
	{
		case DIRECTION_LTR:
			switch (_align)
			{
				case ALIGN_CENTER:
					x = static_cast<int>(std::ceil(
						static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[static_cast<size_t>(line)]) / 2.));
					break;
				case ALIGN_RIGHT:
					x = getWidth() - 1 - _lineWidth[static_cast<size_t>(line)];
			}
			break;

		case DIRECTION_RTL:
			switch (_align)
			{
				case ALIGN_LEFT:
					x = getWidth() - 1;
					break;
				case ALIGN_CENTER:
					x = getWidth() - static_cast<int>(std::ceil(
									 static_cast<double>(getWidth() + _font->getSpacing() - _lineWidth[static_cast<size_t>(line)]) / 2.));
					break;
				case ALIGN_RIGHT:
					x = _lineWidth[static_cast<size_t>(line)];
			}
	}
	return x;
}


namespace
{

struct PaletteShift
{
	///
	static inline void func(
			Uint8& dst,
			Uint8& src,
			int offset,
			int mult,
			int mid)
	{
		if (src != 0u)
		{
			int inverseOffset;
			if (mid != 0)
				inverseOffset = (2 * (mid - static_cast<int>(src)));
			else
				inverseOffset = 0;

			dst = static_cast<Uint8>(offset + (static_cast<int>(src) * mult) + inverseOffset);
		}
	}
};

}


/**
 * Draws all the characters in this Text with really nasty complex gritty
 * text-rendering algorithmic logic-stuff formatting.
 */
void Text::draw()
{
	Surface::draw();

	if (_text.empty() == false && _font != nullptr)
	{
		if (Options::debugUi == true) // show text-borders for debugUI
		{
			SDL_Rect rect;
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Uint16>(getWidth());
			rect.h = static_cast<Uint16>(getHeight());
			this->drawRect(&rect, 5u);

			++rect.x;
			++rect.y;
			rect.w = static_cast<Uint16>(rect.w - 2u);
			rect.h = static_cast<Uint16>(rect.h - 2u);
			this->drawRect(&rect, 0u);
		}

		int
			line	(0),
			x		(getLineX(line)),
			y		(0),
			height	(0),
			dir		(1),
			mid		(0);
		Uint8 color (_color);

		const std::wstring* wst;
		if (_wrap == true)	wst = &_wrappedText;
		else				wst = &_text;

		for (std::vector<int>::const_iterator
				i = _lineHeight.begin();
				i != _lineHeight.end();
				++i)
		{
			height += *i;
		}

		switch (_valign)
		{
			case ALIGN_TOP:
				y = 0;
				break;
			case ALIGN_MIDDLE:
				y = static_cast<int>(std::ceil(static_cast<double>(getHeight() - height) / 2.));
				break;
			case ALIGN_BOTTOM:
				y = getHeight() - height;
		}

		if (_lang->getTextDirection() == DIRECTION_RTL) // set up text direction
			dir = -1;

		if (_invert == true) // invert text by inverting the font-palette on index 3 (font-palettes use indices 1..5)
			mid = 3;

		Font* font (_font);


		for (std::wstring::const_iterator // draw each letter one-by-one
				i = wst->begin();
				i != wst->end();
				++i)
		{
			if (Font::isSpace(*i) == true)
				x += dir * static_cast<int>(font->getCharSize(*i).w);
			else if (Font::isLinebreak(*i) == true)
			{
				++line;

				y += static_cast<int>(font->getCharSize(*i).h);
				x = getLineX(line);

				if (*i == Font::TOKEN_BREAK_SMALLLINE) // switch to small-font
					font = _small;
				// TODO: Implement a switch to large-font.
			}
			else if (*i == Font::TOKEN_FLIP_COLORS) // switch to alternate color or back to original
			{
				if (color == _color) color = _color2;
				else				 color = _color;
			}
			else
			{
				if (dir < 0)
					x += dir * static_cast<int>(font->getCharSize(*i).w);

				Surface* const srfChar (font->getChar(*i));
				srfChar->setX(x);
				srfChar->setY(y);
				ShaderDraw<PaletteShift>(
									ShaderSurface(this, 0,0),
									ShaderCrop(srfChar),
									ShaderScalar(static_cast<int>(color)),
									ShaderScalar(_contrast),
									ShaderScalar(mid));

				if (dir > 0)
					x += dir * static_cast<int>(font->getCharSize(*i).w);
			}
		}
	}
}

}
