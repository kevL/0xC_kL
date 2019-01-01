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

#ifndef OPENXCOM_GLOBE_H
#define OPENXCOM_GLOBE_H

#include <list>
//#include <vector>

#include "Cord.h"

#include "../Engine/FastLineClip.h"
#include "../Engine/InteractiveSurface.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

extern bool kL_reCenter;

enum DebugTypeGlobe
{
	DTG_COUNTRY,	// 0
	DTG_REGION,		// 1
	DTG_ZONE		// 2
};


class Game;
class GeoscapeState;
class NumberText;
class Polygon;
class RuleGlobe;
class SavedGame;
class SurfaceSet;
class Target;
class Timer;


/**
 * Interactive globe-view of the world.
 * @note Takes a flat world map made out of land polygons with polar coordinates
 * and renders it as a 3D-looking globe with cartesian coordinates that the
 * player can interact with.
 */
class Globe final
	:
		public InteractiveSurface
{

private:
	/// This is shade conversion from 0..31 levels of geoscape to levels 0..15 of battlescape.
	static constexpr int GLOBESHADE[32u]
	{
		0,  1,  1,  1,  2,  2,  2,  3,
		3,  3,  4,  4,  4,  5,  5,  5,
		6,  6,  6,  7,  7,  7,  8,  8,
		9,  9, 10, 11, 12, 13, 14, 15
	}; // terminator @ id-25

	static const int NEAR_RADIUS = 25;

	static const Uint8
		C_GREEN	=  8u,
		C_RED	= 14u;

	static const double
		ROTATE_LONGITUDE,
		ROTATE_LATITUDE;

	bool
		_blink,
		_dragScrollActivated,
		_dragScrollPastPixelThreshold,
		_drawCrosshair,
		_forceRadars,
		_globeDetail;
	int
		_blinkVal,
		_dragScrollX,
		_dragScrollY;
	double
		_cenLat,
		_cenLon,
		_crosshairLat,
		_crosshairLon,
		_dfZstep,
		_dragScrollLat,
		_dragScrollLon,
		_hoverLat,
		_hoverLon,
		_radius,
		_rotLat,
		_rotLon;
	Sint16
		_cenX,
		_cenY;
	Uint32 _dragScrollStartTick;
	size_t
		_gZ,
		_zBaseLabels,
		_zBorders,
		_zCities,
		_zCityLabels,
		_zCountryLabels,
		_texOffset;

	FastLineClip* _clipper;
	Game* _game;
	GeoscapeState* _geoState;
	NumberText* _flightData;
	RuleGlobe* _globeRule;
	SavedGame* _playSave;
	Surface
		* _srfLayerDetail,
		* _srfLayerCrosshair,
		* _srfLayerMarkers,
		* _srfLayerRadars;
	SurfaceSet
		* _srtMarkers,
		* _srtTextures;
	Timer
		* _timerBlink,
		* _timerRot;

	/// cache of geographical textures
	std::list<Polygon*> _cacheLand;
	/// normal of each pixel per zoom-level
	std::vector<std::vector<Cord>> _earthData;
	/// data-sample used for noise in terminator-shadow
	std::vector<Sint16> _terminatorFluxions;
	/// list of zoom-level dimensions on-screen
	std::vector<double> _radii;

	DebugTypeGlobe _debugType;
	GlobeRadarDetail _radarDetail;


	/// Sets up the viewport of earth and stuff.
	void setupRadii(
			int width,
			int height);
	/// Sets the Globe's zoom-factor.
	void setGz(size_t gZ);

	/// Checks if a point is behind the Globe.
	bool pointBack(
			double lon,
			double lat) const;
	/// Returns latitude of last visible-to-player point on given longitude.
//	double lastVisibleLat(double lon) const;
	/// Gets the Polygon at specified coordinates (Volutar).
	Polygon* getPolygonAtCoord(
			double lon,
			double lat) const;
	/// Checks if a point is inside a Polygon.
//	bool insidePolygon( // obsolete, see getPolygonAtCoord()
//			double lon,
//			double lat,
//			const Polygon* const poly) const;
	/// Checks if a Target is near a point.
	bool targetNear(
			const Target* const target,
			int x,
			int y) const;

	/// Caches visible Polygons.
	void cachePolygons();

	/// Gets position of sun relative to given position in polar-coords and date.
	Cord getSunDirection(
			double lon,
			double lat) const;

	/// Draws a Globe range-circle.
	void drawGlobeCircle(
			double lat,
			double lon,
			double radius,
			int segs,
			Uint8 color = 0u);
	/// Special "transparent" line.
	void XuLine(
			Surface* surface,
			Surface* src,
			double x1,
			double y1,
			double x2,
			double y2,
			int shade,
			Uint8 color = 0u);
	// Draws a "VH" line.
	void drawVHLine(
			Surface* surface,
			double lon1,
			double lat1,
			double lon2,
			double lat2,
			Uint8 color = 0u);
	/// Draws a flight path.
	void drawPath(
			Surface* surface,
			double lon1,
			double lat1,
			double lon2,
			double lat2);
	/// Draws the end-point of player's Craft on an intercept-trajectory.
	void drawInterceptMarker(
		const double lon,
		const double lat);
	/// Draws a Target marker.
	void drawTarget(
			const Target* const target,
			Surface* const surface);
	/// Draws a big yellow/red crosshair-targeter.
	void drawCrosshair();


	public:
		static const int // GlobeMarkers
			GLM_BASE		= 0,
			GLM_CRAFT		= 1,
			GLM_UFO_FLYING	= 2,
			GLM_UFO_LANDED	= 3,
			GLM_UFO_CRASHED	= 4,
			GLM_TERRORSITE	= 5,
			GLM_WAYPOINT	= 6,
			GLM_ALIENBASE	= 7,
			GLM_CITY		= 8;

		static Uint8
			C_LBLBASE,
			C_LBLCITY,
			C_LBLCOUNTRY,
			C_LINE,
//			C_RADAR1,
			C_RADAR2,
			C_FLIGHT,
			C_OCEAN,
			C_BLACK;

		/// Creates a Globe with the specified position and size.
		Globe(
				Game* const game,
				GeoscapeState* const geoState,
				int cenX,
				int cenY,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the Globe.
		~Globe();

		/// Converts polar coordinates to cartesian coordinates.
		void polarToCart(
				double lon,
				double lat,
				Sint16* x,
				Sint16* y) const;
		/// Converts polar coordinates to cartesian coordinates.
		void polarToCart(
				double lon,
				double lat,
				double* x,
				double* y) const;
		/// Converts cartesian coordinates to polar coordinates.
		void cartToPolar(
				Sint16 x,
				Sint16 y,
				double* lon,
				double* lat) const;

		/// Starts rotating the Globe left.
		void rotateLeft();
		/// Starts rotating the Globe right.
		void rotateRight();
		/// Starts rotating the Globe up.
		void rotateUp();
		/// Starts rotating the Globe down.
		void rotateDown();
		/// Stops rotating the Globe.
		void rotateStop();
		/// Stops longitudinal rotation of the Globe.
		void rotateStopLon();
		/// Stops latitudinal rotation of the Globe.
		void rotateStopLat();

		/// Gets the current zoom.
		size_t getZoom() const;
		/// Gets the number of zoom levels available.
		size_t getZoomLevels() const;

		/// Zooms the Globe in.
		void zoomIn();
		/// Zooms the Globe out.
		void zoomOut();
		/// Zooms the Globe minimum.
//		void zoomMin();
		/// Zooms the Globe maximum.
//		void zoomMax();

		/// Zooms the Globe in for dogfights.
		bool zoomDogfightIn();
		/// Zooms the Globe out for dogfights.
		bool zoomDogfightOut();

		/// Centers the Globe on a point.
		void center(
				double lon,
				double lat);

		/// Checks if a point is inside land.
		bool insideLand(
				double lon,
				double lat) const;

		/// Toggles on/off drawing detail on the Globe.
		bool toggleDetail();
		/// Changes the level of radar-details shown on the Globe.
		GlobeRadarDetail changeRadars();

		/// Gets all the Targets near a point on the Globe.
		std::vector<Target*> getTargets(
				int x,
				int y,
				bool flightTargets = true) const;

		/// Sets the Palette of the Globe.
		void setPalette(
				SDL_Color* const colors,
				int firstcolor = 0,
				int ncolors = 256) override;

		/// Handles the Timers.
		void think() override;
		/// Blinks the markers.
		void blink();
		/// Toggles blinking.
		void toggleBlink();
		/// Rotates the Globe.
		void rotate();

		/// Draws the whole Globe.
		void draw() override;
		/// Draws the ocean of the Globe.
		void drawOcean();
		/// Draws the land of the Globe.
		void drawLand();
		/// Draws the 3d-bevel around the continents.
		void drawBevel();
		/// Draws the radar-ranges of the Globe.
		void drawRadars();
		/// Draws the flight-paths of the Globe.
		void drawFlights();
		/// Draws the shadow-terminator.
		void drawTerminus();
		/// Draws all the markers over the Globe.
		void drawMarkers();
		/// Draws the country-details of the Globe.
		void drawDetail();

		/// Sets the co-ordinates to draw a targeter at.
		void setCrosshair(
				const double lon,
				const double lat);
		/// Hides the targeter.
		void clearCrosshair();

		/// Blits the Globe onto another Surface.
		void blit(const Surface* const srf) override;

		/// Special handling for mouse hover.
		void mouseOver(Action* action, State* state) override;
		/// Special handling for mouse presses.
		void mousePress(Action* action, State* state) override;
		/// Special handling for mouse releases.
		void mouseRelease(Action* action, State* state) override;
		/// Special handling for mouse clicks.
		void mouseClick(Action* action, State* state) override;
		/// Special handling for key presses.
		void keyboardPress(Action* action, State* state) override;

		/// Gets the texture and shade of a Polygon at specified coordinates.
		void getPolygonTextureAndShade(
				double lon,
				double lat,
				int* texture,
				int* shade) const;
		/// Gets the texture of a Polygon at specified coordinates.
		void getPolygonTexture(
				double lon,
				double lat,
				int* texture) const;
		/// Gets the shade of a Polygon at specified coordinates.
		void getPolygonShade(
				double lon,
				double lat,
				int* shade) const;

		/// Shows or hides the build-base hover radars.
		void showBaseBuildRadars(bool hover = true);
		/// Sets the build-base hover coordinates.
		void setBaseBuildHoverCoords(
				double lon,
				double lat);

		/// Gets the current debugType for Geoscape.
		DebugTypeGlobe getDebugType() const;

		/// Updates the resolution settings due to resizing the window.
		void resize();
};

}

#endif
