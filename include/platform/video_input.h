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
#ifndef VIDEO_INPUT_H_INCLUDED
#define VIDEO_INPUT_H_INCLUDED

#include "texture/image.h"
#include <vector>
#include <memory>
#include <iterator>

namespace programmerjake
{
namespace voxels
{
class VideoInputDevice;

class VideoInput
{
    VideoInput(const VideoInput &) = delete;
    VideoInput &operator =(const VideoInput &) = delete;
public:
    const VideoInputDevice *const videoInputDevice;
    explicit VideoInput(const VideoInputDevice *videoInputDevice)
        : videoInputDevice(videoInputDevice)
    {
    }
    virtual ~VideoInput() = default;
    int getWidth()
    {
        int w, h;
        getSize(w, h);
        return w;
    }
    int getHeight()
    {
        int w, h;
        getSize(w, h);
        return h;
    }
    virtual void getSize(int &width, int &height) = 0;
    virtual void readFrameIntoImage(Image &dest) = 0;
};

class VideoInputDevice
{
    VideoInputDevice(const VideoInputDevice &) = delete;
    VideoInputDevice &operator =(const VideoInputDevice &) = delete;
public:
    const std::wstring name;
    VideoInputDevice(std::wstring name)
        : name(name)
    {
    }
    virtual ~VideoInputDevice() = default;
    virtual std::unique_ptr<VideoInput> makeVideoInput(int requestedWidth, int requestedHeight, int requestedFrameRate) const = 0; /// can only have one VideoInput at a time
    std::unique_ptr<VideoInput> makeVideoInput(int requestedWidth, int requestedHeight) const
    {
        return makeVideoInput(requestedWidth, requestedHeight, -1);
    }
    std::unique_ptr<VideoInput> makeVideoInput(int requestedFrameRate = -1) const
    {
        return makeVideoInput(-1, -1, requestedFrameRate);
    }
};

const std::vector<const VideoInputDevice *> &getVideoInputDeviceList();

}
}

#endif // VIDEO_INPUT_H_INCLUDED
