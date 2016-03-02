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
#include "platform/audio.h"
#include "platform/platform.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_set>
#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif // _MSC_VER
#include <iostream>
#include <cstdlib>
#include <sstream>
#include "util/circular_deque.h"
#include "decoder/ogg_vorbis_decoder.h"
#include "platform/thread_priority.h"

using namespace std;

namespace programmerjake
{
namespace voxels
{
struct AudioData
{
    function<shared_ptr<AudioDecoder>()> makeAudioDecoder;
    double duration;
    AudioData(function<shared_ptr<AudioDecoder>()> makeAudioDecoder)
        : makeAudioDecoder(makeAudioDecoder), duration(makeAudioDecoder()->lengthInSeconds())
    {
    }
};

struct PlayingAudioData
{
    shared_ptr<AudioData> audioData;
    shared_ptr<AudioDecoder> decoder;
    static constexpr size_t bufferValueCount = 32768;
    circularDeque<float, bufferValueCount + 1> bufferQueue;
    float volume;
    bool looped;
    bool overallEOF = false;
    uint64_t playedSamples = 0;
    PlayingAudioData(shared_ptr<AudioData> audioData, float volume, bool looped)
        : audioData(audioData), decoder(), bufferQueue(), volume(volume), looped(looped)
    {
        restartDecoder();
    }
    void restartDecoder()
    {
        decoder = audioData->makeAudioDecoder();
        if(decoder->samplesPerSecond() != getGlobalAudioSampleRate())
            decoder = make_shared<ResampleAudioDecoder>(decoder, getGlobalAudioSampleRate());
        if(decoder->channelCount() != getGlobalAudioChannelCount())
            decoder = make_shared<RedistributeChannelsAudioDecoder>(decoder, getGlobalAudioChannelCount());
        if(decoder->isHighLatencySource())
            decoder = make_shared<StreamBufferingAudioDecoder>(decoder);
        unsigned channels = decoder->channelCount();
        typedef checked_array<float, bufferValueCount> BufferType;
        auto pbuffer = std::unique_ptr<BufferType>(new BufferType);
        auto &buffer = *pbuffer;
        uint64_t bufferSamples = (bufferQueue.capacity() - bufferQueue.size()) / channels;
        uint64_t decodedAmount;
        for(decodedAmount = 0;decodedAmount < bufferSamples;)
        {
            uint64_t currentDecodeStep = decoder->decodeAudioBlock(&buffer[static_cast<std::size_t>(decodedAmount * channels)], bufferSamples - decodedAmount);
            if(currentDecodeStep == 0)
            {
                if(!looped)
                    overallEOF = true;
                break;
            }
            decodedAmount += currentDecodeStep;
        }
        for(size_t i = 0; i < decodedAmount * channels; i++)
            bufferQueue.push_back(buffer[i]);
    }
    bool addInAudioSample(float * data)
    {
        unsigned channels = decoder->channelCount();
        if(!overallEOF && bufferQueue.size() < bufferQueue.capacity() / 2)
        {
            typedef checked_array<float, bufferValueCount> BufferType;
            auto pbuffer = std::unique_ptr<BufferType>(new BufferType);
            auto &buffer = *pbuffer;
            uint64_t bufferSamples = (bufferQueue.capacity() - bufferQueue.size()) / channels;
            uint64_t decodedAmount;
            bool hitEnd = false;
            for(decodedAmount = 0;decodedAmount < bufferSamples;)
            {
                uint64_t currentDecodeStep = decoder->decodeAudioBlock(&buffer[static_cast<std::size_t>(decodedAmount * channels)], bufferSamples - decodedAmount);
                if(currentDecodeStep == 0)
                {
                    hitEnd = true;
                    break;
                }
                decodedAmount += currentDecodeStep;
            }
            for(size_t i = 0; i < decodedAmount * channels; i++)
                bufferQueue.push_back(buffer[i]);
            if(hitEnd)
            {
                if(looped)
                    restartDecoder();
                else
                    overallEOF = true;
            }
        }
        if(bufferQueue.size() >= channels)
        {
            for(size_t i = 0; i < channels; i++)
            {
                *data++ += bufferQueue.front() * volume;
                bufferQueue.pop_front();
            }
            playedSamples++;
            return true;
        }
        return false;
    }
    bool addInAudio(float * data, size_t sampleCount)
    {
        unsigned channels = decoder->channelCount();
        for(size_t i = 0; i < sampleCount; i++)
        {
            if(!addInAudioSample(data))
            {
                return false;
            }
            data += channels;
        }
        return true;
    }
    bool hitEOF()
    {
        if(overallEOF && bufferQueue.size() < decoder->channelCount())
            return true;
        return false;
    }
};

namespace
{
mutex &getAudioStateMutex()
{
    static mutex audioStateMutex;
    return audioStateMutex;
}
unordered_set<shared_ptr<PlayingAudioData>> &getPlayingAudioSet()
{
    static unordered_set<shared_ptr<PlayingAudioData>> playingAudioSet;
    return playingAudioSet;
}

void startAudioImp(shared_ptr<PlayingAudioData> audio)
{
    assert(audio);
    {
        unique_lock<mutex> lock(getAudioStateMutex());
        getPlayingAudioSet().insert(audio);
    }

    programmerjake::voxels::startAudio();
}

void stopAudioImp(shared_ptr<PlayingAudioData> audio)
{
    assert(audio);

    bool terminateAudio = false;
    {
        unique_lock<mutex> lock(getAudioStateMutex());
        getPlayingAudioSet().erase(audio);
        if(getPlayingAudioSet().empty())
            terminateAudio = true;
    }
    if(terminateAudio)
        endAudio();
}
}

PlayingAudio::PlayingAudio(shared_ptr<PlayingAudioData> data)
    : data(data)
{
}

void PlayingAudio::audioCallback(void *, uint8_t * buffer_in, int length)
{
    setThreadPriority(ThreadPriority::High);
    static vector<float> buffer;
    unique_lock<mutex> lock(getAudioStateMutex());
    int16_t * buffer16 = (int16_t *)buffer_in;
    memset((void *)buffer16, 0, length);
    assert(length % (getGlobalAudioChannelCount() * sizeof(int16_t)) == 0);
    size_t sampleCount = length / (getGlobalAudioChannelCount() * sizeof(int16_t));
    buffer.assign(sampleCount * getGlobalAudioChannelCount(), 0);
    for(auto i = getPlayingAudioSet().begin(); i != getPlayingAudioSet().end(); )
    {
        if(!(*i)->addInAudio(buffer.data(), sampleCount))
            i = getPlayingAudioSet().erase(i);
        else
            i++;
    }
    for(float v : buffer)
    {
        *buffer16++ = limit<int>(static_cast<int>(v * 0x8000), numeric_limits<int16_t>::min(), numeric_limits<int16_t>::max());
    }
}

bool PlayingAudio::isPlaying()
{
    if(!data)
        return false;
    unique_lock<mutex> lock(getAudioStateMutex());
    return audioRunning() && !data->hitEOF();
}

double PlayingAudio::currentTime()
{
    if(!data)
        return 0;
    unique_lock<mutex> lock(getAudioStateMutex());
    return (double)data->playedSamples / data->decoder->samplesPerSecond();
}

void PlayingAudio::stop()
{
    if(!data)
        return;
    stopAudioImp(data);
}

float PlayingAudio::volume()
{
    if(!data)
        return 0;
    unique_lock<mutex> lock(getAudioStateMutex());
    return data->volume;
}

void PlayingAudio::volume(float v)
{
    if(!data)
        return;
    unique_lock<mutex> lock(getAudioStateMutex());
    data->volume = limit(v, 0.0f, 1.0f);
}

double PlayingAudio::duration()
{
    if(!data)
        return 0;
    unique_lock<mutex> lock(getAudioStateMutex());
    return data->decoder->lengthInSeconds();
}

Audio::Audio(wstring resourceName, bool isStreaming)
    : data()
{
    if(isStreaming)
    {
        data = make_shared<AudioData>([resourceName]()->shared_ptr<AudioDecoder>
        {
            try
            {
                shared_ptr<stream::Reader> preader = getResourceReader(resourceName);
                return make_shared<OggVorbisDecoder>(preader);
            }
            catch(stream::IOException &)
            {
                return make_shared<MemoryAudioDecoder>(vector<float>(), getGlobalAudioSampleRate(), getGlobalAudioChannelCount());
            }
        });
    }
    else
    {
        try
        {
            shared_ptr<stream::Reader> preader = getResourceReader(resourceName);
            shared_ptr<AudioDecoder> decoder = make_shared<OggVorbisDecoder>(preader);
            vector<float> buffer;
            buffer.resize(decoder->channelCount() * 8192);
            uint64_t finalSize = 0;
            for(;;)
            {
                buffer.resize(static_cast<std::size_t>(finalSize + decoder->channelCount() * 8192));
                uint64_t currentSize = decoder->channelCount() * decoder->decodeAudioBlock(&buffer[static_cast<std::size_t>(finalSize)], (buffer.size() - finalSize) / decoder->channelCount());
                if(currentSize == 0)
                    break;
                finalSize += currentSize;
            }
            buffer.resize(static_cast<std::size_t>(finalSize));
            shared_ptr<MemoryAudioDecoder> memDecoder = make_shared<MemoryAudioDecoder>(buffer, decoder->samplesPerSecond(), decoder->channelCount());
            data = make_shared<AudioData>([memDecoder]()->shared_ptr<AudioDecoder>{memDecoder->reset(); return memDecoder;});
        }
        catch(stream::IOException & e)
        {
            throw AudioLoadError(string("io exception : ") + e.what());
        }
    }
}

Audio::Audio(const vector<float> &data, unsigned sampleRate, unsigned channelCount)
    : data()
{
    shared_ptr<MemoryAudioDecoder> memDecoder = make_shared<MemoryAudioDecoder>(data, sampleRate, channelCount);
    this->data = make_shared<AudioData>([memDecoder]()->shared_ptr<AudioDecoder>{memDecoder->reset(); return memDecoder;});
}

shared_ptr<PlayingAudio> Audio::play(float volume, bool looped)
{
    if(!data)
        return shared_ptr<PlayingAudio>(new PlayingAudio(nullptr));
    programmerjake::voxels::startAudio();
    auto playingAudioData = make_shared<PlayingAudioData>(data, volume, looped);
    startAudioImp(playingAudioData);
    return shared_ptr<PlayingAudio>(new PlayingAudio(playingAudioData));
}

double Audio::duration()
{
    if(!data)
        return 0;
    return data->duration;
}

#if 0
namespace
{
initializer init1([]()
{
    cout << "press enter to exit.\n";
    vector<Audio> backgroundSongs;
    for(size_t i = 1; i <= 17; i++)
    {
        wostringstream os;
        os << L"background";
        if(i != 1)
            os << i;
        os << L".ogg";
        backgroundSongs.push_back(Audio(os.str(), true));
    }
    auto playingAudio = backgroundSongs[rand() % backgroundSongs.size()].play();
    atomic_bool done(false);
    thread([&done](){cin.ignore(10000, '\n'); done = true;}).detach();
    while(!done)
    {
        cout << playingAudio->currentTime() << "\x1b[K\r" << flush;
        this_thread::sleep_for(chrono::milliseconds(100));
        if(!playingAudio->isPlaying())
            playingAudio = backgroundSongs[rand() % backgroundSongs.size()].play();
        if(playingAudio->currentTime() >= 10)
            playingAudio->stop();
    }
    exit(0);
});
}
#endif // 1
}
}
