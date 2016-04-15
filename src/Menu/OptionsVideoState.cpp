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
		_gameCurrent(0)
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

	_res = SDL_ListModes(nullptr, SDL_FULLSCREEN); // get available fullscreen modes
	if (   _res != (SDL_Rect**)-1
		&& _res != (SDL_Rect**) 0)
	{
		_resCurrent = -1;

		int i (0);
		for (
				i = 0;
				_res[i];
				++i)
		{
			if (_resCurrent == -1
				&& ((_res[i]->w == Options::displayWidth
						&& _res[i]->h <= Options::displayHeight)
					|| _res[i]->w < Options::displayWidth))
			{
				_resCurrent = i;
			}
		}
		_resAmount = i;
	}
	else
	{
		_resCurrent = -1;
		_resAmount = 0;
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

	centerAllSurfaces();


	_txtDisplayResolution->setText(tr("STR_DISPLAY_RESOLUTION"));

//	_displaySurface->setTooltip("STR_DISPLAY_RESOLUTION_DESC");
//	_displaySurface->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_displaySurface->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

	_edtDisplayWidth->setAlign(ALIGN_CENTER);
	_edtDisplayWidth->setBig();
	_edtDisplayWidth->setNumerical(true);
	_edtDisplayWidth->onTextChange((ActionHandler)& OptionsVideoState::txtDisplayWidthChange);

	_txtDisplayX->setText(L"x");
	_txtDisplayX->setAlign(ALIGN_CENTER);
	_txtDisplayX->setBig();

	_edtDisplayHeight->setAlign(ALIGN_CENTER);
	_edtDisplayHeight->setBig();
	_edtDisplayHeight->setNumerical(true);
	_edtDisplayHeight->onTextChange((ActionHandler)& OptionsVideoState::txtDisplayHeightChange);

	std::wostringstream
		woststr1,
		woststr2;
	woststr1 << Options::displayWidth;
	woststr2 << Options::displayHeight;
	_edtDisplayWidth->setText(woststr1.str());
	_edtDisplayHeight->setText(woststr2.str());

	_btnDisplayResolutionUp->onMouseClick((ActionHandler)& OptionsVideoState::btnDisplayResolutionUpClick);
	_btnDisplayResolutionDown->onMouseClick((ActionHandler)& OptionsVideoState::btnDisplayResolutionDownClick);

	_txtMode->setText(tr("STR_DISPLAY_MODE"));

	_txtOptions->setText(tr("STR_DISPLAY_OPTIONS"));

	_btnLetterbox->setText(tr("STR_LETTERBOXED"));
	_btnLetterbox->setPressed(Options::keepAspectRatio);
	_btnLetterbox->onMouseClick((ActionHandler)& OptionsVideoState::btnLetterboxClick);
//	_btnLetterbox->setTooltip("STR_LETTERBOXED_DESC");
//	_btnLetterbox->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_btnLetterbox->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

	_btnLockMouse->setText(tr("STR_LOCK_MOUSE"));
	_btnLockMouse->setPressed(Options::captureMouse == SDL_GRAB_ON);
	_btnLockMouse->onMouseClick((ActionHandler)& OptionsVideoState::btnLockMouseClick);
//	_btnLockMouse->setTooltip("STR_LOCK_MOUSE_DESC");
//	_btnLockMouse->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_btnLockMouse->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

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
	_cbxLanguage->onComboChange((ActionHandler)& OptionsVideoState::cbxLanguageChange);
//	_cbxLanguage->setTooltip("STR_DISPLAY_LANGUAGE_DESC");
//	_cbxLanguage->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_cbxLanguage->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

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
	std::string file;
	const std::vector<std::string> shaders (CrossPlatform::getFolderContents(CrossPlatform::getDataFolder(GL_FOLDER), GL_EXT));
	for (std::vector<std::string>::const_iterator
			i = shaders.begin();
			i != shaders.end();
			++i)
	{
		file = *i;
		_filters.push_back(GL_FOLDER + file); // 4+ OpenGL shader-filters
		filterLabels.push_back(Language::fsToWstr(file.substr(0u, file.length() - GL_EXT.length() - 1u) + GL_STRING));
	}
#endif // __NO_OPENGL

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
#endif // __NO_OPENGL
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
	_cbxFilter->onComboChange((ActionHandler)& OptionsVideoState::cbxFilterChange);
//	_cbxFilter->setTooltip("STR_DISPLAY_FILTER_DESC");
//	_cbxFilter->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_cbxFilter->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

	std::vector<std::string> displayModes;
	displayModes.push_back("STR_WINDOWED");
	displayModes.push_back("STR_FULLSCREEN");
	displayModes.push_back("STR_BORDERLESS");
	displayModes.push_back("STR_RESIZABLE");

	DisplayMode displayMode;
	if (Options::fullscreen == true)
		displayMode = FULLSCREEN;
	else if (Options::borderless == true)
		displayMode = WINDOWED_BORDERLESS;
	else if (Options::allowResize == true)
		displayMode = WINDOWED_RESIZEABLE;
	else
		displayMode = WINDOWED_STATIC;

	_cbxDisplayMode->setOptions(displayModes);
	_cbxDisplayMode->setSelected(displayMode);
	_cbxDisplayMode->onComboChange((ActionHandler)& OptionsVideoState::updateDisplayMode);
//	_cbxDisplayMode->setTooltip("STR_DISPLAY_MODE_DESC");
//	_cbxDisplayMode->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_cbxDisplayMode->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

	_txtGeoScale->setText(tr("STR_GEOSCAPE_SCALE"));

	std::vector<std::string> scales;
	scales.push_back("STR_ORIGINAL");
	scales.push_back("STR_1_5X");
	scales.push_back("STR_2X");
	scales.push_back("STR_THIRD_DISPLAY");
	scales.push_back("STR_HALF_DISPLAY");
	scales.push_back("STR_FULL_DISPLAY");

	_cbxGeoScale->setOptions(scales);
	_cbxGeoScale->setSelected(Options::geoscapeScale);
	_cbxGeoScale->onComboChange((ActionHandler)& OptionsVideoState::updateGeoscapeScale);
//	_cbxGeoScale->setTooltip("STR_GEOSCAPESCALE_SCALE_DESC");
//	_cbxGeoScale->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_cbxGeoScale->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);

	_txtBattleScale->setText(tr("STR_BATTLESCAPE_SCALE"));

	_cbxBattleScale->setOptions(scales);
	_cbxBattleScale->setSelected(Options::battlescapeScale);
	_cbxBattleScale->onComboChange((ActionHandler)& OptionsVideoState::updateBattlescapeScale);
//	_cbxBattleScale->setTooltip("STR_BATTLESCAPE_SCALE_DESC");
//	_cbxBattleScale->onMouseIn((ActionHandler)& OptionsVideoState::txtTooltipIn);
//	_cbxBattleScale->onMouseOut((ActionHandler)& OptionsVideoState::txtTooltipOut);
}

/**
 * dTor.
 */
OptionsVideoState::~OptionsVideoState()
{}

/**
 * Selects a bigger display resolution.
 * @param action - pointer to an Action
 */
void OptionsVideoState::btnDisplayResolutionUpClick(Action*)
{
	if (_resAmount != 0)
	{
		if (_resCurrent < 1)
			_resCurrent = _resAmount - 1;
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
	if (_resAmount != 0)
	{
		if (_resCurrent > _resAmount - 2)
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
	std::wostringstream
		woststr1,
		woststr2;
	woststr1 << static_cast<int>(_res[_resCurrent]->w);
	woststr2 << static_cast<int>(_res[_resCurrent]->h);
	_edtDisplayWidth->setText(woststr1.str());
	_edtDisplayHeight->setText(woststr2.str());

	Options::newDisplayWidth = _res[_resCurrent]->w;
	Options::newDisplayHeight = _res[_resCurrent]->h;
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
	if (   _res != (SDL_Rect**)-1
		&& _res != (SDL_Rect**) 0)
	{
		_resCurrent = -1;
		for (size_t
				i = 0u;
				_res[i];
				++i)
		{
			if (_resCurrent == -1
				&& ((_res[i]->w == Options::newDisplayWidth
						&& _res[i]->h <= Options::newDisplayHeight)
					|| _res[i]->w < Options::newDisplayWidth))
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
	if (   _res != (SDL_Rect**)-1
		&& _res != (SDL_Rect**) 0)
	{
		_resCurrent = -1;
		for (size_t
				i = 0u;
				_res[i];
				++i)
		{
			if (_resCurrent == -1
				&& ((_res[i]->w == Options::newDisplayWidth
						&& _res[i]->h <= Options::newDisplayHeight)
					|| _res[i]->w < Options::newDisplayWidth))
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
		case 0: // none
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 1: // scale
			Options::newScaleFilter	= true;
			Options::newHQXFilter	= false;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 2: // HQx
			Options::newScaleFilter	= false;
			Options::newHQXFilter	= true;
			Options::newXBRZFilter	= false;
			Options::newOpenGL		= false;
			break;

		case 3: // xBRZ
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
	Options::newGeoscapeScale = _cbxGeoScale->getSelected();
}

/**
 * Updates the Battlescape scale.
 * @param action - pointer to an Action
 */
void OptionsVideoState::updateBattlescapeScale(Action*)
{
	Options::newBattlescapeScale = _cbxBattleScale->getSelected();
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void OptionsVideoState::resize(
		int& dX,
		int& dY)
{
	OptionsBaseState::resize(dX, dY);

	std::wostringstream woststr;
	woststr << Options::displayWidth;
	_edtDisplayWidth->setText(woststr.str());

	woststr.str(L"");
	woststr << Options::displayHeight;
	_edtDisplayHeight->setText(woststr.str());
}

/**
 * Takes care of any events from the core engine.
 * @param action - pointer to an Action
 */
void OptionsVideoState::handle(Action* action)
{
	State::handle(action);

	if (action->getDetails()->type == SDL_KEYDOWN
		&& action->getDetails()->key.keysym.sym == SDLK_g
		&& (SDL_GetModState() & KMOD_CTRL) != 0)
	{
		_btnLockMouse->setPressed(Options::captureMouse == SDL_GRAB_ON);
	}
}

}
