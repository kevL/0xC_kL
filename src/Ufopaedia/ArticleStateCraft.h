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

#ifndef OPENXCOM_ARTICLESTATECRAFT_H
#define OPENXCOM_ARTICLESTATECRAFT_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionCraft;
class Text;


/**
 * ArticleStateCraft has a caption, text, background image and a stats block.
 * @note The layout of the description text and stats block can vary depending
 * on the background craft image.
 */
class ArticleStateCraft
	:
		public ArticleState
{

protected:
	Text
		* _txtInfo,
		* _txtStats,
		* _txtTitle;


	public:
		/// cTor.
		explicit ArticleStateCraft(const ArticleDefinitionCraft* const defs);
		/// dTor.
		virtual ~ArticleStateCraft();
};

}

#endif
