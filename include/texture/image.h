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
#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <cwchar>
#include <string>
#include <stdexcept>
#include <mutex>
#include <memory>
#include <type_traits>
#include "util/color.h"
#include "stream/stream.h"
#include "util/variable_set.h"

using namespace std;

class ImageLoadError final : public runtime_error
{
public:
    explicit ImageLoadError(const string &arg)
        : runtime_error(arg)
    {
    }
};

class Image final
{
public:
    explicit Image(wstring resourceName);
    explicit Image(unsigned w, unsigned h);
    explicit Image(ColorI c);
    Image();
    Image(nullptr_t)
        : Image()
    {
    }

    void setPixel(int x, int y, ColorI c);
    ColorI getPixel(int x, int y) const;
    void bind() const;
    static void unbind();
    unsigned width() const
    {
        return data->w;
    }
    unsigned height() const
    {
        return data->h;
    }
    operator bool() const
    {
        return data != nullptr;
    }
    bool operator !() const
    {
        return data == nullptr;
    }
    friend bool operator ==(Image l, Image r)
    {
        return l.data == r.data;
    }
    friend bool operator !=(Image l, Image r)
    {
        return l.data != r.data;
    }
    void write(stream::Writer &writer, VariableSet &variableSet) const;
    static Image read(stream::Reader &reader, VariableSet &variableSet);
private:
    enum RowOrder
    {
        TopToBottom,
        BottomToTop
    };
    struct data_t
    {
        uint8_t * const data;
        const unsigned w, h;
        RowOrder rowOrder;
        uint32_t texture;
        bool textureValid;
        mutex lock;
        data_t(uint8_t * data, unsigned w, unsigned h, RowOrder rowOrder)
            : data(data), w(w), h(h), rowOrder(rowOrder), texture(0), textureValid(false)
        {
        }
        data_t(uint8_t * data, shared_ptr<data_t> rt)
            : data(data), w(rt->w), h(rt->h), rowOrder(rt->rowOrder), texture(0), textureValid(false)
        {
        }
        ~data_t();
    };
    shared_ptr<data_t> data;
    static constexpr size_t BytesPerPixel = 4;
    void setRowOrder(RowOrder newRowOrder) const;
    void swapRows(unsigned y1, unsigned y2) const;
    void copyOnWrite();
};

namespace stream
{
template <>
struct rw_cached_helper<Image>
{
    typedef Image value_type;
    static value_type read(Reader &reader, VariableSet &variableSet)
    {
        return Image::read(reader, variableSet);
    }
    static void write(Writer &writer, VariableSet &variableSet, value_type value)
    {
        value.write(writer, variableSet);
    }
};
}

#endif // IMAGE_H
