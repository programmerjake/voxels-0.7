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
/**
 * @class ImageLoadError image.h "texture/image.h"
 */
class ImageLoadError final : public std::runtime_error
{
public:
    explicit ImageLoadError(const std::string &arg) : std::runtime_error(arg)
    {
    }
};

struct SerializedImages;

/** copy-on-write Image class
 * @class Image image.h "texture/image.h"
 *
 */
class Image final
{
    friend struct SerializedImages;

public:
    /** load an image resource
     * @param resourceName the file name of the image resource to load
     */
    explicit Image(std::wstring resourceName);
    /** create a new transparent Image
     * @param w the width of the new image
     * @param h the height of the new image
     *
     */
    explicit Image(unsigned w, unsigned h);
    /** create a new 1x1 solid-color Image
     * @param c the color to set the new Image to
     */
    explicit Image(ColorI c);
    /// create a new empty Image
    Image() noexcept : data()
    {
    }
    /// create a new empty Image
    Image(std::nullptr_t) noexcept : Image()
    {
    }

    /** @brief set a pixel
     *
     * Set the pixel at (x, y) to the color c. If (x, y)
     * is outside this Image, then do nothing.
     * @param x the zero-based number of pixels from the left of the pixel to set
     * @param y the zero-based number of pixels from the top of the pixel to set
     * @param c the color to set to
     * @pre this Image is not empty
     */
    void setPixel(int x, int y, ColorI c);
    /** @brief get a pixel
     *
     * get the color of the pixel at (x, y). If (x, y) is
     * outside this Image then return RGBAI(0, 0, 0, 0).
     * @param x the zero-based number of pixels from the left of the pixel to set
     * @param y the zero-based number of pixels from the top of the pixel to set
     * @return the color of the pixel at (x, y). If (x, y) is
     * outside this Image then return RGBAI(0, 0, 0, 0).
     * @pre this Image is not empty
     */
    ColorI getPixel(int x, int y) const;
    /** bind this Image as the current OpenGL texture.
     * @pre this Image is not empty
     */
    void bind() const;
    /** unbind the current OpenGL texture.
     */
    static void unbind();
    /** get the width of this Image
     * @return the width of this Image
     * @pre this Image is not empty
     */
    unsigned width() const
    {
        return data->w;
    }
    /** get the height of this Image
     * @return the height of this Image
     * @pre this Image is not empty
     */
    unsigned height() const
    {
        return data->h;
    }
    /** check if this Image is not empty
     * @return true if this Image is not empty
     */
    explicit operator bool() const
    {
        return data != nullptr;
    }
    /** check if this Image is empty
     * @return true if this Image is empty
     */
    bool operator!() const
    {
        return data == nullptr;
    }
    friend bool operator==(Image l, Image r)
    {
        return l.data == r.data;
    }
    friend bool operator!=(Image l, Image r)
    {
        return l.data != r.data;
    }
    friend bool operator==(std::nullptr_t, Image r)
    {
        return nullptr == r.data;
    }
    friend bool operator!=(std::nullptr_t, Image r)
    {
        return nullptr != r.data;
    }
    friend bool operator==(Image l, std::nullptr_t)
    {
        return l.data == nullptr;
    }
    friend bool operator!=(Image l, std::nullptr_t)
    {
        return l.data != nullptr;
    }
    void write(stream::Writer &writer) const;
    static Image read(stream::Reader &reader);
    /** get the number of bytes used to hold this Image's pixels
     * @return the number of bytes used to hold this Image's pixels
     * @pre this Image is not empty
     */
    std::size_t getDataSize() const
    {
        return static_cast<std::size_t>(data->w) * static_cast<std::size_t>(data->h)
               * BytesPerPixel;
    }
    enum class RowOrder
    {
        TopToBottom,
        BottomToTop
    };
    /** get this Image's pixels
     * @param dest the memory to store this Image's pixels into
     * @param rowOrder the order of the rows of pixels stored into dest
     * @note The image data is stored as unsigned-bytes in the order: red green blue alpha
     * @pre this Image is not empty
     * @see getDataSize
     */
    void getData(std::uint8_t *dest, RowOrder rowOrder = RowOrder::TopToBottom) const;
    /** get this Image's pixels
     * @param dest the memory to store this Image's pixels into
     * @param rowOrder the order of the rows of pixels stored into dest
     * @note The image data is stored as unsigned-bytes in the order: red green blue alpha
     * @pre this Image is not empty
     * @see getDataSize
     */
    void getData(std::vector<std::uint8_t> &dest, RowOrder rowOrder = RowOrder::TopToBottom) const
    {
        dest.resize(getDataSize());
        getData(&dest[0], rowOrder);
    }
    /** set this Image's pixels
     * @param src the memory to read this Image's pixels from
     * @param rowOrder the order of the rows of pixels read from src
     * @note The image data is read as unsigned-bytes in the order: red green blue alpha
     * @pre this Image is not empty
     * @see getDataSize
     */
    void setData(const std::uint8_t *src, RowOrder rowOrder = RowOrder::TopToBottom);
    /** set this Image's pixels
     * @param src the memory to read this Image's pixels from
     * @param rowOrder the order of the rows of pixels read from src
     * @note The image data is read as unsigned-bytes in the order: red green blue alpha
     * @pre this Image is not empty
     * @see getDataSize
     */
    void setData(const std::vector<std::uint8_t> &src, RowOrder rowOrder = RowOrder::TopToBottom)
    {
        assert(src.size() >= getDataSize());
        setData(&src[0], rowOrder);
    }
    /** copy a rectangle-shaped image from the source Image to this Image
     * @param destLeft the left edge of the destination rectangle
     * @param destTop the top edge of the destination rectangle
     * @param destWidth the width of the rectangle to copy
     * @param destHeight the height of the rectangle to copy
     * @param src the Image to copy from
     * @param srcLeft the left edge of the source rectangle
     * @param srcTop the top edge of the source rectangle
     * @note The copied image portions that are outside of src are filled with RGBAI(0, 0, 0, 0)
     * @pre both this Image and src are not empty
     * @see getDataSize
     */
    void copyRect(
        int destLeft, int destTop, int destW, int destH, Image src, int srcLeft, int srcTop);
    static constexpr std::size_t BytesPerPixel = 4;

private:
    struct data_t
    {
        data_t(const data_t &) = delete;
        data_t &operator=(const data_t &) = delete;
        std::uint8_t *const data;
        const unsigned w, h;
        RowOrder rowOrder;
        std::uint32_t texture;
        bool textureValid;
        std::uint64_t textureGraphicsContextId;
        std::mutex lock;
        data_t(std::uint8_t *data, unsigned w, unsigned h, RowOrder rowOrder)
            : data(data),
              w(w),
              h(h),
              rowOrder(rowOrder),
              texture(0),
              textureValid(false),
              textureGraphicsContextId(0),
              lock()
        {
        }
        data_t(std::uint8_t *data, std::shared_ptr<data_t> rt)
            : data(data),
              w(rt->w),
              h(rt->h),
              rowOrder(rt->rowOrder),
              texture(0),
              textureValid(false),
              textureGraphicsContextId(0),
              lock()
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
    static Image retval = Image(GrayscaleI(0xFF));
    return retval;
}
}
}

#endif // IMAGE_H
