#include "platform/audio.h"
#include "platform/platform.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_set>
#include <SDL2/SDL.h>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include "util/circular_deque.h"
#include "decoder/ogg_vorbis_decoder.h"

using namespace std;

struct AudioData
{
    function<shared_ptr<AudioDecoder>()> makeAudioDecoder;
    AudioData(function<shared_ptr<AudioDecoder>()> makeAudioDecoder)
        : makeAudioDecoder(makeAudioDecoder)
    {
    }
};

struct PlayingAudioData
{
    shared_ptr<AudioData> audioData;
    shared_ptr<AudioDecoder> decoder;
    static constexpr size_t bufferValueCount = 32768;
    circularDeque<int16_t, bufferValueCount + 1> bufferQueue;
    float volume;
    bool looped;
    bool overallEOF = false;
    uint64_t playedSamples = 0;
    PlayingAudioData(shared_ptr<AudioData> audioData, float volume, bool looped)
        : audioData(audioData), volume(volume), looped(looped)
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
        unsigned channels = decoder->channelCount();
        int16_t buffer[bufferValueCount];
        uint64_t bufferSamples = (bufferQueue.capacity() - bufferQueue.size()) / channels;
        uint64_t decodedAmount;
        for(decodedAmount = 0;decodedAmount < bufferSamples;)
        {
            uint64_t currentDecodeStep = decoder->decodeAudioBlock(&buffer[decodedAmount * channels], bufferSamples - decodedAmount);
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
    bool addInAudioSample(int * data)
    {
        unsigned channels = decoder->channelCount();
        if(!overallEOF && bufferQueue.size() < bufferQueue.capacity() / 2)
        {
            int16_t buffer[bufferValueCount];
            uint64_t bufferSamples = (bufferQueue.capacity() - bufferQueue.size()) / channels;
            uint64_t decodedAmount;
            bool hitEnd = false;
            for(decodedAmount = 0;decodedAmount < bufferSamples;)
            {
                uint64_t currentDecodeStep = decoder->decodeAudioBlock(&buffer[decodedAmount * channels], bufferSamples - decodedAmount);
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
                *data++ += (int)floor(bufferQueue.front() * volume + 0.5f);
                bufferQueue.pop_front();
            }
            playedSamples++;
            return true;
        }
        return false;
    }
    bool addInAudio(int * data, size_t sampleCount)
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
mutex audioStateMutex;
unordered_set<shared_ptr<PlayingAudioData>> playingAudioSet;

void startAudio(shared_ptr<PlayingAudioData> audio)
{
    assert(audio);
    {
        unique_lock<mutex> lock(audioStateMutex);
        playingAudioSet.insert(audio);
    }

    ::startAudio();
}

void stopAudio(shared_ptr<PlayingAudioData> audio)
{
    assert(audio);

    bool terminateAudio = false;
    {
        unique_lock<mutex> lock(audioStateMutex);
        playingAudioSet.erase(audio);
        if(playingAudioSet.empty())
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
    static vector<int> buffer;
    unique_lock<mutex> lock(audioStateMutex);
    int16_t * buffer16 = (int16_t *)buffer_in;
    memset((void *)buffer16, 0, length);
    assert(length % (getGlobalAudioChannelCount() * sizeof(int16_t)) == 0);
    size_t sampleCount = length / (getGlobalAudioChannelCount() * sizeof(int16_t));
    buffer.assign(sampleCount * getGlobalAudioChannelCount(), 0);
    for(auto i = playingAudioSet.begin(); i != playingAudioSet.end(); )
    {
        if(!(*i)->addInAudio(buffer.data(), sampleCount))
            i = playingAudioSet.erase(i);
        else
            i++;
    }
    for(int v : buffer)
    {
        *buffer16++ = limit<int>(v, numeric_limits<int16_t>::min(), numeric_limits<int16_t>::max());
    }
}

bool PlayingAudio::isPlaying()
{
    return audioRunning() && !data->hitEOF();
}

double PlayingAudio::currentTime()
{
    return (double)data->playedSamples / data->decoder->samplesPerSecond();
}

void PlayingAudio::stop()
{
    stopAudio(data);
}

float PlayingAudio::volume()
{
    return data->volume;
}

void PlayingAudio::volume(float v)
{
    data->volume = limit(v, 0.0f, 1.0f);
}

double PlayingAudio::duration()
{
    return data->decoder->lengthInSeconds();
}

Audio::Audio(wstring resourceName, bool isStreaming)
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
            catch(stream::IOException & e)
            {
                return make_shared<MemoryAudioDecoder>(vector<int16_t>(), getGlobalAudioSampleRate(), getGlobalAudioChannelCount());
            }
        });
    }
    else
    {
        try
        {
            shared_ptr<stream::Reader> preader = getResourceReader(resourceName);
            shared_ptr<AudioDecoder> decoder = make_shared<OggVorbisDecoder>(preader);
            vector<int16_t> buffer;
            buffer.resize(decoder->channelCount() * 8192);
            uint64_t finalSize = 0;
            for(;;)
            {
                buffer.resize(finalSize + decoder->channelCount() * 8192);
                uint64_t currentSize = decoder->channelCount() * decoder->decodeAudioBlock(&buffer[finalSize], (buffer.size() - finalSize) / decoder->channelCount());
                if(currentSize == 0)
                    break;
                finalSize += currentSize;
            }
            buffer.resize(finalSize);
            shared_ptr<MemoryAudioDecoder> memDecoder = make_shared<MemoryAudioDecoder>(buffer, decoder->samplesPerSecond(), decoder->channelCount());
            data = make_shared<AudioData>([memDecoder]()->shared_ptr<AudioDecoder>{memDecoder->reset(); return memDecoder;});
        }
        catch(stream::IOException & e)
        {
            throw AudioLoadError(string("io exception : ") + e.what());
        }
    }
}

Audio::Audio(const vector<int16_t> &data, unsigned sampleRate, unsigned channelCount)
{
    shared_ptr<MemoryAudioDecoder> memDecoder = make_shared<MemoryAudioDecoder>(data, sampleRate, channelCount);
    this->data = make_shared<AudioData>([memDecoder]()->shared_ptr<AudioDecoder>{memDecoder->reset(); return memDecoder;});
}

shared_ptr<PlayingAudio> Audio::play(float volume, bool looped)
{
    ::startAudio();
    auto playingAudioData = make_shared<PlayingAudioData>(data, volume, looped);
    startAudio(playingAudioData);
    return shared_ptr<PlayingAudio>(new PlayingAudio(playingAudioData));
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
