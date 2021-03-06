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

#include "RuleVideo.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param type - reference a video-type
 */
RuleVideo::RuleVideo(const std::string& type)
	:
		_type(type)
{}

/**
 * dTor.
 */
RuleVideo::~RuleVideo()
{}

/**
 * Loads a video
 * @param node - reference a YAML node
 */
void RuleVideo::load(const YAML::Node& node)
{
	if (const YAML::Node& videos = node["videos"])
	{
		for (YAML::const_iterator
				i = videos.begin();
				i != videos.end();
				++i)
		{
			_videos.push_back((*i).as<std::string>());
		}
	}

//	if(const YAML::Node &slides = node["slides"])
//	{
//		for(YAML::const_iterator
//				i = slides.begin();
//				i != slides.end();
//				++i)
//		{
//			_slides.push_back(*i).as<std::string>());
//		}
//	}
}

/**
 * Gets the list of videos.
 * @return, pointer to a vector of video-types
 */
const std::vector<std::string>* RuleVideo::getVideos() const
{
	return &_videos;
}

}
