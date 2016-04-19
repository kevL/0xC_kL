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

#ifndef _USE_MATH_DEFINES
#	define _USE_MATH_DEFINES
#endif

#include "Globe.h"

//#include <algorithm>
//#include <cmath>

//#include "../fmath.h"

#include "GeoscapeState.h" // unitToRads

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
//#include "../Engine/Screen.h"
#include "../Engine/ShaderMove.h"
#include "../Engine/ShaderRepeat.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Cursor.h"
#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Polygon.h"
#include "../Ruleset/Polyline.h"
#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCity.h"
#include "../Ruleset/RuleCountry.h"
//#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleGlobe.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/AlienBase.h"
#include "../Savegame/AlienMission.h"
#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Country.h"
#include "../Savegame/Craft.h"
#include "../Savegame/GameTime.h"
#include "../Savegame/MissionSite.h"
#include "../Savegame/Region.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Target.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

bool kL_reCenter = false;

const double
	Globe::ROTATE_LONGITUDE	= 0.176,
	Globe::ROTATE_LATITUDE	= 0.176;

Uint8 // these are only fallbacks for Geography.rul->globe
	Globe::C_LBLBASE	= 100,	// Palette::blockOffset(6)+4;	// stock 133;
	Globe::C_LBLCITY	= 167,	// Palette::blockOffset(10)+7;	// stock 138
	Globe::C_LBLCOUNTRY	= 227,	// Palette::blockOffset(14)+3;	// stock 239
	Globe::C_LINE		= 162,	// Palette::blockOffset(10)+2;	// light gray
//	Globe::C_RADAR1		=		// let base radars do its own thing in XuLine()
	Globe::C_RADAR2		= 150,	// Palette::blockOffset(9)+6;	// brown
	Globe::C_FLIGHT		= 166,	// Palette::blockOffset(10)+6;	// steel gray
	Globe::C_OCEAN		= 192,	// Palette::blockOffset(12),	// blue ofc.
	Globe::C_BLACK		=  15;


namespace
{

/// A helper class/struct for drawing shadows & noise on the Globe.
struct GlobeStaticData
{
	/// array of shading gradient
	Sint16 shade_gradient[240];
	/// size of x & y of noise surface
	const int random_surf_size;

	/**
	 * Function returning normal vector of sphere surface.
	 * @param ox	- x cord of sphere center
	 * @param oy	- y cord of sphere center
	 * @param r		- radius of sphere
	 * @param x		- cord of point where we getting this vector
	 * @param y		- cord of point where we getting this vector
	 * @return, normal vector of sphere surface
	 */
	inline Cord circle_norm(
			double ox,
			double oy,
			double r,
			double x,
			double y)
	{
		const double
			limit (r * r),
			norm (1. / r);

		Cord ret;
		ret.x = (x - ox);
		ret.y = (y - oy);

		const double temp (ret.x * ret.x + ret.y * ret.y);
		if (limit > temp)
		{
			ret.x *= norm;
			ret.y *= norm;
			ret.z = std::sqrt(limit - temp) * norm;
			return ret;
		}
		else
		{
			ret.x =
			ret.y =
			ret.z = 0.;
			return ret;
		}
	}

	/// initialization
	GlobeStaticData()
		:
			random_surf_size(60)
	{
		// filling terminator gradient LUT
		for (int
				i = 0;
				i != 240;
				++i)
		{
			int j (i - 120);

			if		(j < -66) j = -16;
			else if (j < -48) j = -15;
			else if (j < -33) j = -14;
			else if (j < -22) j = -13;
			else if (j < -15) j = -12;
			else if (j < -11) j = -11;
			else if (j <  -9) j = -10;

			if		(j > 120) j =  19;
			else if (j >  98) j =  18;
			else if (j >  86) j =  17;
			else if (j >  74) j =  16;
			else if (j >  54) j =  15;
			else if (j >  38) j =  14;
			else if (j >  26) j =  13;
			else if (j >  18) j =  12;
			else if (j >  13) j =  11;
			else if (j >  10) j =  10;
			else if (j >   8) j =   9;

			shade_gradient[static_cast<size_t>(i)] = static_cast<Sint16>(j) + 16;
		}
	}
};

GlobeStaticData static_data;


struct Ocean
{
	///
	static inline void func(
			Uint8& dest,
			const int&, // whots this
			const int&, // whots this
			const int&, // whots this
			const int&) // whots this
	{
		dest = Globe::C_OCEAN;
	}
};


struct CreateShadow
{
	///
	static inline Uint8 getShadowValue(
			const Uint8& dest,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise)
	{
		Cord temp (earth);
		temp -= sun;		// diff
		temp.x *= temp.x;	// norm
		temp.y *= temp.y;
		temp.z *= temp.z;
		temp.x += temp.z + temp.y; // we have norm of distance between 2 vectors, now stored in 'x'

		temp.x -= 2.;
		temp.x *= 125.;

		if (temp.x < -110.)
			temp.x = -31.;
		else if (temp.x > 120.)
			temp.x = 50.;
		else
			temp.x = static_cast<double>(static_data.shade_gradient[static_cast<Sint16>(temp.x) + 120]);

		temp.x -= static_cast<double>(noise);

		if (temp.x > 0.)
		{
			const Uint8 d (dest & helper::ColorGroup);
			Uint8 val;

			if (temp.x > 31.)
				val = 31;
			else
				val = static_cast<Uint8>(temp.x);

			if (d == Globe::C_OCEAN
				|| d == Globe::C_OCEAN + 16)
			{
				return Globe::C_OCEAN + val; // this pixel is ocean
			}
			else
			{
				if (dest == 0)
					return val; // this pixel is land

				const Uint8 e (dest + (val / 3));
				if (e > (d + helper::ColorShade))
					return d + helper::ColorShade;

				return static_cast<Uint8>(e);
			}
		}
		else
		{
			const Uint8 d (dest & helper::ColorGroup);
			if (d == Globe::C_OCEAN
				|| d == Globe::C_OCEAN + 16)
			{
				return Globe::C_OCEAN; // this pixel is ocean
			}

			return dest; // this pixel is land
		}
	}

	///
	static inline void func(
			Uint8& dest,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise,
			const int&) // whots this
	{
		if (dest != 0
			&& AreSame(earth.z, 0.) == false)
		{
			dest = getShadowValue(
								dest,
								earth,
								sun,
								noise);
		}
		else
			dest = 0;
	}
};

}


/**
 * Sets up the Globe with a specified size and position.
 * @param game		- pointer to the core Game
 * @param cenX		- x-position of the center of the globe
 * @param cenY		- y-position of the center of the globe
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- x-position in pixels (default 0)
 * @param y			- y-position in pixels (default 0)
 */
Globe::Globe(
		Game* const game,
		int cenX,
		int cenY,
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_rotLon(0.),
		_rotLat(0.),
		_hoverLon(0.),
		_hoverLat(0.),
		_cenX(static_cast<Sint16>(cenX)),
		_cenY(static_cast<Sint16>(cenY)),
		_game(game),
		_rules(game->getRuleset()->getGlobe()),
		_hover(false),
		_isMouseScrolled(false),
		_isMouseScrolling(false),
		_mouseOverThreshold(false),
		_mouseScrollStartTime(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_lonPreMouseScroll(0.),
		_latPreMouseScroll(0.),
		_radius(0.),
		_radiusStep(0.),
		_debugType(DTG_COUNTRY),
		_radarDetail(2),
		_blink(true),
		_blinkVal(-1),
		_drawCrosshair(false),
		_crosshairLat(0.),
		_crosshairLon(0.)
{
	_texture	= new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("TEXTURE.DAT"));
	_markerSet	= new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("GlobeMarkers"));

	_countries	= new Surface(width, height, x,y);
	_crosshair	= new Surface(width, height, x,y);
	_markers	= new Surface(width, height, x,y);
	_radars		= new Surface(width, height, x,y);

	_clipper	= new FastLineClip(
								x, x + width,
								y, y + height);

	_blinkTimer = new Timer(200u);
	_blinkTimer->onTimer((SurfaceHandler)& Globe::blink);
	_blinkTimer->start();

	_rotTimer = new Timer(Options::geoScrollSpeed);
	_rotTimer->onTimer((SurfaceHandler)& Globe::rotate);

	_cenLon = _game->getSavedGame()->getGlobeLongitude();
	_cenLat = _game->getSavedGame()->getGlobeLatitude();

	_zoom = _game->getSavedGame()->getGlobeZoom();
	setupRadii(width, height);
	setZoom(_zoom);

	_terminatorFluxions.resize(static_data.random_surf_size * static_data.random_surf_size);
	for (size_t
			i = 0;
			i != _terminatorFluxions.size();
			++i)
	{
		_terminatorFluxions[i] = static_cast<Sint16>(RNG::seedless(0,4));
	}

	cachePolygons();
}

/**
 * Deletes the contained surfaces, timers, polygons, etc.
 */
Globe::~Globe()
{
	delete _texture;
	delete _markerSet;
	delete _blinkTimer;
	delete _rotTimer;
	delete _countries;
	delete _crosshair;
	delete _markers;
	delete _radars;
	delete _clipper;

	for (std::list<Polygon*>::const_iterator
			i = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
		delete *i;
}

/**
 * Converts a polar-point into a cartesian-point for mapping a Polygon onto the
 * 3D-looking Globe.
 * @param lon	- longitude of the polar point
 * @param lat	- latitude of the polar point
 * @param x		- pointer to the output x-position
 * @param y		- pointer to the output y-position
 */
void Globe::polarToCart( // Orthographic projection
		double lon,
		double lat,
		Sint16* x,
		Sint16* y) const
{
	*x = _cenX + static_cast<Sint16>(std::floor(_radius * std::cos(lat) * std::sin(lon - _cenLon)));
	*y = _cenY + static_cast<Sint16>(std::floor(_radius * (std::cos(_cenLat) * std::sin(lat)
											- std::sin(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon))));
}

/**
 *
 */
void Globe::polarToCart( // Orthographic projection
		double lon,
		double lat,
		double* x,
		double* y) const
{
	*x = _cenX + static_cast<Sint16>(_radius * std::cos(lat) * std::sin(lon - _cenLon));
	*y = _cenY + static_cast<Sint16>(_radius * (std::cos(_cenLat) * std::sin(lat)
									- std::sin(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon)));
}

/**
 * Converts a cartesian-point into a polar-point for mapping a globe-click onto
 * the flat world.
 * @param x		- x-position of the cartesian point
 * @param y		- y-position of the cartesian point
 * @param lon	- pointer to the output longitude
 * @param lat	- pointer to the output latitude
 */
void Globe::cartToPolar( // Orthographic Projection
		Sint16 x,
		Sint16 y,
		double* lon,
		double* lat) const
{
	x -= _cenX;
	y -= _cenY;

	const double
		rho (std::sqrt(static_cast<double>(x * x + y * y))),
		c (std::asin(rho / static_cast<double>(_radius)));

	if (AreSame(rho, 0.))
	{
		*lat = _cenLat;
		*lon = _cenLon;
	}
	else
	{
		*lat = std::asin((y * std::sin(c) * std::cos(_cenLat)) / rho + std::cos(c) * std::sin(_cenLat));
		*lon = std::atan2(
						x * std::sin(c),
						(rho * std::cos(_cenLat) * std::cos(c) - y * std::sin(_cenLat) * std::sin(c)))
					+ _cenLon;
	}

	while (*lon < 0.) // keep between 0 and 2xPI
		*lon += M_PI * 2.;
	while (*lon > M_PI * 2.)
		*lon -= M_PI * 2.;
}

/**
 * Checks if a polar-point is on the back-half of this Globe hence invisible to
 * the player.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 * @return, true if it's on the back, false if it's on the front
 */
bool Globe::pointBack( // private.
		double lon,
		double lat) const
{
	return (std::cos(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon)
		  + std::sin(_cenLat) * std::sin(lat) < 0.);
}

/**
 * Returns latitude of last visible-to-player point on given longitude.
 * @param lon - longitude of the point
 * @return, longitude of last visible point
 *
double Globe::lastVisibleLat(double lon) const
{
//	double c = cos(_cenLat) * cos(lat) * cos(lon - _cenLon) + sin(_cenLat) * sin(lat);
//	tan(lat) = -cos(_cenLat) * cos(lon - _cenLon)/sin(_cenLat);
	return std::atan(-std::cos(_cenLat) * std::cos(lon - _cenLon) / std::sin(_cenLat));
} */

/**
 *
 * @param lon	- longitude of a point
 * @param lat	- latitude of a point
 */
Polygon* Globe::getPolygonAtCoord( // private.
		double lon,
		double lat) const
{
	const double discard (0.75f);
    double
		cosLat (cos(lat)),
		sinLat (sin(lat));

	for (std::list<Polygon*>::const_iterator
			i = _rules->getPolygons()->begin();
			i != _rules->getPolygons()->end();
			++i)
	{
		double
			x,y,
			x2,y2,
			cLat,cLon;

		bool pass (false);
		for (size_t
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			if (cosLat * cos((*i)->getLatitude(j)) * cos((*i)->getLongitude(j) - lon) + sinLat * sin((*i)->getLatitude(j)) < discard)
			{
				pass = true; // discarded
				break;
			}
		}
		if (pass == true) continue;


		bool odd (false);

		cLat = (*i)->getLatitude(0); // initial point
		cLon = (*i)->getLongitude(0);

		x = cos(cLat) * sin(cLon - lon);
		y = cosLat * sin(cLat) - sinLat * cos(cLat) * cos(cLon - lon);

		for (size_t
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			const size_t theSpot ((j + 1) % (*i)->getPoints()); // index of next point in poly
			cLat = (*i)->getLatitude(theSpot);
			cLon = (*i)->getLongitude(theSpot);

			x2 = cos(cLat) * sin(cLon - lon);
			y2 = cosLat * sin(cLat) - sinLat * cos(cLat) * cos(cLon - lon);

			if (((y > 0) != (y2 > 0)) && (0 < (x2 - x) * (0 - y) / (y2 - y) + x))
				odd = !odd;

			x = x2;
			y = y2;
		}

		if (odd == true)
			return *i;
	}

	return nullptr;
}

/**
 * Checks if a polar-point is inside a certain Polygon.
 * @param lon	- longitude of the point
 * @param lat	- latitude of the point
 * @param poly	- pointer to the polygon
 * @return, true if inside
 *
bool Globe::insidePolygon( // private. obsolete, see getPolygonAtCoord()
		double lon,
		double lat,
		const Polygon* const poly) const
{
	bool backFace (true);
	for (size_t i = 0; i != poly->getPoints(); ++i)
	{
		backFace &= pointBack(poly->getLongitude(i), poly->getLatitude(i)) == true;
	}

	if (backFace != pointBack(lon,lat)) return false;

	bool retOdd (false);
	for (size_t i = 0; i != poly->getPoints(); ++i)
	{
		const size_t j ((i + 1) % poly->getPoints());
//		double x = lon, y = lat, x_i = poly->getLongitude(i), y_i = poly->getLatitude(i), x_j = poly->getLongitude(j), y_j = poly->getLatitude(j);
		double
			x,y,
			x_i,x_j,
			y_i,y_j;
		polarToCart(
				poly->getLongitude(i),
				poly->getLatitude(i),
				&x_i,&y_i);
		polarToCart(
				poly->getLongitude(j),
				poly->getLatitude(j),
				&x_j,&y_j);
		polarToCart(
				lon,lat,
				&x,&y);
		if (((		   y_i <  y
					&& y_j >= y)
				|| (   y_j <  y
					&& y_i >= y))
			&& (	   x_i <= x
				||	   x_j <= x))
		{
			retOdd ^= (x_i + (y - y_i) / (y_j - y_i) * (x_j - x_i) < x); // holy space-time continuum batman.
		}
	}
	return retOdd;
} */

/**
 * Sets a leftwards rotation speed and starts the timer.
 */
void Globe::rotateLeft()
{
	_rotLon = -ROTATE_LONGITUDE;
	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a rightwards rotation speed and starts the timer.
 */
void Globe::rotateRight()
{
	_rotLon = ROTATE_LONGITUDE;
	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a upwards rotation speed and starts the timer.
 */
void Globe::rotateUp()
{
	_rotLat = -ROTATE_LATITUDE;
	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Sets a downwards rotation speed and starts the timer.
 */
void Globe::rotateDown()
{
	_rotLat = ROTATE_LATITUDE;
	if (_rotTimer->isRunning() == false)
		_rotTimer->start();
}

/**
 * Resets the rotation speed and timer.
 */
void Globe::rotateStop()
{
	_rotLon =
	_rotLat = 0.;
	_rotTimer->stop();
}

/**
 * Resets longitude rotation speed and timer.
 */
void Globe::rotateStopLon()
{
	_rotLon = 0.;
	if (AreSame(_rotLat, 0.))
		_rotTimer->stop();
}

/**
 * Resets latitude rotation speed and timer.
 */
void Globe::rotateStopLat()
{
	_rotLat = 0.;
	if (AreSame(_rotLon, 0.))
		_rotTimer->stop();
}

/**
 * Sets up the radius of earth at various zoom-levels.
 * @param width		- the new width of the Globe
 * @param height	- the new height of the Globe
 */
void Globe::setupRadii( // private.
		int width,
		int height)
{
	_zoomRadii.clear();
	const double height_d (static_cast<double>(height));

	// These are the globe-zoom magnifications stored as a <vector> of 6 (doubles).
	_zoomRadii.push_back(0.47 * height_d); // 0 - Zoomed all out	// no detail
	_zoomRadii.push_back(0.60 * height_d); // 1						// country borders
	_zoomRadii.push_back(0.85 * height_d); // 2						// country labels
	_zoomRadii.push_back(1.39 * height_d); // 3						// city markers
	_zoomRadii.push_back(2.13 * height_d); // 4						// city labels & all detail
	_zoomRadii.push_back(3.42 * height_d); // 5 - Zoomed all in

	_radius = _zoomRadii[_zoom];
	_radiusStep = (_zoomRadii[_zoomRadii.size() - 1] - _zoomRadii[0]) / 12.2;

	_earthData.resize(_zoomRadii.size());	// data for drawing sun-shadow.
	for (size_t								// filling normal field for each radius
			radiusId = 0;
			radiusId != _zoomRadii.size();
			++radiusId)
	{
		_earthData[radiusId].resize(width * height);
		for (size_t
				j = 0;
				j != static_cast<size_t>(height);
				++j)
			for (size_t
					i = 0;
					i != static_cast<size_t>(width);
					++i)
				_earthData[radiusId]
						  [static_cast<size_t>(width) * j + i] = static_data.circle_norm(
																					static_cast<double>(width) / 2.,
																					height_d / 2.,
																					_zoomRadii[radiusId],
																					static_cast<double>(i) + 0.5,
																					static_cast<double>(j) + 0.5);
	}
}

/**
 * Changes the current globe-zoom factor.
 * @param zoom - the new zoom level
 */
void Globe::setZoom(size_t zoom) // private.
{
	_zoom = std::min(_zoomRadii.size() - 1,
					 std::max(0u, //static_cast<size_t>(0), // go f*cking figure.
							  zoom));

	_zoomTexture = (2 - static_cast<size_t>(std::floor(static_cast<double>(_zoom) / 2.)))
				 * (_texture->getTotalFrames() / 3);

	_radius = _zoomRadii[_zoom];
	_game->getSavedGame()->setGlobeZoom(_zoom);

	if (_isMouseScrolling == true)
	{
		_lonPreMouseScroll = _cenLon;
		_latPreMouseScroll = _cenLat;
		_totalMouseMoveX =
		_totalMouseMoveY = 0;
	}

	invalidate();
}

/**
 * Gets the Globe's current zoom-level.
 * @return, zoom-level
 */
size_t Globe::getZoom() const
{
	return _zoom;
}

/**
 * Gets the number of zoom-levels available.
 * @return, number of zoom-levels
 */
size_t Globe::getZoomLevels() const
{
	return _zoomRadii.size();
}

/**
 * Increases the zoom-level on this Globe.
 */
void Globe::zoomIn()
{
	if (_zoom < _zoomRadii.size() - 1)
		setZoom(_zoom + 1);
}

/**
 * Decreases the zoom-level on this Globe.
 */
void Globe::zoomOut()
{
	if (_zoom > 0)
		setZoom(_zoom - 1);
}

/**
 * Zooms this Globe out as far as possible.
 *
void Globe::zoomMin()
{
	if (_zoom > 0) setZoom(0);
} */

/**
 * Zooms this Globe in as close as possible.
 *
void Globe::zoomMax()
{
	if (_zoom < _zoomRadii.size() - 1) setZoom(_zoomRadii.size() - 1);
} */

/**
 * Zooms this Globe smoothly into a Dogfight.
 * @return, true if zoom has finished
 */
bool Globe::zoomDogfightIn()
{
	const size_t dfZoom (_zoomRadii.size() - 1);

	if (_zoom < dfZoom)
	{
		const double radius (_radius);

		if (radius + _radiusStep >= _zoomRadii[dfZoom])
			setZoom(dfZoom);
		else
		{
			if (radius + _radiusStep >= _zoomRadii[_zoom + 1])
				++_zoom;

			setZoom(_zoom);
			_radius = radius + _radiusStep;
		}
		return false;
	}
	return true;
}

/**
 * Zooms this Globe smoothly out of a Dogfight.
 * @return, true if the zoom has finished
 */
bool Globe::zoomDogfightOut()
{
	const size_t preDfZoom (_game->getSavedGame()->getDfZoom());

	if (_zoom > preDfZoom)
	{
		const double radius (_radius);

		if (radius - _radiusStep <= _zoomRadii[preDfZoom])
			setZoom(preDfZoom);
		else
		{
			if (radius - _radiusStep <= _zoomRadii[_zoom - 1])
				--_zoom;

			setZoom(_zoom);
			_radius = radius - _radiusStep;
		}
		return false;
	}
	return true;
}

/**
 * Rotates this Globe to center on a certain polar-point.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 */
void Globe::center(
		double lon,
		double lat)
{
	_game->getSavedGame()->setGlobeLongitude(_cenLon = lon);
	_game->getSavedGame()->setGlobeLatitude(_cenLat = lat);

	invalidate();
}

/**
 * Checks if a polar-point is inside land.
 * @param lon - longitude of the point
 * @param lat - latitude of the point
 * @return, true if point is over land
 */
bool Globe::insideLand(
		double lon,
		double lat) const
{
	return (getPolygonAtCoord(lon,lat) != nullptr);
}

/**
 * Switches the amount of detail shown on this Globe.
 * @note Country and city details are shown only when zoomed in.
 */
void Globe::toggleDetail()
{
	Options::globeDetail = !Options::globeDetail;
	drawDetail();
}

/**
 * Turns radar-lines on or off.
 */
void Globe::toggleRadarLines()
{
	switch (_radarDetail)
	{
		case 0:
			_radarDetail = 1;
			Options::globeRadarLines = true;
			break;

		case 1:
			_radarDetail = 2;
			break;

		case 2:
			_radarDetail = 3;
			break;

		case 3:
			_radarDetail = 0;
			Options::globeRadarLines = false;
	}
	drawRadars();
}

/**
 * Checks if a certain target is near a certain cartesian-point.
 * @param target	- pointer to Target
 * @param x			- X coordinate of point
 * @param y			- Y coordinate of point
 * @return, true if near
 */
bool Globe::targetNear( // private.
		const Target* const target,
		int x,
		int y) const
{
	double
		lon (target->getLongitude()),
		lat (target->getLatitude());

	if (pointBack(lon,lat) == false)
	{
		Sint16
			tx,ty;

		polarToCart(
				lon,lat,
				&tx,&ty);
		const int
			dx (x - tx),
			dy (y - ty);

		return (dx * dx + dy * dy < NEAR_RADIUS);
	}
	return false;
}

/**
 * Returns a list of all the Targets currently near a cartesian-point.
 * @param x				- X coordinate of point
 * @param y				- Y coordinate of point
 * @param flightTargets	- true to get targets for Craft only (default true)
 * @return, vector of pointers to Targets
 */
std::vector<Target*> Globe::getTargets(
		int x,
		int y,
		bool flightTargets) const
{
	std::vector<Target*> targets;

	if (flightTargets == false)
	{
		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			if ((*i)->getBasePlaced() == true)
			{
				if (targetNear(*i, x,y))
					targets.push_back(*i);

				for (std::vector<Craft*>::const_iterator
						j = (*i)->getCrafts()->begin();
						j != (*i)->getCrafts()->end();
						++j)
				{
					if ((*j)->getCraftStatus() == CS_OUT
						&& targetNear(*j, x,y))
					{
						targets.push_back(*j);
					}
				}
			}
		}
	}

	for (std::vector<Ufo*>::const_iterator
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();
			++i)
	{
		if ((*i)->getDetected() == true
			&& (*i)->getUfoStatus() != Ufo::DESTROYED // ah, here's the workaround.
			&& targetNear(*i, x,y) == true)
		{
			targets.push_back(*i);
		}
	}

	for (std::vector<Waypoint*>::const_iterator
			i = _game->getSavedGame()->getWaypoints()->begin();
			i != _game->getSavedGame()->getWaypoints()->end();
			++i)
	{
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);
	}

	for (std::vector<MissionSite*>::const_iterator
			i = _game->getSavedGame()->getMissionSites()->begin();
			i != _game->getSavedGame()->getMissionSites()->end();
			++i)
	{
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);
	}

	for (std::vector<AlienBase*>::const_iterator
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
	{
		if ((*i)->isDetected() == true
			&& targetNear(*i, x,y) == true)
		{
			targets.push_back(*i);
		}
	}

	return targets;
}

/**
 * Takes care of pre-calculating all the Polygons currently visible on this
 * Globe and caching them so they only need to be recalculated when this Globe
 * is actually moved.
 */
void Globe::cachePolygons()
{
	cache(_rules->getPolygons(), &_cacheLand);
}

/**
 * Caches a set of Polygons.
 * @param polygons	- pointer to list of polygons
 * @param cache		- pointer to cache list
 */
void Globe::cache( // private.
		std::list<Polygon*>* const polygons,
		std::list<Polygon*>* const cache)
{
	for (std::list<Polygon*>::const_iterator
			i = cache->begin();
			i != cache->end();
			++i)
	{
		delete *i;
	}

	cache->clear();

	for (std::list<Polygon*>::const_iterator
			i = polygons->begin();
			i != polygons->end();
			++i)
	{
		double
			closest (0.),
			furthest (0.),
			z;

		for (size_t
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			z = std::cos(_cenLat) * std::cos((*i)->getLatitude(j)) * std::cos((*i)->getLongitude(j) - _cenLon)
			  + std::sin(_cenLat) * std::sin((*i)->getLatitude(j));

			if (z > closest)
				closest = z;
			else if (z < furthest)
				furthest = z;
		}

		if (-furthest > closest)
			continue;

		Polygon* const poly (new Polygon(**i));

		for (size_t
				j = 0;
				j != poly->getPoints();
				++j)
		{
			Sint16
				x,y;
			polarToCart(
					poly->getLongitude(j),
					poly->getLatitude(j),
					&x,&y);
			poly->setX(j,x);
			poly->setY(j,y);
		}

		cache->push_back(poly);
	}
}

/**
 * Replaces a certain amount of colors in the palette of this Globe.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void Globe::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_texture->setPalette(colors, firstcolor, ncolors);
	_markerSet->setPalette(colors, firstcolor, ncolors);

	_countries->setPalette(colors, firstcolor, ncolors);
	_crosshair->setPalette(colors, firstcolor, ncolors);
	_markers->setPalette(colors, firstcolor, ncolors);
	_radars->setPalette(colors, firstcolor, ncolors);
}

/**
 * Keeps the animation & rotation Timers running.
 */
void Globe::think()
{
	if (kL_reCenter == true)
	{
		kL_reCenter = false;
		center(
			_game->getSavedGame()->getGlobeLongitude(),
			_game->getSavedGame()->getGlobeLatitude());
	}

	_blinkTimer->think(nullptr, this);
	_rotTimer->think(nullptr, this);
}

/**
 * Makes this Globe's markers blink.
 */
void Globe::blink()
{
	if (_blink == true)
	{
		_blinkVal = -_blinkVal; // can't use static because, reload.

		const int j (static_cast<int>(_markerSet->getTotalFrames()));
		for (int
				i = 0;
				i != j;
				++i)
		{
			if (i != GLM_CITY)
				_markerSet->getFrame(i)->offset(_blinkVal);
		}
		drawMarkers();
	}
}

/**
 * Toggles the blinking.
 */
void Globe::toggleBlink()
{
	_blink = !_blink;
}

/**
 * Rotates this Globe by a set amount.
 * @note Necessary since the Globe keeps rotating while a button is held down.
 */
void Globe::rotate()
{
	_cenLon += _rotLon * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_zoom + 1);
	_cenLat += _rotLat * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_zoom + 1);

	_game->getSavedGame()->setGlobeLongitude(_cenLon);
	_game->getSavedGame()->setGlobeLatitude(_cenLat);

	invalidate();
}

/**
 * Draws this whole Globe part by part.
 */
void Globe::draw()
{
	if (_redraw == true)
		cachePolygons();

	Surface::draw();

	drawOcean();
	drawLand();
	drawBevel();
	drawRadars();
	drawShadow();
	drawMarkers();
	drawDetail();
	drawFlightPaths();

	if (_drawCrosshair == true) drawCrosshair();
}

/**
 * Renders the ocean and shades it according to the time of day.
 */
void Globe::drawOcean()
{
	lock();
	drawCircle(
			_cenX + 1,
			_cenY,
			static_cast<Sint16>(_radius) + 20,
			C_OCEAN);
	unlock();
}

/**
 * Renders the land taking all the visible world-polygons and texturing and
 * shading them appropriately.
 */
void Globe::drawLand()
{
	Sint16
		x[4],y[4];

	for (std::list<Polygon*>::const_iterator
			i = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
	{
		for (size_t // Convert coordinates
				j = 0;
				j != (*i)->getPoints();
				++j)
		{
			x[j] = (*i)->getX(j);
			y[j] = (*i)->getY(j);
		}

		drawTexturedPolygon( // Apply textures according to zoom and shade
						x,y,
						(*i)->getPoints(),
						_texture->getFrame(static_cast<int>((*i)->getPolyTexture() + _zoomTexture)),
						0,0);
	}
}

/**
 * Draws a 3d-bevel around the continents.
 */
void Globe::drawBevel()
{
	Uint8 p;
	int
		w (this->getWidth()),
		h (this->getHeight());

	for (int
			y = 0;
			y != h;
			++y)
	{
		for (int
				x = 0;
				x != w;
				++x)
		{
			if (this->getPixelColor(x,y) == C_OCEAN)
			{
				p = this->getPixelColor(x - 1, y - 1);
				if (p != C_OCEAN
					&& p != C_BLACK)
				{
					this->setPixelColor(x,y, C_BLACK);
				}
			}
		}
	}
}

/**
 * Gets position of sun from point on this Globe.
 * @param lon - longitude of position
 * @param lat - latitude of position
 * @return, position of sun
 */
Cord Globe::getSunDirection( // private.
		double lon,
		double lat) const
{
	double sun;

	if (Options::globeSeasons == true)
	{
		const int
			monthDays1[] {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
			monthDays2[] {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},

			year (_game->getSavedGame()->getTime()->getYear()),
			month (_game->getSavedGame()->getTime()->getMonth() - 1),
			day (_game->getSavedGame()->getTime()->getDay() - 1);

		const double
			tm (static_cast<double>( // day fraction is also taken into account
						(((   _game->getSavedGame()->getTime()->getHour() * 60)
							+ _game->getSavedGame()->getTime()->getMinute()) * 60)
							+ _game->getSavedGame()->getTime()->getSecond())
						/ 86400.);

		double today;
		if (year % 4 == 0 // spring equinox (start of astronomic year)
			&& !
				(year % 100 == 0
					&& year % 400 != 0))
		{
			today = (static_cast<double>(monthDays2[month] + day) + tm) / 366. - 0.219;
		}
		else
			today = (static_cast<double>(monthDays1[month] + day) + tm) / 365. - 0.219;

		if (today < 0.)
			today += 1.;

		sun = -0.261 * std::sin(today * M_PI * 2.);
	}
	else
		sun = 0.;

	const double rot (_game->getSavedGame()->getTime()->getDaylight() * M_PI * 2.);
	Cord sun_direction (std::cos(rot + lon),
						std::sin(rot + lon) * -std::sin(lat),
						std::sin(rot + lon) *  std::cos(lat));

	if (sun > 0)
		 sun_direction *= 1. - sun;
	else
		 sun_direction *= 1. + sun;

	Cord pole (0,
			   std::cos(lat),
			   std::sin(lat));
	pole *= sun;
	sun_direction += pole;
	double norm (sun_direction.norm());
	norm = 1. / norm; // norm should always be greater than 0 here
	sun_direction *= norm;

	return sun_direction;
}

/**
 * Shadows the earth according to the sun's direction.
 * @note Also handles the terminator-fluxions (noise).
 */
void Globe::drawShadow()
{
	ShaderMove<Cord> earth = ShaderMove<Cord>(
										_earthData[_zoom],
										getWidth(),
										getHeight());
	ShaderRepeat<Sint16> noise = ShaderRepeat<Sint16>(
											_terminatorFluxions,
											static_data.random_surf_size,
											static_data.random_surf_size);

	earth.setMove(
			_cenX-getWidth() / 2,
			_cenY-getHeight() / 2);

	lock();
	ShaderDraw<CreateShadow>(
						ShaderSurface(this),
						earth,
						ShaderScalar(getSunDirection(_cenLon, _cenLat)),
						noise);
	unlock();
}

/**
 * Draws a XuLine!
 * @param surface	-
 * @param src		-
 * @param x1		-
 * @param y1		-
 * @param x2		-
 * @param y2		-
 * @param shade		-
 * @param color		- (default 0)
 */
void Globe::XuLine( // private.
		Surface* surface,
		Surface* src,
		double x1,
		double y1,
		double x2,
		double y2,
		int shade,
		Uint8 color)
{
	if (_clipper->LineClip( // empty line
						&x1,&y1,
						&x2,&y2) != 1)
	{
		return;
	}

	bool inv;
	Uint8 tcol;
	double
		delta_x (x2 - x1),
		delta_y (y2 - y1),
		len,
		x0,y0,
		SX,SY;

	if (std::abs(static_cast<int>(y2) - static_cast<int>(y1)) > std::abs(static_cast<int>(x2) - static_cast<int>(x1)))
	{
		len = std::abs(static_cast<int>(y2) - static_cast<int>(y1));
		inv = false;
	}
	else
	{
		len = std::abs(static_cast<int>(x2) - static_cast<int>(x1));
		inv = true;
	}

	if (y2 < y1)
		SY = -1;
	else if (AreSame(delta_y, 0.))
		SY = 0;
	else
		SY = 1;

	if (x2 < x1)
		SX = -1;
	else if (AreSame(delta_x, 0.))
		SX = 0;
	else
		SX = 1;

	x0 = x1;
	y0 = y1;

	if (inv == true)
		SY = (delta_y / len);
	else
		SX = (delta_x / len);

	while (len > 0.)
	{
		tcol = src->getPixelColor(
							static_cast<int>(x0),
							static_cast<int>(y0));
		if (tcol != 0)
		{
			if (color != 0)
				tcol = color; // flight path or craft radar
			else
			{
				const Uint8 colorBlock (tcol & helper::ColorGroup);

				if (colorBlock == C_OCEAN
					|| colorBlock == C_OCEAN + 16)
				{
					tcol = C_OCEAN + static_cast<Uint8>(shade) + 8; // this pixel is ocean
				}
				else // this pixel is land
				{
					const Uint8 colorShaded (tcol + static_cast<Uint8>(shade));

					if (colorShaded > colorBlock + helper::ColorShade)
						tcol = colorBlock + helper::ColorShade;
					else
						tcol = colorShaded;
				}
			}

			surface->setPixelColor(
								static_cast<int>(x0),
								static_cast<int>(y0),
								tcol);
		}

		x0 += SX;
		y0 += SY;
		len -= 1.;
	}
}

/**
 * Draws the radar-circles of player's Bases and Craft on this Globe.
 */
void Globe::drawRadars()
{
	_radars->clear();

	_radars->lock();
	if (_hover == true // placing new Base.
		&& Options::globeAllRadarsOnBaseBuild == true)
	{
		double range;

		const std::vector<std::string>& facilities (_game->getRuleset()->getBaseFacilitiesList());
		for (std::vector<std::string>::const_iterator
				i = facilities.begin();
				i != facilities.end();
				++i)
		{
			range = static_cast<double>(_game->getRuleset()->getBaseFacility(*i)->getRadarRange());
			if (range > 0.)
			{
				range *= unitToRads;
				drawGlobeCircle(
							_hoverLat,
							_hoverLon,
							range,
							48);
//							C_RADAR1);
			}
		}
	}

	if (_radarDetail > 0
		&& Options::globeRadarLines == true)
	{
		double
			lon,lat,
			range,
			rangeBest;

		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			if ((*i)->getBasePlaced() == true)
			{
				if (_radarDetail > 1)
				{
					rangeBest = 0.;
					lat = (*i)->getLatitude();
					lon = (*i)->getLongitude();

					for (std::vector<BaseFacility*>::const_iterator
							j = (*i)->getFacilities()->begin();
							j != (*i)->getFacilities()->end();
							++j)
					{
						if ((*j)->buildFinished() == true)
						{
							range = static_cast<double>((*j)->getRules()->getRadarRange());
							if (_radarDetail > 2)
							{
								if (range > 0.)
								{
									range *= unitToRads;
									drawGlobeCircle( // Base radars.
												lat,lon,
												range,
												64);
//												C_RADAR1);
								}
							}
							else if (range > rangeBest)
								rangeBest = range;
						}
					}

					if (rangeBest > 0.)
					{
						rangeBest *= unitToRads;
						drawGlobeCircle( // largest Base radar.
									lat,lon,
									rangeBest,
									64);
//									C_RADAR1)
					}
				}

				for (std::vector<Craft*>::const_iterator
						j = (*i)->getCrafts()->begin();
						j != (*i)->getCrafts()->end();
						++j)
				{
					if ((*j)->getCraftStatus() == CS_OUT
						&& (*j)->getTakeoff() == true)
					{
						range = static_cast<double>((*j)->getRules()->getRadarRange());
						if (range > 0.)
						{
							range *= unitToRads;
							drawGlobeCircle( // Craft radars.
										(*j)->getLatitude(),
										(*j)->getLongitude(),
										range,
										48,
										C_RADAR2);
						}
					}
				}
			}
		}
	}
	_radars->unlock();
}

/**
 * Draws a range circle.
 * @param lat		-
 * @param lon		-
 * @param radius	-
 * @param segments	-
 * @param color		- (default 0)
 */
void Globe::drawGlobeCircle( // private.
		double lat,
		double lon,
		double radius,
		int segments,
		Uint8 color)
{
	double
		x,y,
		x2 (0.),
		y2 (0.),
		lat1,
		lon1;

	for (double // 48 circle segments
			az = 0.;
			az <= M_PI * 2. + 0.01;
			az += M_PI * 2. / static_cast<double>(segments))
	{
		// calculating sphere-projected circle
		lat1 = asin(std::sin(lat) * std::cos(radius) + std::cos(lat) * std::sin(radius) * std::cos(az));
		lon1 = lon + std::atan2(
						std::sin(az) * std::sin(radius) * std::cos(lat),
						std::cos(radius) - std::sin(lat) * std::sin(lat1));
		polarToCart(
				lon1,lat1,
				&x,&y);

		if (AreSame(az, 0.)) // first vertex is for initialization only
		{
			x2 = x;
			y2 = y;
			continue;
		}

		if (pointBack(lon1,lat1) == false)
			XuLine(
				_radars,
				this,
				x,y,
				x2,y2,
				4,
				color);

		x2 = x;
		y2 = y;
	}
}

/**
 *
 */
void Globe::setNewBaseHover()
{
	_hover = true;
}

/**
 *
 */
void Globe::unsetNewBaseHover()
{
	_hover = false;
}

/**
 *
 */
bool Globe::getNewBaseHover() const
{
	return _hover;
}

/**
 *
 * @param lon -
 * @param lat -
 */
void Globe::setNewBaseHoverPos(
		double lon,
		double lat)
{
	_hoverLon = lon;
	_hoverLat = lat;
}

/**
 * Draws a VHLine!
 * @param surface	- pointer to a Surface
 * @param lon1		-
 * @param lat1		-
 * @param lon2		-
 * @param lat2		-
 * @param color		- (default 0)
 */
void Globe::drawVHLine( // private.
		Surface* surface,
		double lon1,
		double lat1,
		double lon2,
		double lat2,
		Uint8 color)
{
	double
		sx (lon2 - lon1),
		sy (lat2 - lat1),
		ln1,lt1,
		ln2,lt2;
	Sint16
		x1,y1,
		x2,y2;
	int
		seg;

	if (sx < 0.)
		sx += M_PI * 2.;

	if (std::fabs(sx) < 0.01)
	{
		seg = static_cast<int>(std::abs(sy / (M_PI * 2.) * 48.));
		if (seg == 0)
			++seg;
	}
	else
	{
		seg = static_cast<int>(std::abs(sx / (M_PI * 2.) * 96.));
		if (seg == 0)
			++seg;
	}

	sx /= seg;
	sy /= seg;

	for (int
			i = 0;
			i != seg;
			++i)
	{
		ln1 = lon1 + sx * static_cast<double>(i);
		lt1 = lat1 + sy * static_cast<double>(i);
		ln2 = lon1 + sx * static_cast<double>(i + 1);
		lt2 = lat1 + sy * static_cast<double>(i + 1);

		if (pointBack(ln2, lt2) == false
			&& pointBack(ln1, lt1) == false)
		{
			polarToCart(ln1, lt1, &x1, &y1);
			polarToCart(ln2, lt2, &x2, &y2);

			surface->drawLine(
							x1,y1,
							x2,y2,
							color);
		}
	}
}

/**
 * Draws the details of the countries on this Globe based on the current zoom-level.
 */
void Globe::drawDetail()
{
	_countries->clear();

	double
		lon,lat;

	if (Options::globeDetail == true // draw the Country borders
		&& _zoom > 0)
	{
		double
			lon1,lat1;
		Sint16
			x[2],y[2];

		_countries->lock();
		for (std::list<Polyline*>::const_iterator
				i = _rules->getPolylines()->begin();
				i != _rules->getPolylines()->end();
				++i)
		{
			for (int
					j = 0;
					j != (*i)->getPoints() - 1;
					++j)
			{
				lon = (*i)->getLongitude(j),
				lat = (*i)->getLatitude(j);
				lon1 = (*i)->getLongitude(j + 1),
				lat1 = (*i)->getLatitude(j + 1);

				if (pointBack(lon,lat) == false
					&& pointBack(lon1,lat1) == false)
				{
					polarToCart(
							lon,lat,
							&x[0],&y[0]);
					polarToCart(
							lon1,lat1,
							&x[1],&y[1]);

					_countries->drawLine(
									x[0],y[0],
									x[1],y[1],
									C_LINE);
				}
			}
		}
		_countries->unlock();
	}

	Sint16
		x,y;

	if (_zoom > 1) // draw the City markers
	{
		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end();
				++i)
		{
			for (std::vector<RuleCity*>::const_iterator
					j = (*i)->getRules()->getCities()->begin();
					j != (*i)->getRules()->getCities()->end();
					++j)
			{
				drawTarget(*j, _countries);
			}
		}
	}

	if (Options::globeDetail == true)
	{
		Text* const label (new Text(100,9));

		label->setPalette(getPalette());
		label->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
		label->setAlign(ALIGN_CENTER);

		if (_zoom > 2)
		{
			label->setColor(C_LBLCOUNTRY); // draw the Country labels

			for (std::vector<Country*>::const_iterator
					i = _game->getSavedGame()->getCountries()->begin();
					i != _game->getSavedGame()->getCountries()->end();
					++i)
			{
				lon = (*i)->getRules()->getLabelLongitude(),
				lat = (*i)->getRules()->getLabelLatitude();

				if (pointBack(lon,lat) == false)
				{
					polarToCart(
							lon,lat,
							&x,&y);

					label->setX(x - 50);
					label->setY(y);
					label->setText(_game->getLanguage()->getString((*i)->getRules()->getType()));

					label->blit(_countries);
				}
			}
		}

		label->setColor(C_LBLCITY); // draw the City labels
		int offset_y;

		for (std::vector<Region*>::const_iterator
				i = _game->getSavedGame()->getRegions()->begin();
				i != _game->getSavedGame()->getRegions()->end();
				++i)
		{
			for (std::vector<RuleCity*>::const_iterator
					j = (*i)->getRules()->getCities()->begin();
					j != (*i)->getRules()->getCities()->end();
					++j)
			{
				lon = (*j)->getLongitude(),
				lat = (*j)->getLatitude();

				if (pointBack(lon,lat) == false)
				{
					if (_zoom >= (*j)->getZoomLevel())
					{
						polarToCart(
								lon,lat,
								&x,&y);

						if ((*j)->getLabelTop() == true)
							offset_y = -10;
						else
							offset_y = 2;

						label->setX(x - 50);
						label->setY(y + offset_y);
						label->setText((*j)->getName(_game->getLanguage()));

						label->blit(_countries);
					}
				}
			}
		}

		label->setColor(C_LBLBASE); // draw xCom Base labels
		label->setAlign(ALIGN_LEFT);

		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			if ((*i)->getMarker() != -1) // base is placed.
			{
				lon = (*i)->getLongitude(),
				lat = (*i)->getLatitude();

				if (pointBack(lon,lat) == false)
				{
					polarToCart(
							lon,lat,
							&x,&y);

					label->setX(x - 3);
					label->setY(y - 10);
					label->setText((*i)->getName());

					label->blit(_countries);
				}
			}
		}
		delete label;
	}


	// Debug stuff follows ...
	static bool switchDebugType;

	if (_game->getSavedGame()->getDebugGeo() == true)
	{
		switchDebugType = true;
		int
			cycle (_game->getDebugCycle()),
			area (0),
			color (0);

		switch (_debugType)
		{
			case DTG_COUNTRY:
			{
				if (cycle >= static_cast<int>(_game->getSavedGame()->getCountries()->size()))
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Country*>::const_iterator
						i = _game->getSavedGame()->getCountries()->begin();
						i != _game->getSavedGame()->getCountries()->end();
						++i, ++area)
				{
					if (cycle == area || cycle == -1)
					{
						color += 10;
						for (size_t
								j = 0;
								j != (*i)->getRules()->getLonMin().size();
								++j)
						{
							const double
								lon1 ((*i)->getRules()->getLonMin().at(j)),
								lon2 ((*i)->getRules()->getLonMax().at(j)),
								lat1 ((*i)->getRules()->getLatMin().at(j)),
								lat2 ((*i)->getRules()->getLatMax().at(j));

							drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
						}
					}

					if (area == cycle)
					{
						if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							_game->getSavedGame()->setDebugArg((*i)->getType());
						break;
					}
					else
						_game->getSavedGame()->setDebugArg("");
				}
				break;
			}

			case DTG_REGION:
			{
				if (cycle >= static_cast<int>(_game->getSavedGame()->getRegions()->size()))
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Region*>::const_iterator
						i = _game->getSavedGame()->getRegions()->begin();
						i != _game->getSavedGame()->getRegions()->end();
						++i, ++area)
				{
					if (cycle == area || cycle == -1)
					{
						color += 10;
						for (size_t
								j = 0;
								j != (*i)->getRules()->getLatMax().size();
								++j)
						{
							const double
								lon1 ((*i)->getRules()->getLonMin().at(j)),
								lon2 ((*i)->getRules()->getLonMax().at(j)),
								lat1 ((*i)->getRules()->getLatMin().at(j)),
								lat2 ((*i)->getRules()->getLatMax().at(j));

							drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
							drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
						}
					}

					if (area == cycle)
					{
						if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							_game->getSavedGame()->setDebugArg((*i)->getType());
						break;
					}
					else
						_game->getSavedGame()->setDebugArg("");
				}
				break;
			}

			case DTG_ZONE:
			{
				int limit (0);
				for (std::vector<Region*>::const_iterator
						i = _game->getSavedGame()->getRegions()->begin();
						i != _game->getSavedGame()->getRegions()->end();
						++i)
				{
					for (std::vector<MissionZone>::const_iterator
							j = (*i)->getRules()->getMissionZones().begin();
							j != (*i)->getRules()->getMissionZones().end();
							++j)
					{
						++limit;
					}
				}

				if (cycle >= limit)
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Region*>::const_iterator
						i = _game->getSavedGame()->getRegions()->begin();
						i != _game->getSavedGame()->getRegions()->end();
						++i)
				{
					color = -1;
					int zoneType (0);
					for (std::vector<MissionZone>::const_iterator
							j = (*i)->getRules()->getMissionZones().begin();
							j != (*i)->getRules()->getMissionZones().end();
							++j, ++area, ++zoneType)
					{
						if (cycle == area || cycle == -1)
						{
							color += 2;
							for (std::vector<MissionArea>::const_iterator
									k = (*j).areas.begin();
									k != (*j).areas.end();
									++k)
							{
								const double
									lon1 ((*k).lonMin), // * M_PI / 180.,
									lon2 ((*k).lonMax), // * M_PI / 180.,
									lat1 ((*k).latMin), // * M_PI / 180.,
									lat2 ((*k).latMax); // * M_PI / 180.;

								drawVHLine(_countries, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
								drawVHLine(_countries, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
								drawVHLine(_countries, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
								drawVHLine(_countries, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
							}
						}

						if (area == cycle)
						{
							if (_game->getSavedGame()->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							{
								std::ostringstream oststr;
								oststr << (*i)->getType() << " [" << zoneType << "]";
								_game->getSavedGame()->setDebugArg(oststr.str());
							}
							break;
						}
						else
							_game->getSavedGame()->setDebugArg("");
					}

					if (area == cycle)
						break;
				}
			}
		}
	}
	else if (switchDebugType == true) // effectively toggles debugMode.
	{
		switchDebugType = false;
		switch (_debugType)
		{
			case DTG_COUNTRY:
				_debugType = DTG_REGION;
				break;

			case DTG_REGION:
				_debugType = DTG_ZONE;
				break;

			case DTG_ZONE:
				_debugType = DTG_COUNTRY;
		}
	}
}

/**
 * Gets the current debugType for Geoscape.
 * @return, DebugTypeGlobe (Globe.h)
 */
DebugTypeGlobe Globe::getDebugType() const
{
	return _debugType;
}

/**
 * Draws flight paths.
 * @param surface	- pointer to a Surface
 * @param lon1		-
 * @param lat1		-
 * @param lon2		-
 * @param lat2		-
 */
void Globe::drawPath( // private.
		Surface* surface,
		double lon1,
		double lat1,
		double lon2,
		double lat2)
{
	double
		dist,
		x1,y1,
		x2,y2;
	Sint16 qty;
	CordPolar
		p1,p2;
	Cord
		a (CordPolar(lon1, lat1)),
		b (CordPolar(lon2, lat2));

	if (-b == a)
		return;

	b -= a;

	// longer paths have more parts
	dist = b.norm();
	dist *= dist * 15;

	qty = static_cast<Sint16>(static_cast<int>(dist) + 1);
	b /= qty;

	p1 = CordPolar(a);
	polarToCart(
				p1.lon,
				p1.lat,
				&x1,&y1);
	for (Sint16
			i = 0;
			i != qty;
			++i)
	{
		a += b;
		p2 = CordPolar(a);
		polarToCart(
					p2.lon,
					p2.lat,
					&x2,&y2);

		if (   pointBack(p1.lon, p1.lat) == false
			&& pointBack(p2.lon, p2.lat) == false)
		{
			XuLine(
				surface,
				this,
				x1,y1,
				x2,y2,
				8,
				C_FLIGHT);
		}

		p1 = p2;
		x1 = x2;
		y1 = y2;
	}
}

/**
 * Draws the flight-paths of player's Craft flying around this Globe.
 */
void Globe::drawFlightPaths()
{
	if (Options::globeFlightPaths == true)
	{
		_radars->lock();
		for (std::vector<Base*>::const_iterator
				i = _game->getSavedGame()->getBases()->begin();
				i != _game->getSavedGame()->getBases()->end();
				++i)
		{
			for (std::vector<Craft*>::const_iterator
					j = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end();
					++j)
			{
				if ((*j)->getCraftStatus() == CS_OUT
					&& (*j)->getDestination() != nullptr)
				{
					static const double MIN_Diff (0.005); // radians
					const double
						lon1 ((*j)->getLongitude()),
						lat1 ((*j)->getLatitude()),
						lon2 ((*j)->getDestination()->getLongitude()),
						lat2 ((*j)->getDestination()->getLatitude()),
						lon3 ((*j)->getMeetLongitude()),
						lat3 ((*j)->getMeetLatitude());
					drawPath(
							_radars,
							lon1,lat1,
							lon2,lat2);

					if (   std::fabs(lon3 - lon2) > MIN_Diff
						|| std::fabs(lat3 - lat2) > MIN_Diff)
					{
						drawPath(
								_radars,
								lon1,lat1,
								lon3,lat3);
						drawInterceptMarker(lon3,lat3);
					}
				}
			}
		}
		_radars->unlock();
	}
}

/**
 * Places a marker at the end-point of player's Craft on an intercept-trajectory.
 * @param lon - longitude to draw the marker at
 * @param lat - latitude to draw the marker at
 */
void Globe::drawInterceptMarker( // private.
		const double lon,
		const double lat)
{
	if (pointBack(lon,lat) == false)
	{
		Sint16
			x,y;
		polarToCart(
				lon,lat,
				&x,&y);

		Surface* const marker (_markerSet->getFrame(GLM_WAYPOINT));
		marker->setX(x - 1);
		marker->setY(y - 1);
		marker->blit(_markers);
	}
}

/**
 * Draws the marker for a specified Target on this Globe.
 * @param target	- pointer to globe Target
 * @param surface	- pointer to globe Surface
 */
void Globe::drawTarget( // private.
		const Target* const target,
		Surface* const surface)
{
	const int markerId (target->getMarker());
	if (markerId != -1)
	{
		const double
			lon (target->getLongitude()),
			lat (target->getLatitude());

		if (pointBack(lon,lat) == false)
		{
			Sint16
				x,y;
			polarToCart(
					lon,lat,
					&x,&y);

			Surface* const marker (_markerSet->getFrame(markerId));
			marker->setX(x - 1);
			marker->setY(y - 1);
			marker->blit(surface);
		}
	}
}

/**
 * Draws the markers of all the various things going on around the world except
 * Cities - for which see drawDetail() above.
 */
void Globe::drawMarkers()
{
	_markers->clear();

	for (std::vector<Base*>::const_iterator			// Draw the Base markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
		drawTarget(*i, _markers);

	for (std::vector<Waypoint*>::const_iterator		// Draw the Waypoint markers
			i = _game->getSavedGame()->getWaypoints()->begin();
			i != _game->getSavedGame()->getWaypoints()->end();
			++i)
		drawTarget(*i, _markers);

	for (std::vector<MissionSite*>::const_iterator	// Draw the MissionSite markers
			i = _game->getSavedGame()->getMissionSites()->begin();
			i != _game->getSavedGame()->getMissionSites()->end();
			++i)
		drawTarget(*i, _markers);

	for (std::vector<AlienBase*>::const_iterator	// Draw the AlienBase markers
			i = _game->getSavedGame()->getAlienBases()->begin();
			i != _game->getSavedGame()->getAlienBases()->end();
			++i)
		drawTarget(*i, _markers);

	for (std::vector<Ufo*>::const_iterator			// Draw the Ufo markers
			i = _game->getSavedGame()->getUfos()->begin();
			i != _game->getSavedGame()->getUfos()->end();
			++i)
		drawTarget(*i, _markers);

	for (std::vector<Base*>::const_iterator			// Draw the Craft markers
			i = _game->getSavedGame()->getBases()->begin();
			i != _game->getSavedGame()->getBases()->end();
			++i)
		for (std::vector<Craft*>::const_iterator
				j = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
			drawTarget(*j, _markers);
}

/**
 * Draws a big yellow/red crosshair-targeter.
 * @note Used over last-known-UFO position.
 */
void Globe::drawCrosshair() // private.
{
	if ((AreSame(_crosshairLon, 0.) == false
			|| AreSame(_crosshairLat, 0.) == false)
		&& pointBack(_crosshairLon, _crosshairLat) == false)
	{
		Sint16
			x,y;
		polarToCart(
				_crosshairLon,
				_crosshairLat,
				&x,&y);

		Surface* const crosshair (_game->getResourcePack()->getSurface("Crosshairs"));
		crosshair->setX(x - 14);
		crosshair->setY(y - 14);
		crosshair->blit(_crosshair);
	}
}

/**
 * Sets the co-ordinates to draw a crosshair at.
 * @param lon - longitude to draw the crosshair at
 * @param lat - latitude to draw the crosshair at
 */
void Globe::setCrosshair(
		const double lon,
		const double lat)
{
	_drawCrosshair = true;
	_crosshairLon = lon;
	_crosshairLat = lat;
}

/**
 * Hides the crosshair.
 */
void Globe::clearCrosshair()
{
	_drawCrosshair = false;
	_crosshair->clear();
}

/**
 * Blits this Globe and its sub-elements onto another Surface.
 * @param srf - pointer to a surface
 */
void Globe::blit(Surface* const srf)
{
	Surface::blit(srf);

	_radars->blit(srf);
	_countries->blit(srf);
	_markers->blit(srf);
	_crosshair->blit(srf);
}

/**
 * Ignores any mouse-hovers that are outside this Globe.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Globe::mouseOver(Action* action, State* state)
{
	double
		lon,lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
			&lon,&lat);

	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// What follows is a workaround for a rare problem where sometimes the
		// mouse-release event is missed for some reason. However if SDL also
		// missed the release event then this won't work.
		//
		// This part handles the release if it's missed and another button is used.
		if ((SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::geoDragScrollButton)) == 0)
		{
			// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				center(
					_lonPreMouseScroll,
					_latPreMouseScroll);
			}

			_isMouseScrolled =
			_isMouseScrolling = false;
			return;
		}
		_isMouseScrolled = true;

		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false)
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;

		if (Options::geoDragScrollInvert == true) // scroll. I don't use inverted scrolling.
		{
			const double
				newLon ((static_cast<double>(_totalMouseMoveX) / action->getScaleX()) * ROTATE_LONGITUDE / static_cast<double>(_zoom + 1) / 2.),
				newLat ((static_cast<double>(_totalMouseMoveY) / action->getScaleY()) * ROTATE_LATITUDE  / static_cast<double>(_zoom + 1) / 2.);
			center(
				_lonPreMouseScroll + newLon / static_cast<double>(Options::geoScrollSpeed),
				_latPreMouseScroll + newLat / static_cast<double>(Options::geoScrollSpeed));
		}
		else
		{
			const double
				newLon (static_cast<double>(-action->getDetails()->motion.xrel) * ROTATE_LONGITUDE / static_cast<double>(_zoom + 1) / 2.),
				newLat (static_cast<double>(-action->getDetails()->motion.yrel) * ROTATE_LATITUDE  / static_cast<double>(_zoom + 1) / 2.);
			center(
				_cenLon + newLon / static_cast<double>(Options::geoScrollSpeed),
				_cenLat + newLat / static_cast<double>(Options::geoScrollSpeed));
		}
		_game->getCursor()->handle(action);
	}

	if (isNaNorInf(lon,lat) == false)
		InteractiveSurface::mouseOver(action, state);
}

/**
 * Ignores any mouse-clicks that are outside this Globe.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Globe::mousePress(Action* action, State* state)
{
	clearCrosshair();

	double
		lon,lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
			&lon,&lat);

	if (action->getDetails()->button.button == Options::geoDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		_lonPreMouseScroll = _cenLon;
		_latPreMouseScroll = _cenLat;

		_totalMouseMoveX =
		_totalMouseMoveY = 0;

		_mouseOverThreshold = false;
		_mouseScrollStartTime = SDL_GetTicks();
	}

	if (isNaNorInf(lon,lat) == false)
		InteractiveSurface::mousePress(action, state);
}

/**
 * Ignores any mouse-clicks that are outside this Globe.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Globe::mouseRelease(Action* action, State* state)
{
	double
		lon,lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
			&lon,&lat);

	if (isNaNorInf(lon,lat) == false)
		InteractiveSurface::mouseRelease(action, state);
}

/**
 * Ignores any mouse-clicks that are outside this Globe and handles this Globe's
 * rotation and zooming.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Globe::mouseClick(Action* action, State* state)
{
	const Uint8 btnId (action->getDetails()->button.button);
	switch (btnId)
	{
		case SDL_BUTTON_WHEELUP:
			zoomIn();
			break;
		case SDL_BUTTON_WHEELDOWN:
			zoomOut();
	}

	double
		lon,lat;
	cartToPolar(
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
			static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
			&lon,&lat);

	// What follows is a workaround for a rare problem where sometimes the
	// mouse-release event is missed for some reason. However if SDL also
	// missed the release event then this won't work.
	//
	// This part handles the release if it's missed and another button is used.
	if (_isMouseScrolling == true)
	{
		if (btnId != Options::geoDragScrollButton
			&& (SDL_GetMouseState(nullptr,nullptr) & SDL_BUTTON(Options::geoDragScrollButton)) == 0)
		{
			// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				center(
					_lonPreMouseScroll,
					_latPreMouseScroll);
			}
			_isMouseScrolled = _isMouseScrolling = false;
		}
	}

	if (_isMouseScrolling == true) // dragScroll-button release: release mouse-scroll-mode
	{
		if (btnId != Options::geoDragScrollButton) return; // other buttons are ineffective while scrolling

		_isMouseScrolling = false;

		// Check if the scrolling has to be revoked because it was too short in time and hence was a click.
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
			center(
				_lonPreMouseScroll,
				_latPreMouseScroll);
		}

		if (_isMouseScrolled == true) return;
	}

	if (isNaNorInf(lon,lat) == false)
	{
		InteractiveSurface::mouseClick(action, state);

		if (btnId == SDL_BUTTON_RIGHT)
			center(lon,lat);
	}
}

/**
 * Handles this Globe's keyboard-shortcuts.
 * @param action	- pointer to an Action
 * @param state		- State that the ActionHandlers belong to
 */
void Globe::keyboardPress(Action* action, State* state)
{
	InteractiveSurface::keyboardPress(action, state);

	if (action->getDetails()->key.keysym.sym == Options::keyGeoToggleDetail)
		toggleDetail();

	if (action->getDetails()->key.keysym.sym == Options::keyGeoToggleRadar)
		toggleRadarLines();
}

/**
 * Gets a Polygon's texture & shade at a given coordinate.
 * @param lon		- longitude of the point
 * @param lat 		- latitude of the point
 * @param texture	- pointer to texture ID (returns -1 if polygon not found)
 * @param shade		- pointer to shade-level
 */
void Globe::getPolygonTextureAndShade(
		double lon,
		double lat,
		int* texture,
		int* shade) const
{
	// this is shade conversion from 0..31 levels of geoscape to battlescape levels 0..15
	const int worldshades[32]
	{
		0, 1, 1, 1, 2, 2, 2, 3,
		3, 3, 4, 4, 4, 5, 5, 5,
		6, 6, 6, 7, 7, 7, 8, 8,
		9, 9,10,11,12,13,14,15
	}; // terminator @ 25

	*shade = worldshades[CreateShadow::getShadowValue(
													0,
													Cord(0.,0.,1.),
													getSunDirection(lon,lat),
													0)];

	const Polygon* const poly (getPolygonAtCoord(lon,lat));
	if (poly != nullptr)
		*texture = poly->getPolyTexture();
	else
		*texture = -1;
}

/**
 * Gets a Polygon's texture at a given coordinate.
 * @param lon		- longitude of the point
 * @param lat 		- latitude of the point
 * @param texture	- pointer to texture ID (returns -1 if polygon not found)
 */
void Globe::getPolygonTexture(
		double lon,
		double lat,
		int* texture) const
{
	const Polygon* const poly (getPolygonAtCoord(lon,lat));
	if (poly != nullptr)
		*texture = poly->getPolyTexture();
	else
		*texture = -1;
}

/**
 * Gets a Polygon's shade at a given coordinate.
 * @param lon	- longitude of the point
 * @param lat 	- latitude of the point
 * @param shade	- pointer to shade-level
 */
void Globe::getPolygonShade(
		double lon,
		double lat,
		int* shade) const
{
	// this is shade conversion from 0..31 levels of geoscape to battlescape levels 0..15
	const int worldshades[32]
	{
		0, 1, 1, 1, 2, 2, 2, 3,
		3, 3, 4, 4, 4, 5, 5, 5,
		6, 6, 6, 7, 7, 7, 8, 8,
		9, 9,10,11,12,13,14,15
	}; // terminator @ 25

	*shade = worldshades[CreateShadow::getShadowValue(
													0,
													Cord(0.,0.,1.),
													getSunDirection(lon,lat),
													0)];
}

/**
 * Resizes this Globe.
 */
void Globe::resize()
{
	static const size_t SRF (4);
	Surface* const surfaces[SRF]
	{
		this,
		_markers,
		_countries,
		_radars
	};

	const int
		width  (Options::baseXGeoscape - 64), // TODO: '64' should be a constant.
		height (Options::baseYGeoscape);

	for (size_t
			i = 0;
			i != SRF;
			++i)
	{
		surfaces[i]->setWidth(width);
		surfaces[i]->setHeight(height);
		surfaces[i]->invalidate();
	}

	_clipper->Wxrig = static_cast<double>(width);
	_clipper->Wybot = static_cast<double>(height);
	_cenX = static_cast<Sint16>(width  / 2);
	_cenY = static_cast<Sint16>(height / 2);

	setupRadii(width, height);
	invalidate();
}

}
