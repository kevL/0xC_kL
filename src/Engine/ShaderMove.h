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
		_x_move,
		_y_move;

	public:
		typedef helper::ShaderBase<Pixel> _base;
		friend struct helper::controler<ShaderMove<Pixel>>;

		/// cTor [0]
		inline ShaderMove(Surface* const srf)
			:
				_base(srf),
				_x_move(srf->getX()),
				_y_move(srf->getY())
		{}

		/// cTor [1]
		inline ShaderMove(
				Surface* const srf,
				int x_move,
				int y_move)
			:
				_base(srf),
				_x_move(x_move),
				_y_move(y_move)
		{}

		/// cTor [2] - copy constructor
		inline ShaderMove(const ShaderMove& move)
			:
				_base(move),
				_x_move(move._x_move),
				_y_move(move._y_move)
		{}

		/// cTor [3]
		inline ShaderMove(
				std::vector<Pixel>& data,
				int max_x,
				int max_y)
			:
				_base(
					data,
					max_x,
					max_y),
				_x_move(),
				_y_move()
		{}

		/// cTor [4]
		inline ShaderMove(
				std::vector<Pixel>& data,
				int max_x,
				int max_y,
				int move_x,
				int move_y)
			:
				_base(
					data,
					max_x,
					max_y),
				_x_move(move_x),
				_y_move(move_y)
		{}

		inline GraphSubset getArea() const // NOTE: Hides superclass ShaderBase::getImage(). kL_Fixed.
		{
			return _base::_range_domain.offset(
											_x_move,
											_y_move);
		}

		inline void setMove(
				int x,
				int y)
		{
			_x_move = x;
			_y_move = y;
		}

		inline void addMove(
				int x,
				int y)
		{
			_x_move += x;
			_y_move += y;
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
	controler(const ShaderMove<Pixel>& move)
		:
			base_type(
					move.ptr(),
					move.getDomain(),
					move.getArea(),
					std::make_pair(
								1,
								move.pitch()))
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
	ShaderMove<Uint8> area (srf, x,y);

	SDL_Rect* const rect (srf->getCrop());
	if (rect->w != 0u && rect->h != 0u)
	{
		GraphSubset crop (std::make_pair(
									rect->x,
									rect->x + rect->w),
						  std::make_pair(
									rect->y,
									rect->y + rect->h));
		area.setDomain(crop);
		area.addMove(
				-rect->x,
				-rect->y);
	}

	return area;
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
