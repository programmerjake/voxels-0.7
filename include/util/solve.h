#ifndef SOLVE_H_INCLUDED
#define SOLVE_H_INCLUDED

#include <cmath>

using namespace std;

inline int solveLinear(float a/*constant*/, float b/*linear*/, float retval[1])
{
    retval[0] = 0;

    if(abs(b) < eps)
    {
        return (abs(a) < eps) ? 1 : 0;
    }

    retval[0] = -a / b;
    return 1;
}

inline int solveQuadratic(float a/*constant*/, float b/*linear*/, float c/*quadratic*/, float retval[2])
{
    if(abs(c) < eps)
    {
        return solveLinear(a, b, retval);
    }

    float sqrtArg = b * b - 4 * c * a;

    if(sqrtArg < 0)
    {
        return 0;
    }

    if(c < 0)
    {
        a = -a;
        b = -b;
        c = -c;
    }

    float sqrtV = sqrt(sqrtArg);
    retval[0] = (-b - sqrtV) / (2 * c);
    retval[1] = (-b + sqrtV) / (2 * c);
    return 2;
}

inline int solveCubic(float a/*constant*/, float b/*linear*/, float c/*quadratic*/, float d/*cubic*/,
               float retval[3])
{
    if(abs(d) < eps)
    {
        return solveQuadratic(a, b, c, retval);
    }

    a /= d;
    b /= d;
    c /= d;
    float Q = (c * c - 3 * b) / 9;
    float R = (2 * (c * c * c) - 9 * (c * b) + 27 * a) / 54;
    float R2 = R * R;
    float Q3 = Q * Q * Q;

    if(R2 < Q3)
    {
        float theta = acos(R / sqrt(Q3));
        float SQ = sqrt(Q);
        retval[0] = -2 * SQ * cos(theta / 3) - c / 3;
        retval[1] = -2 * SQ * cos((theta + 2 * M_PI) / 3) - c / 3;
        retval[2] = -2 * SQ * cos((theta - 2 * M_PI) / 3) - c / 3;
        return 3;
    }

    float A = -cbrt(abs(R) + sqrt(R2 - Q3));

    if(R < 0)
    {
        A = -A;
    }

    float B;
    if(A == 0)
    {
        B = 0;
    }
    else
    {
        B = Q / A;
    }

    float AB = A + B;
    retval[0] = AB - c / 3;
    return 1;
}

#endif // SOLVE_H_INCLUDED
