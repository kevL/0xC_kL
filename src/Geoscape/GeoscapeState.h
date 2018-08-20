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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_GEOSCAPESTATE_H
#define OPENXCOM_GEOSCAPESTATE_H

#include <list>

#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"

namespace OpenXcom
{

extern size_t kL_curBase;
extern bool
	kL_geoMusicPlaying,
	kL_geoMusicReturnState;

extern const double
	radius_earth,
	arcToRads;

class Base;
class Craft;
class DogfightState;
class Globe;
class ImageButton;
class InteractiveSurface;
class TerrorSite;
class NumberText;
class RuleMissionScript;
class Ruleset;
class SavedGame;
class Sound;
class Surface;
class Text;
class TextButton;
class Timer;
class Ufo;


/**
 * Geoscape screen shows an overview of the planet.
 */
class GeoscapeState final
	:
		public State
{

private:
	static const Uint32 FAST_GEO_INTERVAL = 20u;
	static const size_t
		UFO_HOTBLOBS = 16u,
		BLOBSIZE     = 13u;

	static const Uint8
		YELLOW_D	=  12u,
		BLACK		=  15u,
		BROWN		=  53u,
		GREEN		= 112u,
		RED			= 133u,
		SLATE		= 164u,
		SLATE_D		= 170u,
		GREEN_SEA	= 242u,
		PURPLE_D	= 249u;

	bool
		_dfCenterCurrentCoords,
		_dfZout,
		_pause,
		_pauseHard;
	int
		_delayMusicDfCheck,
		_day,
		_month,
		_year,
		_timeSurplus,
		_score;
	int64_t _ufosDetected;
	size_t _dfReduced;
	double
		_dfCCC_lon,
		_dfCCC_lat;

	std::string _stDebug;
	std::wstring _wstScore;

	Globe* _globe;
	ImageButton
		* _btnIntercept,
		* _btnBases,
		* _btnGraphs,
		* _btnUfopaedia,
		* _btnOptions,
		* _btnFunding,

		* _btn5Secs,
		* _btn1Min,
		* _btn5Mins,
		* _btn30Mins,
		* _btn1Hour,
		* _btn1Day,

		* _btnDetail,
		* _btnGroup;
//	InteractiveSurface* _btnRotateLeft, * _btnRotateRight, * _btnRotateUp, * _btnRotateDown, * _btnZoomIn, * _btnZoomOut;
	InteractiveSurface
		* _isfUfoBlobs[UFO_HOTBLOBS],
		* _isfTime;
	NumberText* _numUfoBlobs[UFO_HOTBLOBS];
	Ruleset* _rules;
	SavedGame* _playSave;
	Surface
		* _srfSideBlack,
		* _srfSpace;
	Text
		* _txtDebug,
		* _txtFunds,
		* _txtScore,

		* _txtHour,
		* _txtColon,
		* _txtMin,
		* _txtSec,
		* _txtDay,
		* _txtMonth,
		* _txtYear,

		* _txtZoom,
		* _txtRadars,
		* _txtLabels,

		* _ufoDetected;
	TextButton
		* _sideTop,
		* _sideBottom;
	Timer
		* _trGeo,
		* _trDf,
		* _trDfStart,
		* _trDfZinn,
		* _trDfZout;
	Ufo* _hostileUfos[UFO_HOTBLOBS];

	std::list<State*> _popups;
	std::list<DogfightState*>
		_dogfights,
		_dogfightsToStart;

	/// Creates the prefix for a debugging message.
	void fabricateDebugPretext();

	/// Advances time on the Geoscape.
	void timeAdvance();
	/// Displays current time/date/funds.
	void updateTimeDisplay();
	/// Converts the date to a month string.
	std::wstring convertDateToMonth(int date);

	/// Scores points for UFOs that are Flying/Landed or Crashed.
	void scoreUfos(bool hour) const;
	/// Processes a TerrorSite.
	bool processTerrorSite(TerrorSite* const terrorSite) const;

	/// Assigns whether an aLien cracked under pressure.
	void doesAlienCrack(
				const std::string& alienType,
				bool& gof,
				bool& requested) const;

	/// Starts a new Dogfight.
	void startDogfight();

	/// Handle AlienMission generation.
	void deterAlienMissions();
//	void deterAlienMissions(bool atGameStart = false);
//	/// Handle land-mission generation.
//	void setupLandMission();
	/// Process each individual MissionScript directive.
	bool processDirective(RuleMissionScript* const directive);

	/// Handler for hot-keying time-compression.
	void keyTimeCompression(Action* action);
	/// Handler for clicking a time-compression button.
	void btnTimeCompression(Action* action);
	/// Handler for clicking pause.
	void btnPauseClick(Action* action);
	/// Handler for clicking a visible UFO button.
	void btnUfoBlobPress(Action* action);


	public:
		static const int _ufoBlobs[8u][13u][13u]; // used also by DogfightState

		enum DetectType
		{
			DT_UNDETECTED,		// 0
			DT_HYPERDECODED,	// 1
			DT_DETECTED,		// 2
			DT_HYPERDETECTED	// 3
		};

		/// Creates a Geoscape state.
		GeoscapeState();
		/// Cleans up the Geoscape state.
		~GeoscapeState();

		/// Blit method - renders the state and dogfights.
		void blit() override;

		/// Handles keyboard-shortcuts.
		void handle(Action* action) override;

		/// Updates the palette and timer.
		void init() override;
		/// Runs the state-timers.
		void think() override;

		/// Draws the UFO indicators for known UFOs.
		void drawUfoBlobs();

		/// Triggers if 5 seconds pass.
		void time5Seconds();
		/// Triggers if 10 minutes pass.
		void time10Minutes();
		/// Triggers if 30 minutes pass.
		void time30Minutes();
		/// Triggers if 1 hour passes.
		void time1Hour();
		/// Triggers if 1 day passes.
		void time1Day();
		/// Triggers if 1 month passes.
		void time1Month();

		/// Sets the time-compression to 5-sec intervals.
		void resetTimer();
		/// Checks if time-compression is set to 5-sec intervals.
		bool is5Sec() const;

		/// Displays a popup window.
		void popupGeo(State* const state);

		/// Gets the geoscape Globe.
		Globe* getGlobe() const;

		/// Checks if user-input is not allowed.
		bool inputDisabled();

		/// Handler for clicking the Globe.
		void globeClick(Action* action);
		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Bases button.
		void btnBasesClick(Action* action);
		/// Handler for clicking the Graph button.
		void btnGraphsClick(Action* action);
		/// Handler for clicking the Ufopaedia button.
		void btnUfopaediaClick(Action* action);
		/// Handler for clicking the Options button.
		void btnOptionsClick(Action* action);
		/// Handler for clicking the Funding button.
		void btnFundingClick(Action* action);

		/// Handler for clicking the Detail area.
		void btnDetailPress(Action* action);
		/// Updates the detail-info.
		void printDetailInfo(bool detail);
		/// Updates the radar-info.
		void printRadarInfo(GlobeRadarDetail radars);

		/// Handler for pressing the Rotate Left arrow.
		void btnRotateLeftPress(Action* action);
		/// Handler for pressing the Rotate Right arrow.
		void btnRotateRightPress(Action* action);
		/// Handler for releasing the Rotate Left/Right arrows.
		void btnRotateLonStop(Action* action);

		/// Handler for pressing the Rotate Up arrow.
		void btnRotateUpPress(Action* action);
		/// Handler for pressing the Rotate Down arrow.
		void btnRotateDownPress(Action* action);
		/// Handler for releasing the Rotate Up/Down arrows.
		void btnRotateLatStop(Action* action);

		/// Handler for pressing the Rotate Left Up arrow.
		void btnRotateLeftUpPress(Action* action);
		/// Handler for pressing the Rotate Left Down arrow.
		void btnRotateLeftDownPress(Action* action);
		/// Handler for pressing the Rotate Right Up arrow.
		void btnRotateRightUpPress(Action* action);
		/// Handler for pressing the Rotate Right Down arrow.
		void btnRotateRightDownPress(Action* action);
		/// Handler for releasing the Rotate Diagonally arrows.
		void btnRotateStop(Action* action);

		/// Handler for left-clicking the Zoom In icon.
		void btnZoomInLeftClick(Action* action);
		/// Handler for left-clicking the Zoom Out icon.
		void btnZoomOutLeftClick(Action* action);

		/// Updates the zoom-text in the lower left corner of the screen.
		void updateZoomText();

		/// Handler for right-clicking the Zoom In icon.
//		void btnZoomInRightClick(Action* action);
		/// Handler for right-clicking the Zoom Out icon.
//		void btnZoomOutRightClick(Action* action);

		/// Handles zoom-in to zoom-out or vice versa for a dogfight.
		void toggleDogfight(bool in) const;
		/// Globe zoom-in effect for dogfights.
		void dfZoomIn();
		/// Globe zoom-out effect for dogfights.
		void dfZoomOut();
		/// Tells Dogfight zoom-out to ignore stored DF coordinates.
		void setDfCCC(
				double lon,
				double lat);
		/// Checks if the Dogfight zoom-out should ignore stored DF-coordinates.
		bool getDfCCC() const;
		/// Gets the quantity of iconized dogfights.
		size_t getReducedDogfights() const;
		/// Logic for all current dogfights.
		void thinkDogfights();
		/// Updates intercept-data and repositions ports for all Dogfights.
		void resetInterceptPorts();

		/// Gets the dogfights.
		std::list<DogfightState*>& getDogfights();

		/// Starts base-defense tactical.
		void baseDefenseTactical(
				Base* const base,
				Ufo* const ufo);

		/// Sets hard-pause.
		void setPaused();
		/// Checks hard-pause.
		bool isPaused() const;

		/// Examines the quantity of remaining UFO-detected popups.
		void assessUfoPopups();

		/// Update the resolution settings - the window was resized.
		void resize(
				int& dX,
				int& dY) override;
};

}

#endif
