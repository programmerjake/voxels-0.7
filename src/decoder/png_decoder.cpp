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
#include "decoder/png_decoder.h"
#include <png.h>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include "util/util.h"

using namespace std;

namespace
{

void pngloaderror(png_structp png_ptr, png_const_charp msg)
{
    *(string *)png_get_error_ptr(png_ptr) = msg;
    longjmp(png_jmpbuf(png_ptr), 1);
}

void pngloadwarning(png_structp, png_const_charp)
{
    // do nothing
}

void readBytes(png_structp png_ptr, png_bytep output, png_size_t count)
{
    png_voidp user_data = png_get_io_ptr(png_ptr);
    stream::Reader & reader = *(stream::Reader *)user_data;
    try
    {
        reader.readBytes((uint8_t *)output, count);
    }
    catch(stream::IOException & e)
    {
        *(string *)png_get_error_ptr(png_ptr) = e.what();
        longjmp(png_jmpbuf(png_ptr), 1);
    }
}

inline bool LoadPNG(stream::Reader * preader, uint8_t *&pixels, unsigned &width, unsigned &height, string &errorMsg)
{
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (void *)&errorMsg, pngloaderror, pngloadwarning);
    if(!png_ptr)
    {
        errorMsg = "can't create png read struct";
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        errorMsg = "can't create png info struct";
        return false;
    }

    uint8_t *volatile retval = NULL;  // so that it isn't messed up by longjmp
    volatile png_bytepp rows = NULL;

    if(setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        delete []retval;
        delete []rows;
        return false;
    }

    png_set_read_fn(png_ptr, (png_voidp)preader, readBytes);

    png_read_info(png_ptr, info_ptr);

    png_uint_32 XRes, YRes;
    int bit_depth, color_type, interlace_type;
    png_get_IHDR(png_ptr, info_ptr, &XRes, &YRes, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

    png_set_strip_16(png_ptr);

    png_set_packing(png_ptr);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_palette_to_rgb(png_ptr);
    }

    if((color_type & PNG_COLOR_MASK_ALPHA) == 0)
    {
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    }

    width = XRes;
    height = YRes;
    retval = new uint8_t[width * height * 4];
    if(!retval)
    {
        errorMsg = "can't allocate image data array";
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    rows = (png_bytepp)new png_bytep[YRes];
    if(!rows)
    {
        errorMsg = "can't allocate image row indirection array";
        longjmp(png_jmpbuf(png_ptr), 1);
    }
    for(unsigned y = 0; y < YRes; y++)
    {
        rows[y] = (png_bytep)&retval[y * XRes * 4];
    }

    png_read_image(png_ptr, rows);

    png_read_end(png_ptr, info_ptr);

    delete []rows;

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    pixels = (uint8_t *)retval;
    return true;
}

}

PngDecoder::PngDecoder(stream::Reader & reader)
{
    string errorMsg;
    if(!LoadPNG(&reader, data, w, h, errorMsg))
    {
        throw PngLoadError(errorMsg);
    }
}
