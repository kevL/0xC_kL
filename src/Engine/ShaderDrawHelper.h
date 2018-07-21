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

#ifndef OPENXCOM_SHADERDRAWHELPER_H
#define	OPENXCOM_SHADERDRAWHELPER_H

//#include <vector>

#include "GraphSubset.h"
#include "Surface.h"


namespace OpenXcom
{

namespace helper
{

const Uint8
	ColorGroup    = 15 << 4u,
	ColorShade    = 15;
//	ColorShadeMax = 15
//	BLACK         = 15


/**
 * This is empty argument to 'ShaderDraw'.
 * When used in 'ShaderDraw' return always 0 to 'ColorFunc::func' for every pixel.
 */
class Nothing
{};


/**
 * This is scalar argument to 'ShaderDraw'.
 * When used in 'ShaderDraw' return value of 't' to 'ColorFunc::func' for every pixel.
 */
template<typename T>
class Scalar
{
	public:
		T& ref;
		inline explicit Scalar(T& t)
			:
				ref(t)
		{}
};


/**
 * This is surface argument to 'ShaderDraw'.
 * Every pixel of this surface will have type 'Pixel'.
 * Modifying pixels of this surface will modify original data.
 */
template<typename Pixel>
class ShaderBase
{
	public:
		typedef Pixel* PixelPtr;
		typedef Pixel& PixelRef;


protected:
	const PixelPtr _origin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;


	public:
		/// cTor [0] - copy constructor
		inline ShaderBase(const ShaderBase& base)
			:
				_origin(base.ptr()),
				_range_base(base._range_base),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/**
		 * Creates surface using vector 'data' as data source.
		 * Surface will have 'max_y' x 'max_x' dimensions.
		 * Size of 'data' should be bigger than 'max_y * max_x'.
		 * ATTENTION: after use of this constructor if you change size of 'data'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param data  - vector that is treated as the surface
		 * @param max_x - x dimension of 'data'
		 * @param max_y - y dimension of 'data'
		 */
		/// cTor [1]
		inline ShaderBase(
				std::vector<Pixel>& data,
				int max_x,
				int max_y)
			:
				_origin(&data[0u]),
				_range_base(
						max_x,
						max_y),
				_range_domain(
						max_x,
						max_y),
				_pitch(max_x)
		{}

		inline PixelPtr ptr() const
		{
			return _origin;
		}
		inline int pitch() const
		{
			return _pitch;
		}
		inline void setDomain(const GraphSubset& graph)
		{
			_range_domain = GraphSubset::intersection(
													graph,
													_range_base);
		}
		inline const GraphSubset& getDomain() const
		{
			return _range_domain;
		}
		inline const GraphSubset& getBaseDomain() const
		{
			return _range_base;
		}
//		inline const GraphSubset& getImage() const	// kL_Note: Used to be hidden by ShaderMove::getImage().
//		{											// Use getDomain() instead THANKS.
//			return _range_domain;
//		}
};

/**
 * This is surface argument to 'ShaderDraw'.
 * Every pixel of this surface will have type 'Pixel'.
 * You can't modify pixel in that surface.
 */
template<typename Pixel>
class ShaderBase<const Pixel>
{
	public:
		typedef const Pixel* PixelPtr;
		typedef const Pixel& PixelRef;


protected:
	const PixelPtr _origin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;


	public:
		/// cTor [0] - copy constructor
		inline ShaderBase(const ShaderBase& base)
			:
				_origin(base.ptr()),
				_range_base(base.getBaseDomain()),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/// cTor [1] - copy constructor
		inline ShaderBase(const ShaderBase<Pixel>& base)
			:
				_origin(base.ptr()),
				_range_base(base.getBaseDomain()),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/**
		 * Creates surface using vector 'data' as data source.
		 * Surface will have 'max_y' x 'max_x' dimensions.
		 * Size of 'data' should be bigger than 'max_y * max_x'.
		 * ATTENTION: after use of this constructor if you change size of 'data'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param data  - vector that is treated as the surface
		 * @param max_x - x dimension of 'f'
		 * @param max_y - y dimension of 'f'
		 */
		/// cTor [2]
		inline ShaderBase(
				const std::vector<Pixel>& data,
				int max_x,
				int max_y)
			:
				_origin(&data[0u]),
				_range_base(
						max_x,
						max_y),
				_range_domain(
						max_x,
						max_y),
				_pitch(max_x)
		{}

		inline PixelPtr ptr() const
		{
			return _origin;
		}
		inline int pitch() const
		{
			return _pitch;
		}
		inline void setDomain(const GraphSubset& graph)
		{
			_range_domain = GraphSubset::intersection(
													graph,
													_range_base);
		}
		inline const GraphSubset& getDomain() const
		{
			return _range_domain;
		}
		inline const GraphSubset& getBaseDomain() const
		{
			return _range_base;
		}
//		inline const GraphSubset& getImage() const // Use getDomain() instead THANKS.
//		{
//			return _range_domain;
//		}
};


/**
 * This is surface argument to 'ShaderDraw'.
 * Every pixel of this surface will have type 'Uint8'.
 * Can be constructed from 'Surface*'.
 * Modifying pixels of this surface will modify original data.
 */
template<>
class ShaderBase<Uint8>
{
	public:
		typedef Uint8* PixelPtr;
		typedef Uint8& PixelRef;


protected:
	const PixelPtr _origin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;


	public:
		/// cTor [0] - copy constructor
		inline ShaderBase(const ShaderBase& base)
			:
				_origin(base.ptr()),
				_range_base(base.getBaseDomain()),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/**
		 * Creates surface using surface 'srf' as data source.
		 * Surface will have same dimensions as 'srf'.
		 * ATTENTION: after use of this constructor if you change size of 'srf'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param srf - surface that is treated as the surface
		 */
		/// cTor [1]
		inline ShaderBase(const Surface* const srf)
			:
				_origin(static_cast<Uint8*>(srf->getSurface()->pixels)),
				_range_base(
						srf->getWidth(),
						srf->getHeight()),
				_range_domain(
						srf->getWidth(),
						srf->getHeight()),
				_pitch(srf->getSurface()->pitch)
		{}

		/**
		 * Creates surface using vector 'data' as data source.
		 * Surface will have 'max_y' x 'max_x' dimensions.
		 * Size of 'data' should be bigger than 'max_y * max_x'.
		 * ATTENTION: after use of this constructor if you change size of 'data'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param data  - vector that is treated as the surface
		 * @param max_x - x dimension of 'data'
		 * @param max_y - y dimension of 'data'
		 */
		/// cTor [2]
		inline ShaderBase(
				std::vector<Uint8>& data,
				int max_x,
				int max_y)
			:
				_origin(&data[0u]),
				_range_base(
						max_x,
						max_y),
				_range_domain(
						max_x,
						max_y),
				_pitch(max_x)
		{}

		inline PixelPtr ptr() const
		{
			return _origin;
		}
		inline int pitch() const
		{
			return _pitch;
		}
		inline void setDomain(const GraphSubset& graph)
		{
			_range_domain = GraphSubset::intersection(
													graph,
													_range_base);
		}
		inline const GraphSubset& getDomain() const
		{
			return _range_domain;
		}
		inline const GraphSubset& getBaseDomain() const
		{
			return _range_base;
		}
//		inline const GraphSubset& getImage() const // Use getDomain() instead THANKS.
//		{
//			return _range_domain;
//		}
};


/**
 * This is surface argument to 'ShaderDraw'.
 * Every pixel of this surface will have type 'const Uint8'.
 * Can be constructed from 'const Surface*'.
 * You can't modify pixel in that surface.
 */
template<>
class ShaderBase<const Uint8>
{
	public:
		typedef const Uint8* PixelPtr;
		typedef const Uint8& PixelRef;


protected:
	const PixelPtr _origin;
	const GraphSubset _range_base;
	GraphSubset _range_domain;
	const int _pitch;


	public:
		/// cTor [0] - copy constructor
		inline ShaderBase(const ShaderBase& base)
			:
				_origin(base.ptr()),
				_range_base(base.getBaseDomain()),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/// cTor [1] - copy constructor
		inline ShaderBase(const ShaderBase<Uint8>& base)
			:
				_origin(base.ptr()),
				_range_base(base.getBaseDomain()),
				_range_domain(base.getDomain()),
				_pitch(base.pitch())
		{}

		/**
		 * Creates surface using surface 'srf' as data source.
		 * Surface will have same dimensions as 'srf'.
		 * ATTENTION: after use of this constructor if you change size of 'srf'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param srf - surface that is treated as the surface
		 */
		/// cTor [2]
		inline ShaderBase(const Surface* const srf)
			:
				_origin(static_cast<Uint8*>(srf->getSurface()->pixels)),
				_range_base(
						srf->getWidth(),
						srf->getHeight()),
				_range_domain(
						srf->getWidth(),
						srf->getHeight()),
				_pitch(srf->getSurface()->pitch)
		{}

		/**
		 * Creates surface using vector 'data' as data source.
		 * Surface will have 'max_y' x 'max_x' dimensions.
		 * Size of 'data' should be bigger than 'max_y * max_x'.
		 * ATTENTION: after use of this constructor if you change size of 'data'
		 * then '_origin' will be invalid and use of this object will cause a
		 * memory exception.
		 * @param data  - vector that is treated as the surface
		 * @param max_x - x dimension of 'f'
		 * @param max_y - y dimension of 'f'
		 */
		/// cTor [3]
		inline ShaderBase(
				const std::vector<Uint8>& data,
				int max_x,
				int max_y)
			:
				_origin(&data[0u]),
				_range_base(
						max_x,
						max_y),
				_range_domain(
						max_x,
						max_y),
				_pitch(max_x)
		{}

		inline PixelPtr ptr() const
		{
			return _origin;
		}
		inline int pitch() const
		{
			return _pitch;
		}
		inline void setDomain(const GraphSubset& graph)
		{
			_range_domain = GraphSubset::intersection(
													graph,
													_range_base);
		}
		inline const GraphSubset& getDomain() const
		{
			return _range_domain;
		}
		inline const GraphSubset& getBaseDomain() const
		{
			return _range_base;
		}
//		inline const GraphSubset& getImage() const // Use getDomain() instead THANKS.
//		{
//			return _range_domain;
//		}
};


/// A helper-class for handling implementation differences in different surfaces
/// types.
/// Used in function 'ShaderDraw'.
template<typename SurfaceType>
struct controler
{
	// NOT IMPLEMENTED ANYWHERE!
	// you need create your own specification or use different type, no default version

	/**
	 * Function used only when 'SurfaceType' can be used as destination surface.
	 * If that type should not be used as 'dest' don't implement this.
	 * @return, start drawing range
	 */
	inline const GraphSubset& get_range();
	/**
	 * Function used only when 'SurfaceType' is used as source surface.
	 * Function reduce drawing range.
	 * @param graph - modify drawing range
	 */
	inline void mod_range(GraphSubset& graph);
	/**
	 * Set final drawing range.
	 * @param graph - drawing range
	 */
	inline void set_range(const GraphSubset& graph);

	inline void mod_y(int& begin, int& end);
	inline void set_y(const int& begin, const int& end);
	inline void inc_y();

	inline void mod_x(int& begin, int& end);
	inline void set_x(const int& begin, const int& end);
	inline void inc_x();

	inline int& get_ref();
};


/// implementation for scalar types aka 'int', 'double', 'float'
template<typename T>
struct controler<Scalar<T>>
{
	T& ref;

	inline explicit controler(const Scalar<T>& scalar)
		:
			ref(scalar.ref)
	{}

	// can't use this function
//	inline GraphSubset get_range()

	inline void mod_range(GraphSubset&)
	{}
	inline void set_range(const GraphSubset&)
	{}

	inline void mod_y(int&, int&)
	{}
	inline void set_y(const int&, const int&)
	{}
	inline void inc_y()
	{}

	inline void mod_x(int&, int&)
	{}
	inline void set_x(const int&, const int&)
	{}
	inline void inc_x()
	{}

	inline T& get_ref()
	{
		return ref;
	}
};

// kL_note: Is this just a bunch of code for the sake of having code.

/// Implementation for not used arg.
template<>
struct controler<Nothing>
{
	const int i;
	inline controler(const Nothing&)
		:
			i(0)
	{}

	// can't use this function
//	inline GraphSubset get_range()

	inline void mod_range(GraphSubset&)
	{}
	inline void set_range(const GraphSubset&)
	{}

	inline void mod_y(int&, int&)
	{}
	inline void set_y(const int&, const int&)
	{}
	inline void inc_y()
	{}

	inline void mod_x(int&, int&)
	{}
	inline void set_x(const int&, const int&)
	{}
	inline void inc_x()
	{}

	inline const int& get_ref()
	{
		return i;
	}
};


template<typename PixelPtr, typename PixelRef>
struct controler_base
{
	const PixelPtr data;
	PixelPtr
		ptr_pos_y,
		ptr_pos_x;
	GraphSubset range;
	int
		start_x,
		start_y;

	const std::pair<int, int> step;

	controler_base(
			PixelPtr base,
			const GraphSubset& d,
			const GraphSubset& r,
			const std::pair<int, int>& s) // pair of WHAT. Thanks
		:
			data(base + d.beg_x * s.first + d.beg_y * s.second),
			ptr_pos_y(nullptr),
			ptr_pos_x(nullptr),
			range(r),
			start_x(),
			start_y(),
			step(s)
	{}


	inline const GraphSubset& get_range()
	{
		return range;
	}

	inline void mod_range(GraphSubset& r)
	{
		r = GraphSubset::intersection(range, r);
	}

	inline void set_range(const GraphSubset& r)
	{
		start_x = r.beg_x - range.beg_x;
		start_y = r.beg_y - range.beg_y;
		range = r;
	}

	inline void mod_y(
			int&,
			int&)
	{
		ptr_pos_y = data + step.first * start_x + step.second * start_y;
	}
	inline void set_y(
			const int& begin,
			const int&)
	{
		ptr_pos_y += step.second * begin;
	}
	inline void inc_y()
	{
		ptr_pos_y += step.second;
	}

	inline void mod_x(
			int&,
			int&)
	{
		ptr_pos_x = ptr_pos_y;
	}
	inline void set_x(
			const int& begin,
			const int&)
	{
		ptr_pos_x += step.first * begin;
	}
	inline void inc_x()
	{
		ptr_pos_x += step.first;
	}

	inline PixelRef get_ref()
	{
		return *ptr_pos_x;
	}
};


template<typename Pixel>
struct controler<ShaderBase<Pixel>>
	:
		public controler_base<typename ShaderBase<Pixel>::PixelPtr, typename ShaderBase<Pixel>::PixelRef>
{
	typedef typename ShaderBase<Pixel>::PixelPtr PixelPtr;
	typedef typename ShaderBase<Pixel>::PixelRef PixelRef;

	typedef controler_base<PixelPtr, PixelRef> base_type;

	controler(const ShaderBase<Pixel>& base)
		:
			base_type(
					base.ptr(),
					base.getDomain(),
					base.getDomain(), //f.getImage() aka. f.getArea()
					std::make_pair(
								1,
								base.pitch()))
	{}
};

}

}

#endif
