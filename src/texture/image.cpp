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
#include "texture/image.h"
#include "decoder/png_decoder.h"
#include "platform/platformgl.h"
#include <cstring>
#include <iostream>

Image::Image(wstring resourceName)
{
    try
    {
        shared_ptr<stream::Reader> preader = getResourceReader(resourceName);
        PngDecoder decoder(*preader);
        data = shared_ptr<data_t>(new data_t(decoder.removeData(), decoder.width(), decoder.height(), TopToBottom));
    }
    catch(stream::IOException &e)
    {
        throw ImageLoadError(e.what());
    }
}

Image::Image(unsigned w, unsigned h)
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel * w * h], w, h, TopToBottom));
    memset((void *)data->data, 0, BytesPerPixel * w * h);
}

Image::Image(ColorI c)
{
    data = shared_ptr<data_t>(new data_t(new uint8_t[BytesPerPixel], 1, 1, TopToBottom));
    setPixel(0, 0, c);
}

Image::Image()
    : data(nullptr)
{
}

Image::data_t::~data_t()
{
    static_assert(sizeof(uint32_t) == sizeof(GLuint), "GLuint is not the same size as uint32_t");
    if(texture != 0)
    {
        glDeleteTextures(1, (const GLuint *)&texture);
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

    if(data->rowOrder == BottomToTop)
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

    if(data->rowOrder == BottomToTop)
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
    setRowOrder(BottomToTop);

    if(data->textureValid)
    {
        glBindTexture(GL_TEXTURE_2D, data->texture);
        data->lock.unlock();
        return;
    }

    if(data->texture == 0)
    {
        glGenTextures(1, (GLuint *)&data->texture);
    }

    glBindTexture(GL_TEXTURE_2D, data->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0);
    glPixelTransferf(GL_ALPHA_BIAS, 0.0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->w, data->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)data->data);
    data->textureValid = true;
    data->lock.unlock();
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

void Image::write(stream::Writer &writer, VariableSet &variableSet) const
{
    if(!*this)
    {
        VariableSet::Descriptor<data_t>::null().write(writer);
        return;
    }
    pair<VariableSet::Descriptor<data_t>, bool> findOrMakeReturnValue = variableSet.findOrMake<data_t>(data);
    get<0>(findOrMakeReturnValue).write(writer);
    if(get<1>(findOrMakeReturnValue))
    {
        return;
    }
    cout << "Server : writing image\n";
    writer.writeU32(width());
    writer.writeU32(height());
    vector<uint8_t> row;
    row.resize(width() * BytesPerPixel);
    for(size_t y = 0; y < height(); y++)
    {
        data->lock.lock();
        size_t adjustedY = y;
        if(data->rowOrder == BottomToTop)
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
        for(uint8_t v : row)
        {
            writer.writeU8(v);
        }
    }
}

Image Image::read(stream::Reader &reader, VariableSet &variableSet)
{
    VariableSet::Descriptor<data_t> descriptor = VariableSet::Descriptor<data_t>::read(reader);
    if(!descriptor)
        return Image(nullptr);
    Image retval;
    retval.data = variableSet.get(descriptor);
    if(retval.data != nullptr)
    {
        DUMP_V(Image::read, "read old image");
        return retval;
    }
    cout << "Client : reading image\n";
    DUMP_V(Image::read, "reading new image");
    uint32_t w, h;
    w = reader.readU32();
    h = reader.readU32();
    retval = Image(w, h);
    retval.setRowOrder(RowOrder::TopToBottom);
    for(size_t i = 0; i < BytesPerPixel * w * h; i++)
    {
        retval.data->data[i] = reader.readU8();
    }
    variableSet.set(descriptor, retval.data);
    return retval;
}

