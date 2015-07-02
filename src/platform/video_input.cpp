/*
 * Copyright (C) 2012-2015 Jacob R. Lifshay
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
#include "platform/video_input.h"
#include "util/util.h"
#include "util/string_cast.h"
#include <cstdlib>

namespace programmerjake
{
namespace voxels
{
namespace
{
std::vector<const VideoInputDevice *> makeVideoInputDeviceList();
}
}
}

#if _WIN64 || _WIN32

#include "videoInput.h"

namespace programmerjake
{
namespace voxels
{
namespace
{
class MyVideoInputDevice;

class MyVideoInput final : public VideoInput
{
    friend class MyVideoInputDevice;
    MyVideoInput(const MyVideoInput &) = delete;
    MyVideoInput &operator =(const MyVideoInput &) = delete;
private:
    ::videoInput input;
    bool good;
    const int deviceId;
    int actualWidth, actualHeight;
    std::vector<std::uint8_t> buffer;
    MyVideoInput(int deviceId, int requestedWidth, int requestedHeight, int requestedFrameRate)
        : input(), good(true), deviceId(deviceId), buffer()
    {
        if(requestedFrameRate > 0)
            input.setIdealFramerate(deviceId, requestedFrameRate);
        if(requestedWidth > 0 && requestedHeight > 0)
        {
            if(!input.setupDevice(deviceId, requestedWidth, requestedHeight))
            {
                good = false;
                return;
            }
        }
        else
        {
            if(!input.setupDevice(deviceId))
            {
                good = false;
                return;
            }
        }
        actualWidth = input.getWidth(deviceId);
        actualHeight = input.getHeight(deviceId);
        buffer.resize(static_cast<std::size_t>(actualWidth) * static_cast<std::size_t>(actualWidth) * Image::BytesPerPixel, 0);
    }
public:
    virtual ~MyVideoInput()
    {
        if(good)
        {
            input.stopDevice(deviceId);
        }
    }
    virtual void getSize(int &width, int &height) override
    {
        assert(good);
        width = actualWidth;
        height = actualHeight;
    }
    virtual void readFrameIntoImage(Image &dest) override
    {
        assert(good);
        if(input.isFrameNew(deviceId))
        {
            static_assert(sizeof(std::uint8_t) == sizeof(unsigned char), "std::uint8_t is not unsigned char");
            input.getPixels(deviceId, static_cast<unsigned char *>(&buffer[0]), false, false);
            for(std::size_t i = (std::size_t)actualWidth * (std::size_t)actualHeight; i > 0; i--)
            {
                std::uint8_t r = buffer[i * 3 + 2];
                std::uint8_t g = buffer[i * 3 + 1];
                std::uint8_t b = buffer[i * 3 + 0];
                std::uint8_t a = 0xFF;
                buffer[i * Image::BytesPerPixel + 0] = r;
                buffer[i * Image::BytesPerPixel + 1] = g;
                buffer[i * Image::BytesPerPixel + 2] = b;
                buffer[i * Image::BytesPerPixel + 3] = a;
            }
        }
        dest.setData(buffer, Image::RowOrder::BottomToTop);
    }
};

std::vector<const VideoInputDevice *> makeVideoInputDeviceList();

class MyVideoInputDevice final : public VideoInputDevice
{
    friend std::vector<const VideoInputDevice *> makeVideoInputDeviceList();
private:
    const int deviceId;
    MyVideoInputDevice(int deviceId, std::wstring name)
        : VideoInputDevice(name), deviceId(deviceId)
    {
    }
public:
    virtual std::unique_ptr<VideoInput> makeVideoInput(int requestedWidth, int requestedHeight, int requestedFrameRate) const override
    {
        std::unique_ptr<MyVideoInput> retval = std::unique_ptr<MyVideoInput>(new MyVideoInput(deviceId, requestedWidth, requestedHeight, requestedFrameRate));
        if(!retval->good)
            return std::unique_ptr<VideoInput>();
        return std::move(retval);
    }
};

std::vector<const VideoInputDevice *> makeVideoInputDeviceList()
{
    ::videoInput::setComMultiThreaded(true);
    ::videoInput::setVerbose(false);
    std::vector<const VideoInputDevice *> deviceList;
    int deviceCount = ::videoInput::listDevices(true);
    for(int deviceId = 0; deviceId < deviceCount; deviceId++)
    {
        const char *name = ::videoInput::getDeviceName(deviceId);
        if(name == nullptr)
            continue;
        deviceList.push_back(new MyVideoInputDevice(deviceId, string_cast<std::wstring>(name)));
    }
    return std::move(deviceList);
}
}
}
}
#else
FIXME_MESSAGE(implement video input for platform)
namespace programmerjake
{
namespace voxels
{
namespace
{
std::vector<const VideoInputDevice *> makeVideoInputDeviceList()
{
    return std::vector<const VideoInputDevice *>();
}
}
}
}
#endif

namespace programmerjake
{
namespace voxels
{
const std::vector<const VideoInputDevice *> &getVideoInputDeviceList()
{
    static std::vector<const VideoInputDevice *> retval = makeVideoInputDeviceList();
    return retval;
}
}
}
