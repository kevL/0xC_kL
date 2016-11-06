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

#include "BaseDefenseState.h"

#include "GeoscapeState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Ufo.h"


namespace OpenXcom
{


/**
 * Initializes all the elements in the BaseDefense screen.
 * @param base	- pointer to the Base being attacked
 * @param ufo	- pointer to the attacking Ufo
 * @param geo	- pointer to GeoscapeState
 */
BaseDefenseState::BaseDefenseState(
		Base* const base,
		Ufo* const ufo,
		GeoscapeState* const geo)
	:
		_base(base),
		_ufo(ufo),
		_geo(geo),
		_action(BD_NONE),
		_thinkCycles(0),
		_row(0u),
		_passes(0u),
		_attacks(0u),
		_explosionCount(0u),
		_stLen_destroyed(0u),
		_stLen_initiate(0u),
		_stLen_repulsed(0u)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(288, 17, 16, 10);
	_txtInit		= new Text(320, 9, 0, 30);
	_lstDefenses	= new TextList(277, 121, 24, 43);
	_txtDestroyed	= new Text(320, 9, 0, 167);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("baseDefense");

	add(_window,		"window",	"baseDefense");
	add(_txtTitle,		"text",		"baseDefense");
	add(_txtInit,		"text",		"baseDefense");
	add(_lstDefenses,	"text",		"baseDefense");
	add(_txtDestroyed,	"text",		"baseDefense");
	add(_btnOk,			"button",	"baseDefense");

	centerSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK04.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick(	static_cast<ActionHandler>(&BaseDefenseState::btnOkClick));
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDefenseState::btnOkClick),
							Options::keyOk);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDefenseState::btnOkClick),
							Options::keyOkKeypad);
	_btnOk->onKeyboardPress(static_cast<ActionHandler>(&BaseDefenseState::btnOkClick),
							Options::keyCancel);
	_btnOk->setVisible(false);

	_txtTitle->setText(tr("STR_BASE_UNDER_ATTACK").arg(_base->getLabel()));
	_txtTitle->setBig();

	_txtInit->setAlign(ALIGN_CENTER);

	_lstDefenses->setColumns(3, 130,80,67);

	_txtDestroyed->setAlign(ALIGN_CENTER);

	_timer = new Timer(TI_FAST);
	_timer->onTimer(static_cast<StateHandler>(&BaseDefenseState::next));
	_timer->start();


	_destroyed	= tr("STR_UFO_DESTROYED");
	_initiate	= tr("STR_BASE_DEFENSES_INITIATED");
	_repulsed	= tr("STR_GRAV_SHIELD_REPELS_UFO");

	_gravShields = _base->getGravShields();
	_defenses = _base->getDefenses()->size();

	_game->getResourcePack()->fadeMusic(_game, 863);
//	_game->getCursor()->setVisible(false);
}

/**
 * dTor.
 */
BaseDefenseState::~BaseDefenseState()
{
	delete _timer;
}

/**
 * Advances the state of the State.
 */
void BaseDefenseState::think()
{
	_timer->think(this, nullptr);
}

/**
 * Advances the state of the State by doing this.
 */
void BaseDefenseState::next() // private.
{
	if (_thinkCycles != -1)
	{
		if (_stLen_initiate <= _initiate.size())
		{
			_txtInit->setText(_initiate.substr(0u, _stLen_initiate++));
			return;
		}

		if (++_thinkCycles > 3)
		{
			switch (_action)
			{
				case BD_DESTROY:
					if (_explosionCount == 0u)
					{
						if (_stLen_destroyed == 0u)
						{
							_lstDefenses->addRow(3, L" ",L" ",L" ");
							++_row;
							_timer->setInterval(TI_FAST);
						}

						if (_row > DISPLAYED)
							_lstDefenses->scrollDown(true);

						if (_stLen_destroyed <= _destroyed.size())
						{
							_txtDestroyed->setText(_destroyed.substr(0u, _stLen_destroyed++));
							return;
						}

						_timer->setInterval(TI_MEDIUM);
					}

					_game->getResourcePack()->playSoundFx(ResourcePack::UFO_EXPLODE, true);

					if (++_explosionCount == 3u)
						_action = BD_END;

					return;

				case BD_END:
					_thinkCycles = -1;
//					_game->getCursor()->setVisible();
					_btnOk->setVisible();

					return;
			}

			if (_attacks == _defenses)
			{
				if (_passes == _gravShields)
				{
					_action = BD_END;
					return;
				}

				if (_passes < _gravShields)
				{
					if (_stLen_repulsed == 0u)
					{
						_lstDefenses->addRow(3, L" ",L" ",L" ");
						_lstDefenses->addRow(3, L" ",L" ",L" "); // <- gravShield repels UFO

						if ((_row += 2u) > DISPLAYED)
							_lstDefenses->scrollDown(true);

						_timer->setInterval(TI_FAST);
					}

					if (_stLen_repulsed <= _repulsed.size())
					{
						_lstDefenses->setCellText(
											_row - 1u, 0u,
											_repulsed.substr(0u, _stLen_repulsed++));

						if (_stLen_repulsed > _repulsed.size())
						{
							_lstDefenses->addRow(3, L" ",L" ",L" ");
							++_row;
						}
						return;
					}

					++_passes;
					_attacks = 0u;
					_timer->setInterval(TI_MEDIUM);
					return;
				}
			}

			const BaseFacility* const fac (_base->getDefenses()->at(_attacks));

			switch (_action)
			{
				case BD_NONE:
					_action = BD_FIRE;

					_lstDefenses->addRow(
									3,
									tr(fac->getRules()->getType()),
									L" ",L" ");
					if (++_row > DISPLAYED) _lstDefenses->scrollDown(true);

					return;

				case BD_FIRE:
					_lstDefenses->setCellText(
											_row - 1u, 1u,
											tr("STR_FIRING"));
//					_lstDefenses->setCellColor(_row - 1, 1, 160, /* slate */ true);

					_game->getResourcePack()->playSoundFx(
													static_cast<unsigned>(fac->getRules()->getFireSound()),
													true);
					_action = BD_RESOLVE;
					_timer->setInterval(TI_SLOW);

					return;

				case BD_RESOLVE:
					if (RNG::percent(fac->getRules()->getHitRatio()) == true)
					{
						_game->getResourcePack()->playSoundFx(static_cast<unsigned>(fac->getRules()->getHitSound()));

						int power (fac->getRules()->getDefenseValue());
						power = RNG::generate( // vary power between 75% and 133% ( stock is 50..150% )
											(power * 3) >> 2u,
											(power << 2u) / 3);
						_ufo->setUfoHull(power);

						_lstDefenses->setCellText(
											_row - 1u, 2u,
											tr("STR_HIT"));
//						_lstDefenses->setCellColor(_row - 1u, 2u, 32u, /*green*/ true);
					}
					else
					{
						_lstDefenses->setCellText(
											_row - 1u, 2u,
											tr("STR_MISSED"));
//						_lstDefenses->setCellColor(_row - 1u, 2u, 144u, /*brown*/ true);
					}

					switch (_ufo->getUfoStatus())
					{
						case Ufo::DESTROYED:
							_action = BD_DESTROY;
							break;

						default:
						case Ufo::FLYING:
						case Ufo::LANDED:
						case Ufo::CRASHED:
							_action = BD_NONE;
							break;
					}

					++_attacks;
					_timer->setInterval(TI_MEDIUM);
			}
		}
	}
}

/**
 * Starts tactical or exits to Geoscape.
 * @param action - pointer to an Action
 */
void BaseDefenseState::btnOkClick(Action*)
{
	_timer->stop();
	_game->popState();

	_base->clearBaseDefense();

	switch (_ufo->getUfoStatus()) // TODO: if Status_Crashed, set up Battleship crash-site near the xCom Base.
	{
		case Ufo::FLYING:
		case Ufo::LANDED:
		case Ufo::CRASHED:
			_base->setDefenseReduction(100 - _ufo->getUfoHullPct());
			_geo->baseDefenseTactical(_base, _ufo);
			break;

		case Ufo::DESTROYED:
			_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}
}

}
