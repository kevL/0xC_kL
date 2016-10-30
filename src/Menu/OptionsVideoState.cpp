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

#include "OptionsVideoState.h"

#include "../Engine/Action.h"
#include "../Engine/CrossPlatform.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Text.h"
#include "../Interface/TextEdit.h"
#include "../Interface/ToggleTextButton.h"


namespace OpenXcom
{

const std::string
	OptionsVideoState::GL_EXT		= "OpenGL.shader",
	OptionsVideoState::GL_FOLDER	= "Shaders/",
	OptionsVideoState::GL_STRING	= "*";


/**
 * Initializes all the elements in the VideoOptions screen.
 * @param origin - game-section that originated this state (OptionsBaseState.h)
 */
OptionsVideoState::OptionsVideoState(OptionsOrigin origin)
	:
		OptionsBaseState(origin),
		_resCurrent (-1),
		_resQuantity (0)
//		_displayMode(nullptr),
//		_btnWindowed(nullptr),
//		_btnFullscreen(nullptr),
//		_btnBorderless(nullptr)
{
	setCategory(_btnVideo);

	_displaySurface				= new InteractiveSurface(110, 32, 94, 18);
	_txtDisplayResolution		= new Text(114, 9, 94, 8);
	_edtDisplayWidth			= new TextEdit(this, 40, 17, 94, 26);
	_txtDisplayX				= new Text(16, 17, 132, 26);
	_edtDisplayHeight			= new TextEdit(this, 40, 17, 144, 26);
	_btnDisplayResolutionUp		= new ArrowButton(ARROW_BIG_UP, 14, 14, 186, 18);
	_btnDisplayResolutionDown	= new ArrowButton(ARROW_BIG_DOWN, 14, 14, 186, 36);

	_txtLanguage				= new Text(114, 9, 94, 52);
	_cbxLanguage				= new ComboBox(this, 104, 16, 94, 62);

	_txtFilter					= new Text(114, 9, 206, 52);
	_cbxFilter					= new ComboBox(this, 104, 16, 206, 62);

	_txtMode					= new Text(114, 9, 206, 22);
	_cbxDisplayMode				= new ComboBox(this, 104, 16, 206, 32);

	_txtGeoScale				= new Text(114, 9, 94, 82);
	_cbxGeoScale				= new ComboBox(this, 104, 16, 94, 92);

	_txtBattleScale				= new Text(114, 9, 94, 112);
	_cbxBattleScale				= new ComboBox(this, 104, 16, 94, 122);

	_txtOptions					= new Text(114, 9, 206, 82);
	_btnLetterbox				= new ToggleTextButton(104, 16, 206, 92);
	_btnLockMouse				= new ToggleTextButton(104, 16, 206, 110);


	// "Returns NULL if there are no dimensions available for a particular format,
	// or (SDL_Rect **)-1 if any dimension is okay for the given format." (SDL_video.h)
	_res = SDL_ListModes(nullptr, SDL_FULLSCREEN); // get available fullscreen modes

	if (   _res != reinterpret_cast<SDL_Rect**>(-1)	// NOT all resolutions available for Fullscreen
		&& _res != nullptr)							// NOT  no resolutions available for Fullscreen
	{
		size_t i (0u);
		for (
				i = 0u;
				_res[i] != nullptr;
				++i)
		{
			if (_resCurrent == -1
				&& (  (_res[i]->w == Options::displayWidth && _res[i]->h <= Options::displayHeight)
					|| _res[i]->w <  Options::displayWidth))
			{
				_resCurrent = static_cast<int>(i);
			}
		}
		_resQuantity = static_cast<int>(i);
	}
	else
	{
		_btnDisplayResolutionDown->setVisible(false);
		_btnDisplayResolutionUp->setVisible(false);
		Log(LOG_WARNING) << "Couldn't get display resolutions.";
	}

	add(_displaySurface);
	add(_txtDisplayResolution,		"text",			"videoMenu");
	add(_edtDisplayWidth,			"resolution",	"videoMenu");
	add(_txtDisplayX,				"resolution",	"videoMenu");
	add(_edtDisplayHeight,			"resolution",	"videoMenu");
	add(_btnDisplayResolutionUp,	"button",		"videoMenu");
	add(_btnDisplayResolutionDown,	"button",		"videoMenu");

	add(_txtLanguage,	"text", "videoMenu");
	add(_txtFilter,		"text", "videoMenu");

	add(_txtMode, "text", "videoMenu");

	add(_txtOptions,	"text",		"videoMenu");
	add(_btnLetterbox,	"button",	"videoMenu");
	add(_btnLockMouse,	"button",	"videoMenu");

	add(_cbxFilter,			"button", "videoMenu");
	add(_cbxDisplayMode,	"button", "videoMenu");

	add(_txtBattleScale, "text",	"videoMenu");
	add(_cbxBattleScale, "button",	"videoMenu");

	add(_txtGeoScale, "text",	"videoMenu");
	add(_cbxGeoScale, "button",	"videoMenu");

	add(_cbxLanguage, "button", "videoMenu");

	centerSurfaces();


	_txtDisplayResolution->setText(tr("STR_DISPLAY_RESOLUTION"));

//	_displaySurface->onMouseIn(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_displaySurface->onMouseOut(static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_displaySurface->setTooltip("STR_DISPLAY_RESOLUTION_DESC");

	_edtDisplayWidth->setAlign(ALIGN_CENTER);
	_edtDisplayWidth->setBig();
	_edtDisplayWidth->setConstraint(TEC_UNSIGNED);
	_edtDisplayWidth->onTextChange(static_cast<ActionHandler>(&OptionsVideoState::txtDisplayWidthChange));

	_txtDisplayX->setText(L"x");
	_txtDisplayX->setAlign(ALIGN_CENTER);
	_txtDisplayX->setBig();

	_edtDisplayHeight->setAlign(ALIGN_CENTER);
	_edtDisplayHeight->setBig();
	_edtDisplayHeight->setConstraint(TEC_UNSIGNED);
	_edtDisplayHeight->onTextChange(static_cast<ActionHandler>(&OptionsVideoState::txtDisplayHeightChange));

	_edtDisplayWidth ->setText(Text::intWide(Options::displayWidth));
	_edtDisplayHeight->setText(Text::intWide(Options::displayHeight));

	_btnDisplayResolutionUp->onMouseClick(	static_cast<ActionHandler>(&OptionsVideoState::btnDisplayResolutionUpClick));
	_btnDisplayResolutionDown->onMouseClick(static_cast<ActionHandler>(&OptionsVideoState::btnDisplayResolutionDownClick));

	_txtMode->setText(tr("STR_DISPLAY_MODE"));

	_txtOptions->setText(tr("STR_DISPLAY_OPTIONS"));

	_btnLetterbox->setText(tr("STR_LETTERBOXED"));
	_btnLetterbox->setPressed(Options::keepAspectRatio);
	_btnLetterbox->onMouseClick(static_cast<ActionHandler>(&OptionsVideoState::btnLetterboxClick));
//	_btnLetterbox->onMouseIn(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_btnLetterbox->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_btnLetterbox->setTooltip("STR_LETTERBOXED_DESC");

	_btnLockMouse->setText(tr("STR_LOCK_MOUSE"));
	_btnLockMouse->setPressed(Options::captureMouse == SDL_GRAB_ON);
	_btnLockMouse->onMouseClick(static_cast<ActionHandler>(&OptionsVideoState::btnLockMouseClick));
//	_btnLockMouse->onMouseIn(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_btnLockMouse->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_btnLockMouse->setTooltip("STR_LOCK_MOUSE_DESC");

	_txtLanguage->setText(tr("STR_DISPLAY_LANGUAGE"));

	std::vector<std::wstring> languages;
	Language::getList(_langs, languages);
	_cbxLanguage->setOptions(languages);
	for (size_t
			i = 0u;
			i != languages.size();
			++i)
	{
		if (_langs[i] == Options::language)
		{
			_cbxLanguage->setSelected(i);
			break;
		}
	}
	_cbxLanguage->onComboChange(static_cast<ActionHandler>(&OptionsVideoState::cbxLanguageChange));
//	_cbxLanguage->onMouseIn(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_cbxLanguage->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_cbxLanguage->setTooltip("STR_DISPLAY_LANGUAGE_DESC");

	std::vector<std::wstring> filterLabels;
	filterLabels.push_back(tr("STR_DISABLED"));
	filterLabels.push_back(L"Scale");
	filterLabels.push_back(L"HQx");
	filterLabels.push_back(L"xBRZ");
	_filters.push_back(""); // 0 - none
	_filters.push_back(""); // 1 - scale
	_filters.push_back(""); // 2 - HQx
	_filters.push_back(""); // 3 - xBRZ

#ifndef __NO_OPENGL
	const std::vector<std::string> shaders (CrossPlatform::getFolderContents(CrossPlatform::getDataFolder(GL_FOLDER), GL_EXT));
	for (std::vector<std::string>::const_iterator
			i = shaders.begin();
			i != shaders.end();
			++i)
	{
		_filters.push_back(GL_FOLDER + *i);
		filterLabels.push_back(Language::fsToWstr(ucWords(i->substr(0u, i->length() - GL_EXT.length() - 1u) + GL_STRING)));
	}
#endif

	size_t selFilter (0u);
	if (Screen::isOpenGLEnabled() == true)
	{
#ifndef __NO_OPENGL
		for (size_t
				i = 0u;
				i != _filters.size();
				++i)
		{
			if (_filters[i] == Options::openGLShader)
			{
				selFilter = i;
				break;
			}
		}
#endif
	}
	else if (Options::useScaleFilter == true)
		selFilter = 1u;
	else if (Options::useHQXFilter == true)
		selFilter = 2u;
	else if (Options::useXBRZFilter == true)
		selFilter = 3u;

	_txtFilter->setText(tr("STR_DISPLAY_FILTER"));

	_cbxFilter->setOptions(filterLabels);
	_cbxFilter->setSelected(selFilter);
	_cbxFilter->onComboChange(	static_cast<ActionHandler>(&OptionsVideoState::cbxFilterChange));
//	_cbxFilter->onMouseIn(		static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_cbxFilter->onMouseOut(		static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_cbxFilter->setTooltip("STR_DISPLAY_FILTER_DESC");

	std::vector<std::string> displayModes;
	displayModes.push_back("STR_WINDOWED");
	displayModes.push_back("STR_FULLSCREEN");
	displayModes.push_back("STR_BORDERLESS");
	displayModes.push_back("STR_RESIZABLE");

	DisplayMode displayMode;
	if		(Options::fullscreen == true)	displayMode = FULLSCREEN;
	else if	(Options::borderless == true)	displayMode = WINDOWED_BORDERLESS;
	else if	(Options::allowResize == true)	displayMode = WINDOWED_RESIZEABLE;
	else									displayMode = WINDOWED_STATIC;

	_cbxDisplayMode->setOptions(displayModes);
	_cbxDisplayMode->setSelected(displayMode);
	_cbxDisplayMode->onComboChange(	static_cast<ActionHandler>(&OptionsVideoState::updateDisplayMode));
//	_cbxDisplayMode->onMouseIn(		static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_cbxDisplayMode->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_cbxDisplayMode->setTooltip("STR_DISPLAY_MODE_DESC");

	_txtGeoScale->setText(tr("STR_GEOSCAPE_SCALE"));

	std::vector<std::string> scales;
	scales.push_back("STR_ORIGINAL");
	scales.push_back("STR_1_5X");
	scales.push_back("STR_2X");
	scales.push_back("STR_THIRD_DISPLAY");
	scales.push_back("STR_HALF_DISPLAY");
	scales.push_back("STR_FULL_DISPLAY");

	_cbxGeoScale->setOptions(scales);
	_cbxGeoScale->setSelected(static_cast<size_t>(Options::geoscapeScale));
	_cbxGeoScale->onComboChange(static_cast<ActionHandler>(&OptionsVideoState::updateGeoscapeScale));
//	_cbxGeoScale->onMouseIn(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_cbxGeoScale->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_cbxGeoScale->setTooltip("STR_GEOSCAPESCALE_SCALE_DESC");

	_txtBattleScale->setText(tr("STR_BATTLESCAPE_SCALE"));

	_cbxBattleScale->setOptions(scales);
	_cbxBattleScale->setSelected(static_cast<size_t>(Options::battlescapeScale));
	_cbxBattleScale->onComboChange(	static_cast<ActionHandler>(&OptionsVideoState::updateBattlescapeScale));
//	_cbxBattleScale->onMouseIn(		static_cast<ActionHandler>(&OptionsVideoState::txtTooltipIn));
//	_cbxBattleScale->onMouseOut(	static_cast<ActionHandler>(&OptionsVideoState::txtTooltipOut));
//	_cbxBattleScale->setTooltip("STR_BATTLESCAPE_SCALE_DESC");
}

/**
 * dTor.
 */
OptionsVideoState::~OptionsVideoState()
{}

/**
 * Capitalizes each word in a string.
 * @param str - source string
 * @return, destination string
 */
std::string OptionsVideoState::ucWords(std::string st) // private.
{
	for (size_t
			i = 0u;
			i != st.length();
			++i)
	{
		switch (i)
		{
			case 0u: st[i] = toupper(st[i]);
				break;

			default:
				if (st[i] == '-' || st[i] == '_')
					st[i] = ' ';

				if (st[i] == ' ' && st.length() > i + 1u)
					st[i + 1u] = toupper(st[i + 1u]);
		}
	}
	return st;
}

/**
 * Selects a bigger display resolution.
 * @param action - pointer to an Action
 */
void OptionsVideoState::btnDisplayResolutionUpClick(Action*)
{
	if (_resQuantity != 0)
	{
		if (_resCurrent < 1)
			_resCurrent = _resQuantity - 1;
		else
			--_resCurrent;

		updateDisplayResolution();
	}
}

/**
 * Selects a smaller display resolution.
 * @param action - pointer to an Action
 */
void OptionsVideoState::btnDisplayResolutionDownClick(Action*)
{
	if (_resQuantity != 0)
	{
		if (_resCurrent > _resQuantity - 2)
			_resCurrent = 0;
		else
			++_resCurrent;

		updateDisplayResolution();
	}
}

/**
 * Updates the display resolution based on the selection.
 */
void OptionsVideoState::updateDisplayResolution() // private.
{
	const int
		width  (static_cast<int>(_res[_resCurrent]->w)),
		height (static_cast<int>(_res[_resCurrent]->h));

	_edtDisplayWidth ->setText(Text::intWide(width));
	_edtDisplayHeight->setText(Text::intWide(height));

	Options::newDisplayWidth  = width;
	Options::newDisplayHeight = height;
}

/**
 * Changes the Display Width option.
 * @param action - pointer to an Action
 */
void OptionsVideoState::txtDisplayWidthChange(Action*)
{
	int width;
	std::wstringstream wststr;
	wststr << std::dec << _edtDisplayWidth->getText();
	wststr >> std::dec >> width;
	Options::newDisplayWidth = width;

	// Update resolution mode
	if (   _res != reinterpret_cast<SDL_Rect**>(-1)
		&& _res != nullptr)
	{
		_resCurrent = -1;
		for (size_t
				i = 0u;
				_res[i] != nullptr;
				++i)
		{
			if (_resCurrent == -1
				&& (  (_res[i]->w == Options::newDisplayWidth && _res[i]->h <= Options::newDisplayHeight)
					|| _res[i]->w <  Options::newDisplayWidth))
			{
				_resCurrent = static_cast<int>(i);
			}
		}
	}
}

/**
 * Changes the Display Height option.
 * @param action - pointer to an Action
 */
void OptionsVideoState::txtDisplayHeightChange(Action*)
{
	int height;
	std::wstringstream wststr;
	wststr << std::dec << _edtDisplayHeight->getText();
	wststr >> std::dec >> height;
	Options::newDisplayHeight = height;

	// Update resolution mode
	if (   _res != reinterpret_cast<SDL_Rect**>(-1)
		&& _res != nullptr)
	{
		_resCurrent = -1;
		for (size_t
				i = 0u;
				_res[i];
				++i)
		{
			if (_resCurrent == -1
				&& (  (_res[i]->w == Options::newDisplayWidth && _res[i]->h <= Options::newDisplayHeight)
					|| _res[i]->w <  Options::newDisplayWidth))
			{
				_resCurrent = static_cast<int>(i);
			}
		}
	}
}

/**
 * Changes the Language option.
 * @param action - pointer to an Action
 */
void OptionsVideoState::cbxLanguageChange(Action*)
{
	Options::language = _langs[_cbxLanguage->getSelected()];
}

/**
 * Changes the Filter options.
 * @param action - pointer to an Action
 */
void OptionsVideoState::cbxFilterChange(Action*)
{
	switch (_cbxFilter->getSelected())
	{
		case 0u: // none
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 1u: // scale
			Options::newScaleFilter	= true;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 2u: // HQx
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= true;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 3u: // xBRZ
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= true;
			Options::newOpenGL		= false;
			break;

		default: // OpenGL
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= true;

			Options::newOpenGLShader = _filters[_cbxFilter->getSelected()];
	}
}

/**
 * Changes the Display Mode options.
 * @param action - pointer to an Action
 */
void OptionsVideoState::updateDisplayMode(Action*)
{
	switch (_cbxDisplayMode->getSelected())
	{
		case FULLSCREEN:
			Options::fullscreen		= true;
			Options::borderless		= false;
			Options::allowResize	= false;
			break;

		case WINDOWED_STATIC:
			Options::fullscreen		= false;
			Options::borderless		= false;
			Options::allowResize	= false;
			break;

		case WINDOWED_BORDERLESS:
			Options::fullscreen		= false;
			Options::borderless		= true;
			Options::allowResize	= false;
			break;

		case WINDOWED_RESIZEABLE:
			Options::fullscreen		= false;
			Options::borderless		= false;
			Options::allowResize	= true;
	}
}

/**
 * Changes the Letterboxing option.
 * @param action - pointer to an Action
 */
void OptionsVideoState::btnLetterboxClick(Action*)
{
	Options::keepAspectRatio = _btnLetterbox->getPressed();
}

/**
 * Changes the Lock Mouse option.
 * @param action - pointer to an Action
 */
void OptionsVideoState::btnLockMouseClick(Action*)
{
	Options::captureMouse = static_cast<SDL_GrabMode>(_btnLockMouse->getPressed());
	SDL_WM_GrabInput(Options::captureMouse);
}

/**
 * Changes the geoscape scale.
 * @param action - pointer to an Action
 */
void OptionsVideoState::updateGeoscapeScale(Action*)
{
	Options::newGeoscapeScale = static_cast<int>(_cbxGeoScale->getSelected());
}

/**
 * Updates the Battlescape scale.
 * @param action - pointer to an Action
 */
void OptionsVideoState::updateBattlescapeScale(Action*)
{
	Options::newBattlescapeScale = static_cast<int>(_cbxBattleScale->getSelected());
}

/**
 * Updates the scale.
 * @param dX x-delta;
 * @param dY y-delta;
 */
void OptionsVideoState::resize(
		int& dX,
		int& dY)
{
	OptionsBaseState::resize(dX,dY);
	_edtDisplayWidth ->setText(Text::intWide(Options::displayWidth));
	_edtDisplayHeight->setText(Text::intWide(Options::displayHeight));
}

/**
 * Takes care of any events from the core engine.
 * @param action - pointer to an Action
 */
void OptionsVideoState::handle(Action* action)
{
	State::handle(action);

	if (   action->getDetails()->type == SDL_KEYDOWN
		&& action->getDetails()->key.keysym.sym == SDLK_g
		&& (SDL_GetModState() & KMOD_CTRL) != 0)
	{
		_btnLockMouse->setPressed(Options::captureMouse == SDL_GRAB_ON);
	}
}

}
