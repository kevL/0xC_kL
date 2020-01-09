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

#include "TextEdit.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Sets up a blank TextEdit with the specified size and position.
 * @param state		- pointer to state this TextEdit belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
TextEdit::TextEdit(
		State* state,
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
				width,
				height,
				x,y),
		_state(state),
		_blink(true),
		_lock(true),
		_ascii(L'A'),
		_caretPlace(0u),
		_inputConstraint(TEC_NONE),
		_change(nullptr),
		_enter(nullptr),
		_bypassChar(false)
{
	_isFocused = false;

	_text = new Text(width, height);

	_timer = new Timer(166u);
	_timer->onTimer(static_cast<SurfaceHandler>(&TextEdit::blink));

	_caret = new Text(5,16);
	_caret->setText(L"|");
}

/**
 * Deletes contents.
 */
TextEdit::~TextEdit()
{
	delete _text;
	delete _caret;
	delete _timer;

	SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL); // in case it was left focused
	_state->setModal(nullptr);
}

/**
 * Passes events to internal components.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandler's belong to
 */
void TextEdit::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state);

	if (_isFocused == true
		&& _lock == true
		&& action->getDetails()->type == SDL_MOUSEBUTTONDOWN
		&& (   action->getAbsoluteMouseX() <  getX()
			|| action->getAbsoluteMouseX() >= getX() + getWidth()
			|| action->getAbsoluteMouseY() <  getY()
			|| action->getAbsoluteMouseY() >= getY() + getHeight()))
	{
		setFocusEdit(false);
	}
}

/**
 * Controls the blinking animation when this TextEdit is focused.
 * @param focus	- true if focused (default true)
 * @param lock	- true to lock input to this control (default false)
 */
void TextEdit::setFocusEdit(
		bool focus,
		bool lock)
{
	_lock = lock;

	if (focus != _isFocused)
	{
		_isFocused = focus;
		_redraw = true;

		if (_isFocused == true)
		{
			_blink = true;
			_timer->start();

			_caretPlace = _edit.length();

			SDL_EnableKeyRepeat(
							180, //SDL_DEFAULT_REPEAT_DELAY,
							60); //SDL_DEFAULT_REPEAT_INTERVAL);

			if (_lock == true) _state->setModal(this);
		}
		else
		{
			_blink = false;
			_timer->stop();

			SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_INTERVAL);

			_state->setModal(nullptr);
		}
	}
}

/**
 * Sets this TextEdit to use the big-sized Font.
 */
void TextEdit::setBig()
{
	_text->setBig();
	_caret->setBig();
}

/**
 * Sets this TextEdit to use the small-sized Font.
 */
void TextEdit::setSmall()
{
	_text->setSmall();
	_caret->setSmall();
}

/**
 * Sets the various Fonts for this TextEdit to use.
 * @note The different fonts need to be passed in advance since the text-size
 * can change mid-text.
 * @param big	- pointer to large-sized font
 * @param small	- pointer to small-sized font
 * @param lang	- pointer to current language
 */
void TextEdit::initText(
		Font* const big,
		Font* const small,
		const Language* const lang)
{
	_text->initText(big, small, lang);
	_caret->initText(big, small, lang);
}

/**
 * Sets the string displayed on screen.
 * @param text - reference to a wide-string
 */
void TextEdit::setText(const std::wstring& text)
{
	_edit = text;
	_caretPlace = _edit.length();
	_redraw = true;
}

/**
 * Gets the string displayed on screen.
 * @return, text-string
 */
std::wstring TextEdit::getText() const
{
	return _edit;
}

/**
 * Stores the previous string-value (before editing) so that it can be retrieved
 * if the editing operation is cancelled.
 * @param text - reference to a wide-string
 */
void TextEdit::setStoredText(const std::wstring& text)
{
	_editCache = text;
}

/**
 * Gets the previous string-value so that it can be reinstated if the editing
 * operation is cancelled.
 * @return, wide-string
 */
std::wstring TextEdit::getStoredText() const
{
	return _editCache;
}

/**
 * Enables/disables wordwrapping.
 * @note The lines of text are automatically split to ensure they stay within
 * the drawing area if enabled; otherwise they simply go off the edge.
 * @param wrap - wordwrapping setting (default true)
 */
void TextEdit::setWordWrap(bool wrap)
{
	_text->setWordWrap(wrap);
}

/**
 * Enables/disables color-inverting.
 * @note Mostly used to make button-text look pressed along with the button.
 * @param invert - invert setting (default true)
 */
void TextEdit::setInvert(bool invert)
{
	_text->setInvert(invert);
	_caret->setInvert(invert);
}

/**
 * Enables/disables high-contrast color.
 * @note Mostly used for Battlescape text.
 * @param contrast - high-contrast setting (default true)
 */
void TextEdit::setHighContrast(bool contrast)
{
	_text->setHighContrast(contrast);
	_caret->setHighContrast(contrast);
}

/**
 * Sets the way the text is aligned horizontally relative to the drawing area.
 * @param align - horizontal alignment
 */
void TextEdit::setAlign(TextHAlign align)
{
	_text->setAlign(align);
}

/**
 * Sets the way the text is aligned vertically relative to the drawing area.
 * @param valign - vertical alignment
 */
void TextEdit::setVerticalAlign(TextVAlign valign)
{
	_text->setVerticalAlign(valign);
}

/**
 * Restricts the text to only numerical input or signed numerical input.
 * @param constraint - TextEditConstraint to be applied (TextEdit.h)
 */
void TextEdit::setConstraint(TextEditConstraint constraint)
{
	_inputConstraint = constraint;
}

/**
 * Sets the color used to render the text.
 * @note Unlike regular graphics, fonts are greyscale so they need to be
 * assigned a specific position in the palette to be displayed.
 * @param color - color value
 */
void TextEdit::setColor(Uint8 color)
{
	_text->setColor(color);
	_caret->setColor(color);
}

/**
 * Gets the color used to render the text.
 * @return, color value
 */
Uint8 TextEdit::getColor() const
{
	return _text->getColor();
}

/**
 * Sets the secondary color used to render the text.
 * @note The text switches between the primary and secondary color whenever
 * there's a 0x01 in the string.
 * @param color - color value
 */
void TextEdit::setSecondaryColor(Uint8 color)
{
	_text->setSecondaryColor(color);
}

/**
 * Gets the secondary color used to render the text.
 * @return, color value
 */
Uint8 TextEdit::getSecondaryColor() const
{
	return _text->getSecondaryColor();
}

/**
 * Replaces a specified quantity of colors in this TextEdit's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void TextEdit::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_text->setPalette(colors, firstcolor, ncolors);
	_caret->setPalette(colors, firstcolor, ncolors);
}

/**
 * Keeps the animation timers running.
 */
void TextEdit::think()
{
	_timer->think(nullptr, this);
}

/**
 * Plays the blinking animation when this TextEdit is focused.
 */
void TextEdit::blink()
{
	_blink = !_blink;
	_redraw = true;
}

/**
 * Adds a blinking '|' caret to the text to show when this TextEdit is focused
 * and editable.
 */
void TextEdit::draw()
{
	Surface::draw();
	_text->setText(_edit);

	if (Options::keyboardMode == KEYBOARD_OFF)
	{
		if (_isFocused == true && _blink == true)
			_text->setText(_edit + _ascii);
	}

	clear();

	if (_enter)
	{
		SDL_Rect rect;
		rect.x =
		rect.y = 0;
		rect.w = static_cast<Uint16>(getWidth());
		rect.h = static_cast<Uint16>(getHeight());
		drawRect(&rect, _text->getColor());
	}

	_text->blit(this);

	if (Options::keyboardMode == KEYBOARD_ON)
	{
		if (_isFocused == true && _blink == true)
		{
			int
				x,y;

			switch (_text->getAlign())
			{
				default:
				case ALIGN_LEFT:
					x = 0;
					break;

				case ALIGN_CENTER:
					x = (_text->getWidth() - _text->getTextWidth()) >> 1u;
					break;

				case ALIGN_RIGHT:
					x = _text->getWidth() - _text->getTextWidth();
			}

			for (size_t
					i = 0u;
					i != _caretPlace;
					++i)
			{
				x += _text->getFont()->getCharSize(_edit[i]).w;
			}

			switch (_text->getVerticalAlign())
			{
				default:
				case ALIGN_TOP:
					y = 0;
					break;

				case ALIGN_MIDDLE:
					y = (getHeight() - _text->getTextHeight() + 1) >> 1u; // round up.
					break;

				case ALIGN_BOTTOM:
					y = getHeight() - _text->getTextHeight();
			}

			_caret->setX(x);
			_caret->setY(y);
			_caret->blit(this);
		}
	}
}

/**
 * Checks if adding a certain character to this TextEdit will exceed the maximum
 * width.
 * @note Used to make sure user input stays within bounds.
 * @param fontChar - character to add
 * @return, true if it exceeds
 */
bool TextEdit::exceedsMaxWidth(wchar_t fontChar) // private.
{
	int width (0);
	std::wstring wst (_edit + fontChar);
	for (std::wstring::const_iterator
			i = wst.begin();
			i != wst.end();
			++i)
	{
		width += _text->getFont()->getCharSize(*i).w;
	}
	return (width > getWidth());
}

/**
 * Checks if input-key character is valid to be inserted at the caret position
 * without breaking the TextEditConstraint.
 * @param key - keycode
 * @return, true if character can be inserted
 */
bool TextEdit::isValidChar(Uint16 key) // private.
{
	switch (_inputConstraint)
	{
		case TEC_NONE:
			return (key > 31 && key < 127); // NOTE: Does not include extended characters.
//			return (key >= L' ' && key <= L'~') || key >= 160;

		// If constraint is signed need to check:
		// - user does not input a digit before '+' or '-'
		// - user can enter a digit anywhere but a sign only at the first position
		case TEC_SIGNED:
			if (_caretPlace == 0u)
			{
				return ((key > 47 && key < 58) || key == 43 || key == 45)
					&& (_edit.size() == 0u || (_edit[0u] != 43 && _edit[0u] != 45));
//				return ((key >= L'0' && key <= L'9') || key == L'+' || key == L'-')
//					&& (_edit.size() == 0u || (_edit[0u] != L'+' && _edit[0u] != L'-'));
			}
			// no break;

		case TEC_UNSIGNED:
			return (key > 47 && key < 58);
//			return (key >= L'0' && key <= L'9');
	}
	return false;
}

/**
 * Focuses this TextEdit when it's pressed on; positions the caret otherwise.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void TextEdit::mousePress(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_isFocused == false)
			setFocusEdit(true, true);
		else
		{
			const double
				mX (action->getRelativeMouseX()),
				scale (action->getScaleX());
			double pX (_text->getLineX(0) * scale);
			if (mX > pX)
			{
				size_t caret (0u);
				for (std::wstring::const_iterator
						i = _edit.begin();
						i != _edit.end();
						++i)
				{
					pX += static_cast<double>(_text->getFont()->getCharSize(*i).w) * scale;
					if (mX < pX)
						break;

					++caret;
				}
				_caretPlace = caret;
			}
			else
				_caretPlace = 0u;
		}
	}
	InteractiveSurface::mousePress(action, state);
}

/**
 * Changes this TextEdit according to keyboard input and unfocuses the text if
 * Enter is pressed.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void TextEdit::keyboardPress(Action* action, State* state)
{
	bool enterPress (false);

	switch (Options::keyboardMode)
	{
		case KEYBOARD_OFF:
			switch (action->getDetails()->key.keysym.sym)
			{
				case SDLK_UP:
					if (++_ascii > L'~')
						_ascii = L' ';
					break;
				case SDLK_DOWN:
					if (--_ascii < L' ')
						_ascii = L'~';
					break;
				case SDLK_LEFT:
					if (_edit.length() > 0u)
						_edit.resize(_edit.length() - 1u);
					break;
				case SDLK_RIGHT:
					if (exceedsMaxWidth(_ascii) == false)
						_edit += _ascii;
			}
			break;

		case KEYBOARD_ON:
			switch (action->getDetails()->key.keysym.sym)
			{
				case SDLK_LEFT:
					if (_caretPlace > 0u)
						--_caretPlace;
					break;
				case SDLK_RIGHT:
					if (_caretPlace < _edit.length())
						++_caretPlace;
					break;
				case SDLK_HOME:
					_caretPlace = 0u;
					break;
				case SDLK_END:
					_caretPlace = _edit.length();
					break;
				case SDLK_BACKSPACE:
					if (_caretPlace > 0u)
					{
						_edit.erase(_caretPlace - 1u, 1u);
						--_caretPlace;
					}
					break;
				case SDLK_DELETE:
					if (_caretPlace < _edit.length())
						_edit.erase(_caretPlace, 1u);
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					if (_edit.empty() == false || _enter != nullptr)
					{
						enterPress = true;
						setFocusEdit(false);
					}
					break;

				default:
					if (_bypassChar == false)
					{
						const Uint16 keycode (action->getDetails()->key.keysym.unicode);
						if (isValidChar(keycode) && exceedsMaxWidth(static_cast<wchar_t>(keycode)) == false)
						{
							_edit.insert(
										_caretPlace,
										1u,
										static_cast<wchar_t>(keycode));
							++_caretPlace;
						}
					}
					else
						_bypassChar = false;
			}
	}
	_redraw = true;

	if (_change != nullptr)
		(state->*_change)(action);

	if (_enter != nullptr && enterPress == true)
		(state->*_enter)(action);

	InteractiveSurface::keyboardPress(action, state);
}

/**
 * Sets a function to be called every time the text changes.
 * @param handler - ActionHandler
 */
void TextEdit::onTextChange(ActionHandler handler)
{
	_change = handler;
}

/**
 * Sets a function to be called every time ENTER is pressed.
 * @param handler - ActionHandler
 */
void TextEdit::onEnter(ActionHandler handler)
{
	_enter = handler;
}

/**
 * I wish i didn't have to do this ....
 * @note Can be used to ignore a key that might be used to activate this
 * TextEdit. CTRL+key won't work 'cause CTRL is already bypassed in
 * InteractiveSurface::keyboardPress() and without it the letter is going to
 * appear in the field.
 */
void TextEdit::setBypass()
{
	_bypassChar = true;
}

}
