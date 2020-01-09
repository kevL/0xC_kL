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

#ifndef OPENXCOM_SHADERMOVE_H
#define	OPENXCOM_SHADERMOVE_H

#include "ShaderDraw.h"


namespace OpenXcom
{

template<typename Pixel>
class ShaderMove
	:
		public helper::ShaderBase<Pixel>
{

private:
	int
		_x,
		_y;

	public:
		typedef helper::ShaderBase<Pixel> _base;
		friend struct helper::controler<ShaderMove<Pixel>>;

		/// cTor [0]
		inline ShaderMove(Surface* const srf)
			:
				_base(srf),
				_x(srf->getX()),
				_y(srf->getY())
		{}

		/// cTor [1]
		inline ShaderMove(
				Surface* const srf,
				int x,
				int y)
			:
				_base(srf),
				_x(x),
				_y(y)
		{}

		/// cTor [2] - copy constructor
		inline ShaderMove(const ShaderMove& notashader)
			:
				_base(notashader),
				_x(notashader._x),
				_y(notashader._y)
		{}

		/// cTor [3]
		inline ShaderMove(
				std::vector<Pixel>& data,
				int x_max,
				int y_max)
			:
				_base(
					data,
					x_max,
					y_max),
				_x(0), // _x()
				_y(0)  // _y()
		{}

		/// cTor [4]
		inline ShaderMove(
				std::vector<Pixel>& data,
				int x_max,
				int y_max,
				int x,
				int y)
			:
				_base(
					data,
					x_max,
					y_max),
				_x(x),
				_y(y)
		{}


		///
		inline GraphSubset getOffsetRange() const
		{
			return _base::_range.offset(_x,_y);
		}

		///
		inline void setMove(
				int x,
				int y)
		{
			_x = x;
			_y = y;
		}

		///
		inline void addMove(
				int x,
				int y)
		{
			_x += x;
			_y += y;
		}
};


namespace helper
{

template<typename Pixel>
struct controler<ShaderMove<Pixel>>
	:
		public
			controler_base<typename ShaderMove<Pixel>::PixelPtr,
						   typename ShaderMove<Pixel>::PixelRef>
{
	typedef typename ShaderMove<Pixel>::PixelPtr PixelPtr;
	typedef typename ShaderMove<Pixel>::PixelRef PixelRef;

	typedef controler_base<PixelPtr, PixelRef> base_type;

	/// cTor.
	controler(const ShaderMove<Pixel>& notashader)
		:
			base_type(
					notashader.ptr(),
					notashader.getRange(),
					notashader.getOffsetRange(),
					std::make_pair(
								1,
								notashader.pitch()))
	{}
};

}


// WARPERS (sic) ... and to think we've been calling them "wrappers"

/**
 * Creates warper from Surface.
 * @param srf - standard 8-bit OpenXcom surface
 * @return,
 */
inline ShaderMove<Uint8> ShaderSurface(Surface* const srf)
{
	return ShaderMove<Uint8>(srf);
}

/**
 * Creates warper from Surface and given offset.
 * @param srf - standard 8-bit OpenXcom surface
 * @param x   - x-offset
 * @param y   - y-offset
 * @return,
 */
inline ShaderMove<Uint8> ShaderSurface(
		Surface* const srf,
		int x,
		int y)
{
	return ShaderMove<Uint8>(srf, x,y);
}

/**
 * Creates warper from cropped Surface and given offset.
 * @param srf - standard 8-bit OpenXcom surface
 * @param x   - x-offset
 * @param y   - y-offset
 * @return,
 */
inline ShaderMove<Uint8> ShaderCrop(
		Surface* const srf,
		int x,
		int y)
{
	ShaderMove<Uint8> notashader (srf, x,y);

	SDL_Rect* const rect (srf->getCrop());
	if (rect->w != 0u && rect->h != 0u)
	{
		GraphSubset area (std::make_pair(
									rect->x,
									rect->x + rect->w),
						  std::make_pair(
									rect->y,
									rect->y + rect->h));
		notashader.setRange(area);
		notashader.addMove(
						-rect->x,
						-rect->y);
	}

	return notashader;
}

/**
 * Creates warper from cropped Surface.
 * @param srf - standard 8-bit OpenXcom surface
 * @return,
 */
inline ShaderMove<Uint8> ShaderCrop(Surface* const srf)
{
	return ShaderCrop(
					srf,
					srf->getX(),
					srf->getY());
}

}

#endif
