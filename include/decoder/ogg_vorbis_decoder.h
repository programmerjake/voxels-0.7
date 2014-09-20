#ifndef OGG_VORBIS_DECODER_H_INCLUDED
#define OGG_VORBIS_DECODER_H_INCLUDED

#include "stream/stream.h"
#include "platform/audio.h"
#include <vorbis/vorbisfile.h>
#include <cerrno>
#include <iostream>
#include <endian.h>

class OggVorbisDecoder final : public AudioDecoder
{
private:
    OggVorbis_File ovf;
    shared_ptr<stream::Reader> reader;
    uint64_t samples;
    unsigned channels;
    unsigned sampleRate;
    uint64_t curPos = 0;
    vector<int16_t> buffer;
    size_t currentBufferPos = 0;
    static size_t read_fn(void * dataPtr_in, size_t blockSize, size_t numBlocks, void * dataSource)
    {
        OggVorbisDecoder & decoder = *(OggVorbisDecoder *)dataSource;
        size_t readCount = 0;
        try
        {
            uint8_t * dataPtr = (uint8_t *)dataPtr_in;
            for(size_t i = 0; i < numBlocks; i++, readCount++)
            {
                decoder.reader->readBytes(dataPtr, blockSize);
                dataPtr += blockSize;
            }
        }
        catch(stream::EOFException & e)
        {
            return readCount;
        }
        catch(stream::IOException & e)
        {
            errno = EIO;
            return readCount;
        }
        return readCount;
    }
    inline void readBuffer()
    {
        buffer.resize(buffer.capacity());
        int currentSection;
        currentBufferPos = 0;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        buffer.resize(ov_read(&ovf, (char *)buffer.data(), buffer.size() * sizeof(int16_t), 0, sizeof(int16_t), 1, &currentSection) / sizeof(int16_t));
#elif __BYTE_ORDER == __BIG_ENDIAN
        buffer.resize(ov_read(&ovf, (char *)buffer.data(), buffer.size() * sizeof(int16_t), 1, sizeof(int16_t), 1, &currentSection) / sizeof(int16_t));
#else
#error invalid endian value
#endif
        assert(buffer.size() % channels == 0);
    }
public:
    OggVorbisDecoder(shared_ptr<stream::Reader> reader)
        : reader(reader)
    {
        ov_callbacks callbacks;
        callbacks.read_func = &read_fn;
        callbacks.seek_func = nullptr;
        callbacks.close_func = nullptr;
        callbacks.tell_func = nullptr;
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
    virtual uint64_t numSamples() override
    {
        return samples;
    }
    virtual unsigned channelCount() override
    {
        return channels;
    }
    virtual uint64_t decodeAudioBlock(int16_t * data, uint64_t readCount) override // returns number of samples decoded
    {
        uint64_t retval = 0;
        size_t dataIndex = 0;
        for(uint64_t i = 0; i < readCount; i++, retval++)
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
};

#endif // OGG_VORBIS_DECODER_H_INCLUDED
