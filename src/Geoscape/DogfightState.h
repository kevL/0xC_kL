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

#ifndef OPENXCOM_DOGFIGHTSTATE_H
#define OPENXCOM_DOGFIGHTSTATE_H

//#include <vector>
//#include <string>

#include "../Engine/State.h"


namespace OpenXcom
{

enum ColorDf
{
	CRAFT_MIN,			//  0
	CRAFT_MAX,			//  1
	RADAR_MIN,			//  2
	RADAR_MAX,			//  3
	DAMAGE_RED,			//  4
	DAMAGE_YEL,			//  5
	BLOB_MIN,			//  6
	BLOB_MAX,			//  7
	RANGE_METER,		//  8
	DISABLED_WEAPON,	//  9
	DISABLED_AMMO,		// 10
	DISABLED_RANGE,		// 11
	UFO_BEAM			// 12
};

enum CautionLevel
{
	CAUTION_NONE,	// 0
	CAUTION_LOW,	// 1
	CAUTION_HIGH	// 2
};


class Craft;
class CraftWeaponProjectile;
class GeoscapeState;
class Globe;
class ImageButton;
class InteractiveSurface;
class SavedGame;
class Surface;
class Text;
class Timer;
class Ufo;


/**
 * Shows a Dogfight (interception port) between a player's Craft and a UFO.
 */
class DogfightState final
	:
		public State
{

private:
	static const int
		_projectileBlobs[4u][6u][3u],

		DIST_ENGAGE   = 635,
		DIST_STANDOFF = 595,
		DIST_CLOSE    =  60,

		STAT_PERSIST  =  25;


	bool
		_destroyCraft,
		_destroyUfo,
		_finish,
		_finishRequest,
		_reduced,
		_ufoBreakingOff,
		_w1Enabled,
		_w2Enabled;
	int
		_desired,
		_diff,
		_dist,
		_textPersistence,

		_ufoSize,
		_craftHeight,
		_craftHeight_pre,
		_refreshCraft,

		_x,
		_y,
		_restoreIconX,
		_restoreIconY,

		_w1FireCountdown,
		_w2FireCountdown,
		_w1FireInterval,
		_w2FireInterval;
	size_t
		_slot,
		_slotsTotal;
	Uint8 _colors[13u]; // see ColorDf enum above^

	CautionLevel _cautionLevel;

	std::vector<CraftWeaponProjectile*> _projectiles;

	Craft* _craft;
	GeoscapeState* _geoState;
	Globe* _globe;
	ImageButton
		* _btnStandoff,
		* _btnCautious,
		* _btnStandard,
		* _btnAggressive,
		* _btnDisengage,
		* _btnUfo,
		* _craftStance;
	InteractiveSurface
		* _btnReduce,
		* _btnRestoreIcon,
		* _previewUfo,
		* _isfCw1,
		* _isfCw2;
	SavedGame* _playSave;
	Surface
		* _battleScope,
		* _srfHull,
		* _srfCwRange1,
		* _srfCwRange2,
		* _srfTexIcon,
		* _window;
	Text
		* _txtLoad1,
		* _txtLoad2,
		* _txtDistance,
		* _txtRestoreIcon,
		* _txtStatus,
		* _txtTitle;
	Ufo* _ufo;

	/// Changes distance between the Craft and UFO.
	void adjustDistance(bool hasWeapons = true);

	/// Changes the status text.
	void updateStatus(const std::string& status);

	/// Draws UFO.
	void drawUfo();
	/// Draws projectiles.
	void drawProjectile(const CraftWeaponProjectile* const prj);
	/// Draws Craft.
	void drawCraft(bool init = false);

	/// Moves window to new position.
	void placePort();
	/// Gets the globe texture icon to display for the interception.
	const std::string& getTextureIcon() const;

	/// Checks if the Craft and UFO are still actively engaged.
	bool checkTargets() const;

	/// Checks for and determines a retaliation mission if applicable.
	void checkRetaliation(double lon, double lat) const;

	/// Sets this Dogfight's '_statusPersistence' value.
	void setTextPersistence(int ticks)
	{ _textPersistence = ticks; }


	public:
		/// Creates a Dogfight state.
		DogfightState(
					Globe* const globe,
					Craft* const craft,
					Ufo* const ufo,
					GeoscapeState* const geoState);
		/// Cleans up the Dogfight state.
		~DogfightState();

		/// Runs the timers.
		void think() override;
		/// Animates the Dogfight view-port.
		void aniPort();
		/// Advances the Dogfight and updates the view-port.
		void advanceDogfight();

		/// Fires the first Craft weapon.
		void fireWeapon1();
		/// Fires the second Craft weapon.
		void fireWeapon2();
		/// Fires UFO weapon.
		void fireWeaponUfo();

		/// Handler for the escape/cancel keyboard button.
		void keyEscape(Action* action);
		/// Handler for pressing the Standoff button.
		void btnStandoffClick(Action* action);
		/// Handler for pressing the Cautious Attack button.
		void btnCautiousClick(Action* action);
		/// Handler for pressing the Standard Attack button.
		void btnStandardClick(Action* action);
		/// Handler for pressing the Aggressive Attack button.
		void btnAggressiveClick(Action* action);
		/// Handler for pressing the Disengage button.
		void btnDisengageClick(Action* action);
		/// Handler for clicking the Ufo button.
		void btnUfoClick(Action* action);
		/// Handler for clicking the Preview graphic.
		void previewClick(Action* action);
		/// Handler for clicking the Minimize Dogfight button.
		void btnReduceToIconClick(Action* action);
		/// Handler for clicking the Restore Dogfight icon.
		void btnShowPortPress(Action* action);

		/// Checks if the state is iconized.
		bool isReduced() const;
		/// Checks if Craft stance is in stand-off.
		bool checkStandoff() const;

		/// Toggles usage of Craft weapon 1.
		void weapon1Click(Action* action);
		/// Toggles usage of Craft weapon 2.
		void weapon2Click(Action* action);

		/// Sets interception slot.
		void setInterceptSlot(size_t slot);
		/// Gets interception slot.
		size_t getInterceptSlot() const;
		/// Sets total interceptions in progress.
		void setTotalInterceptSlots(size_t total);
		/// Calculates positions for the intercept-ports.
		void resetInterceptPort(
				size_t port,
				size_t portsTotal);

		/// Checks if the Dogfight should be stopped and deleted.
		bool isFinished() const;

		/// Gets pointer to the UFO in the Dogfight.
		Ufo* getUfo() const;
		/// Sets the UFO associated with the Dogfight to nullptr.
		void clearUfo();
		/// Gets pointer to the xCom Craft in the Dogfight.
		Craft* getCraft() const;
		/// Sets pointer to the xCom Craft in the Dogfight to nullptr.
		void clearCraft();

		/// Gets the current distance between Craft and UFO.
		int getDistance() const;
};

}

#endif
