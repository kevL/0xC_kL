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
		_move_x,
		_move_y;

	public:
		typedef helper::ShaderBase<Pixel> _base;
		friend struct helper::controler<ShaderMove<Pixel>>;

		/// cTor [0]
		inline ShaderMove(Surface* const srf)
			:
				_base(srf),
				_move_x(srf->getX()),
				_move_y(srf->getY())
		{}

		/// cTor [1]
		inline ShaderMove(
				Surface* const srf,
				int move_x,
				int move_y)
			:
				_base(srf),
				_move_x(move_x),
				_move_y(move_y)
		{}

		/// cTor [2]
		inline ShaderMove(const ShaderMove& f)
			:
				_base(f),
				_move_x(f._move_x),
				_move_y(f._move_y)
		{}

		/// cTor [3]
		inline ShaderMove(
				std::vector<Pixel>& f,
				int max_x,
				int max_y)
			:
				_base(
					f,
					max_x,
					max_y),
				_move_x(),
				_move_y()
		{}

		/// cTor [4]
		inline ShaderMove(
				std::vector<Pixel>& f,
				int max_x,
				int max_y,
				int move_x,
				int move_y)
			:
				_base(
					f,
					max_x,
					max_y),
				_move_x(move_x),
				_move_y(move_y)
		{}

		inline GraphSubset getArea() const // NOTE: Hides superclass ShaderBase::getImage(). kL_Fixed.
		{ return _base::_range_domain.offset(
										_move_x,
										_move_y); }

		inline void setMove(
				int x,
				int y)
		{	_move_x = x;
			_move_y = y; }

		inline void addMove(
				int x,
				int y)
		{	_move_x += x;
			_move_y += y; }
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

	controler(const ShaderMove<Pixel>& f)
		:
			base_type(
					f.ptr(),
					f.getDomain(),
					f.getArea(),
					std::make_pair(
								1,
								f.pitch()))
	{}
};

}


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
 * Creates warper from Surface and provided offset.
 * @param srf - standard 8-bit OpenXcom surface
 * @param x   - offset on x
 * @param y   - offset on y
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
 * Creates warper from cropped Surface and provided offset.
 * @param srf - standard 8-bit OpenXcom surface
 * @param x   - offset on x
 * @param y   - offset on y
 * @return,
 */
inline ShaderMove<Uint8> ShaderCrop(
		Surface* const srf,
		int x,
		int y)
{
	ShaderMove<Uint8> area (srf, x,y);

	SDL_Rect* const s_crop = srf->getCrop();
	if (s_crop->w && s_crop->h)
	{
		GraphSubset crop (std::make_pair(
									s_crop->x,
									s_crop->x + s_crop->w),
						  std::make_pair(
									s_crop->y,
									s_crop->y + s_crop->h));
		area.setDomain(crop);
		area.addMove(
				-s_crop->x,
				-s_crop->y);
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
