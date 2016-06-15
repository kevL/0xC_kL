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

#include "BasescapeState.h"

#include "AlienContainmentState.h"
#include "BaseDetectionState.h"
#include "BaseInfoState.h"
#include "BaseView.h"
#include "BuildFacilitiesState.h"
#include "CraftInfoState.h"
#include "CraftsState.h"
#include "DismantleFacilityState.h"
#include "ManufactureState.h"
#include "MiniBaseView.h"
#include "MonthlyCostsState.h"
#include "PsiTrainingState.h"
#include "PurchaseState.h"
#include "ResearchState.h"
#include "SellState.h"
#include "SoldierMemorialState.h"
#include "StoresMatrixState.h"
#include "StoresState.h"
#include "SoldiersState.h"
#include "TransferBaseState.h"
#include "TransfersState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"

#include "../Geoscape/BuildNewBaseState.h"
#include "../Geoscape/GeoscapeState.h"	// kL_geoMusicPlaying
#include "../Geoscape/Globe.h"			// kL_reCenter

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"

#include "../Menu/ErrorMessageState.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Basescape screen.
 * @param base	- pointer to the Base to get info from
 * @param globe	- pointer to the geoscape Globe
 */
BasescapeState::BasescapeState(
		Base* const base,
		Globe* const globe)
	:
		_base(base),
		_globe(globe),
		_baseList(_game->getSavedGame()->getBases()),
		_allowStoresWarning(true)
{
	_baseLayout		= new BaseView(192, 192, 0, 8);
	_miniBases		= new MiniBaseView(128, 22, 192, 33);

	_txtFacility	= new Text(192, 9);

	_edtBase		= new TextEdit(this, 126, 16, 194, 0);
	_txtRegion		= new Text(126, 9, 194, 15);
	_txtFunds		= new Text(126, 9, 194, 24);

	_btnBaseInfo	= new TextButton( 64, 12, 192,  56);
	_btnStores		= new TextButton( 64, 12, 256,  56);
	_btnSoldiers	= new TextButton( 64, 12, 192,  68);
	_btnMemorial	= new TextButton( 64, 12, 256,  68);
	_btnCrafts		= new TextButton(128, 12, 192,  80);
	_btnAliens		= new TextButton(128, 12, 192,  92);
	_btnResearch	= new TextButton(128, 12, 192, 104);
	_btnManufacture	= new TextButton(128, 12, 192, 116);
	_btnPurchase	= new TextButton( 64, 12, 192, 128);
	_btnSell		= new TextButton( 64, 12, 256, 128);
	_btnDaMatrix	= new TextButton(128, 12, 192, 140);
	_btnTransfer	= new TextButton(128, 12, 192, 152);
	_btnIncTrans	= new TextButton(128, 12, 192, 164);
	_btnFacilities	= new TextButton(128, 12, 192, 176);
	_btnGeoscape	= new TextButton(128, 12, 192, 188);
//	_btnNewBase		= new TextButton(128, 12, 192, 176);

	setInterface("basescape");

	add(_baseLayout,		"baseView",		"basescape");
	add(_miniBases,			"miniBase",		"basescape");

	add(_txtFacility,		"textTooltip",	"basescape");

	add(_edtBase,			"text1",		"basescape");
	add(_txtRegion,			"text2",		"basescape");
	add(_txtFunds,			"text3",		"basescape");

	add(_btnBaseInfo,		"button",		"basescape");
	add(_btnStores,			"button",		"basescape");
	add(_btnSoldiers,		"button",		"basescape");
	add(_btnMemorial,		"button",		"basescape");
	add(_btnCrafts,			"button",		"basescape");
	add(_btnAliens,			"button",		"basescape");
	add(_btnResearch,		"button",		"basescape");
	add(_btnManufacture,	"button",		"basescape");
	add(_btnPurchase,		"button",		"basescape");
	add(_btnSell,			"button",		"basescape");
	add(_btnDaMatrix,		"button",		"basescape");
	add(_btnTransfer,		"button",		"basescape");
	add(_btnIncTrans,		"button",		"basescape");
	add(_btnFacilities,		"button",		"basescape");
	add(_btnGeoscape,		"button",		"basescape");
//	add(_btnNewBase,		"button",		"basescape");

	centerAllSurfaces();


	_baseLayout->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_baseLayout->setDog(_game->getResourcePack()->getSurface("BASEDOG"));
	_baseLayout->onMouseClick(
					(ActionHandler)& BasescapeState::layoutLeftClick,
					SDL_BUTTON_LEFT);
	_baseLayout->onMouseClick(
					(ActionHandler)& BasescapeState::layoutRightClick,
					SDL_BUTTON_RIGHT);
	_baseLayout->onMouseOver((ActionHandler)& BasescapeState::layoutMouseOver);
	_baseLayout->onMouseOut((ActionHandler)& BasescapeState::layoutMouseOut);

	_miniBases->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_miniBases->setBases(_baseList);
	_miniBases->onMouseClick(
					(ActionHandler)& BasescapeState::basesLeftClick,
					SDL_BUTTON_LEFT);
	_miniBases->onMouseClick(
					(ActionHandler)& BasescapeState::basesRightClick,
					SDL_BUTTON_RIGHT);
	_miniBases->onKeyboardPress((ActionHandler)& BasescapeState::handleKeyPress);
	_miniBases->onMouseOver((ActionHandler)& BasescapeState::layoutMouseOver);
	_miniBases->onMouseOut((ActionHandler)& BasescapeState::layoutMouseOut);

	_edtBase->setBig();
	_edtBase->onTextChange((ActionHandler)& BasescapeState::edtLabelChange);

	_txtRegion->setAlign(ALIGN_RIGHT);

	_btnBaseInfo->setText(tr("STR_BASE_INFORMATION"));
	_btnBaseInfo->onMouseClick((ActionHandler)& BasescapeState::btnBaseInfoClick);

	_btnStores->setText(tr("STR_STORES"));
	_btnStores->onMouseClick((ActionHandler)& BasescapeState::btnStoresClick);

	_btnSoldiers->setText(tr("STR_SOLDIERS_UC"));
	_btnSoldiers->onMouseClick((ActionHandler)& BasescapeState::btnSoldiersClick);

	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)& BasescapeState::btnMemorialClick);
	_btnMemorial->setVisible(_game->getSavedGame()->getDeadSoldiers()->empty() == false);

	_btnCrafts->setText(tr("STR_EQUIP_CRAFT"));
	_btnCrafts->onMouseClick((ActionHandler)& BasescapeState::btnCraftsClick);

	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& BasescapeState::btnAliens);

	_btnResearch->setText(tr("STR_RESEARCH"));
	_btnResearch->onMouseClick((ActionHandler)& BasescapeState::btnResearchClick);

	_btnManufacture->setText(tr("STR_MANUFACTURE"));
	_btnManufacture->onMouseClick((ActionHandler)& BasescapeState::btnManufactureClick);

	_btnPurchase->setText(tr("STR_PURCHASE_RECRUIT"));
	_btnPurchase->onMouseClick((ActionHandler)& BasescapeState::btnPurchaseClick);

	_btnSell->setText(tr("STR_SELL_SACK_UC"));
	_btnSell->onMouseClick((ActionHandler)& BasescapeState::btnSellClick);

	_btnDaMatrix->setText(tr("STR_MATRIX"));
	_btnDaMatrix->onMouseClick((ActionHandler)& BasescapeState::btnMatrixClick);

	_btnTransfer->setText(tr("STR_TRANSFER_UC"));
	_btnTransfer->onMouseClick((ActionHandler)& BasescapeState::btnTransferClick);

	_btnIncTrans->setText(tr("STR_TRANSIT_LC"));
	_btnIncTrans->onMouseClick((ActionHandler)& BasescapeState::btnIncTransClick);

	_btnFacilities->setText(tr("STR_BUILD_FACILITIES"));
	_btnFacilities->onMouseClick((ActionHandler)& BasescapeState::btnFacilitiesClick);

	_btnGeoscape->setText(tr("STR_GEOSCAPE_UC"));
	_btnGeoscape->onMouseClick((ActionHandler)& BasescapeState::btnGeoscapeClick);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& BasescapeState::btnGeoscapeClick,
					Options::keyOk);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& BasescapeState::btnGeoscapeClick,
					Options::keyOkKeypad);
	_btnGeoscape->onKeyboardPress(
					(ActionHandler)& BasescapeState::btnGeoscapeClick,
					Options::keyCancel);

//	_btnNewBase->setText(tr("STR_BUILD_NEW_BASE_UC"));
//	_btnNewBase->onMouseClick((ActionHandler)& BasescapeState::btnNewBaseClick);
}

/**
 * dTor.
 */
BasescapeState::~BasescapeState()
{
	for (std::vector<Base*>::const_iterator
			i = _baseList->begin();
			i != _baseList->end();
			++i)
	{
		if (*i == _base) return;
	}
	delete _base;
}

/**
 * The player can change the selected base or change info on other screens.
 */
void BasescapeState::init()
{
	State::init();

	setBase(_base);

	_baseLayout->setBase(_base);
	_miniBases->draw();
	_edtBase->setText(_base->getName());

	for (std::vector<Region*>::const_iterator
			i = _game->getSavedGame()->getRegions()->begin();
			i != _game->getSavedGame()->getRegions()->end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(
										_base->getLongitude(),
										_base->getLatitude()) == true)
		{
			_txtRegion->setText(tr((*i)->getRules()->getType()));
			break;
		}
	}

	_txtFunds->setText(tr("STR_FUNDS")
						.arg(Text::formatCurrency(_game->getSavedGame()->getFunds())));

//	_btnNewBase->setVisible(_baseList->size() < Base::MAX_BASES);

	bool
		hasFunds		(_game->getSavedGame()->getFunds() > 0),
		hasQuarters		(false),
		hasSoldiers		(false),
		hasScientists	(false),
		hasEngineers	(false),
		hasHangar		(false),
		hasCraft		(false),
		hasAlienCont	(false),
		hasLabs			(false),
		hasProd			(false),
		hasStorage		(false),
		hasStores		(false);

	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->buildFinished() == true)
		{
			if ((*i)->getRules()->getPersonnel() != 0)
			{
				hasQuarters = true;

				if (_base->getSoldiers()->empty() == false)
					hasSoldiers = true;

				if (_base->getScientists() != 0
					|| _base->getAllocatedScientists() != 0)
				{
					hasScientists = true;
				}

				if (_base->getEngineers() != 0
					|| _base->getAllocatedEngineers() != 0)
				{
					hasEngineers = true;
				}
			}

			if ((*i)->getRules()->getCrafts() != 0)
			{
				hasHangar = true;
				if (_base->getCrafts()->empty() == false)
					hasCraft = true;
			}

			if ((*i)->getRules()->getAliens() != 0)
				hasAlienCont = true;

			if ((*i)->getRules()->getLaboratories() != 0)
				hasLabs = true;

			if ((*i)->getRules()->getWorkshops() != 0)
				hasProd = true;

			if ((*i)->getRules()->getStorage() != 0)
			{
				hasStorage = true;
				if (_base->getStorageItems()->getTotalQuantity() != 0)
					 hasStores = true;
			}
		}
	}

	_btnStores->setVisible(hasStores);
	_btnSoldiers->setVisible(hasSoldiers);
	_btnCrafts->setVisible(hasCraft);
	_btnAliens->setVisible(hasAlienCont); // TODO: hasLiveAliens
	_btnResearch->setVisible(hasLabs && hasScientists);
	_btnManufacture->setVisible(hasProd && hasEngineers);
	_btnPurchase->setVisible(hasFunds && (hasStorage || hasQuarters || hasHangar));
	_btnSell->setVisible(hasStores || hasQuarters || hasCraft || hasAlienCont); // TODO: hasFreeSoldiers || hasFreeScientists || hasFreeEngineers || hasLiveAliens
	_btnTransfer->setVisible(hasFunds && (hasStores || hasQuarters || hasCraft || hasAlienCont)); // TODO: ditto.
	_btnFacilities->setVisible(hasFunds);

	_btnIncTrans->setVisible(_base->getTransfers()->empty() == false);


	const std::string track (OpenXcom::res_MUSIC_GEO_GLOBE);
	if (_game->getResourcePack()->isMusicPlaying(track) == false) // stop Dogfight music, Pls.
	{
		_game->getResourcePack()->fadeMusic(_game, 345);
		_game->getResourcePack()->playMusic(track);
	}

	if (_allowStoresWarning == true
		&& _base->storesOverfull() == true
		&& _game->getSavedGame()->getMonthsPassed() != -1)
	{
		_allowStoresWarning = false;
//		_game->pushState(new SellState(_base));

		const RuleInterface* const uiRule (_game->getRuleset()->getInterface("basescape"));
		_game->pushState(new ErrorMessageState(
										tr("STR_STORAGE_EXCEEDED").arg(_base->getName()).c_str(),
										_palette,
										uiRule->getElement("errorMessage")->color,
										_game->getResourcePack()->getBackgroundRand(),
										uiRule->getElement("errorPalette")->color));
	}
}

/**
 * Changes the Base currently displayed on screen.
 * @param base - pointer to new base to display
 */
void BasescapeState::setBase(Base* const base)
{
	for (size_t
			i = 0u;
			i != _baseList->size();
			++i)
	{
		if (_baseList->at(i) == base)
		{
			_base = base;
			_miniBases->setSelectedBase(i);
			return;
		}
	}
	_base = _baseList->front();
	_miniBases->setSelectedBase(0u);
}

/**
 * Goes to the Build New Base screen.
 * @param action - pointer to an Action
 *
void BasescapeState::btnNewBaseClick(Action*)
{
	Base* base = new Base(_game->getRuleset());
	_game->popState();
	_game->pushState(new BuildNewBaseState(base, _globe));
} */

/**
 * Goes to the Base Info screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnBaseInfoClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new BaseInfoState(_base, this));
}

/**
 * Goes to the Stores screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnStoresClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new StoresState(_base));
}

/**
 * Goes to the Soldiers screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnSoldiersClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new SoldiersState(_base));
}

/**
 * Opens the Memorial screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnMemorialClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
		_game->pushState(new SoldierMemorialState());
	}
}

/**
 * Goes to the Crafts screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnCraftsClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new CraftsState(_base));
}

/**
 * Goes to the Manage Alien Containment screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnAliens(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new AlienContainmentState(_base, OPT_GEOSCAPE, this));
}

/**
 * Goes to the Research screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnResearchClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new ResearchState(_base, this));
}

/**
 * Goes to the Manufacture screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnManufactureClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new ManufactureState(_base, this));
}

/**
 * Goes to the Purchase screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnPurchaseClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new PurchaseState(_base));
}

/**
 * Goes to the Sell screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnSellClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new SellState(_base));
}

/**
 * Shows da Matrix.
 * @param action - pointer to an Action
 */
void BasescapeState::btnMatrixClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new StoresMatrixState(_base));
}

/**
 * Goes to the Select Destination Base window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnTransferClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new TransferBaseState(_base));
}

/**
 * Goes to the incoming Transfers window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnIncTransClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new TransfersState(_base));
}

/**
 * Opens the Build Facilities window.
 * @param action - pointer to an Action
 */
void BasescapeState::btnFacilitiesClick(Action*)
{
	if (_edtBase->isFocused() == false)
		_game->pushState(new BuildFacilitiesState(_base, this));
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void BasescapeState::btnGeoscapeClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		_game->getScreen()->fadeScreen();

		kL_geoMusicPlaying = false;
		kL_geoMusicReturnState = true;
		_game->popState();
	}
}

/**
 * Processes left clicking on facilities.
 * @param action - pointer to an Action
 */
void BasescapeState::layoutLeftClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		const BaseFacility* const fac (_baseLayout->getSelectedFacility());
		bool bPop (false);

		if (fac == nullptr) // dirt.
		{
			_game->pushState(new MonthlyCostsState(_base));
			bPop = true;
		}
		else if (fac->buildFinished() == true)
		{
			if (fac->getRules()->getCrafts() != 0)
			{
				for (size_t
						i = 0u;
						i != _base->getCrafts()->size();
						++i)
				{
					if (fac->getCraft() == nullptr
						|| fac->getCraft()->getCraftStatus() == CS_OUT)
					{
						_game->pushState(new CraftsState(_base));
						bPop = true;
						break;
					}
					else if (fac->getCraft() == _base->getCrafts()->at(i))
					{
						_game->pushState(new CraftInfoState(_base, i));
						return;
					}
				}
			}
			else if (fac->getRules()->getStorage() != 0)
			{
				if (_base->getStorageItems()->getTotalQuantity() != 0)
				{
					_game->pushState(new StoresState(_base));
					bPop = true;
				}
			}
			else if (fac->getRules()->getPersonnel() != 0)
			{
				if (_base->getSoldiers()->empty() == false)
				{
					_game->pushState(new SoldiersState(_base));
					bPop = true;
				}
			}
			else if (fac->getRules()->getPsiLaboratories() != 0)
			{
				_game->pushState(new PsiTrainingState(_base));
				bPop = true;
			}
			else if (fac->getRules()->getLaboratories() != 0)
			{
				_game->pushState(new ResearchState(_base, this));
				bPop = true;
			}
			else if (fac->getRules()->getWorkshops() != 0)
			{
				_game->pushState(new ManufactureState(_base, this));
				bPop = true;
			}
			else if (fac->getRules()->getAliens() != 0)
			{
				_game->pushState(new AlienContainmentState(_base, OPT_GEOSCAPE, this));
				bPop = true;
			}
			else if (fac->getRules()->isLift() == true) // Lift has radar range (cf. next)
			{
				_game->pushState(new BaseDetectionState(_base));
				return;
			}
			else if (fac->getRules()->getRadarRange() != 0
				|| fac->getRules()->isMindShield() == true
				|| fac->getRules()->isHyperwave() == true)
			{
				_game->pushState(new BaseInfoState(_base, this));
				bPop = true;
/*				_game->getSavedGame()->setGlobeLongitude(_base->getLongitude());
				_game->getSavedGame()->setGlobeLatitude(_base->getLatitude());
				kL_reCenter = true;
				_game->popState(); */
			}
		}

		if (bPop == true)
			kL_soundPop->play(Mix_GroupAvailable(0)); // play "wha-wha" sound
	}
}

/**
 * Processes right clicking on facilities.
 * @param action - pointer to an Action
 */
void BasescapeState::layoutRightClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		const BaseFacility* const fac (_baseLayout->getSelectedFacility());
		if (fac != nullptr)
		{
			if (fac->inUse() == true)
			{
				const RuleInterface* const uiRule (_game->getRuleset()->getInterface("basescape"));
				_game->pushState(new ErrorMessageState(
												tr("STR_FACILITY_IN_USE"),
												_palette,
												uiRule->getElement("errorMessage")->color,
												_game->getResourcePack()->getBackgroundRand(),
												uiRule->getElement("errorPalette")->color));
			}
			else if (_base->getDisconnectedFacilities(fac).empty() == false)
			{
				const RuleInterface* const uiRule (_game->getRuleset()->getInterface("basescape"));
				_game->pushState(new ErrorMessageState(
												tr("STR_CANNOT_DISMANTLE_FACILITY"),
												_palette,
												uiRule->getElement("errorMessage")->color,
												_game->getResourcePack()->getBackgroundRand(),
												uiRule->getElement("errorPalette")->color));
			}
			else
				_game->pushState(new DismantleFacilityState(_base, _baseLayout, fac));
		}
	}
}

/**
 * Displays either the name of the Facility the mouse is over or the name of the
 * Base the mouse is over.
 * @param action - pointer to an Action
 */
void BasescapeState::layoutMouseOver(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		std::wostringstream woststr;

		const BaseFacility* const fac (_baseLayout->getSelectedFacility());
		if (fac != nullptr)
		{
			_txtFacility->setAlign(ALIGN_LEFT);
			woststr << tr(fac->getRules()->getType());

			if (fac->getCraft() != nullptr)
				woststr << L" " << tr("STR_CRAFT_")
									.arg(fac->getCraft()->getName(_game->getLanguage()));
		}
		else
		{
			const size_t baseId (_miniBases->getHoveredBase());
			if (baseId < _baseList->size())
			{
				if (_base != _baseList->at(baseId))
				{
					_txtFacility->setAlign(ALIGN_RIGHT);
					woststr << _baseList->at(baseId)->getName().c_str();
				}
			}
			else if (baseId == _baseList->size() && baseId < Base::MAX_BASES)
			{
				_txtFacility->setAlign(ALIGN_RIGHT);
				woststr << tr("STR_BUILD_NEW_BASE");
			}
		}
		_txtFacility->setText(woststr.str());
	}
}

/**
 * Clears the Facility or other Base's name.
 * @param action - pointer to an Action
 */
void BasescapeState::layoutMouseOut(Action*)
{
	_txtFacility->setText(L"");
}

/**
 * Selects a different Base to display. Also builds a new Base on the globe.
 * @param action - pointer to an Action
 */
void BasescapeState::basesLeftClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		const size_t baseId (_miniBases->getHoveredBase());
		if (baseId < _baseList->size())
		{
			if (_base != _baseList->at(baseId))
			{
				_allowStoresWarning = true;
				_txtFacility->setText(L"");
				_base = _baseList->at(baseId);
				init();
			}
		}
		else if (baseId == _baseList->size() && baseId < Base::MAX_BASES)
		{
			kL_geoMusicPlaying = false;
			kL_geoMusicReturnState = true;

			Base* const base (new Base(_game->getRuleset()));

			_game->popState();
			_game->pushState(new BuildNewBaseState(base, _globe));
		}
	}
}

/**
 * Pops to globe with selected Base centered.
 * @param action - pointer to an Action
 */
void BasescapeState::basesRightClick(Action*)
{
	if (_edtBase->isFocused() == false)
	{
		const size_t baseId (_miniBases->getHoveredBase());
		if (baseId < _baseList->size())
		{
			_game->getScreen()->fadeScreen();

			kL_geoMusicPlaying = false;
			kL_geoMusicReturnState =
			kL_reCenter = true;

			const Base* const base (_baseList->at(baseId));
			_game->getSavedGame()->setGlobeLongitude(base->getLongitude());
			_game->getSavedGame()->setGlobeLatitude(base->getLatitude());

			_game->popState();
			kL_soundPop->play(Mix_GroupAvailable(0));
		}
	}
}

/**
 * Selects a different Base to display.
 * @param action - pointer to an Action
 */
void BasescapeState::handleKeyPress(Action* action)
{
	if (_edtBase->isFocused() == false
		&& action->getDetails()->type == SDL_KEYDOWN)
	{
		const size_t baseId (getKeyedBaseId(action->getDetails()->key.keysym.sym));
		if (baseId != Base::MAX_BASES)
		{
			_allowStoresWarning = true;
			_txtFacility->setText(L"");
			_base = _baseList->at(baseId);
			init();
		}
	}
}

/**
 * Returns the baseId that player chooses or MAX_BASES if the current Base is
 * re-chosen.
 * @param keyId	- SDL key pressed
 * @return, baseId of the base to switch to (0 to MAX_BASES-1)
 */
size_t BasescapeState::getKeyedBaseId(SDLKey keyId) const
{
	static const SDLKey baseKeys[Base::MAX_BASES]	// note that 'static' means these keys will not be
	{												// changed (in Options) while the program is running.
		Options::keyBaseSelect1,
		Options::keyBaseSelect2,
		Options::keyBaseSelect3,
		Options::keyBaseSelect4,
		Options::keyBaseSelect5,
		Options::keyBaseSelect6,
		Options::keyBaseSelect7,
		Options::keyBaseSelect8
	};

	size_t baseId (0u);
	for (std::vector<Base*>::const_iterator
			i = _baseList->begin();
			i != _baseList->end();
			++i, ++baseId)
	{
		if (baseKeys[baseId] == keyId)
		{
			if (*i != _base) return baseId;
			break;
		}
	}
	return Base::MAX_BASES; // invalid.
}

/**
 * Changes the Base name.
 * @param action - pointer to an Action
 */
void BasescapeState::edtLabelChange(Action*)
{
	_base->setName(_edtBase->getText());
}

/**
 * Allows the storage-overfull warning to be displayed to the player.
 */
void BasescapeState::resetStoresWarning()
{
	_allowStoresWarning = true;
}

}
