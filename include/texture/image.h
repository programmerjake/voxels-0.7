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
#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <cwchar>
#include <string>
#include <stdexcept>
#include <mutex>
#include <memory>
#include <type_traits>
#include <vector>
#include "util/color.h"
#include "stream/stream.h"

namespace programmerjake
{
namespace voxels
{
class ImageLoadError final : public std::runtime_error
{
public:
    explicit ImageLoadError(const std::string &arg)
        : std::runtime_error(arg)
    {
    }
};

struct SerializedImages;

class Image final
{
    friend struct SerializedImages;
public:
    explicit Image(std::wstring resourceName);
    explicit Image(unsigned w, unsigned h);
    explicit Image(ColorI c);
    Image() noexcept
        : data()
    {
    }
    Image(std::nullptr_t) noexcept
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
    explicit operator bool() const
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
    friend bool operator ==(std::nullptr_t, Image r)
    {
        return nullptr == r.data;
    }
    friend bool operator !=(std::nullptr_t, Image r)
    {
        return nullptr != r.data;
    }
    friend bool operator ==(Image l, std::nullptr_t)
    {
        return l.data == nullptr;
    }
    friend bool operator !=(Image l, std::nullptr_t)
    {
        return l.data != nullptr;
    }
    void write(stream::Writer &writer) const;
    static Image read(stream::Reader &reader);
    std::size_t getDataSize() const
    {
        return static_cast<std::size_t>(data->w) * static_cast<std::size_t>(data->h) * BytesPerPixel;
    }
    enum class RowOrder
    {
        TopToBottom,
        BottomToTop
    };
    void getData(std::uint8_t *dest, RowOrder rowOrder = RowOrder::TopToBottom) const;
    void getData(std::vector<std::uint8_t> &dest, RowOrder rowOrder = RowOrder::TopToBottom) const
    {
        dest.resize(getDataSize());
        getData(&dest[0], rowOrder);
    }
    void setData(const std::uint8_t *src, RowOrder rowOrder = RowOrder::TopToBottom);
    void setData(const std::vector<std::uint8_t> &src, RowOrder rowOrder = RowOrder::TopToBottom)
    {
        assert(src.size() >= getDataSize());
        setData(&src[0], rowOrder);
    }
    void copyRect(int destLeft, int destTop, int destW, int destH, Image src, int srcLeft, int srcTop);
    static constexpr std::size_t BytesPerPixel = 4;
private:
    struct data_t
    {
        data_t(const data_t &) = delete;
        data_t &operator =(const data_t &) = delete;
        std::uint8_t * const data;
        const unsigned w, h;
        RowOrder rowOrder;
        std::uint32_t texture;
        bool textureValid;
        std::mutex lock;
        data_t(std::uint8_t * data, unsigned w, unsigned h, RowOrder rowOrder)
            : data(data), w(w), h(h), rowOrder(rowOrder), texture(0), textureValid(false), lock()
        {
        }
        data_t(std::uint8_t * data, std::shared_ptr<data_t> rt)
            : data(data), w(rt->w), h(rt->h), rowOrder(rt->rowOrder), texture(0), textureValid(false), lock()
        {
        }
        ~data_t();
    };
    std::shared_ptr<data_t> data;
    void setRowOrder(RowOrder newRowOrder) const;
    void swapRows(unsigned y1, unsigned y2) const;
    void copyOnWrite();
    struct Hasher final
    {
        std::hash<std::shared_ptr<data_t>> hasher;
        std::size_t operator()(const Image &image) const
        {
            return hasher(image.data);
        }
    };
};

inline Image whiteTexture()
{
    static Image retval;
    if(retval == nullptr)
        retval = Image(GrayscaleI(0xFF));
    return retval;
}
}
}

#endif // IMAGE_H
