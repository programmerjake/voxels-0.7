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
#include "texture/image.h"
#include "decoder/png_decoder.h"
#include "platform/platformgl.h"
#include <cstring>
#include <iostream>
#include "util/logging.h"
#include <unordered_map>

using namespace std;

namespace programmerjake
{
namespace voxels
{
Image::Image(wstring resourceName)
    : data()
{
    try
    {
        shared_ptr<stream::Reader> preader = getResourceReader(resourceName);
        PngDecoder decoder(*preader);
        data = shared_ptr<data_t>(new data_t(decoder.removeData(), decoder.width(), decoder.height(), RowOrder::TopToBottom));
    }
    catch(stream::IOException &e)
    {
        throw ImageLoadError(e.what());
    }
}

Image::Image(unsigned w, unsigned h)
    : data()
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel * w * h], w, h, RowOrder::TopToBottom));
    memset((void *)data->data, 0, BytesPerPixel * w * h);
}

Image::Image(ColorI c)
    : data()
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel], 1, 1, RowOrder::TopToBottom));
    setPixel(0, 0, c);
}

Image::data_t::~data_t()
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");
    if(texture != 0)
    {
        freeTexture(texture);
    }
    delete []data;
}

void Image::setPixel(int x, int y, ColorI c)
{
    if(!data)
    {
        return;
    }

    data->lock.lock();

    if(data->rowOrder == RowOrder::BottomToTop)
    {
        y = data->h - y - 1;
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        data->lock.unlock();
        return;
    }

    copyOnWrite();
    data->textureValid = false;
    uint8_t *pixel = &data->data[BytesPerPixel * (x + y * data->w)];
    pixel[0] = c.r;
    pixel[1] = c.g;
    pixel[2] = c.b;
    pixel[3] = c.a;
    data->lock.unlock();
}

ColorI Image::getPixel(int x, int y) const
{
    if(!data)
    {
        return RGBAI(0, 0, 0, 0);
    }

    data->lock.lock();

    if(data->rowOrder == RowOrder::BottomToTop)
    {
        y = data->h - y - 1;
    }

    if(y < 0 || (unsigned)y >= data->h || x < 0 || (unsigned)x >= data->w)
    {
        data->lock.unlock();
        return RGBAI(0, 0, 0, 0);
    }

    uint8_t *pixel = &data->data[BytesPerPixel * (x + y * data->w)];
    ColorI retval = RGBAI(pixel[0], pixel[1], pixel[2], pixel[3]);
    data->lock.unlock();
    return retval;
}

void Image::bind() const
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");

    if(!data)
    {
        unbind();
        return;
    }

    data->lock.lock();
    setRowOrder(RowOrder::BottomToTop);

    if(data->textureValid)
    {
        glBindTexture(GL_TEXTURE_2D, data->texture);
        data->lock.unlock();
        return;
    }

    if(data->texture == 0)
    {
        data->texture = allocateTexture();
        glBindTexture(GL_TEXTURE_2D, data->texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelTransferf(GL_ALPHA_SCALE, 1.0);
        glPixelTransferf(GL_ALPHA_BIAS, 0.0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->w, data->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)data->data);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, data->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, data->w, data->h, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)data->data);
    }

    data->textureValid = true;
    data->lock.unlock();
}

void Image::getData(std::uint8_t *dest, RowOrder rowOrder) const
{
    assert(dest != nullptr);
    assert(rowOrder == RowOrder::TopToBottom || rowOrder == RowOrder::BottomToTop);
    data->lock.lock();
    setRowOrder(rowOrder);
    const std::uint8_t *src = data->data;
    for(std::size_t i = getDataSize(); i > 0; i--)
    {
        *dest++ = *src++;
    }
    data->lock.unlock();
}

void Image::setData(const std::uint8_t *src, RowOrder rowOrder)
{
    assert(src != nullptr);
    assert(rowOrder == RowOrder::TopToBottom || rowOrder == RowOrder::BottomToTop);
    data->lock.lock();
    copyOnWrite();
    data->textureValid = false;
    data->rowOrder = rowOrder; // everything is going to be overwritten so we don't need to use setRowOrder
    std::uint8_t *dest = data->data;
    for(std::size_t i = getDataSize(); i > 0; i--)
    {
        *dest++ = *src++;
    }
    data->lock.unlock();
}

void Image::copyRect(int destLeft, int destTop, int destW, int destH, Image src, int srcLeft, int srcTop)
{
    if(destLeft < 0)
    {
        destW += destLeft;
        srcLeft -= destLeft;
        destLeft = 0;
    }
    if(destTop < 0)
    {
        destH += destTop;
        srcTop -= destTop;
        destTop = 0;
    }
    if(destW > (int)data->w - destLeft)
        destW = (int)data->w - destLeft;
    if(destH > (int)data->h - destTop)
        destH = (int)data->h - destTop;
    if(destW <= 0 || destH <= 0)
        return;
    std::unique_lock<std::mutex> destLock(data->lock, std::defer_lock);
    std::unique_lock<std::mutex> srcLock;
    if(src.data == data)
    {
        destLock.lock();
    }
    else
    {
        srcLock = std::unique_lock<std::mutex>(src.data->lock, std::defer_lock);
        std::lock(srcLock, destLock);
    }
    setRowOrder(RowOrder::TopToBottom);
    copyOnWrite();
    src.setRowOrder(RowOrder::TopToBottom);
    data->textureValid = false;
    for(int y = 0; y < destH; y++)
    {
        int destY = y + destTop;
        if(data->rowOrder == RowOrder::BottomToTop)
        {
            destY = data->h - destY - 1;
        }
        int srcY = y + srcTop;
        if(src.data->rowOrder == RowOrder::BottomToTop)
        {
            srcY = src.data->h - srcY - 1;
        }
        if(srcY < 0 || srcY >= (int)src.data->h)
        {
            std::uint8_t *pDest = &data->data[BytesPerPixel * (destLeft + destY * data->w)];
            for(int x = 0; x < destW; x++, pDest += BytesPerPixel)
            {
                pDest[0] = 0;
                pDest[1] = 0;
                pDest[2] = 0;
                pDest[3] = 0;
            }
        }
        else
        {
            int x = 0;
            std::uint8_t *pDest = &data->data[BytesPerPixel * (destLeft + destY * data->w)];
            if(srcLeft < 0)
            {
                int limit = destW;
                if(limit > -srcLeft)
                    limit = -srcLeft;
                for(; x < limit; x++, pDest += BytesPerPixel)
                {
                    pDest[0] = 0;
                    pDest[1] = 0;
                    pDest[2] = 0;
                    pDest[3] = 0;
                }
            }
            const std::uint8_t *pSrc = &src.data->data[BytesPerPixel * (srcLeft + x + srcY * src.data->w)];
            int limit = (int)src.data->w - srcLeft;
            if(limit > destW)
                limit = destW;
            for(; x < limit; x++, pDest += BytesPerPixel, pSrc += BytesPerPixel)
            {
                std::uint8_t r = pSrc[0];
                std::uint8_t g = pSrc[1];
                std::uint8_t b = pSrc[2];
                std::uint8_t a = pSrc[3];
                pDest[0] = r;
                pDest[1] = g;
                pDest[2] = b;
                pDest[3] = a;
            }
            for(; x < destW; x++, pDest += BytesPerPixel)
            {
                pDest[0] = 0;
                pDest[1] = 0;
                pDest[2] = 0;
                pDest[3] = 0;
            }
        }
    }
}

void Image::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Image::setRowOrder(RowOrder newRowOrder) const
{
    if(data->rowOrder == newRowOrder)
    {
        return;
    }

    data->rowOrder = newRowOrder;

    for(unsigned y1 = 0, y2 = data->h - 1; y1 < y2; y1++, y2--)
    {
        swapRows(y1, y2);
    }
}

void Image::swapRows(unsigned y1, unsigned y2) const
{
    size_t index1 = y1 * BytesPerPixel * data->w, index2 = y2 * BytesPerPixel * data->w;

    for(size_t i = 0; i < data->w * BytesPerPixel; i++)
    {
        uint8_t t = data->data[index1];
        data->data[index1++] = data->data[index2];
        data->data[index2++] = t;
    }
}

void Image::copyOnWrite()
{
    if(data.unique())
    {
        return;
    }

    shared_ptr<data_t> newData = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel * data->w * data->h], data));

    for(size_t i = 0; i < BytesPerPixel * data->w * data->h; i++)
    {
        newData->data[i] = data->data[i];
    }

    data->lock.unlock();
    data = newData;
    data->lock.lock();
}

struct SerializedImages final
{
public:
    typedef std::uint64_t Descriptor;
    static constexpr Descriptor NullDescriptor = 0;
private:
    Descriptor validDescriptorCount = 0;
    std::unordered_map<Descriptor, Image> descriptorToImageMap;
    std::unordered_map<Image, Descriptor, Image::Hasher> imageToDescriptorMap;
    struct construct_tag_t
    {
    };
public:
    SerializedImages(construct_tag_t)
        : descriptorToImageMap(),
        imageToDescriptorMap()
    {
    }
private:
    static SerializedImages &get(stream::Stream &stream)
    {
        struct tag_t
        {
        };
        std::shared_ptr<SerializedImages> pretval = stream.getAssociatedValue<SerializedImages, tag_t>();
        if(pretval == nullptr)
        {
            pretval = std::make_shared<SerializedImages>(construct_tag_t());
            stream.setAssociatedValue<SerializedImages, tag_t>(pretval);
        }
        return *pretval;
    }
public:
    static bool readDescriptor(stream::Reader &reader, Image &image, Descriptor &descriptor)
    {
        SerializedImages &me = get(reader);
        descriptor = stream::read_limited<Descriptor>(reader, NullDescriptor, me.validDescriptorCount + 1);
        if(descriptor == me.validDescriptorCount + 1)
        {
            ++me.validDescriptorCount;
            return true;
        }
        image = me.descriptorToImageMap[descriptor];
        return false;
    }
    static void setAfterRead(stream::Reader &reader, Descriptor descriptor, Image image)
    {
        SerializedImages &me = get(reader);
        me.descriptorToImageMap[descriptor] = image;
        me.imageToDescriptorMap[image] = descriptor;
    }
    static bool writeDescriptor(stream::Writer &writer, Image image)
    {
        if(image == nullptr)
        {
            stream::write<Descriptor>(writer, NullDescriptor);
            return false;
        }
        SerializedImages &me = get(writer);
        Descriptor descriptor;
        auto iter = me.imageToDescriptorMap.find(image);
        if(iter == me.imageToDescriptorMap.end())
        {
            descriptor = ++me.validDescriptorCount;
            me.imageToDescriptorMap[image] = descriptor;
            stream::write<Descriptor>(writer, descriptor);
            return true;
        }
        else
        {
            descriptor = std::get<1>(*iter);
            stream::write<Descriptor>(writer, descriptor);
            return false;
        }
    }
};

void Image::write(stream::Writer &writer) const
{
    if(!SerializedImages::writeDescriptor(writer, *this))
        return;
    getDebugLog() << L"Server : writing image" << postnl;
    stream::write<std::uint32_t>(writer, width());
    stream::write<std::uint32_t>(writer, height());
    vector<uint8_t> row;
    row.resize(width() * BytesPerPixel);
    for(size_t y = 0; y < height(); y++)
    {
        data->lock.lock();
        size_t adjustedY = y;
        if(data->rowOrder == RowOrder::BottomToTop)
        {
            adjustedY = data->h - adjustedY - 1;
        }

        uint8_t *prow = &data->data[BytesPerPixel * (adjustedY * data->w)];
        for(size_t x = 0, i = 0; x < width(); x++)
        {
            row[i++] = *prow++;
            row[i++] = *prow++;
            row[i++] = *prow++;
            row[i++] = *prow++;
        }
        data->lock.unlock();
        writer.writeBytes(row.data(), row.size());
    }
}

Image Image::read(stream::Reader &reader)
{
    Image retval;
    SerializedImages::Descriptor descriptor;
    if(!SerializedImages::readDescriptor(reader, retval, descriptor))
        return retval;
    getDebugLog() << L"Client : reading image" << postnl;
    std::uint32_t w, h;
    w = stream::read<std::uint32_t>(reader);
    h = stream::read<std::uint32_t>(reader);
    retval = Image(w, h);
    retval.setRowOrder(RowOrder::TopToBottom);
    reader.readAllBytes(&retval.data->data[0], BytesPerPixel * w * h);
    SerializedImages::setAfterRead(reader, descriptor, retval);
    return retval;
}
}
}
