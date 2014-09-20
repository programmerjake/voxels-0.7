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
#ifndef MATRIX_H
#define MATRIX_H

#include "util/vector.h"
#include "stream/stream.h"
#include <cmath>

struct Matrix
{
    union
    {
        struct
        {
            float x00;
            float x01;
            float x02;
            float x03;
            float x10;
            float x11;
            float x12;
            float x13;
            float x20;
            float x21;
            float x22;
            float x23;
            float x30;
            float x31;
            float x32;
            float x33;
        };
        float x[4][4];
    };

    float get(const int x, const int y) const
    {
        assert(x >= 0 && x < 4 && y >= 0 && y < 4);
        return this->x[x][y];
    }

    void set(const int x, const int y, float value)
    {
        assert(x >= 0 && x < 4 && y >= 0 && y < 4);
        this->x[x][y] = value;
    }

    float(& operator [](size_t index))[4]
    {
        return x[index];
    }

    constexpr const float(& operator [](size_t index) const)[4]
    {
        return x[index];
    }

    constexpr Matrix(float x00,
           float x10,
           float x20,
           float x30,
           float x01,
           float x11,
           float x21,
           float x31,
           float x02,
           float x12,
           float x22,
           float x32,
           float x03 = 0,
           float x13 = 0,
           float x23 = 0,
           float x33 = 1)
        : x00(x00), x01(x01), x02(x02), x03(x03), x10(x10), x11(x11), x12(x12), x13(x13), x20(x20),
          x21(x21), x22(x22), x23(x23), x30(x30), x31(x31), x32(x32), x33(x33)
    {
    }

    constexpr Matrix()
        : x00(1), x01(0), x02(0), x03(0), x10(0), x11(1), x12(0), x13(0), x20(0), x21(0), x22(1), x23(0),
          x30(0), x31(0), x32(0), x33(1)
    {
    }

    constexpr static Matrix identity()
    {
        return Matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0);
    }

    /** creates a rotation matrix
     *
     * @param axis
     *            axis to rotate around
     * @param angle
     *            angle to rotate in radians
     * @return the new rotation matrix
     * @see #rotateX(double angle)
     * @see #rotateY(double angle)
     * @see #rotateZ(double angle) */
    static Matrix rotate(const VectorF axis, const double angle)
    {
        VectorF axisv = normalize(axis);
        float c, s, v;
        c = cos(angle);
        s = sin(angle);
        v = 1 - c; // Versine
        float xx, xy, xz, yy, yz, zz;
        xx = axisv.x * axisv.x;
        xy = axisv.x * axisv.y;
        xz = axisv.x * axisv.z;
        yy = axisv.y * axisv.y;
        yz = axisv.y * axisv.z;
        zz = axisv.z * axisv.z;
        return Matrix(xx + (1 - xx) * c, xy * v - axisv.z * s, xz * v
                      + axisv.y * s, 0, xy * v + axisv.z * s, yy + (1 - yy) * c, yz
                      * v - axisv.x * s, 0, xz * v - axisv.y * s, yz * v + axisv.x
                      * s, zz + (1 - zz) * c, 0);
    }


    /** creates a rotation matrix<br/>
     * the same as <code>Matrix::rotate(VectorF(1, 0, 0), angle)</code>
     *
     * @param angle
     *            angle to rotate around the x axis in radians
     * @return the new rotation matrix
     * @see #rotate(VectorF axis, double angle)
     * @see #rotateY(double angle)
     * @see #rotateZ(double angle) */
    static Matrix rotateX(double angle)
    {
        return rotate(VectorF(1, 0, 0), angle);
    }

    /** creates a rotation matrix<br/>
     * the same as <code>Matrix::rotate(VectorF(0, 1, 0), angle)</code>
     *
     * @param angle
     *            angle to rotate around the y axis in radians
     * @return the new rotation matrix
     * @see #rotate(VectorF axis, double angle)
     * @see #rotateX(double angle)
     * @see #rotateZ(double angle) */
    static Matrix rotateY(double angle)
    {
        return rotate(VectorF(0, 1, 0), angle);
    }

    /** creates a rotation matrix<br/>
     * the same as <code>Matrix::rotate(VectorF(0, 0, 1), angle)</code>
     *
     * @param angle
     *            angle to rotate around the z axis in radians
     * @return the new rotation matrix
     * @see #rotate(VectorF axis, double angle)
     * @see #rotateX(double angle)
     * @see #rotateY(double angle) */
    static Matrix rotateZ(double angle)
    {
        return rotate(VectorF(0, 0, 1), angle);
    }

    /** creates a translation matrix
     *
     * @param position
     *            the position to translate (0, 0, 0) to
     * @return the new translation matrix */
    constexpr static Matrix translate(VectorF position)
    {
        return Matrix(1,
                      0,
                      0,
                      position.x,
                      0,
                      1,
                      0,
                      position.y,
                      0,
                      0,
                      1,
                      position.z);
    }

    /** creates a translation matrix
     *
     * @param x
     *            the x coordinate to translate (0, 0, 0) to
     * @param y
     *            the y coordinate to translate (0, 0, 0) to
     * @param z
     *            the z coordinate to translate (0, 0, 0) to
     * @return the new translation matrix */
    constexpr static Matrix translate(float x, float y, float z)
    {
        return Matrix(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z);
    }

    /** creates a scaling matrix
     *
     * @param x
     *            the amount to scale the x coordinate by
     * @param y
     *            the amount to scale the y coordinate by
     * @param z
     *            the amount to scale the z coordinate by
     * @return the new scaling matrix */
    constexpr static Matrix scale(float x, float y, float z)
    {
        return Matrix(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0);
    }

    /** creates a scaling matrix
     *
     * @param s
     *            <code>s.x</code> is the amount to scale the x coordinate by.<br/>
     *            <code>s.y</code> is the amount to scale the y coordinate by.<br/>
     *            <code>s.z</code> is the amount to scale the z coordinate by.
     * @return the new scaling matrix */
    constexpr static Matrix scale(VectorF s)
    {
        return Matrix(s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0);
    }

    /** creates a scaling matrix
     *
     * @param s
     *            the amount to scale by
     * @return the new scaling matrix */
    constexpr static Matrix scale(float s)
    {
        return Matrix(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0);
    }

    /** @return the determinant of this matrix */
    constexpr float determinant() const
    {
        return x03 * x12 * x21 * x30 - x02 * x13 * x21 * x30 - x03 * x11 * x22 * x30 + x01 * x13 * x22 * x30
               + x02 * x11 * x23 * x30 - x01 * x12 * x23 * x30 - x03 * x12 * x20 * x31 + x02 * x13 * x20 * x31 +
               x03 * x10 * x22 * x31 - x00 * x13 * x22 * x31 - x02 * x10 * x23 * x31 + x00 * x12 * x23 * x31 + x03
               * x11 * x20 * x32 - x01 * x13 * x20 * x32 - x03 * x10 * x21 * x32 + x00 * x13 * x21 * x32 + x01 *
               x10 * x23 * x32 - x00 * x11 * x23 * x32 - x02 * x11 * x20 * x33 + x01 * x12 * x20 * x33 + x02 * x10
               * x21 * x33 - x00 * x12 * x21 * x33 - x01 * x10 * x22 * x33 + x00 * x11 * x22 * x33;
    }

    /** @return the inverse of this matrix. */
    Matrix inverse() const
    {
        float det = determinant();

        if(det == 0.0f)
        {
            throw domain_error("can't invert singular matrix");
        }

        float factor = 1.0f / det;
        return Matrix((x11 * x22 * x33 - x12 * x21 * x33 - x11 * x23 * x32 + x13 * x21 * x32 + x12 * x23 * x31 - x13 * x22 * x31) * factor,
                      (-x10 * x22 * x33 + x12 * x20 * x33 + x10 * x23 * x32 - x13 * x20 * x32 - x12 * x23 * x30 + x13 * x22 * x30) * factor,
                      (x10 * x21 * x33 - x11 * x20 * x33 - x10 * x23 * x31 + x13 * x20 * x31 + x11 * x23 * x30 - x13 * x21 * x30) * factor,
                      (-x10 * x21 * x32 + x11 * x20 * x32 + x10 * x22 * x31 - x12 * x20 * x31 - x11 * x22 * x30 + x12 * x21 * x30) * factor,
                      (-x01 * x22 * x33 + x02 * x21 * x33 + x01 * x23 * x32 - x03 * x21 * x32 - x02 * x23 * x31 + x03 * x22 * x31) * factor,
                      (x00 * x22 * x33 - x02 * x20 * x33 - x00 * x23 * x32 + x03 * x20 * x32 + x02 * x23 * x30 - x03 * x22 * x30) * factor,
                      (-x00 * x21 * x33 + x01 * x20 * x33 + x00 * x23 * x31 - x03 * x20 * x31 - x01 * x23 * x30 + x03 * x21 * x30) * factor,
                      (x00 * x21 * x32 - x01 * x20 * x32 - x00 * x22 * x31 + x02 * x20 * x31 + x01 * x22 * x30 - x02 * x21 * x30) * factor,
                      (x01 * x12 * x33 - x02 * x11 * x33 - x01 * x13 * x32 + x03 * x11 * x32 + x02 * x13 * x31 - x03 * x12 * x31) * factor,
                      (-x00 * x12 * x33 + x02 * x10 * x33 + x00 * x13 * x32 - x03 * x10 * x32 - x02 * x13 * x30 + x03 * x12 * x30) * factor,
                      (x00 * x11 * x33 - x01 * x10 * x33 - x00 * x13 * x31 + x03 * x10 * x31 + x01 * x13 * x30 - x03 * x11 * x30) * factor,
                      (-x00 * x11 * x32 + x01 * x10 * x32 + x00 * x12 * x31 - x02 * x10 * x31 - x01 * x12 * x30 + x02 * x11 * x30) * factor,
                      (-x01 * x12 * x23 + x02 * x11 * x23 + x01 * x13 * x22 - x03 * x11 * x22 - x02 * x13 * x21 + x03 * x12 * x21) * factor,
                      (x00 * x12 * x23 - x02 * x10 * x23 - x00 * x13 * x22 + x03 * x10 * x22 + x02 * x13 * x20 - x03 * x12 * x20) * factor,
                      (-x00 * x11 * x23 + x01 * x10 * x23 + x00 * x13 * x21 - x03 * x10 * x21 - x01 * x13 * x20 + x03 * x11 * x20) * factor,
                      (x00 * x11 * x22 - x01 * x10 * x22 - x00 * x12 * x21 + x02 * x10 * x21 + x01 * x12 * x20 - x02 * x11 * x20) * factor);
    }

    /** @return the inverse of this matrix. */
    friend Matrix inverse(const Matrix &m)
    {
        return m.inverse();
    }

    friend Matrix transpose(const Matrix &m)
    {
        return Matrix(m.x00, m.x01, m.x02, m.x03,
                       m.x10, m.x11, m.x12, m.x13,
                       m.x20, m.x21, m.x22, m.x23,
                       m.x30, m.x31, m.x32, m.x33);
    }

    constexpr Matrix concat(Matrix rt) const
    {
        return Matrix(/* x00*/ x00 * rt.x00 + x01 * rt.x10 + x02 * rt.x20 + x03 * rt.x30,
                               /* x10*/ x10 * rt.x00 + x11 * rt.x10 + x12 * rt.x20 + x13 * rt.x30,
                               /* x20*/ x20 * rt.x00 + x21 * rt.x10 + x22 * rt.x20 + x23 * rt.x30,
                               /* x30*/ x30 * rt.x00 + x31 * rt.x10 + x32 * rt.x20 + x33 * rt.x30,
                               /* x01*/ x00 * rt.x01 + x01 * rt.x11 + x02 * rt.x21 + x03 * rt.x31,
                               /* x11*/ x10 * rt.x01 + x11 * rt.x11 + x12 * rt.x21 + x13 * rt.x31,
                               /* x21*/ x20 * rt.x01 + x21 * rt.x11 + x22 * rt.x21 + x23 * rt.x31,
                               /* x31*/ x30 * rt.x01 + x31 * rt.x11 + x32 * rt.x21 + x33 * rt.x31,
                               /* x02*/ x00 * rt.x02 + x01 * rt.x12 + x02 * rt.x22 + x03 * rt.x32,
                               /* x12*/ x10 * rt.x02 + x11 * rt.x12 + x12 * rt.x22 + x13 * rt.x32,
                               /* x22*/ x20 * rt.x02 + x21 * rt.x12 + x22 * rt.x22 + x23 * rt.x32,
                               /* x32*/ x30 * rt.x02 + x31 * rt.x12 + x32 * rt.x22 + x33 * rt.x32,
                               /* x03*/ x00 * rt.x03 + x01 * rt.x13 + x02 * rt.x23 + x03 * rt.x33,
                               /* x13*/ x10 * rt.x03 + x11 * rt.x13 + x12 * rt.x23 + x13 * rt.x33,
                               /* x23*/ x20 * rt.x03 + x21 * rt.x13 + x22 * rt.x23 + x23 * rt.x33,
                               /* x33*/ x30 * rt.x03 + x31 * rt.x13 + x32 * rt.x23 + x33 * rt.x33);
    }

    constexpr VectorF apply(VectorF v) const
    {
        return VectorF(v.x * this->x00 + v.y * this->x10 + v.z * this->x20
                      + this->x30, v.x * this->x01 + v.y * this->x11 + v.z * this->x21
                      + this->x31, v.x * this->x02 + v.y * this->x12 + v.z * this->x22
                      + this->x32);
    }

    constexpr VectorF applyNoTranslate(VectorF v) const
    {
        return VectorF(v.x * this->x00 + v.y * this->x10 + v.z * this->x20, v.x
                                * this->x01 + v.y * this->x11 + v.z * this->x21, v.x * this->x02
                                + v.y * this->x12 + v.z * this->x22);
    }

    static Matrix thetaPhi(double theta, double phi)
    {
        Matrix t = rotateX(-phi);
        return rotateY(theta).concat(t);
    }

    static Matrix read(stream::Reader &reader)
    {
        float x00 = stream::read_finite<float32_t>(reader);
        float x01 = stream::read_finite<float32_t>(reader);
        float x02 = stream::read_finite<float32_t>(reader);
        float x03 = stream::read_finite<float32_t>(reader);
        float x10 = stream::read_finite<float32_t>(reader);
        float x11 = stream::read_finite<float32_t>(reader);
        float x12 = stream::read_finite<float32_t>(reader);
        float x13 = stream::read_finite<float32_t>(reader);
        float x20 = stream::read_finite<float32_t>(reader);
        float x21 = stream::read_finite<float32_t>(reader);
        float x22 = stream::read_finite<float32_t>(reader);
        float x23 = stream::read_finite<float32_t>(reader);
        float x30 = stream::read_finite<float32_t>(reader);
        float x31 = stream::read_finite<float32_t>(reader);
        float x32 = stream::read_finite<float32_t>(reader);
        float x33 = stream::read_finite<float32_t>(reader);
        return Matrix(x00, x10, x20, x30,
                       x01, x11, x21, x31,
                       x02, x12, x22, x32,
                       x03, x13, x23, x33);
    }

    void write(stream::Writer &writer) const
    {
        stream::write<float32_t>(writer, x00);
        stream::write<float32_t>(writer, x01);
        stream::write<float32_t>(writer, x02);
        stream::write<float32_t>(writer, x03);
        stream::write<float32_t>(writer, x10);
        stream::write<float32_t>(writer, x11);
        stream::write<float32_t>(writer, x12);
        stream::write<float32_t>(writer, x13);
        stream::write<float32_t>(writer, x20);
        stream::write<float32_t>(writer, x21);
        stream::write<float32_t>(writer, x22);
        stream::write<float32_t>(writer, x23);
        stream::write<float32_t>(writer, x30);
        stream::write<float32_t>(writer, x31);
        stream::write<float32_t>(writer, x32);
        stream::write<float32_t>(writer, x33);
    }
};

constexpr VectorF transform(const Matrix &m, VectorF v)
{
    return m.apply(v);
}

inline VectorF transformNormal(const Matrix &m, VectorF v)
{
    return normalizeNoThrow(transpose(inverse(m)).applyNoTranslate(v));
}

constexpr Matrix transform(const Matrix & a, const Matrix & b)
{
    return b.concat(a);
}

inline bool operator ==(const Matrix &a, const Matrix &b)
{
    for(int y = 0; y < 4; y++)
    {
        for(int x = 0; x < 4; x++)
        {
            if(a.get(x, y) != b.get(x, y))
            {
                return false;
            }
        }
    }

    return true;
}

inline bool operator !=(const Matrix &a, const Matrix &b)
{
    for(int y = 0; y < 4; y++)
    {
        for(int x = 0; x < 4; x++)
        {
            if(a.get(x, y) != b.get(x, y))
            {
                return true;
            }
        }
    }

    return false;
}

#endif // MATRIX_H
