/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef AUDIO_SCHEDULER_H_INCLUDED
#define AUDIO_SCHEDULER_H_INCLUDED

#include "platform/audio.h"
#include <vector>
#include "util/dimension.h"
#include "util/enum_traits.h"
#include <cmath>
#include <algorithm>

namespace programmerjake
{
namespace voxels
{
class AudioScheduler final
{
private:
    struct SongDescriptor final
    {
        std::shared_ptr<Audio> song;
        double duration;
        enum_array<float, Dimension> dimensionFocusAmounts; /// amount of focus for each dimension
        float focusedTimeOfDay; /// time of day to play
        float timeOfDayFocusAmount; /// amount of focus on time of day
        float creepiness;
        SongDescriptor()
            : song(),
            duration(),
            dimensionFocusAmounts(),
            focusedTimeOfDay(),
            timeOfDayFocusAmount(),
            creepiness()
        {
        }
        float getSongTimeOfDayFactor(float timeOfDayFraction, float lengthOfDayInSeconds) const;
    };
    std::vector<SongDescriptor> songs;
    std::vector<std::size_t> lastPlayedSongs;
    std::size_t doNotRepeatSongLength;
public:
    AudioScheduler();
    /** @brief get next background song
     * @param timeOfDayInSeconds the current time of day in seconds or 0 if there is no day/night cycle for `dimension`
     * @param lengthOfDayInSeconds the length of a day in seconds or 0 if there is no day/night cycle for `dimension`
     * @param relativeHeight the relative player height :<br/>0 to 1 : below ground<br/>1 or more : above ground
     * @param dimension the dimension to get the song for
     * @param playVolume the volume to start the song playing at
     * @return the next background song
     */
    std::shared_ptr<PlayingAudio> next(float timeOfDayInSeconds,
                                       float lengthOfDayInSeconds,
                                       float relativeHeight,
                                       Dimension dimension,
                                       float playVolume);
    void reset()
    {
        lastPlayedSongs.clear();
    }
};
}
}

#endif // AUDIO_SCHEDULER_H_INCLUDED
