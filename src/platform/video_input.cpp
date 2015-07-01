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
#include "platform/video_input.h"
#include "util/util.h"
#include <cstdlib>

namespace programmerjake
{
namespace voxels
{
namespace
{

typedef void (*PFN_avdevice_register_all)(void);
PFN_avdevice_register_all avdevice_register_all = nullptr;
typedef AVInputFormat *(*PFN_av_input_video_device_next)(AVInputFormat *d);
PFN_av_input_video_device_next av_input_video_device_next = nullptr;

bool loadFFmpeg()
{
#if _WIN64 || _WIN32
    static DynamicLinkLibrary avformat(L"avformat-56.dll");
    static DynamicLinkLibrary avdevice(L"avdevice-56.dll");
#else
    FIXME_MESSAGE(implement loadFFmpeg for platform)
#endif
    if(!avformat)
        return false;
    if(!avdevice)
        return false;
    avdevice_register_all = (PFN_avdevice_register_all)avdevice.resolve(L"avdevice_register_all");
    av_input_video_device_next = (PFN_av_input_video_device_next)avdevice.resolve(L"av_input_video_device_next");
    avdevice_register_all();
    return true;
}

#if 1
FIXME_MESSAGE(finish implementing video_input.cpp)
#else
class FFmpegVideoInput final : public VideoInput
{

};

class FFmpegCameraDevice final : public CameraDevice
{

};
#endif

std::vector<const CameraDevice *> makeCameraDeviceList()
{
    std::vector<const CameraDevice *> deviceList;
    if(!loadFFmpeg())
        return std::move(deviceList);
    FIXME_MESSAGE(finish makeCameraDeviceList)
    return std::move(deviceList);
}
}

const std::vector<const CameraDevice *> &getCameraDeviceList()
{
    static std::vector<const CameraDevice *> retval = makeCameraDeviceList();
}
}
}
