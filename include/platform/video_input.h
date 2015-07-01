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
#ifndef VIDEO_INPUT_H_INCLUDED
#define VIDEO_INPUT_H_INCLUDED

#include "texture/image.h"
#include <vector>
#include <memory>

namespace programmerjake
{
namespace voxels
{
class VideoInput
{
    VideoInput(const VideoInput &) = delete;
    VideoInput &operator =(const VideoInput &) = delete;
public:
    VideoInput() = default;
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
    virtual void readFrameIntoImage(Image &dest, int left, int top) = 0;
    void readFrameIntoImage(Image &dest)
    {
        readFrameIntoImage(dest, 0, 0);
    }
};

class CameraDevice
{
    CameraDevice(const CameraDevice &) = delete;
    CameraDevice &operator =(const CameraDevice &) = delete;
public:
    const std::wstring name;
    CameraDevice(std::wstring name)
        : name(name)
    {
    }
    virtual ~CameraDevice() = default;
    virtual std::unique_ptr<VideoInput> makeVideoInput() const = 0;
};

const std::vector<const CameraDevice *> &getCameraDeviceList();

}
}

#endif // VIDEO_INPUT_H_INCLUDED
