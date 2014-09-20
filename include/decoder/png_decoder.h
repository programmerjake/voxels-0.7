/*
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
#ifndef PNG_DECODER_H_INCLUDED
#define PNG_DECODER_H_INCLUDED

#include <string>
#include <cstdint>
#include <stdexcept>
#include "stream/stream.h"

using namespace std;

class PngLoadError final : public stream::IOException
{
public:
    explicit PngLoadError(const string & arg)
        : IOException(arg)
    {
    }
};

/** read and decode png files<br/>
    bytes in RGBA format
 */
class PngDecoder final
{
private:
    unsigned w, h;
    uint8_t * data;
    PngDecoder(const PngDecoder &) = delete;
    const PngDecoder &operator =(const PngDecoder &) = delete;
public:
    explicit PngDecoder(stream::Reader & reader);
    PngDecoder(PngDecoder && rt)
    {
        w = rt.w;
        h = rt.h;
        data = rt.data;
        rt.data = nullptr;
    }
    ~PngDecoder()
    {
        delete []data;
    }
    uint8_t operator()(int x, int y, int byteNum) const
    {
        if(x < 0 || (unsigned)x >= w || y < 0 || (unsigned)y >= h || byteNum < 0 || byteNum >= 4)
            throw range_error("index out of range in PngDecoder::operator()(int x, int y, int byteNum) const");
        size_t index = y;
        index *= w;
        index += x;
        index *= 4;
        index += byteNum;
        return data[index];
    }
    int width() const
    {
        return w;
    }
    int height() const
    {
        return h;
    }
    uint8_t * removeData()
    {
        uint8_t * retval = data;
        data = nullptr;
        return retval;
    }
};

#endif // PNG_DECODER_H_INCLUDED
