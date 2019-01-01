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

#ifndef OPENXCOM_ARTICLESTATEITEM_H
#define OPENXCOM_ARTICLESTATEITEM_H

#include "ArticleState.h"


namespace OpenXcom
{

/// For the info to display on the table of a firearm.
enum FirearmInfo
{
	MODE_SHOT,	// 0
	MODE_MELEE	// 1
};


class ArticleDefinitionItem;
class InteractiveSurface;
class Surface;
class Text;
class TextList;


/**
 * ArticleStateItem has a caption, text, preview-image and a stats-block.
 */
class ArticleStateItem
	:
		public ArticleState
{

private:
	/// Retains whether to display shot-info or melee-info for firearms.
	static FirearmInfo _infoMode;

	/// Switches the info-table for Firearms between displaying shot-or-melee data.
	void toggleTable(Action* action);


	protected:
		InteractiveSurface* _isfMode;
		Surface
			*_image,
			*_imageAmmo[3u];
		Text
			* _txtTitle,
			* _txtTwoHand,
			* _txtInfo,
			* _txtShotType,
			* _txtAccuracy,
			* _txtTuCost,
//			* _txtDamage,
//			* _txtAmmo,
			* _txtAmmoType[3u],
			* _txtAmmoDamage[3u];
		TextList
			* _lstInfo,
			* _lstInfoMelee;


		public:
			/// cTor.
			explicit ArticleStateItem(const ArticleDefinitionItem* const defs);
			/// dTor.
			virtual ~ArticleStateItem();

			/// Resets the FirearmInfo enumerator.
			static void resetFirearmInfo();
};

}

#endif
