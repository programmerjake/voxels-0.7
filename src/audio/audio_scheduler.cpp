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
#include "audio/audio_scheduler.h"
#include "stream/iostream.h"
#include "platform/platform.h"
#include <string>
#include <sstream>
#include <cassert>
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
float AudioScheduler::SongDescriptor::getSongTimeOfDayFactor(float timeOfDayFraction,
                                                             float lengthOfDayInSeconds) const
{
    if(timeOfDayFocusAmount <= 0 || lengthOfDayInSeconds <= 0)
        return 1;
    // auto &log = getDebugLog();
    // log << "timeOfDayFraction: " << timeOfDayFraction << " focusedTimeOfDay: " <<
    // focusedTimeOfDay;
    float x = timeOfDayFraction - focusedTimeOfDay;
    x -= std::floor(x);
    // log << " x: " << x << "\n";
    float durationFraction = static_cast<float>(duration / lengthOfDayInSeconds);
    if(durationFraction <= 1e-3)
        durationFraction = 1e-3;
    // log << "durationFraction: " << durationFraction;
    float start = x, end = x + durationFraction;
    float outputFactor = 1 / durationFraction;
    float integral = std::floor(end) - std::floor(start);
    start -= std::floor(start);
    end -= std::floor(end);
    // log << " start: " << start << " end: " << end;
    float startBase = std::fabs(0.5f - start) * 2;
    float endBase = std::fabs(0.5f - end) * 2;
    // log << " startBase: " << startBase << " endBase: " << endBase;
    float v = std::pow(startBase, timeOfDayFocusAmount) * (0.5f - start)
              - std::pow(endBase, timeOfDayFocusAmount) * (0.5f - end);
    // log << " v: " << v;
    integral += v;
    // log << " integral: " << integral << postnl;
    return outputFactor * integral;
}

AudioScheduler::AudioScheduler() : songs(), lastPlayedSongs(), doNotRepeatSongLength()
{
    std::shared_ptr<stream::Reader> preader = getResourceReader(L"background-songs.txt");
    stream::ReaderIStream is(preader);
    std::wstring line;
    const auto spaceCharacters = L" \t\n\r";
    while(std::getline(is, line))
    {
        std::size_t firstNonSpace = line.find_first_not_of(spaceCharacters);
        if(firstNonSpace != std::wstring::npos)
            line.erase(0, firstNonSpace);
        else
            continue;
        std::size_t lastNonSpace = line.find_last_not_of(spaceCharacters);
        if(lastNonSpace != std::wstring::npos)
            line.erase(lastNonSpace + 1);
        if(line.empty())
            continue;
        if(line[0] == L'#') // comment
            continue;
        std::wstringstream ss(line);
        std::wstring fileName;
        if(!(ss >> fileName))
            throw std::runtime_error("can't read audio file name");
        if(fileName.find_first_of(L"\\/:") != std::wstring::npos)
            throw std::runtime_error("invalid audio file name");
        SongDescriptor song;
        if(!(ss >> song.creepiness) || !std::isfinite(song.creepiness) || song.creepiness < -10
           || song.creepiness > 10)
            throw std::runtime_error("can't read creepiness amount");
        if(!(ss >> song.timeOfDayFocusAmount) || !std::isfinite(song.timeOfDayFocusAmount)
           || song.timeOfDayFocusAmount < 0
           || song.timeOfDayFocusAmount > 1000)
            throw std::runtime_error("can't read time of day focus amount");
        if(song.timeOfDayFocusAmount > 0)
            if(!(ss >> song.focusedTimeOfDay) || !std::isfinite(song.focusedTimeOfDay)
               || song.focusedTimeOfDay < 0
               || song.focusedTimeOfDay >= 1)
                throw std::runtime_error("can't read focused time of day");
        enum_array<bool, Dimension> setDimensionFocusAmount = {};
        std::wstring dimensionName;
        float dimensionFocusAmount;
        if(!(ss >> dimensionName >> dimensionFocusAmount) || !std::isfinite(dimensionFocusAmount)
           || dimensionFocusAmount < 0
           || dimensionFocusAmount > 1)
            throw std::runtime_error("can't read dimension name and focus amount");
        while(dimensionName != L"*")
        {
            bool found = false;
            for(Dimension d : enum_traits<Dimension>())
            {
                if(getDimensionName(d) == dimensionName)
                {
                    found = true;
                    song.dimensionFocusAmounts[d] = dimensionFocusAmount;
                    setDimensionFocusAmount[d] = true;
                    break;
                }
            }
            if(!found)
                throw std::runtime_error("invalid dimension name");
            if(!(ss >> dimensionName >> dimensionFocusAmount)
               || !std::isfinite(dimensionFocusAmount) || dimensionFocusAmount < 0)
                throw std::runtime_error("can't read dimension name and focus amount");
        }
        for(Dimension d : enum_traits<Dimension>())
            if(!setDimensionFocusAmount[d])
                song.dimensionFocusAmounts[d] = dimensionFocusAmount;
        song.song = std::make_shared<Audio>(fileName, true);
        song.duration = song.song->duration();
        if(song.duration < 0)
            song.duration = 60; // good default
        songs.push_back(song);
    }
    if(songs.empty())
        throw std::runtime_error("didn't read any songs");
    doNotRepeatSongLength = songs.size() / 7;
    if(doNotRepeatSongLength < 1)
        doNotRepeatSongLength = 1;
    else if(doNotRepeatSongLength > 3)
        doNotRepeatSongLength = 3;
    lastPlayedSongs.reserve(doNotRepeatSongLength);
}

std::shared_ptr<PlayingAudio> AudioScheduler::next(float timeOfDayInSeconds,
                                                   float lengthOfDayInSeconds,
                                                   float relativeHeight,
                                                   Dimension dimension,
                                                   float playVolume)
{
    assert(!songs.empty());
    std::size_t bestSong = 0;
    float bestSongFactor = 0;
    bool isFirst = true;
    float heightCreepiness = 1 - relativeHeight;
    float creepiness = getDimensionalCreepiness(dimension) + heightCreepiness;
    for(std::size_t i = 0; i < songs.size(); i++)
    {
        const SongDescriptor &song = songs[i];
        float creepinessDifference = creepiness - song.creepiness;
        float songFactor = std::exp(-creepinessDifference * creepinessDifference);
        songFactor *= song.getSongTimeOfDayFactor(timeOfDayInSeconds / lengthOfDayInSeconds,
                                                  lengthOfDayInSeconds);
        songFactor *= song.dimensionFocusAmounts[dimension];
        for(std::size_t v : lastPlayedSongs)
        {
            if(i == v)
            {
                songFactor = 0;
                break;
            }
        }
        // getDebugLog() << L"song " << i << " : " << songFactor << postnl;
        if(isFirst || songFactor > bestSongFactor)
        {
            isFirst = false;
            bestSong = i;
            bestSongFactor = songFactor;
        }
    }
    if(lastPlayedSongs.size() >= doNotRepeatSongLength)
    {
        lastPlayedSongs.erase(lastPlayedSongs.begin(), lastPlayedSongs.begin() + 1);
    }
    lastPlayedSongs.push_back(bestSong);
    return songs[bestSong].song->play(playVolume);
}
}
}
