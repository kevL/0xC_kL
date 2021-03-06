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

#include "Globe.h"

//#include <algorithm>

//#include "../fmath.h"

#include "GeoscapeState.h" // arcToRads

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
#include "../Interface/NumberText.h"
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
#include "../Savegame/Region.h"
#include "../Savegame/Target.h"
#include "../Savegame/TerrorSite.h"
#include "../Savegame/Ufo.h"
#include "../Savegame/Waypoint.h"


namespace OpenXcom
{

bool kL_reCenter = false;

constexpr int Globe::GLOBESHADE[32u];

const double
	Globe::ROTATE_LONGITUDE = 0.176,
	Globe::ROTATE_LATITUDE  = 0.176;

Uint8 // these are only fallbacks for Geography.rul->globe
	Globe::C_LBLBASE    = 100u,	// Palette::blockOffset(6)+4;	// stock 133;
	Globe::C_LBLCITY    = 167u,	// Palette::blockOffset(10)+7;	// stock 138
	Globe::C_LBLCOUNTRY = 227u,	// Palette::blockOffset(14)+3;	// stock 239
	Globe::C_LINE       = 162u,	// Palette::blockOffset(10)+2;	// light gray
//	Globe::C_RADAR1     =		// let base radars do its own thing in XuLine()
	Globe::C_RADAR2     = 150u,	// Palette::blockOffset(9)+6;	// brown
	Globe::C_FLIGHT     = 166u,	// Palette::blockOffset(10)+6;	// steel gray
	Globe::C_OCEAN      = 192u,	// Palette::blockOffset(12),	// blue ofc.
	Globe::C_BLACK      =  15u;


namespace
{

/// A helper struct for drawing the terminator and its shadow-fluxions (surf/noise).
struct Terminator
{
	/// array of shading gradient
	Sint16 shade_gradient[240u];
	/// size of x & y of noise surface
	const int random_surf_size;

	/**
	 * Function returning normal vector of sphere surface.
	 * @param ox - x cord of sphere center
	 * @param oy - y cord of sphere center
	 * @param r  - radius of sphere
	 * @param x  - cord of point where we getting this vector
	 * @param y  - cord of point where we getting this vector
	 * @return, normal vector of sphere surface
	 */
	static inline Cord circle_norm(
			double ox,
			double oy,
			double r,
			double x,
			double y)
	{
		const double
			limit  (r * r),
			normal (1. / r);

		Cord cord;
		cord.x = (x - ox);
		cord.y = (y - oy);

		const double t (cord.x * cord.x + cord.y * cord.y);
		if (limit > t)
		{
			cord.x *= normal;
			cord.y *= normal;
			cord.z = std::sqrt(limit - t) * normal;
			return cord;
		}

		cord.x =
		cord.y =
		cord.z = 0.;
		return cord;
	}


	/**
	 * Constructs a Terminator object and initializes it.
	 */
	Terminator()
		:
			random_surf_size(60)
	{
		// filling terminator-gradient LUT ...
		int j;
		for (size_t
				i  = 0u;
				i != 240u; // -120 .. +119
				++i)
		{
			j = static_cast<int>(i) - 120;

//			if      (j < -64) j = -16; // bladum -> no.
//			else if (j < -56) j = -15;
//			else if (j < -48) j = -14;
//			else if (j < -40) j = -13;
//			else if (j < -32) j = -12;
//			else if (j < -24) j = -11;
//			else if (j < -16) j = -10;
//			else if (j <  -8) j =  -9;
//
//			else if (j >  64) j =  16;
//			else if (j >  56) j =  15;
//			else if (j >  48) j =  14;
//			else if (j >  40) j =  13;
//			else if (j >  32) j =  12;
//			else if (j >  24) j =  11;
//			else if (j >  16) j =  10;
//			else if (j >   8) j =   9;

			if      (j < -66) j = -16; // stock ->
			else if (j < -48) j = -15;
			else if (j < -33) j = -14;
			else if (j < -22) j = -13;
			else if (j < -15) j = -12;
			else if (j < -11) j = -11;
			else if (j <  -9) j = -10;

			else if	(j > 120) j =  19;
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

			shade_gradient[i] = static_cast<Sint16>(j + 16);
		}
	}
} static_data;


///
struct Ocean
{
	///
	static inline void func(
			Uint8& dest,
			const int&, // whots this <- unused pars
			const int&, // whots this
			const int&, // whots this
			const int&) // whots this
	{
		dest = Globe::C_OCEAN;
	}
};


///
struct CreateTerminator
{
	///
	static inline Uint8 getTerminatorShade(
			const Uint8& dst,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise)
	{
		Cord cord (earth); // copy 'earth'
		cord -= sun;

		double sqr (cord.x * cord.x + cord.y * cord.y + cord.z * cord.z);

		sqr -= 2.;
		sqr *= 125.;

		if (sqr < -110.)
			sqr = -31.;
		else if (sqr > 120.)
			sqr = 50.;
		else
			sqr = static_cast<double>(static_data.shade_gradient[static_cast<size_t>(sqr) + 120u]);

		sqr -= static_cast<double>(noise);

		if (sqr > 0.)
		{
			const Uint8 d (dst & helper::ColorGroup);
			Uint8 val;

			if (sqr > 31.)
				val = 31u;
			else
				val = static_cast<Uint8>(sqr);

			if (   d == Globe::C_OCEAN
				|| d == Globe::C_OCEAN + 16u)
			{
				return static_cast<Uint8>(Globe::C_OCEAN + val); // this pixel is ocean
			}

			if (dst == 0u)
				return val; // this pixel is land

			const Uint8 e (static_cast<Uint8>(static_cast<unsigned>(dst) + (static_cast<unsigned>(val) / 3u))); // to be precise & explicit.

			if (e > (d + helper::ColorShade))
				return static_cast<Uint8>(d + helper::ColorShade);

			return static_cast<Uint8>(e);
		}

		const Uint8 d (dst & helper::ColorGroup);
		if (   d == Globe::C_OCEAN
			|| d == Globe::C_OCEAN + 16u)
		{
			return Globe::C_OCEAN; // this pixel is ocean
		}

		return dst; // this pixel is land
	}

	///
	static inline void func(
			Uint8& dst,
			const Cord& earth,
			const Cord& sun,
			const Sint16& noise,
			const int&) // whots this <- unused par
	{
		if (dst != 0u && AreSame(earth.z, 0.) == false)
			dst = getTerminatorShade(
									dst,
									earth,
									sun,
									noise);
		else
			dst = 0u;
	}
};

}


/**
 * Sets up the Globe with a specified size and position.
 * @param game     - pointer to the core Game
 * @param geoState - pointer to the GeoscapeState
 * @param cenX     - x-position of the center of the globe
 * @param cenY     - y-position of the center of the globe
 * @param width    - width in pixels
 * @param height   - height in pixels
 * @param x        - x-position in pixels (default 0)
 * @param y        - y-position in pixels (default 0)
 */
Globe::Globe(
		Game* const game,
		GeoscapeState* const geoState,
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
		_game(game),
		_geoState(geoState),
		_playSave(game->getSavedGame()),
		_globeRule(game->getRuleset()->getGlobe()),
		_rotLon(0.),
		_rotLat(0.),
		_hoverLon(0.),
		_hoverLat(0.),
		_cenX(static_cast<Sint16>(cenX)),
		_cenY(static_cast<Sint16>(cenY)),
		_forceRadars(false),
		_dragScrollActivated(false),
		_dragScrollPastPixelThreshold(false),
		_dragScrollStartTick(0u),
		_dragScrollX(0),
		_dragScrollY(0),
		_dragScrollLon(0.),
		_dragScrollLat(0.),
		_radius(0.),
		_dfZstep(0.),
		_debugType(DTG_COUNTRY),
		_radarDetail(game->getSavedGame()->getRadarDetail()),
		_globeDetail(game->getSavedGame()->isGlobeDetail()),
		_blink(true),
		_blinkVal(-1),
		_drawCrosshair(false),
		_crosshairLon(0.),
		_crosshairLat(0.)
{
	_srtTextures = new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("GlobeTextures")); //"TEXTURE.DAT"
	_srtMarkers  = new SurfaceSet(*_game->getResourcePack()->getSurfaceSet("GlobeMarkers"));

	_srfLayerDetail    = new Surface(width, height, x,y);
	_srfLayerCrosshair = new Surface(width, height, x,y); // TODO: Persist the crosshair if visible until play is unpaused or Globe is rotated.
	_srfLayerMarkers   = new Surface(width, height, x,y); // Perhaps if Globe is rotated the crosshair should re-locate to next coordinates.
	_srfLayerRadars    = new Surface(width, height, x,y);

	_clipper = new FastLineClip(
							x, x + width,
							y, y + height);

	_timerBlink = new Timer(200u);
	_timerBlink->onTimer(static_cast<SurfaceHandler>(&Globe::blink));
	_timerBlink->start();

	_timerRot = new Timer(static_cast<Uint32>(Options::geoScrollSpeed));
	_timerRot->onTimer(static_cast<SurfaceHandler>(&Globe::rotate));

	_cenLon = _playSave->getGlobeLongitude();
	_cenLat = _playSave->getGlobeLatitude();

	setupRadii(width, height);

	_terminatorFluxions.resize(static_cast<size_t>(static_data.random_surf_size * static_data.random_surf_size));
	for (size_t
			i  = 0u;
			i != _terminatorFluxions.size();
			++i)
	{
		_terminatorFluxions[i] = static_cast<Sint16>(RNG::seedless(0,4));
	}

	cachePolygons();

	_flightData = new NumberText(27,5);
}

/**
 * Deletes the contained surfaces, timers, polygons, etc.
 */
Globe::~Globe()
{
	delete _srtTextures;
	delete _srtMarkers;
	delete _timerBlink;
	delete _timerRot;
	delete _srfLayerDetail;
	delete _srfLayerCrosshair;
	delete _srfLayerMarkers;
	delete _srfLayerRadars;
	delete _clipper;

	for (std::list<Polygon*>::const_iterator
			i  = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
		delete *i;

	delete _flightData;
}

/**
 * Converts a polar-point into a cartesian-point for mapping a Polygon onto the
 * 3D-looking Globe.
 * @param lon - longitude of the polar point
 * @param lat - latitude of the polar point
 * @param x   - pointer to the output x-position
 * @param y   - pointer to the output y-position
 */
void Globe::polarToCart( // Orthographic projection
		double lon,
		double lat,
		Sint16* x,
		Sint16* y) const
{
	*x = static_cast<Sint16>(_cenX + static_cast<int>(std::floor(_radius * std::cos(lat) * std::sin(lon - _cenLon))));
	*y = static_cast<Sint16>(_cenY + static_cast<int>(std::floor(_radius * (std::cos(_cenLat) * std::sin(lat)
													- std::sin(_cenLat) * std::cos(lat) * std::cos(lon - _cenLon)))));
	// casting 'irregularities' irritate
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
 * @param x   - x-position of the cartesian point
 * @param y   - y-position of the cartesian point
 * @param lon - pointer to the output longitude
 * @param lat - pointer to the output latitude
 */
void Globe::cartToPolar( // Orthographic Projection
		Sint16 x,
		Sint16 y,
		double* lon,
		double* lat) const
{
	x = static_cast<Sint16>(x - _cenX);
	y = static_cast<Sint16>(y - _cenY);

	const double
		rho (std::sqrt(static_cast<double>(x * x + y * y))),
		c   (std::asin(rho / static_cast<double>(_radius)));

	if (AreSame(rho, 0.) == true)
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

	while (*lon  < 0.) // keep between 0 and 2PI
		   *lon += M_PI * 2.;

	while (*lon  > M_PI * 2.)
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
	if (_gZ == 0u) return true; // bypass if earthradius=0

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
//	double c =  cos(_cenLat) * cos(lat) * cos(lon - _cenLon) + sin(_cenLat) * sin(lat);
//	tan(lat) = -cos(_cenLat) * cos(lon - _cenLon) / sin(_cenLat);
	return std::atan(-std::cos(_cenLat) * std::cos(lon - _cenLon) / std::sin(_cenLat));
} */

/**
 * Gets the Polygon at specified coordinates.
 * @param lon - longitude
 * @param lat - latitude
 * @return, pointer to the Polygon
 */
Polygon* Globe::getPolygonAtCoord( // private.
		double lon,
		double lat) const
{
    double
		cosLat (cos(lat)),
		sinLat (sin(lat)),

		x,y,
		x2,y2,
		cLat,cLon;

	const double discard (0.75);
	bool
		bypass,
		isOdd;
	size_t verts;

	for (std::list<Polygon*>::const_iterator
			i  = _globeRule->getPolygons()->begin();
			i != _globeRule->getPolygons()->end();
			++i)
	{
		bypass = false;

		verts = (*i)->getPoints();
		for (size_t
				j  = 0u;
				j != verts;
				++j)
		{
			if (cosLat * cos((*i)->getLatitude(j)) * cos((*i)->getLongitude(j) - lon)
			  + sinLat * sin((*i)->getLatitude(j)) < discard)
			{
				bypass = true; // discarded
				break;
			}
		}

		if (bypass == false)
		{
			isOdd = false;

			cLat = (*i)->getLatitude(0u); // initial point
			cLon = (*i)->getLongitude(0u);

			x = cos(cLat) * sin(cLon - lon);
			y = cosLat * sin(cLat) - sinLat * cos(cLat) * cos(cLon - lon);

			for (size_t
					j  = 0u;
					j != (*i)->getPoints();
					++j)
			{
				const size_t id ((j + 1u) % (*i)->getPoints()); // index of next point in poly
				cLat = (*i)->getLatitude(id);
				cLon = (*i)->getLongitude(id);

				x2 = cos(cLat) * sin(cLon - lon);
				y2 = cosLat * sin(cLat) - sinLat * cos(cLat) * cos(cLon - lon);

				if (((y > 0.) != (y2 > 0.)) && (0. < (x2 - x) * (0. - y) / (y2 - y) + x))
					isOdd = !isOdd;

				x = x2;
				y = y2;
			}

			if (isOdd == true) return *i;
		}
	}
	return nullptr;
}

/**
 * Checks if a polar-point is inside a certain Polygon.
 * @param lon  - longitude of the point
 * @param lat  - latitude of the point
 * @param poly - pointer to the polygon
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
	if (_drawCrosshair == true)
		clearCrosshair();

	_rotLon = -ROTATE_LONGITUDE;
	if (_timerRot->isRunning() == false)
		_timerRot->start();
}

/**
 * Sets a rightwards rotation speed and starts the timer.
 */
void Globe::rotateRight()
{
	if (_drawCrosshair == true)
		clearCrosshair();

	_rotLon = ROTATE_LONGITUDE;
	if (_timerRot->isRunning() == false)
		_timerRot->start();
}

/**
 * Sets a upwards rotation speed and starts the timer.
 */
void Globe::rotateUp()
{
	if (_drawCrosshair == true)
		clearCrosshair();

	_rotLat = -ROTATE_LATITUDE;
	if (_timerRot->isRunning() == false)
		_timerRot->start();
}

/**
 * Sets a downwards rotation speed and starts the timer.
 */
void Globe::rotateDown()
{
	if (_drawCrosshair == true)
		clearCrosshair();

	_rotLat = ROTATE_LATITUDE;
	if (_timerRot->isRunning() == false)
		_timerRot->start();
}

/**
 * Stops this Globe's rotation-speed and Timer.
 * @note If a message-window displays while drag-scrolling '_dragScrollActivated' needs
 * to be cleared explicitly here.
 */
void Globe::rotateStop()
{
	_timerRot->stop();

	_rotLon =
	_rotLat = 0.;
	_dragScrollActivated = false; // TODO: Perhaps an ActionHandler needs to be reset and/or the rodentState-buttons cleared.
}

/**
 * Stops this Globe's longitudinal rotation-speed and Timer.
 */
void Globe::rotateStopLon()
{
	_rotLon = 0.;
	if (AreSame(_rotLat, 0.) == true)
		_timerRot->stop();
}

/**
 * Stops this Globe's latitudinal rotation-speed and Timer.
 */
void Globe::rotateStopLat()
{
	_rotLat = 0.;
	if (AreSame(_rotLon, 0.) == true)
		_timerRot->stop();
}

/**
 * Sets up the perceived radii of the Earth for the several zoom-levels.
 * @param width  - the new width of the Globe
 * @param height - the new height of the Globe
 */
void Globe::setupRadii( // private.
		int width,
		int height)
{
	_radii.clear();

	const double height_d (static_cast<double>(height));

	// These are the globe-zoom magnifications stored as a <vector> of 7 (doubles).
	_radii.push_back(0.04 * height_d); //  0
	_radii.push_back(0.08 * height_d); //  1
	_radii.push_back(0.16 * height_d); //  2
	_radii.push_back(0.24 * height_d); //  3
	_radii.push_back(0.32 * height_d); //  4
	_radii.push_back(0.48 * height_d); //  5 <- this is max zoomout per stock code
	_radii.push_back(0.64 * height_d); //  6
	_radii.push_back(0.96 * height_d); //  7
	_radii.push_back(1.28 * height_d); //  8
	_radii.push_back(2.24 * height_d); //  9
	_radii.push_back(3.52 * height_d); // 10

	setGz(_playSave->getGlobeZoom());

	_zBorders       = 2u; // these are the levels at which detail appears ->
	_zCities        = 3u;
	_zBaseLabels    = 4u;
	_zCountryLabels = 6u;
	_zCityLabels    = 8u;

	_dfZstep = (_radii[_radii.size() - 1u] - _radii[0u]) / 20.; // used to be 12.2 (was faster z)

	_earthData.resize(_radii.size());	// data for drawing sun-shadow.
	for (size_t							// filling normal field for each radius
			i  = 0u;
			i != _radii.size();
			++i)
	{
		_earthData[i].resize(static_cast<size_t>(width * height));
		for (size_t
				j  = 0u;
				j != static_cast<size_t>(height);
				++j)
			for (size_t
					k  = 0u;
					k != static_cast<size_t>(width);
					++k)
				_earthData[i]
						  [static_cast<size_t>(width) * j + k] = static_data.circle_norm(
																					static_cast<double>(width) / 2.,
																					height_d / 2.,
																					_radii[i],
																					static_cast<double>(k) + 0.5,
																					static_cast<double>(j) + 0.5);
	}
}

/**
 * Changes this Globe's current zoom-level.
 * @note Zoomed-out is "0".
 * @param level - zoom-level
 */
void Globe::setGz(size_t gZ) // private.
{
	rotateStop();

//	_texOffset = (2u - (_gZ >> 1u)) * (_srtTextures->getTotalFrames() / 3u);

	switch (_gZ = gZ)
	{
		default:
		case  0:							// far out (earthradius=0)
		case  1:
		case  2:
		case  3:
		case  4:
		case  5:
		case  6: _texOffset = 26u; break;
		case  7:							// mid
		case  8: _texOffset = 13u; break;
		case  9:							// close up
		case 10: _texOffset =  0u;
	}
	// NOTE: The above^ relies on "GlobeTextures"/"WORLD.DAT" being divided up
	// as 13 textures with 3 zoom-levels apiece. Globe-drawing is basically
	// hard-coded to see things that way.

	_radius = _radii[_gZ];

	_playSave->setGlobeZoom(_gZ);
	_geoState->updateZoomText();

	_redraw = true;
}

/**
 * Gets the Globe's current zoom-level.
 * @return, zoom-level
 */
size_t Globe::getZoom() const
{
	return _gZ;
}

/**
 * Gets the number of zoom-levels available.
 * @return, number of zoom-levels
 */
size_t Globe::getZoomLevels() const
{
	return _radii.size();
}

/**
 * Increases the zoom-level of this Globe.
 */
void Globe::zoomIn()
{
	if (_gZ < _radii.size() - 1u)
		setGz(_gZ + 1u);
}

/**
 * Decreases the zoom-level of this Globe.
 */
void Globe::zoomOut()
{
	if (_gZ > 0u)
		setGz(_gZ - 1u);
}

/**
 * Zooms this Globe out as far as possible.
 *
void Globe::zoomMin()
{
	if (_zoom > 0u) setZoom(0u);
} */

/**
 * Zooms this Globe in as close as possible.
 *
void Globe::zoomMax()
{
	if (_zoom < _radii.size() - 1u) setZoom(_radii.size() - 1u);
} */

/**
 * Zooms this Globe smoothly into a Dogfight.
 * @return, true if zoom has finished
 */
bool Globe::zoomDogfightIn()
{
	const size_t dfZ (_radii.size() - 1u);

	if (_gZ < dfZ)
	{
		const double radius (_radius);

		if (radius + _dfZstep >= _radii[dfZ])
			setGz(dfZ);
		else
		{
			if (radius + _dfZstep >= _radii[_gZ + 1u])
				++_gZ;

			setGz(_gZ);
			_radius = radius + _dfZstep;
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
	const size_t dfZpre (_playSave->getDfZoom());

	if (_gZ > dfZpre)
	{
		const double radius (_radius);

		if (radius - _dfZstep <= _radii[dfZpre])
			setGz(dfZpre);
		else
		{
			if (radius - _dfZstep <= _radii[_gZ - 1u])
				--_gZ;

			setGz(_gZ);
			_radius = radius - _dfZstep;
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
	_playSave->setGlobeLongitude(_cenLon = lon);
	_playSave->setGlobeLatitude(_cenLat = lat);

	_redraw = true;
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
 * Toggles on/off drawing detail on the Globe.
 * @note Country and city details are shown only when zoomed in.
 * @return, true if toggled on
 */
bool Globe::toggleDetail()
{
	if ((_globeDetail = _playSave->toggleGlobeDetail()) == true)
		drawDetail();
	else
		_srfLayerDetail->clear();

	return _globeDetail;
}

/**
 * Changes the level of radar-details shown on the Globe.
 * @return, value of the radar-detail
 */
GlobeRadarDetail Globe::changeRadars()
{
	switch (_radarDetail)
	{
		case GRD_NONE:
			_playSave->setRadarDetail(_radarDetail = GRD_CRAFT);
			drawRadars();
			break;

		case GRD_CRAFT:
			_playSave->setRadarDetail(_radarDetail = GRD_BASE);
			drawRadars();
			break;

		case GRD_BASE:
			_playSave->setRadarDetail(_radarDetail = GRD_ALL);
			drawRadars();
			break;

		case GRD_ALL:
			_playSave->setRadarDetail(_radarDetail = GRD_NONE);
			_srfLayerRadars->clear();
	}

	return _radarDetail;
}

/**
 * Checks if a certain target is near a certain cartesian-point.
 * @param target - pointer to Target
 * @param x      - x-coordinate of point
 * @param y      - y-coordinate of point
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
 * @param x             - x-coordinate of point
 * @param y             - y-coordinate of point
 * @param flightTargets - true to get targets for Craft only (default true)
 * @return, vector of pointers to Targets
 */
std::vector<Target*> Globe::getTargets(
		int x,
		int y,
		bool flightTargets) const
{
	std::vector<Target*> targets;

	if (flightTargets == false)
		for (std::vector<Base*>::const_iterator
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
			if ((*i)->isBasePlaced() == true)
			{
				if (targetNear(*i, x,y))
					targets.push_back(*i);

				for (std::vector<Craft*>::const_iterator
						j  = (*i)->getCrafts()->begin();
						j != (*i)->getCrafts()->end();
						++j)
					if ((*j)->getCraftStatus() == CS_OUT && targetNear(*j, x,y))
						targets.push_back(*j);
			}

	for (std::vector<Ufo*>::const_iterator
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			++i)
		if ((*i)->getDetected() == true && targetNear(*i, x,y) == true)
			targets.push_back(*i);

	for (std::vector<Waypoint*>::const_iterator
			i  = _playSave->getWaypoints()->begin();
			i != _playSave->getWaypoints()->end();
			++i)
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);

	for (std::vector<TerrorSite*>::const_iterator
			i  = _playSave->getTerrorSites()->begin();
			i != _playSave->getTerrorSites()->end();
			++i)
		if (targetNear(*i, x,y) == true)
			targets.push_back(*i);

	for (std::vector<AlienBase*>::const_iterator
			i  = _playSave->getAlienBases()->begin();
			i != _playSave->getAlienBases()->end();
			++i)
		if ((*i)->isDetected() == true && targetNear(*i, x,y) == true)
			targets.push_back(*i);

	return targets;
}

/**
 * Caches a set of Polygons.
 * @note Takes care of pre-calculating all the Polygons currently visible on this
 * Globe and caching them so they only need to be recalculated when this Globe
 * is actually moved.
 */
void Globe::cachePolygons() // private.
{
	for (std::list<Polygon*>::const_iterator
			i  = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
	{
		delete *i;
	}
	_cacheLand.clear();

	std::list<Polygon*>* const allPolygons (_globeRule->getPolygons());
	for (std::list<Polygon*>::const_iterator
			i  = allPolygons->begin();
			i != allPolygons->end();
			++i)
	{
		double
			closest (0.),
			furthest (0.),
			z;

		for (size_t
				j  = 0u;
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
				j  = 0u;
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

		_cacheLand.push_back(poly);
	}
}

/**
 * Replaces a specified quantity of colors in the palette of this Globe.
 * @param colors     - pointer to the set of colors
 * @param firstcolor - offset of the first color to replace
 * @param ncolors    - amount of colors to replace
 */
void Globe::setPalette(
		SDL_Color* const colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_flightData->setPalette(getPalette());

	_srtTextures->setPalette(colors, firstcolor, ncolors);
	_srtMarkers ->setPalette(colors, firstcolor, ncolors);

	_srfLayerDetail   ->setPalette(colors, firstcolor, ncolors);
	_srfLayerCrosshair->setPalette(colors, firstcolor, ncolors);
	_srfLayerMarkers  ->setPalette(colors, firstcolor, ncolors);
	_srfLayerRadars   ->setPalette(colors, firstcolor, ncolors);
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
			_playSave->getGlobeLongitude(),
			_playSave->getGlobeLatitude());
	}

	_timerBlink->think(nullptr, this);
	_timerRot  ->think(nullptr, this);
}

/**
 * Makes this Globe's markers blink.
 */
void Globe::blink()
{
	if (_blink == true)
	{
		_blinkVal = -_blinkVal; // can't use static because, reload.

		for (std::map<int, Surface*>::const_iterator
				i  = _srtMarkers->getFrames()->begin();
				i != _srtMarkers->getFrames()->end();
			  ++i)
		{
			if (i->first != GLM_CITY)
				i->second->offset(_blinkVal);
		}
		drawMarkers();	// NOTE: There's no need to clear '_srfLayerMarkers' for blink().
	}					// Doing so would erase any intercept-markers anyway because
}						// they aren't handled by drawMarkers() but by drawFlights().

/**
 * Toggles the blinking.
 */
void Globe::toggleBlink()
{
	_blink = !_blink;
}

/**
 * Rotates this Globe by timer-ticks.
 */
void Globe::rotate()
{
	_cenLon += _rotLon * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_gZ + 1u);
	_cenLat += _rotLat * (static_cast<double>(110 - Options::geoScrollSpeed) / 100.) / static_cast<double>(_gZ + 1u);

	_playSave->setGlobeLongitude(_cenLon);
	_playSave->setGlobeLatitude(_cenLat);

	_redraw = true;
}

/**
 * Draws this Globe blit by blit.
 */
void Globe::draw()
{
	if (_redraw == true) cachePolygons();

	Surface::draw();

	_srfLayerRadars   ->clear();
	_srfLayerMarkers  ->clear();
	_srfLayerDetail   ->clear();
	_srfLayerCrosshair->clear();

//	if (_gZ != 0u) // bypass if earthradius=0
//	{
	drawOcean();
	drawLand();
	drawBevel();
	drawRadars();	// '_srfLayerRadars'
	drawFlights();	// '_srfLayerRadars' [also draws intercept-markers]
	drawTerminus();
	drawMarkers();	// '_srfLayerMarkers'
	drawDetail();	// '_srfLayerDetail'

	if (_drawCrosshair == true) drawCrosshair();
//	}
}

/**
 * Renders this Globe as a blue primordial ocean.
 */
void Globe::drawOcean()
{
	lock();
	drawCircle(
			static_cast<Sint16>(_cenX + 1),
			_cenY,
			static_cast<Sint16>(_radius + 20.),
			C_OCEAN);
	unlock();
}

/**
 * Renders the land with textured Polygons.
 */
void Globe::drawLand()
{
	Sint16
		x[4u],y[4u];

	for (std::list<Polygon*>::const_iterator
			i  = _cacheLand.begin();
			i != _cacheLand.end();
			++i)
	{
		for (size_t // Convert coordinates
				j  = 0u;
				j != (*i)->getPoints();
				++j)
		{
			x[j] = (*i)->getX(j);
			y[j] = (*i)->getY(j);
		}

		drawTexturedPolygon( // apply textures according to zoom-level
						x,y,
						(*i)->getPoints(),
						_srtTextures->getFrame(static_cast<int>((*i)->getPolyTexture() + _texOffset)),
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
		w (getWidth()),
		h (getHeight());

	for (int
			y  = 0;
			y != h;
			++y)
	{
		for (int
				x  = 0;
				x != w;
				++x)
		{
			if (getPixelColor(x,y) == C_OCEAN
				&& (p = getPixelColor(x - 1, y - 1)) != C_OCEAN
				&& p != C_BLACK)
			{
				setPixelColor(x,y, C_BLACK);
			}
		}
	}
}

/**
 * Gets direction of the sun from a point on this Globe.
 * @param lon - longitude of position
 * @param lat - latitude of position
 * @return, position of sun
 */
Cord Globe::getSunDirection( // private.
		double lon,
		double lat) const
{
	double sun;

	const GameTime* const gt (_playSave->getTime());

	if (Options::globeSeasons == true)
	{
		static const int
			days1[13u] { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
			days2[13u] { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

		const int
			year  (gt->getYear()),
			month (gt->getMonth() - 1),
			day   (gt->getDay() - 1);

		const double
			sec (static_cast<double>( // day-fraction is also taken into account
								  gt->getHour() * 3600
								+ gt->getMinute() * 60
								+ gt->getSecond())
							/ 86400.); // secs per day.

		double today;
		if (    year %   4 == 0 // spring equinox (start of astronomical year)
			&& (year % 100 != 0 || year % 400 == 0))
		{
			today = (static_cast<double>(days2[month] + day) + sec) / 366. - 0.219;
		}
		else
			today = (static_cast<double>(days1[month] + day) + sec) / 365. - 0.219;

		if (today < 0.)
			today += 1.;

		sun = -0.261 * std::sin(today * M_PI * 2.);
	}
	else
		sun = 0.;

	const double rot (gt->getDaylight() * M_PI * 2.);
	Cord sun_direction (std::cos(rot + lon),
						std::sin(rot + lon) * -std::sin(lat),
						std::sin(rot + lon) *  std::cos(lat));

	if (sun > 0.)
		 sun_direction *= 1. - sun;
	else
		 sun_direction *= 1. + sun;

	Cord pole (0.,
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
 * Draws the radar-circles of player's Bases and Craft on this Globe.
 */
void Globe::drawRadars()
{
	_srfLayerRadars->lock();
	if (_forceRadars == true) //&& Options::globeAllRadarsOnBaseBuild == true // placing a Base.
	{
		int range;

		const Ruleset* const rules (_game->getRuleset());
		const std::vector<std::string>& allFacilities (rules->getBaseFacilitiesList());
		for (std::vector<std::string>::const_iterator
				i  = allFacilities.begin();
				i != allFacilities.end();
				++i)
		{
			if ((range = rules->getBaseFacility(*i)->getRadarRange()) != 0)
				drawGlobeCircle(
							_hoverLat,
							_hoverLon,
							static_cast<double>(range) * arcToRads,
							48);
//							C_RADAR1);
		}
	}

	if (_radarDetail != GRD_NONE)
	{
		int
			range,
			rangeFarthest;
		double
			lon,lat;

		for (std::vector<Base*>::const_iterator
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
		{
			if ((*i)->isBasePlaced() == true)
			{
				switch (_radarDetail)
				{
					case GRD_ALL:
					case GRD_BASE: // NOTE: Show Base-radars can also show Craft-radars.
						rangeFarthest = 0;
						lat = (*i)->getLatitude();
						lon = (*i)->getLongitude();

						for (std::vector<BaseFacility*>::const_iterator
								j  = (*i)->getFacilities()->begin();
								j != (*i)->getFacilities()->end();
								++j)
						{
							if ((*j)->buildFinished() == true)
							{
								range = (*j)->getRules()->getRadarRange();
								if (_radarDetail == GRD_ALL)
								{
									if (range != 0)
										drawGlobeCircle( // Base radars.
													lat,lon,
													static_cast<double>(range) * arcToRads,
													64);
//													C_RADAR1);
								}
								else if (range > rangeFarthest)
									rangeFarthest = range;
							}
						}

						if (rangeFarthest != 0)
							drawGlobeCircle( // largest Base radar.
										lat,lon,
										static_cast<double>(rangeFarthest) * arcToRads,
										64);
//										C_RADAR1);
						// no break;

					case GRD_CRAFT:
						for (std::vector<Craft*>::const_iterator
								j  = (*i)->getCrafts()->begin();
								j != (*i)->getCrafts()->end();
								++j)
						{
							if ((*j)->getCraftStatus() == CS_OUT
								&& (*j)->hasLeftGround() == true
								&& (range = (*j)->getRules()->getRangeRadar()) != 0)
							{
								drawGlobeCircle( // Craft radars.
											(*j)->getLatitude(),
											(*j)->getLongitude(),
											static_cast<double>(range) * arcToRads,
											48,
											C_RADAR2);
							}
						}
				}
			}
		}
	}
	_srfLayerRadars->unlock();
}

/**
 * Draws a range circle.
 * @param lat    -
 * @param lon    -
 * @param radius -
 * @param segs   -
 * @param color  - (default 0)
 */
void Globe::drawGlobeCircle( // private.
		double lat,
		double lon,
		double radius,
		int segs,
		Uint8 color)
{
	double
		x,y,
		x2 (0.),
		y2 (0.),
		lat1,
		lon1,
		segs_d (static_cast<double>(segs));

	for (double // 48 segments in circle
			az  = 0.;
			az <= M_PI * 2. + 0.01;
			az += M_PI * 2. / segs_d)
	{
		// calculating sphere-projected circle
		lat1 = asin(std::sin(lat) * std::cos(radius) + std::cos(lat) * std::sin(radius) * std::cos(az));
		lon1 = lon + std::atan2(
						std::sin(az) * std::sin(radius) * std::cos(lat),
						std::cos(radius) - std::sin(lat) * std::sin(lat1));
		polarToCart(
				lon1,lat1,
				&x,&y);

		if (AreSame(az, 0.) == false // first vertex is for initialization only
			&& pointBack(lon1,lat1) == false)
		{
			XuLine(
				_srfLayerRadars,
				this,
				x,y,
				x2,y2,
				6,
				color);
		}

		x2 = x;
		y2 = y;
	}
}

/**
 * Draws the flight-paths of player's Craft flying around this Globe.
 */
void Globe::drawFlights()
{
	if (Options::globeFlightPaths == true)
	{
		_srfLayerRadars->lock();
		for (std::vector<Base*>::const_iterator
				i  = _playSave->getBases()->begin();
				i != _playSave->getBases()->end();
				++i)
		{
			for (std::vector<Craft*>::const_iterator
					j  = (*i)->getCrafts()->begin();
					j != (*i)->getCrafts()->end();
					++j)
			{
				if ((*j)->getCraftStatus() == CS_OUT
					&& (*j)->getTarget() != nullptr
					&& (*j)->inDogfight() == false)
				{
					const double
						lon1 ((*j)->getLongitude()),
						lat1 ((*j)->getLatitude()),
						lon2 ((*j)->getTarget()->getLongitude()),
						lat2 ((*j)->getTarget()->getLatitude());
					drawPath(
							_srfLayerRadars,
							lon1,lat1,
							lon2,lat2);

					if (dynamic_cast<const Ufo*>((*j)->getTarget()) != nullptr)
					{
						double lon3 ((*j)->getMeetLongitude()); // radians all the way down ->
						if (std::signbit(lon3) == true) lon3 += M_PI * 2.;

						const double
							lon_d (std::fabs(lon3 - lon2)),
							lat3 ((*j)->getMeetLatitude());

						if ((lon_d > 0.005 && lon_d < M_PI * 2. - 0.005) // account for Greenwich meridian.
							|| std::fabs(lat3 - lat2) > 0.005)
						{
							drawPath(
									_srfLayerRadars,
									lon1,lat1,
									lon3,lat3);
							drawInterceptMarker(lon3,lat3);
						}
					}
				}
			}
		}
		_srfLayerRadars->unlock();
	}
}

/**
 * Draws flight paths.
 * @param surface - pointer to a Surface
 * @param lon1    -
 * @param lat1    -
 * @param lon2    -
 * @param lat2    -
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
			i  = 0;
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
 * Draws a XuLine!
 * @param surface -
 * @param src     -
 * @param x1      -
 * @param y1      -
 * @param x2      -
 * @param y2      -
 * @param shade   -
 * @param color   - (default 0)
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
	if (_clipper->LineClip(
						&x1,&y1,
						&x2,&y2) == 1) // not empty line
	{
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
		else if (AreSame(delta_y, 0.) == true)
			SY = 0;
		else
			SY = 1;

		if (x2 < x1)
			SX = -1;
		else if (AreSame(delta_x, 0.) == true)
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
			if (tcol != 0u)
			{
				switch (color)
				{
					case 0u:
					{
						const Uint8 colorBlock (tcol & helper::ColorGroup);
						if (   colorBlock == C_OCEAN
							|| colorBlock == C_OCEAN + 16u)
						{
							tcol = static_cast<Uint8>(C_OCEAN + shade + 8); // this pixel is Ocean
						}
						else // this pixel is land
						{
							const Uint8 colorShaded (static_cast<Uint8>(shade + static_cast<int>(tcol)));
							if (colorShaded > colorBlock + helper::ColorShade)
								tcol = static_cast<Uint8>(colorBlock + helper::ColorShade);
							else
								tcol = colorShaded;
						}
						break;
					}

					default:
						tcol = color; // flight path or craft radar
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

		Surface* const marker (_srtMarkers->getFrame(GLM_WAYPOINT));
		marker->setX(x - 1);
		marker->setY(y - 1);
		marker->blit(_srfLayerMarkers); // NOTE: These will blink.
	}
}

/**
 * Shadows the earth and adds terminator-fluxions (surf/noise) according to the
 * sun's direction.
 */
void Globe::drawTerminus()
{
	ShaderMove<Cord> earth (ShaderMove<Cord>(
										_earthData[_gZ],
										getWidth(),
										getHeight()));
	ShaderRepeat<Sint16> noise (ShaderRepeat<Sint16>(
												_terminatorFluxions,
												static_data.random_surf_size,
												static_data.random_surf_size));

	earth.setMove(
			_cenX - (getWidth()  >> 1u),
			_cenY - (getHeight() >> 1u));

	lock();
	ShaderDraw<CreateTerminator>(
							ShaderSurface(this),
							earth,
							ShaderScalar(getSunDirection(_cenLon, _cenLat)),
							noise);
	unlock();
}

/**
 * Draws the markers of all the various things going on around the world except
 * Cities - for which see drawDetail() above.
 */
void Globe::drawMarkers()
{
	for (std::vector<Base*>::const_iterator			// Draw the Base markers
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
		drawTarget(*i, _srfLayerMarkers);

	for (std::vector<Waypoint*>::const_iterator		// Draw the Waypoint markers
			i  = _playSave->getWaypoints()->begin();
			i != _playSave->getWaypoints()->end();
			++i)
		drawTarget(*i, _srfLayerMarkers);

	for (std::vector<TerrorSite*>::const_iterator	// Draw the TerrorSite markers
			i  = _playSave->getTerrorSites()->begin();
			i != _playSave->getTerrorSites()->end();
			++i)
		drawTarget(*i, _srfLayerMarkers);

	for (std::vector<AlienBase*>::const_iterator	// Draw the AlienBase markers
			i  = _playSave->getAlienBases()->begin();
			i != _playSave->getAlienBases()->end();
			++i)
		drawTarget(*i, _srfLayerMarkers);

	for (std::vector<Ufo*>::const_iterator			// Draw the Ufo markers
			i  = _playSave->getUfos()->begin();
			i != _playSave->getUfos()->end();
			++i)
		drawTarget(*i, _srfLayerMarkers);

	for (std::vector<Base*>::const_iterator			// Draw the Craft markers
			i  = _playSave->getBases()->begin();
			i != _playSave->getBases()->end();
			++i)
		for (std::vector<Craft*>::const_iterator
				j  = (*i)->getCrafts()->begin();
				j != (*i)->getCrafts()->end();
				++j)
			drawTarget(*j, _srfLayerMarkers);
}

/**
 * Draws the marker for a specified Target on this Globe.
 * @param target   - pointer to the target
 * @param srfGlobe - pointer to the globe's Surface
 */
void Globe::drawTarget( // private.
		const Target* const target,
		Surface* const srfGlobe)
{
	const int id (target->getMarker());
	if (id != -1)
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

			Surface* const marker (_srtMarkers->getFrame(id));
			marker->setX(x - 1);
			marker->setY(y - 1);
			marker->blit(srfGlobe);

			switch (id)
			{
				case Globe::GLM_CRAFT:
				case Globe::GLM_UFO_FLYING:
				{
					unsigned data; // "headingInt[1 digit] - altitudeInt[1 digit] - 0 - speed[4 digits]"

					// TODO: Stack flight-data vertically for multiple Craft in the same Dogfight.

					// TODO: Deal with multiple Craft patrolling over the same Crashed/Landed UFO or TerrorSite.

					const Craft* const craft (dynamic_cast<const Craft*>(target));
					bool vis (true);

					if (craft != nullptr)
					{
						if (craft->inDogfight() == false)
						{
							_flightData->setY(y - 7);
							_flightData->setColor(C_GREEN);

							data  = craft->getHeadingInt()  * 1000000u;
							data += craft->getAltitudeInt() * 100000u;
							data += static_cast<unsigned>(craft->getSpeed());
						}
						else
							vis = false;
					}
					else //if (ufo != nullptr)
					{
						const Ufo* const ufo (dynamic_cast<const Ufo*>(target));

						_flightData->setY(y + 4);
						_flightData->setColor(C_RED);

						data  = ufo->getHeadingInt()  * 1000000u;
						data += ufo->getAltitudeInt() * 100000u;
						data += static_cast<unsigned>(ufo->getSpeed());
					}

					if (vis == true)
					{
						_flightData->setX(x);
						_flightData->setValue(data);
						_flightData->draw();
						_flightData->blit(srfGlobe);
					}
				}
			}
		}
	}
}

/**
 * Draws the details of the countries on this Globe based on the current
 * zoom-level.
 */
void Globe::drawDetail()
{
	if (_globeDetail == true)
	{
		double
			lon,lat;

		if (_gZ >= _zBorders) // draw the Country borders
		{
			double
				lon1,lat1;
			Sint16
				x[2u],y[2u];

			_srfLayerDetail->lock();
			for (std::list<Polyline*>::const_iterator
					i  = _globeRule->getPolylines()->begin();
					i != _globeRule->getPolylines()->end();
					++i)
			{
				for (size_t
						j  = 0u;
						j != (*i)->getPoints() - 1u;
						++j)
				{
					lon  = (*i)->getLongitude(j),
					lat  = (*i)->getLatitude(j);
					lon1 = (*i)->getLongitude(j + 1u),
					lat1 = (*i)->getLatitude(j + 1u);

					if (   pointBack(lon, lat)  == false
						&& pointBack(lon1,lat1) == false)
					{
						polarToCart(
								lon,lat,
								&x[0u],&y[0u]);
						polarToCart(
								lon1,lat1,
								&x[1u],&y[1u]);

						_srfLayerDetail->drawLine(
												x[0u],y[0u],
												x[1u],y[1u],
												C_LINE);
					}
				}
			}
			_srfLayerDetail->unlock();
		}

		if (_gZ >= _zCities) // draw the City markers
		{
			RuleRegion* regionRule;
			for (std::vector<Region*>::const_iterator
					i  = _playSave->getRegions()->begin();
					i != _playSave->getRegions()->end();
					++i)
			{
				regionRule = const_cast<RuleRegion*>((*i)->getRules()); // strip const for iteration.
				for (std::vector<RuleCity*>::const_iterator
						j  = regionRule->getCities().begin();
						j != regionRule->getCities().end();
						++j)
				{
					drawTarget(*j, _srfLayerDetail);
				}
			}
		}

		Sint16
			x,y;

		Text* const label (new Text(100,9));

		label->setPalette(getPalette());
		label->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
		label->setAlign(ALIGN_CENTER);

		if (_gZ >= _zCountryLabels)
		{
			label->setColor(C_LBLCOUNTRY); // draw the Country labels

			for (std::vector<Country*>::const_iterator
					i  = _playSave->getCountries()->begin();
					i != _playSave->getCountries()->end();
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

					label->blit(_srfLayerDetail);
				}
			}
		}

		if (_gZ >= _zCityLabels) // draw the City labels. NOTE: Cities (their labels) also get their own level.
		{
			label->setColor(C_LBLCITY);
			int offset_y;

			RuleRegion* regionRule;
			for (std::vector<Region*>::const_iterator
					i  = _playSave->getRegions()->begin();
					i != _playSave->getRegions()->end();
					++i)
			{
				regionRule = const_cast<RuleRegion*>((*i)->getRules()); // strip const for iteration.
				for (std::vector<RuleCity*>::const_iterator
						j  = regionRule->getCities().begin();
						j != regionRule->getCities().end();
						++j)
				{
					if (_gZ >= (*j)->getZoomLevel())
					{
						lon = (*j)->getLongitude(),
						lat = (*j)->getLatitude();

						if (pointBack(lon,lat) == false)
						{
							polarToCart(
									lon,lat,
									&x,&y);

							if ((*j)->isLabelTop() == true)
								offset_y = -10;
							else
								offset_y = 2;

							label->setX(x - 50);
							label->setY(y + offset_y);
							label->setText((*j)->getLabel(_game->getLanguage()));

							label->blit(_srfLayerDetail);
						}
					}
				}
			}
		}

		if (_gZ >= _zBaseLabels) // draw xCom Base labels
		{
			label->setColor(C_LBLBASE);
			label->setAlign(ALIGN_LEFT);

			for (std::vector<Base*>::const_iterator
					i  = _playSave->getBases()->begin();
					i != _playSave->getBases()->end();
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
						label->setText((*i)->getLabel());

						label->blit(_srfLayerDetail);
					}
				}
			}
		}
		delete label;
	}


	// Debug stuff follows ...
	static bool readyLinetypeSwitch (false);

	if (_playSave->debugCountryLines() == true)
	{
		readyLinetypeSwitch = true;
		int
			cycle (_game->getDebugCycle()),
			area  (0),
			color (0);

		switch (_debugType)
		{
			case DTG_COUNTRY:
			{
				if (cycle >= static_cast<int>(_playSave->getCountries()->size()))
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Country*>::const_iterator
						i  = _playSave->getCountries()->begin();
						i != _playSave->getCountries()->end();
						++i, ++area)
				{
					if (cycle == area || cycle == -1)
					{
						color += 10;
						for (size_t
								j  = 0u;
								j != (*i)->getRules()->getLonMin().size();
								++j)
						{
							const RuleCountry* const countryRule ((*i)->getRules());
							const double
								lon1 (countryRule->getLonMin().at(j)),
								lon2 (countryRule->getLonMax().at(j)),
								lat1 (countryRule->getLatMin().at(j)),
								lat2 (countryRule->getLatMax().at(j));

							drawVHLine(_srfLayerDetail, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
						}
					}

					if (area == cycle)
					{
						if (_playSave->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							_playSave->setDebugArg((*i)->getType());
						break;
					}
					else
						_playSave->setDebugArg("");
				}
				break;
			}

			case DTG_REGION:
			{
				if (cycle >= static_cast<int>(_playSave->getRegions()->size()))
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Region*>::const_iterator
						i  = _playSave->getRegions()->begin();
						i != _playSave->getRegions()->end();
						++i, ++area)
				{
					if (cycle == area || cycle == -1)
					{
						color += 10;
						for (size_t
								j  = 0u;
								j != (*i)->getRules()->getLatMax().size();
								++j)
						{
							const RuleRegion* const regionRule ((*i)->getRules());
							const double
								lon1 (regionRule->getLonMin().at(j)),
								lon2 (regionRule->getLonMax().at(j)),
								lat1 (regionRule->getLatMin().at(j)),
								lat2 (regionRule->getLatMax().at(j));

							drawVHLine(_srfLayerDetail, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
							drawVHLine(_srfLayerDetail, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
						}
					}

					if (area == cycle)
					{
						if (_playSave->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							_playSave->setDebugArg((*i)->getType());
						break;
					}
					else
						_playSave->setDebugArg("");
				}
				break;
			}

			case DTG_ZONE:
			{
				int limit (0);
				for (std::vector<Region*>::const_iterator
						i  = _playSave->getRegions()->begin();
						i != _playSave->getRegions()->end();
						++i)
				{
					for (std::vector<MissionZone>::const_iterator
							j  = (*i)->getRules()->getMissionZones().begin();
							j != (*i)->getRules()->getMissionZones().end();
							++j)
					{
						++limit;
					}
				}

				if (cycle >= limit)
					_game->setDebugCycle(cycle = -1);

				for (std::vector<Region*>::const_iterator
						i  = _playSave->getRegions()->begin();
						i != _playSave->getRegions()->end();
						++i)
				{
					color = -1;
					int zoneType (0);
					for (std::vector<MissionZone>::const_iterator
							j  = (*i)->getRules()->getMissionZones().begin();
							j != (*i)->getRules()->getMissionZones().end();
							++j, ++area, ++zoneType)
					{
						if (cycle == area || cycle == -1)
						{
							color += 2;
							for (std::vector<MissionArea>::const_iterator
									k  = j->areas.begin();
									k != j->areas.end();
									++k)
							{
								const double
									lon1 (k->lonMin), // * M_PI / 180.,
									lon2 (k->lonMax), // * M_PI / 180.,
									lat1 (k->latMin), // * M_PI / 180.,
									lat2 (k->latMax); // * M_PI / 180.;

								drawVHLine(_srfLayerDetail, lon1, lat1, lon2, lat1, static_cast<Uint8>(color));
								drawVHLine(_srfLayerDetail, lon1, lat2, lon2, lat2, static_cast<Uint8>(color));
								drawVHLine(_srfLayerDetail, lon1, lat1, lon1, lat2, static_cast<Uint8>(color));
								drawVHLine(_srfLayerDetail, lon2, lat1, lon2, lat2, static_cast<Uint8>(color));
							}
						}

						if (area == cycle)
						{
							if (_playSave->getDebugArg().compare("COORD") != 0) // ie. don't display area-info if co-ordinates are currently displayed.
							{
								std::ostringstream oststr;
								oststr << (*i)->getType() << " [" << zoneType << "]";
								_playSave->setDebugArg(oststr.str());
							}
							break;
						}
						else
							_playSave->setDebugArg("");
					}

					if (area == cycle)
						break;
				}
			}
		}
		// TODO: blit & flip so the Globe doesn't have to be jiggled to show the lines.
	}
	else if (readyLinetypeSwitch == true) // effectively toggles debugMode.
	{
		readyLinetypeSwitch = false;
		switch (_debugType)
		{
			case DTG_COUNTRY: _debugType = DTG_REGION;  break;
			case DTG_REGION:  _debugType = DTG_ZONE;    break;
			case DTG_ZONE:    _debugType = DTG_COUNTRY;
		}
	}
}

/**
 * Draws a VHLine!
 * @param surface - pointer to a Surface
 * @param lon1    -
 * @param lat1    -
 * @param lon2    -
 * @param lat2    -
 * @param color   - (default 0)
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
		seg = static_cast<int>(std::fabs(sy / (M_PI * 2.) * 48.));
		if (seg == 0)
			++seg;
	}
	else
	{
		seg = static_cast<int>(std::fabs(sx / (M_PI * 2.) * 96.));
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

		if (   pointBack(ln2, lt2) == false
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
 * Draws a big yellow/red crosshair-targeter.
 * @note Used over last-known-UFO position, etc.
 */
void Globe::drawCrosshair() // private.
{
	if (AreSameTwo(
				_crosshairLon, 0.,
				_crosshairLat, 0.) == false
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
		crosshair->blit(_srfLayerCrosshair);
	}
}

/**
 * Sets the coordinates to draw a targeter at.
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

	drawCrosshair();
}

/**
 * Hides the targeter.
 */
void Globe::clearCrosshair()
{
	_drawCrosshair = false;
}

/**
 * Blits this Globe and its sub-elements onto another Surface.
 * @param srf - pointer to a surface
 */
void Globe::blit(const Surface* const srf)
{
	Surface::blit(srf);

	_srfLayerRadars   ->blit(srf);
	_srfLayerDetail   ->blit(srf);
	_srfLayerMarkers  ->blit(srf);
	_srfLayerCrosshair->blit(srf);
}

/**
 * Handles mouse-motion/drag-scroll for this Globe.
 * @param action - pointer to an Action
 * @param state  - State that the ActionHandlers belong to
 */
void Globe::mouseOver(Action* action, State* state)
{
	if (_geoState->inputDisabled() == false)
	{
		if (action != nullptr && action->getDetails()->type == SDL_MOUSEMOTION)
		{
			_dragScrollX += static_cast<int>(action->getDetails()->motion.xrel);
			_dragScrollY += static_cast<int>(action->getDetails()->motion.yrel);

			if (_dragScrollPastPixelThreshold == false)
				_dragScrollPastPixelThreshold = std::abs(_dragScrollX) > Options::dragScrollPixelTolerance
											 || std::abs(_dragScrollY) > Options::dragScrollPixelTolerance;

			if (_dragScrollActivated == true && _dragScrollPastPixelThreshold == true)
			{
//				if (Options::geoDragScrollInvert == true) // scroll. I don't use inverted scrolling.
//				{
//					const double
//						newLon ((static_cast<double>(_dragScrollTotalX) / action->getScaleX()) * ROTATE_LONGITUDE / static_cast<double>(_zoom + 1) / 2.),
//						newLat ((static_cast<double>(_dragScrollTotalY) / action->getScaleY()) * ROTATE_LATITUDE  / static_cast<double>(_zoom + 1) / 2.);
//					center(
//						_cenLon + newLon / static_cast<double>(Options::geoScrollSpeed),
//						_cenLat + newLat / static_cast<double>(Options::geoScrollSpeed));
//				}
//				else
//				{
				const double
					newLon (static_cast<double>(-action->getDetails()->motion.xrel) * ROTATE_LONGITUDE / static_cast<double>(_gZ + 1u) / 2.),
					newLat (static_cast<double>(-action->getDetails()->motion.yrel) * ROTATE_LATITUDE  / static_cast<double>(_gZ + 1u) / 2.);
				center(
					_cenLon + newLon / static_cast<double>(Options::geoScrollSpeed),
					_cenLat + newLat / static_cast<double>(Options::geoScrollSpeed));
//				}

				_game->getCursor()->handle(action);
			}
		}

		double // ignore mouse-overs that are outside this Globe.
			lon,lat;
		cartToPolar(
				static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
				static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
				&lon,&lat);

		if (isNaNorInf(lon,lat) == false)
			InteractiveSurface::mouseOver(action, state);
	}
}

/**
 * Initiates drag-scroll.
 * @param action - pointer to an Action
 * @param state  - State that the ActionHandlers belong to
 */
void Globe::mousePress(Action* action, State* state)
{
	if (_geoState->inputDisabled() == false)
	{
		const Uint8 btnId (action->getDetails()->button.button);
		switch (btnId)
		{
			case SDL_BUTTON_WHEELUP:
			case SDL_BUTTON_WHEELDOWN:
				break;

			default:
				if (_drawCrosshair == true)
					clearCrosshair();

				_dragScrollStartTick = SDL_GetTicks();

				_dragScrollX =
				_dragScrollY = 0;
				_dragScrollPastPixelThreshold = false;

				if (btnId == Options::geoDragScrollButton)
				{
					_dragScrollActivated = true;

					_dragScrollLon = _cenLon;
					_dragScrollLat = _cenLat;
				}

				double // ignore mouse-presses that are outside this Globe.
					lon,lat;
				cartToPolar(
						static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
						static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
						&lon,&lat);

				if (isNaNorInf(lon,lat) == false)
					InteractiveSurface::mousePress(action, state);
		}
	}
}

/**
 * 
 * @param action - pointer to an Action
 * @param state  - State that the ActionHandlers belong to
 */
void Globe::mouseRelease(Action* action, State* state)
{
	if (_geoState->inputDisabled() == false)
	{
		double // ignore mouse-releases that are outside this Globe.
			lon,lat;
		cartToPolar(
				static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
				static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
				&lon,&lat);

		if (isNaNorInf(lon,lat) == false)
			InteractiveSurface::mouseRelease(action, state);
	}
}

/**
 * Finalizes drag-scroll and handles mousewheel-zoom.
 * @param action - pointer to an Action
 * @param state  - State that the ActionHandlers belong to
 */
void Globe::mouseClick(Action* action, State* state)
{
	if (_geoState->inputDisabled() == false)
	{
		const Uint8 btnId (action->getDetails()->button.button);
		switch (btnId)
		{
			case SDL_BUTTON_WHEELUP:   zoomIn();  break;
			case SDL_BUTTON_WHEELDOWN: zoomOut(); break;

			default:
			{
				_dragScrollActivated = false;

				if (_dragScrollPastPixelThreshold == false
					&& SDL_GetTicks() - _dragScrollStartTick < static_cast<Uint32>(Options::dragScrollTimeTolerance))
				{
					double // ignore mouse-clicks that are outside this Globe.
						lon,lat;
					cartToPolar(
							static_cast<Sint16>(std::floor(action->getAbsoluteMouseX())),
							static_cast<Sint16>(std::floor(action->getAbsoluteMouseY())),
							&lon,&lat);

					if (isNaNorInf(lon,lat) == false)
					{
						InteractiveSurface::mouseClick(action, state);

						if (btnId == SDL_BUTTON_RIGHT)
							center(lon,lat);
					}
				}
			}
		}
	}
}

/**
 * Handles this Globe's keyboard-shortcuts.
 * @param action - pointer to an Action
 * @param state  - State that the ActionHandlers belong to
 */
void Globe::keyboardPress(Action* action, State* state)
{
	if (_geoState->inputDisabled() == false)
	{
		InteractiveSurface::keyboardPress(action, state);

		const SDLKey key (action->getDetails()->key.keysym.sym);

		if (key == Options::keyGeoToggleDetail)
		{
			_geoState->printDetailInfo(toggleDetail());
		}
		else if (key == Options::keyGeoToggleRadar)
		{
			_geoState->printRadarInfo(changeRadars());
		}
	}
}

/**
 * Gets the texture and shade of a Polygon at specified coordinates.
 * @param lon     - longitude of the point
 * @param lat     - latitude of the point
 * @param texture - pointer to texture ID (-1 if polygon not found)
 * @param shade   - pointer to shade-level
 */
void Globe::getPolygonTextureAndShade(
		double lon,
		double lat,
		int* texture,
		int* shade) const
{
	*shade = GLOBESHADE[CreateTerminator::getTerminatorShade(
														0,
														Cord(0.,0.,1.),
														getSunDirection(lon,lat),
														0)];

	const Polygon* const poly (getPolygonAtCoord(lon,lat));
	if (poly != nullptr)
		*texture = static_cast<int>(poly->getPolyTexture());
	else
		*texture = -1;
}

/**
 * Gets the texture of a Polygon at specified coordinates.
 * @param lon     - longitude of the point
 * @param lat     - latitude of the point
 * @param texture - pointer to texture ID (-1 if polygon not found)
 */
void Globe::getPolygonTexture(
		double lon,
		double lat,
		int* texture) const
{
	const Polygon* const poly (getPolygonAtCoord(lon,lat));
	if (poly != nullptr)
		*texture = static_cast<int>(poly->getPolyTexture());
	else
		*texture = -1;
}

/**
 * Gets the shade of a Polygon at specified coordinates.
 * @param lon   - longitude of the point
 * @param lat   - latitude of the point
 * @param shade - pointer to shade-level
 */
void Globe::getPolygonShade(
		double lon,
		double lat,
		int* shade) const
{
	*shade = GLOBESHADE[CreateTerminator::getTerminatorShade(
														0,
														Cord(0.,0.,1.),
														getSunDirection(lon,lat),
														0)];
}

/**
 * Shows or hides the build-base hover radars.
 * @param hover - true if hover (default true)
 */
void Globe::showBaseBuildRadars(bool hover)
{
	_forceRadars = hover;
}

/**
 * Sets the build-base hover coordinates.
 * @param lon - the longitude
 * @param lat - the latitude
 */
void Globe::setBaseBuildHoverCoords(
		double lon,
		double lat)
{
	_hoverLon = lon;
	_hoverLat = lat;
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
 * Resizes this Globe.
 */
void Globe::resize()
{
	static const size_t SRF (4u);
	Surface* const surfaces[SRF]
	{
		this,
		_srfLayerMarkers,
		_srfLayerDetail,
		_srfLayerRadars
	};

	const int
		width  (Options::baseXGeoscape - 64), // TODO: '64' should be a constant.
		height (Options::baseYGeoscape);

	for (size_t
			i  = 0u;
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

	_redraw = true;
}

}
