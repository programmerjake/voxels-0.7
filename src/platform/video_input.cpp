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

#if 0
extern "C"
{
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}
#endif

namespace programmerjake
{
namespace voxels
{
namespace
{

#if 0
#define DECLARE_FUNCTION(fn) \
typedef decltype(::fn) *PFN_##fn; \
PFN_##fn fn = nullptr;

DECLARE_FUNCTION(avdevice_register_all)
DECLARE_FUNCTION(av_input_video_device_next)
DECLARE_FUNCTION(av_register_all)

#undef DECLARE_FUNCTION
#endif

bool loadFFmpeg()
{
#if 1
    return false;
#else
    static bool loaded = false;
    if(loaded)
        return true;
    static DynamicLinkLibrary avformat, avdevice;
#if _WIN64 || _WIN32
    avformat = DynamicLinkLibrary(L"avformat-56.dll");
    avdevice = DynamicLinkLibrary(L"avdevice-56.dll");
#else
    FIXME_MESSAGE(implement loadFFmpeg for platform)
#endif
    if(!avformat)
        return false;
    if(!avdevice)
        return false;
#define LOAD_FUNCTION(fn, lib) \
    fn = (PFN_##fn)lib.resolve(L #fn);
    LOAD_FUNCTION(avdevice_register_all, avdevice)
    LOAD_FUNCTION(av_input_video_device_next, avdevice)
    LOAD_FUNCTION(av_register_all, avformat)
#undef LOAD_FUNCTION
    av_register_all();
    avdevice_register_all();
    loaded = true;
    return true;
#endif
}

#if 1
FIXME_MESSAGE(finish implementing video_input.cpp : change to use "https://github.com/ofTheo/videoInput/tree/2014-Stable")
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
#if 0
    for(AVInputFormat *inputFormat = av_input_video_device_next(nullptr); inputFormat != nullptr; inputFormat = av_input_video_device_next(inputFormat))
    {

    }
#endif
    return std::move(deviceList);
}
}

const std::vector<const CameraDevice *> &getCameraDeviceList()
{
    static std::vector<const CameraDevice *> retval = makeCameraDeviceList();
}
}
}
