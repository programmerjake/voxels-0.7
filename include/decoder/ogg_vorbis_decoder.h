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
#ifndef OGG_VORBIS_DECODER_H_INCLUDED
#define OGG_VORBIS_DECODER_H_INCLUDED

#include "stream/stream.h"
#include "platform/audio.h"
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#include <cerrno>
#include <iostream>
#include "util/logging.h"

namespace programmerjake
{
namespace voxels
{
class OggVorbisDecoder final : public AudioDecoder
{
private:
    OggVorbis_File ovf;
    std::shared_ptr<stream::Reader> reader;
    bool seekFailed = false;
    std::uint64_t samples;
    unsigned channels;
    unsigned sampleRate;
    std::uint64_t curPos = 0;
    std::vector<float> buffer;
    std::size_t currentBufferPos = 0;
    static std::size_t read_fn(void *dataPtr_in, std::size_t blockSize, std::size_t numBlocks, void *dataSource)
    {
        OggVorbisDecoder &decoder = *(OggVorbisDecoder *)dataSource;
        if(decoder.seekFailed)
        {
            errno = EIO;
            return 0;
        }
        std::size_t readCount = 0;
        try
        {
            std::uint8_t * dataPtr = (std::uint8_t *)dataPtr_in;
            for(std::size_t i = 0; i < numBlocks; i++, readCount++)
            {
                std::size_t currentBlockSize = decoder.reader->readBytes(dataPtr, blockSize);
                if(currentBlockSize == 0)
                {
                    return readCount;
                }
                if(currentBlockSize != blockSize)
                {
                    errno = EIO;
                    return 0;
                }
                dataPtr += blockSize;
            }
        }
        catch(stream::IOException &)
        {
            errno = EIO;
            return 0;
        }
        return readCount;
    }
    static long tell_fn(void *dataSource)
    {
        OggVorbisDecoder &decoder = *(OggVorbisDecoder *)dataSource;
        try
        {
            std::int64_t retval = decoder.reader->tell();
            return (long)retval;
        }
        catch(stream::IOException &)
        {
            errno = EIO;
            return -1;
        }
    }
    static int seek_fn(void *dataSource, ogg_int64_t offset, int whence)
    {
        OggVorbisDecoder &decoder = *(OggVorbisDecoder *)dataSource;
        stream::SeekPosition seekPosition;
        decoder.seekFailed = false;
        switch(whence)
        {
        case SEEK_SET:
            seekPosition = stream::SeekPosition::Start;
            break;
        case SEEK_CUR:
            seekPosition = stream::SeekPosition::Current;
            break;
        case SEEK_END:
            seekPosition = stream::SeekPosition::End;
            break;
        default:
            decoder.seekFailed = true;
            return 0;
        }
        try
        {
            decoder.reader->seek(offset, seekPosition);
        }
        catch(stream::IOException &)
        {
            decoder.seekFailed = true;
            return 0;
        }
        return 0;
    }
    inline void readBuffer()
    {
        buffer.resize(buffer.capacity());
        int currentSection;
        currentBufferPos = 0;
        float **pcmChannels = nullptr;
        long sampleCountLong = ov_read_float(&ovf, &pcmChannels, buffer.size() / channels, &currentSection);
        if(sampleCountLong < 0)
            sampleCountLong = 0;
        std::size_t sampleCount = sampleCountLong;
        buffer.resize(sampleCount * channels);
        for(std::size_t sample = 0, bufferIndex = 0; sample < sampleCount; sample++)
        {
            for(unsigned channel = 0; channel < channels; channel++)
            {
                buffer[bufferIndex++] = pcmChannels[channel][sample];
            }
        }
    }
public:
    OggVorbisDecoder(std::shared_ptr<stream::Reader> reader)
        : ovf(), reader(reader), samples(), channels(), sampleRate(), buffer()
    {
        ov_callbacks callbacks;
        callbacks.read_func = &read_fn;
        callbacks.close_func = nullptr;
        callbacks.seek_func = &seek_fn;
        callbacks.tell_func = &tell_fn;
        try
        {
            reader->tell();
        }
        catch(stream::NonSeekableException &)
        {
            callbacks.seek_func = nullptr;
            callbacks.tell_func = nullptr;
        }
        int openRetval = ov_open_callbacks((void *)this, &ovf, NULL, 0, callbacks);
        switch(openRetval)
        {
        case 0:
            break;
        default:
            throw stream::IOException("invalid ogg vorbis audio");
        }
        vorbis_info *info = ov_info(&ovf, -1);
        channels = info->channels;
        buffer.reserve(channels * 8192);
        sampleRate = info->rate;
        readBuffer();
        auto samples = ov_pcm_total(&ovf, -1);
        if(samples == OV_EINVAL)
            this->samples = Unknown;
        else
            this->samples = samples;
        if(samples == 0 || channels == 0 || sampleRate == 0)
        {
            ov_clear(&ovf);
            throw stream::IOException("invalid ogg vorbis audio");
        }
    }
    virtual ~OggVorbisDecoder()
    {
        ov_clear(&ovf);
    }
    virtual unsigned samplesPerSecond() override
    {
        return sampleRate;
    }
    virtual std::uint64_t numSamples() override
    {
        return samples;
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual std::uint64_t decodeAudioBlock(float * data, std::uint64_t readCount) override // returns number of samples decoded
    {
        std::uint64_t retval = 0;
        std::size_t dataIndex = 0;
        for(std::uint64_t i = 0; i < readCount; i++, retval++)
        {
            if(currentBufferPos >= buffer.size())
            {
                readBuffer();
                if(buffer.size() == 0)
                    break;
            }
            for(unsigned j = 0; j < channels; j++)
            {
                data[dataIndex++] = buffer[currentBufferPos++];
            }
        }
        curPos += retval;
        return retval;
    }
    virtual bool isHighLatencySource() const override
    {
        return true;
    }
};
}
}

#endif // OGG_VORBIS_DECODER_H_INCLUDED
