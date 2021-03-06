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

#ifndef OPENXCOM_ARTICLESTATEAWARD_H
#define OPENXCOM_ARTICLESTATEAWARD_H

#include "ArticleState.h"


namespace OpenXcom
{

class ArticleDefinitionAward;
class Surface;
class Text;
class TextList;


/**
 * ArticleStateAward has a text and background.
 */
class ArticleStateAward
	:
		public ArticleState
{

protected:
	Text
		* _txtInfo,
		* _txtTitle;


	public:
		/// cTor.
		explicit ArticleStateAward(const ArticleDefinitionAward* const defs);
		/// dTor.
		virtual ~ArticleStateAward();
};

}

#endif
