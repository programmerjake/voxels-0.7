/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include <cwchar>
#include <string>
#include <memory>
#include <cstdint>
#include <vector>
#include <array>
#include <limits>
#include "stream/stream.h"
#include "util/util.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

namespace programmerjake
{
namespace voxels
{
class AudioLoadError final : public std::runtime_error
{
public:
    explicit AudioLoadError(const std::string &arg)
        : runtime_error(arg)
    {
    }
};

class AudioDecoder
{
    AudioDecoder(const AudioDecoder &) = delete;
    const AudioDecoder & operator =(const AudioDecoder &) = delete;
public:
    static constexpr std::uint64_t Unknown = ~(std::uint64_t)0;
    AudioDecoder()
    {
    }
    virtual ~AudioDecoder()
    {
    }
    virtual unsigned samplesPerSecond() = 0;
    virtual std::uint64_t numSamples() = 0;
    virtual unsigned channelCount() = 0;
    double lengthInSeconds()
    {
        std::uint64_t count = numSamples();
        if(count == Unknown)
            return -1;
        return (double)count / samplesPerSecond();
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t samplesCount) = 0; // returns number of samples decoded
    virtual bool isHighLatencySource() const = 0;
};

class MemoryAudioDecoder final : public AudioDecoder
{
    std::vector<float> samples;
    unsigned sampleRate;
    std::size_t currentLocation;
    unsigned channels;
public:
    MemoryAudioDecoder(const std::vector<float> &data, unsigned sampleRate, unsigned channelCount)
        : samples(data), sampleRate(sampleRate), currentLocation(0), channels(channelCount)
    {
        assert(sampleRate > 0);
        assert(channels > 0);
        assert(samples.size() % channels == 0);
    }
    virtual unsigned samplesPerSecond() override
    {
        return sampleRate;
    }
    virtual std::uint64_t numSamples() override
    {
        return samples.size() / channels;
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t samplesCount) override // returns number of samples decoded
    {
        std::uint64_t retval = 0;
        for(std::uint64_t i = 0; i < samplesCount && currentLocation < samples.size(); i++)
        {
            for(unsigned j = 0; j < channels; j++)
                *data++ = samples[currentLocation++];
            retval++;
        }
        return retval;
    }
    void reset()
    {
        currentLocation = 0;
    }
    virtual bool isHighLatencySource() const override
    {
        return false;
    }
};

class ResampleAudioDecoder final : public AudioDecoder
{
    std::shared_ptr<AudioDecoder> decoder;
    std::uint64_t position = 0; // in samples
    std::vector<float> buffer;
    std::uint64_t bufferStartPosition = 0; // in values
    unsigned sampleRate, channels;
public:
    ResampleAudioDecoder(std::shared_ptr<AudioDecoder> decoder, unsigned sampleRate)
        : decoder(decoder), sampleRate(sampleRate), channels(decoder->channelCount())
    {
        constexpr std::size_t size = 1024;
        buffer.resize(channels * size);
        buffer.resize(channels * decoder->decodeAudioBlock(buffer.data(), size));
    }
    virtual unsigned samplesPerSecond() override
    {
        return sampleRate;
    }
    virtual std::uint64_t numSamples() override
    {
        std::uint64_t count = decoder->numSamples();
        if(count == Unknown)
            return Unknown;
        return (count * sampleRate) / decoder->samplesPerSecond();
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t sampleCount) override
    {
        if(numSamples() != Unknown)
            sampleCount = std::min(numSamples() - position, sampleCount);
        if(sampleCount == 0 || buffer.size() == 0)
            return sampleCount;
        double rateConversionFactor = (double)decoder->samplesPerSecond() / sampleRate;
        std::uint64_t retval = 0;
        for(std::uint64_t i = 0; i < sampleCount; i++, position++, retval++)
        {
            double finalPosition = position * rateConversionFactor;
            std::uint64_t startIndex = (std::uint64_t)floor(finalPosition);
            float t = (float)(finalPosition - startIndex);
            startIndex *= channels;
            std::uint64_t endIndex = startIndex + channels;
            assert(startIndex >= bufferStartPosition);
            if(endIndex - bufferStartPosition >= buffer.size())
            {
                for(unsigned i = 0; i < channels; i++)
                    buffer[i] = buffer[buffer.size() - channels + i];
                bufferStartPosition += buffer.size() - channels;
                buffer.resize(buffer.capacity());
                std::uint64_t decodeCount = 1, lastDecodeCount;
                do
                {
                    lastDecodeCount = decodeCount;
                    decodeCount += decoder->decodeAudioBlock(&buffer[decodeCount * channels], buffer.capacity() / channels - decodeCount);
                }
                while(decodeCount != lastDecodeCount && buffer.capacity() / channels < decodeCount);
                buffer.resize(decodeCount * channels);
            }
            if(startIndex - bufferStartPosition >= buffer.size() || (t > 0 && endIndex - bufferStartPosition >= buffer.size()))
                return retval;
            if(t > 0)
            {
                for(unsigned j = 0; j < channels; j++)
                {
                    *data++ = ((1 - t) * buffer[j + (std::size_t)(startIndex - bufferStartPosition)] + t * buffer[j + (std::size_t)(endIndex - bufferStartPosition)]);
                }
            }
            else
            {
                for(unsigned j = 0; j < channels; j++)
                {
                    *data++ = buffer[j + (std::size_t)(startIndex - bufferStartPosition)];
                }
            }
        }
        return retval;
    }
    virtual bool isHighLatencySource() const override
    {
        return decoder->isHighLatencySource();
    }
};

class RedistributeChannelsAudioDecoder final : public AudioDecoder
{
    std::shared_ptr<AudioDecoder> decoder;
    unsigned channels;
    std::vector<float> buffer;
public:
    RedistributeChannelsAudioDecoder(std::shared_ptr<AudioDecoder> decoder, unsigned channelCountIn)
        : decoder(decoder), channels(channelCountIn)
    {
    }
    virtual unsigned samplesPerSecond() override
    {
        return decoder->samplesPerSecond();
    }
    virtual std::uint64_t numSamples() override
    {
        return decoder->numSamples();
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t sampleCount) override
    {
        unsigned sourceChannels = decoder->channelCount();
        if(sourceChannels == channels)
            return decoder->decodeAudioBlock(data, sampleCount);
        buffer.resize(sampleCount * sourceChannels);
        sampleCount = decoder->decodeAudioBlock(buffer.data(), sampleCount);
        switch(channels)
        {
        case 1:
        {
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                int sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                *data++ = sum / sourceChannels;
            }
            break;
        }
        case 2:
        {
            switch(sourceChannels)
            {
            case 1:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex++];
                }
                break;
            case 3:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2) / 3;
                    *data++ = (channel2 + 2 * channel3) / 3;
                }
                break;
            case 4:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    *data++ = (channel1 + channel3) / 2;
                    *data++ = (channel2 + channel4) / 2;
                }
                break;
            case 5:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2 + 2 * channel4) / 5;
                    *data++ = (2 * channel3 + channel2 + 2 * channel5) / 5;
                }
                break;
            case 6:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    float channel6 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2 + 2 * channel4) / 5 + channel6;
                    *data++ = (2 * channel3 + channel2 + 2 * channel5) / 5 + channel6;
                }
                break;
            default:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float sum = 0;
                    for(std::size_t j = 0; j < sourceChannels; j++)
                        sum += buffer[bufferIndex++];
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                }
                break;
            }
            break;
        }
        case 3:
        {
            switch(sourceChannels)
            {
            case 1:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex++];
                }
                break;
            case 2:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (channel1 + channel2) / 2;
                    *data++ = (5 * channel2 - channel1) / 4;
                }
                break;
            case 4:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    *data++ = (5 * (channel1 + channel3) - channel2 - channel4) / 8;
                    *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                    *data++ = (5 * (channel2 + channel4) - channel1 - channel3) / 8;
                }
                break;
            case 5:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    *data++ = (channel1 + channel4) / 2;
                    *data++ = channel2;
                    *data++ = (channel3 + channel5) / 2;
                }
                break;
            case 6:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    float channel6 = buffer[bufferIndex++];
                    *data++ = (channel1 + channel4) / 2 + channel6;
                    *data++ = channel2 + channel6;
                    *data++ = (channel3 + channel5) / 2 + channel6;
                }
                break;
            default:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float sum = 0;
                    for(std::size_t j = 0; j < sourceChannels; j++)
                        sum += buffer[bufferIndex++];
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                }
                break;
            }
            break;
        }
        case 4:
        {
            switch(sourceChannels)
            {
            case 1:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex++];
                }
                break;
            case 2:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    *data++ = channel1;
                    *data++ = channel2;
                    *data++ = channel1;
                    *data++ = channel2;
                }
                break;
            case 3:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2) / 3;
                    *data++ = (channel2 + 2 * channel3) / 3;
                    *data++ = (2 * channel1 + channel2) / 3;
                    *data++ = (channel2 + 2 * channel3) / 3;
                }
                break;
            case 5:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2) / 3;
                    *data++ = (2 * channel3 + channel2) / 3;
                    *data++ = channel4;
                    *data++ = channel5;
                }
                break;
            case 6:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    float channel6 = buffer[bufferIndex++];
                    *data++ = (2 * channel1 + channel2) / 3 + channel6;
                    *data++ = (2 * channel3 + channel2) / 3 + channel6;
                    *data++ = channel4 + channel6;
                    *data++ = channel5 + channel6;
                }
                break;
            default:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float sum = 0;
                    for(std::size_t j = 0; j < sourceChannels; j++)
                        sum += buffer[bufferIndex++];
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                }
                break;
            }
            break;
        }
        case 5:
        {
            switch(sourceChannels)
            {
            case 1:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex++];
                }
                break;
            case 2:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (channel1 + channel2) / 2;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                }
                break;
            case 3:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    *data++ = channel1;
                    *data++ = channel2;
                    *data++ = channel3;
                    *data++ = channel1;
                    *data++ = channel3;
                }
                break;
            case 4:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                }
                break;
            case 6:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    float channel6 = buffer[bufferIndex++];
                    *data++ = channel1 + channel6;
                    *data++ = channel2 + channel6;
                    *data++ = channel3 + channel6;
                    *data++ = channel4 + channel6;
                    *data++ = channel5 + channel6;
                }
                break;
            default:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float sum = 0;
                    for(std::size_t j = 0; j < sourceChannels; j++)
                        sum += buffer[bufferIndex++];
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                }
                break;
            }
            break;
        }
        case 6:
        {
            switch(sourceChannels)
            {
            case 1:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex];
                    *data++ = buffer[bufferIndex++];
                }
                break;
            case 2:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (channel1 + channel2) / 2;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (channel1 + channel2) / 2;
                }
                break;
            case 3:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    *data++ = channel1;
                    *data++ = channel2;
                    *data++ = channel3;
                    *data++ = channel1;
                    *data++ = channel3;
                    *data++ = (channel1 + channel2 + channel3) / 3;
                }
                break;
            case 4:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (5 * channel1 - channel2) / 4;
                    *data++ = (5 * channel2 - channel1) / 4;
                    *data++ = (channel1 + channel2 + channel3 + channel4) / 4;
                }
                break;
            case 5:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float channel1 = buffer[bufferIndex++];
                    float channel2 = buffer[bufferIndex++];
                    float channel3 = buffer[bufferIndex++];
                    float channel4 = buffer[bufferIndex++];
                    float channel5 = buffer[bufferIndex++];
                    *data++ = channel1;
                    *data++ = channel2;
                    *data++ = channel3;
                    *data++ = channel4;
                    *data++ = channel5;
                    *data++ = (channel1 + channel2 + channel3 + channel4 + channel5) / 5;
                }
                break;
            default:
                for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
                {
                    float sum = 0;
                    for(std::size_t j = 0; j < sourceChannels; j++)
                        sum += buffer[bufferIndex++];
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                    *data++ = sum / sourceChannels;
                }
                break;
            }
            break;
        }
        default:
        {
            for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
            {
                float sum = 0;
                for(std::size_t j = 0; j < sourceChannels; j++)
                    sum += buffer[bufferIndex++];
                for(std::size_t j = 0; j < channels; j++)
                    *data++ = sum / sourceChannels;
            }
            break;
        }
        }
        return sampleCount;
    }
    virtual bool isHighLatencySource() const override
    {
        return decoder->isHighLatencySource();
    }
};

class StreamBufferingAudioDecoder final : public AudioDecoder
{
    std::shared_ptr<AudioDecoder> decoder;
    static constexpr std::size_t bufferSize = 256, bufferCount = 256;
    std::vector<float> leftovers;
    std::size_t leftoversCurrentLocation = 0;
    std::list<std::vector<float>> fullBuffers;
    std::list<std::vector<float>> emptyBuffers;
    std::size_t fullBufferCount = 0, emptyBufferCount = 0;
    std::mutex buffersListsLock;
    std::condition_variable readerCond;
    std::condition_variable writerCond;
    bool done;
    std::thread decoderThread;
    void decoderThreadFn()
    {
        std::unique_lock<std::mutex> lockIt(buffersListsLock);
        while(!done)
        {
            while(!done && emptyBufferCount == 0)
                writerCond.wait(lockIt);
            if(done)
                break;
            std::list<std::vector<float>> theBufferInList;
            theBufferInList.splice(theBufferInList.begin(), emptyBuffers, emptyBuffers.begin());
            emptyBufferCount--;
            lockIt.unlock();
            std::vector<float> &buffer = theBufferInList.front();
            buffer.resize(bufferSize * channelCountValue);
            std::uint64_t decodedSampleCount = decoder->decodeAudioBlock(buffer.data(), bufferSize);
            buffer.resize(decodedSampleCount * channelCountValue);
            lockIt.lock();
            if(buffer.empty())
            {
                done = true;
            }
            else
            {
                fullBuffers.splice(fullBuffers.end(), theBufferInList, theBufferInList.begin());
                fullBufferCount++;
            }
            readerCond.notify_all();
        }
    }
    std::uint64_t numSamplesValue;
    unsigned channelCountValue;
    unsigned samplesPerSecondValue;
public:
    StreamBufferingAudioDecoder(std::shared_ptr<AudioDecoder> decoder)
        : decoder(decoder)
    {
        numSamplesValue = decoder->numSamples();
        channelCountValue = decoder->channelCount();
        samplesPerSecondValue = decoder->samplesPerSecond();
        emptyBuffers.resize(bufferCount);
        fullBufferCount = 0;
        emptyBufferCount = bufferCount;
        leftoversCurrentLocation = 0;
        done = false;
        decoderThread = std::thread([this]()
        {
            decoderThreadFn();
        });
    }
    ~StreamBufferingAudioDecoder()
    {
        std::unique_lock<std::mutex> lockIt(buffersListsLock);
        done = true;
        writerCond.notify_all();
        lockIt.unlock();
        decoderThread.join();
    }
    virtual unsigned samplesPerSecond() override
    {
        return samplesPerSecondValue;
    }
    virtual std::uint64_t numSamples() override
    {
        return numSamplesValue;
    }
    virtual unsigned channelCount() override
    {
        return channelCountValue;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t samplesCount) override // returns number of samples decoded
    {
        std::uint64_t retval = 0;
        std::unique_lock<std::mutex> lockIt(buffersListsLock);
        for(; samplesCount > 0; samplesCount--, retval++)
        {
            if(leftoversCurrentLocation + channelCountValue > leftovers.size())
            {
                leftoversCurrentLocation = 0;
                leftovers.clear();
                while(fullBufferCount == 0)
                {
                    if(done)
                        return retval;
                    readerCond.wait(lockIt);
                }
                leftovers.swap(fullBuffers.front());
                emptyBuffers.splice(emptyBuffers.end(), fullBuffers, fullBuffers.begin());
                emptyBufferCount++;
                fullBufferCount--;
                writerCond.notify_all();
            }
            for(unsigned i = 0; i < channelCountValue; i++)
            {
                *data++ = leftovers[leftoversCurrentLocation++];
            }
        }
        return retval;
    }
    virtual bool isHighLatencySource() const override
    {
        return false;
    }
};

class LoopingAudioDecoder final : public AudioDecoder
{
    std::shared_ptr<AudioDecoder> decoder;
    std::function<std::shared_ptr<AudioDecoder>()> decoderFactory;
    unsigned samplesPerSecondValue;
    unsigned channelCountValue;
    bool isHighLatencySourceValue;
public:
    LoopingAudioDecoder(std::function<std::shared_ptr<AudioDecoder>()> decoderFactory, bool isHighLatencySourceValue = true)
        : decoderFactory(decoderFactory), isHighLatencySourceValue(isHighLatencySourceValue)
    {
        decoder = decoderFactory();
        if(!decoder)
        {
            samplesPerSecondValue = 44100;
            channelCountValue = 1;
        }
        else
        {
            samplesPerSecondValue = decoder->samplesPerSecond();
            channelCountValue = decoder->channelCount();
        }
    }
    virtual unsigned samplesPerSecond() override
    {
        return samplesPerSecondValue;
    }
    virtual std::uint64_t numSamples() override
    {
        return Unknown;
    }
    virtual unsigned channelCount() override
    {
        return channelCountValue;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t samplesCount) override // returns number of samples decoded
    {
        std::uint64_t retval = 0;
        while(samplesCount > 0)
        {
            if(!decoder)
                return retval;
            std::uint64_t decodedAmount = decoder->decodeAudioBlock(data, samplesCount);
            retval += decodedAmount;
            samplesCount -= decodedAmount;
            if(decodedAmount == 0)
            {
                decoder = nullptr; // free decoder first to save memory
                decoder = decoderFactory();
                if(!decoder)
                    return retval;
                if(decoder->channelCount() != channelCountValue)
                    decoder = std::make_shared<RedistributeChannelsAudioDecoder>(decoder, channelCountValue);
                if(decoder->samplesPerSecond() != samplesPerSecondValue)
                    decoder = std::make_shared<ResampleAudioDecoder>(decoder, samplesPerSecondValue);
            }
        }
        return retval;
    }
    virtual bool isHighLatencySource() const override
    {
        return isHighLatencySourceValue;
    }
};

struct AudioData;
struct PlayingAudioData;

class PlayingAudio final
{
    std::shared_ptr<PlayingAudioData> data;
    PlayingAudio(std::shared_ptr<PlayingAudioData> data);
    friend class Audio;
public:
    bool isPlaying();
    double currentTime();
    void stop();
    float volume();
    void volume(float v);
    double duration();
    ~PlayingAudio()
    {
        stop();
    }

#ifndef DOXYGEN
    //for integration with SDL
    static void audioCallback(void * userData, std::uint8_t * buffer, int length);
#endif
};

class Audio final
{
    std::shared_ptr<AudioData> data;
public:
    Audio()
        : data(nullptr)
    {
    }
    Audio(std::nullptr_t)
        : data(nullptr)
    {
    }
    explicit Audio(std::wstring resourceName, bool isStreaming = false);
    explicit Audio(const std::vector<float> &data, unsigned sampleRate, unsigned channelCount);
    std::shared_ptr<PlayingAudio> play(float volume = 1, bool looped = false);
    inline std::shared_ptr<PlayingAudio> play(bool looped)
    {
        return play(1, looped);
    }
};
}
}

#endif // AUDIO_H_INCLUDED
