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

#include "OptionsBattlescapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape Options screen.
 * @param origin - game section that originated this state
 */
OptionsBattlescapeState::OptionsBattlescapeState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnBattlescape);

	_txtEdgeScroll	= new Text(114, 9, 94, 8);
	_cbxEdgeScroll	= new ComboBox(this, 104, 16, 94, 18);

	_txtDragScroll	= new Text(114, 9, 206, 8);
	_cbxDragScroll	= new ComboBox(this, 104, 16, 206, 18);

	_txtScrollSpeed	= new Text(114, 9, 94, 40);
	_slrScrollSpeed	= new Slider(104, 16, 94, 50);

	_txtFireSpeed	= new Text(114, 9, 206, 40);
	_slrFireSpeed	= new Slider(104, 16, 206, 50);

	_txtXcomSpeed	= new Text(114, 9, 94, 72);
	_slrXcomSpeed	= new Slider(104, 16, 94, 82);

	_txtAlienSpeed	= new Text(114, 9, 206, 72);
	_slrAlienSpeed	= new Slider(104, 16, 206, 82);

	_txtPathPreview	= new Text(114, 9, 94, 100);
	_btnArrows		= new ToggleTextButton(104, 16, 94, 110);
	_btnTuCost		= new ToggleTextButton(104, 16, 94, 128);

	_txtOptions		= new Text(114, 9, 206, 100);
	_btnTooltips	= new ToggleTextButton(104, 16, 206, 110);
	_btnDeaths		= new ToggleTextButton(104, 16, 206, 128);

	add(_txtEdgeScroll, "text", "battlescapeMenu");
	add(_txtDragScroll, "text", "battlescapeMenu");

	add(_txtScrollSpeed, "text", "battlescapeMenu");
	add(_slrScrollSpeed, "button", "battlescapeMenu");

	add(_txtFireSpeed, "text", "battlescapeMenu");
	add(_slrFireSpeed, "button", "battlescapeMenu");

	add(_txtXcomSpeed, "text", "battlescapeMenu");
	add(_slrXcomSpeed, "button", "battlescapeMenu");

	add(_txtAlienSpeed, "text", "battlescapeMenu");
	add(_slrAlienSpeed, "button", "battlescapeMenu");

	add(_txtPathPreview, "text", "battlescapeMenu");
	add(_btnArrows, "button", "battlescapeMenu");
	add(_btnTuCost, "button", "battlescapeMenu");

	add(_txtOptions, "text", "battlescapeMenu");
	add(_btnTooltips, "button", "battlescapeMenu");
	add(_btnDeaths, "button", "battlescapeMenu");

	add(_cbxEdgeScroll, "button", "battlescapeMenu");
	add(_cbxDragScroll, "button", "battlescapeMenu");

	centerSurfaces();

	_txtEdgeScroll->setText(tr("STR_EDGE_SCROLL"));

	std::vector<std::string> edgeScrolls;
	edgeScrolls.push_back("STR_DISABLED");
	edgeScrolls.push_back("STR_TRIGGER_SCROLL");
	edgeScrolls.push_back("STR_AUTO_SCROLL");

	_cbxEdgeScroll->setOptions(edgeScrolls);
	_cbxEdgeScroll->setSelected(Options::battleEdgeScroll);
	_cbxEdgeScroll->onComboChange(	static_cast<ActionHandler>(&OptionsBattlescapeState::cbxEdgeScrollChange));
//	_cbxEdgeScroll->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_cbxEdgeScroll->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_cbxEdgeScroll->setTooltip("STR_EDGE_SCROLL_DESC");

	_txtDragScroll->setText(tr("STR_DRAG_SCROLL"));

	std::vector<std::string> dragScrolls;
	dragScrolls.push_back("STR_DISABLED");
	dragScrolls.push_back("STR_LEFT_MOUSE_BUTTON");
	dragScrolls.push_back("STR_MIDDLE_MOUSE_BUTTON");
	dragScrolls.push_back("STR_RIGHT_MOUSE_BUTTON");

	_cbxDragScroll->setOptions(dragScrolls);
	_cbxDragScroll->setSelected(static_cast<size_t>(Options::battleDragScrollButton));
	_cbxDragScroll->onComboChange(	static_cast<ActionHandler>(&OptionsBattlescapeState::cbxDragScrollChange));
//	_cbxDragScroll->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_cbxDragScroll->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_cbxDragScroll->setTooltip("STR_DRAG_SCROLL_DESC");

	_txtScrollSpeed->setText(tr("STR_SCROLL_SPEED"));

	_slrScrollSpeed->setRange(2, 20);
	_slrScrollSpeed->setValue(Options::battleScrollSpeed);
	_slrScrollSpeed->onSliderChange(static_cast<ActionHandler>(&OptionsBattlescapeState::slrScrollSpeedChange));
//	_slrScrollSpeed->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_slrScrollSpeed->onMouseOut(	static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_slrScrollSpeed->setTooltip("STR_SCROLL_SPEED_BATTLE_DESC");

	_txtFireSpeed->setText(tr("STR_FIRE_SPEED"));

	_slrFireSpeed->setRange(1, 20);
	_slrFireSpeed->setValue(Options::battleFireSpeed);
	_slrFireSpeed->onSliderChange(	static_cast<ActionHandler>(&OptionsBattlescapeState::slrFireSpeedChange));
//	_slrFireSpeed->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_slrFireSpeed->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_slrFireSpeed->setTooltip("STR_FIRE_SPEED_DESC");

	_txtXcomSpeed->setText(tr("STR_PLAYER_MOVEMENT_SPEED"));

	_slrXcomSpeed->setRange(80, 1); // was 40
	_slrXcomSpeed->setValue(Options::battleXcomSpeed);
	_slrXcomSpeed->onSliderChange(	static_cast<ActionHandler>(&OptionsBattlescapeState::slrXcomSpeedChange));
//	_slrXcomSpeed->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_slrXcomSpeed->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_slrXcomSpeed->setTooltip("STR_PLAYER_MOVEMENT_SPEED_DESC");

	_txtAlienSpeed->setText(tr("STR_COMPUTER_MOVEMENT_SPEED"));

	_slrAlienSpeed->setRange(40, 1);
	_slrAlienSpeed->setValue(Options::battleAlienSpeed);
	_slrAlienSpeed->onSliderChange(	static_cast<ActionHandler>(&OptionsBattlescapeState::slrAlienSpeedChange));
//	_slrAlienSpeed->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_slrAlienSpeed->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_slrAlienSpeed->setTooltip("STR_COMPUTER_MOVEMENT_SPEED_DESC");

	_txtPathPreview->setText(tr("STR_PATH_PREVIEW"));

	_btnArrows->setText(tr("STR_PATH_ARROWS"));
	_btnArrows->setPressed((Options::battlePreviewPath & PATH_ARROWS) != 0);
	_btnArrows->onMouseClick(	static_cast<ActionHandler>(&OptionsBattlescapeState::btnPathPreviewClick));
//	_btnArrows->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_btnArrows->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_btnArrows->setTooltip("STR_PATH_ARROWS_DESC");

	_btnTuCost->setText(tr("STR_PATH_TIME_UNIT_COST"));
	_btnTuCost->setPressed((Options::battlePreviewPath & PATH_TU_COST) != 0);
	_btnTuCost->onMouseClick(	static_cast<ActionHandler>(&OptionsBattlescapeState::btnPathPreviewClick));
//	_btnTuCost->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_btnTuCost->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_btnTuCost->setTooltip("STR_PATH_TIME_UNIT_COST_DESC");

	_txtOptions->setText(tr("STR_USER_INTERFACE_OPTIONS"));

	_btnTooltips->setText(tr("STR_TOOLTIPS"));
	_btnTooltips->setPressed(Options::battleTooltips);
	_btnTooltips->onMouseClick(	static_cast<ActionHandler>(&OptionsBattlescapeState::btnTooltipsClick));
//	_btnTooltips->onMouseIn(	static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_btnTooltips->onMouseOut(	static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_btnTooltips->setTooltip("STR_TOOLTIPS_DESC");

	_btnDeaths->setText(tr("STR_DEATH_NOTIFICATIONS"));
	_btnDeaths->setPressed(Options::battleNotifyDeath);
	_btnDeaths->onMouseClick(	static_cast<ActionHandler>(&OptionsBattlescapeState::btnDeathsClick));
//	_btnDeaths->onMouseIn(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipIn));
//	_btnDeaths->onMouseOut(		static_cast<ActionHandler>(&OptionsBattlescapeState::txtTooltipOut));
//	_btnDeaths->setTooltip("STR_DEATH_NOTIFICATIONS_DESC");
}

/**
 * dTor.
 */
OptionsBattlescapeState::~OptionsBattlescapeState()
{}

/**
 * Changes the edge-scroll option.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::cbxEdgeScrollChange(Action*)
{
	Options::battleEdgeScroll = static_cast<ScrollType>(_cbxEdgeScroll->getSelected());
}

/**
 * Changes the drag-scroll option.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::cbxDragScrollChange(Action*)
{
	Options::battleDragScrollButton = static_cast<int>(_cbxDragScroll->getSelected());
}

/**
 * Updates the scroll-speed.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::slrScrollSpeedChange(Action*)
{
	Options::battleScrollSpeed = _slrScrollSpeed->getValue();
}

/**
 * Updates the fire-speed.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::slrFireSpeedChange(Action*)
{
	Options::battleFireSpeed = _slrFireSpeed->getValue();
}

/**
 * Updates the X-COM movement-speed.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::slrXcomSpeedChange(Action*)
{
	Options::battleXcomSpeed = _slrXcomSpeed->getValue();
}

/**
 * Updates the alien movement-speed.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::slrAlienSpeedChange(Action*)
{
	Options::battleAlienSpeed = _slrAlienSpeed->getValue();
}

/**
 * Updates the path-preview options.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::btnPathPreviewClick(Action*)
{
	int mode (static_cast<int>(PATH_NONE));

	if (_btnArrows->getPressed() == true)
		mode |= PATH_ARROWS;

	if (_btnTuCost->getPressed() == true)
		mode |= PATH_TU_COST;

	Options::battlePreviewPath = static_cast<PathPreview>(mode);
}

/**
 * Updates the Tooltips option.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::btnTooltipsClick(Action*)
{
	Options::battleTooltips = _btnTooltips->getPressed();
}

/**
 * Updates the Death Notifications option.
 * @param action - pointer to an Action
 */
void OptionsBattlescapeState::btnDeathsClick(Action*)
{
	Options::battleNotifyDeath = _btnDeaths->getPressed();
}

}
