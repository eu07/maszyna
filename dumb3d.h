/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#ifndef MATH3D_H
#define MATH3D_H

//#include <cmath>
#include <fastmath.h>

namespace Math3D
{

// Define this to have Math3D.cp generate a main which tests these classes
//#define TEST_MATH3D

// Define this to allow streaming output of vectors and matrices
// Automatically enabled by TEST_MATH3D
//#define OSTREAM_MATH3D

// definition of the scalar type
typedef double scalar_t;
// inline pass-throughs to various basic math functions
// written in this style to allow for easy substitution with more efficient versions
inline scalar_t SINE_FUNCTION(scalar_t x)
{
    return sin(x);
}
inline scalar_t COSINE_FUNCTION(scalar_t x)
{
    return cos(x);
}
inline scalar_t SQRT_FUNCTION(scalar_t x)
{
    return sqrt(x);
}

// 2 element vector
class vector2
{
  public:
    vector2(void)
    {
    }
    vector2(scalar_t a, scalar_t b)
    {
        x = a;
        y = b;
    }
    double x;
    union
    {
        double y;
        double z;
    };
};
// 3 element vector
class vector3
{
  public:
    vector3(void)
    {
    }
    vector3(scalar_t a, scalar_t b, scalar_t c)
    {
        x = a;
        y = b;
        z = c;
    }

    // The int parameter is the number of elements to copy from initArray (3 or 4)
    //	explicit vector3(scalar_t* initArray, int arraySize = 3)
    //	{ for (int i = 0;i<arraySize;++i) e[i] = initArray[i]; }

    void RotateX(double angle);
    void RotateY(double angle);
    void RotateZ(double angle);

    void inline Normalize();
    void inline SafeNormalize();
    double inline Length();
    void inline Zero()
    {
        x = y = z = 0.0;
    };

    // [] is to read, () is to write (const correctness)
    //	const scalar_t& operator[] (int i) const { return e[i]; }
    //	scalar_t& operator() (int i) { return e[i]; }

    // Provides access to the underlying array; useful for passing this class off to C APIs
    const scalar_t *readArray(void)
    {
        return &x;
    }
    scalar_t *getArray(void)
    {
        return &x;
    }

    //    union
    //  {
    //        struct
    //        {
    double x, y, z;
    //        };
    //    	scalar_t e[3];
    //    };
    bool inline Equal(vector3 *v)
    { // sprawdzenie odleg³oœci punktów
        if (fabs(x - v->x) > 0.02)
            return false; // szeœcian zamiast kuli
        if (fabs(z - v->z) > 0.02)
            return false;
        if (fabs(y - v->y) > 0.02)
            return false;
        return true;
    };

  private:
};

// 4 element matrix
class matrix4x4
{
  public:
    matrix4x4(void)
    {
    }

    // When defining matrices in C arrays, it is easiest to define them with
    // the column increasing fastest.  However, some APIs (OpenGL in particular) do this
    // backwards, hence the "constructor" from C matrices, or from OpenGL matrices.
    // Note that matrices are stored internally in OpenGL format.
    void C_Matrix(scalar_t *initArray)
    {
        int i = 0;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                (*this)(x)[y] = initArray[i++];
    }
    void OpenGL_Matrix(scalar_t *initArray)
    {
        int i = 0;
        for (int x = 0; x < 4; ++x)
            for (int y = 0; y < 4; ++y)
                (*this)(x)[y] = initArray[i++];
    }

    // [] is to read, () is to write (const correctness)
    // m[x][y] or m(x)[y] is the correct form
    const scalar_t *operator[](int i) const
    {
        return &e[i << 2];
    }
    scalar_t *operator()(int i)
    {
        return &e[i << 2];
    }

    // Low-level access to the array.
    const scalar_t *readArray(void)
    {
        return e;
    }
    scalar_t *getArray(void)
    {
        return e;
    }

    // Construct various matrices; REPLACES CURRENT CONTENTS OF THE MATRIX!
    // Written this way to work in-place and hence be somewhat more efficient
    void Identity(void)
    {
        for (int i = 0; i < 16; ++i)
            e[i] = 0;
        e[0] = 1;
        e[5] = 1;
        e[10] = 1;
        e[15] = 1;
    }
    inline matrix4x4 &Rotation(scalar_t angle, vector3 axis);
    inline matrix4x4 &Translation(const vector3 &translation);
    inline matrix4x4 &Scale(scalar_t x, scalar_t y, scalar_t z);
    inline matrix4x4 &BasisChange(const vector3 &v, const vector3 &n);
    inline matrix4x4 &BasisChange(const vector3 &u, const vector3 &v, const vector3 &n);
    inline matrix4x4 &ProjectionMatrix(bool perspective, scalar_t l, scalar_t r, scalar_t t,
                                       scalar_t b, scalar_t n, scalar_t f);
    void InitialRotate()
    { // taka specjalna rotacja, nie ma co ci¹gaæ trygonometrii
        double f;
        for (int i = 0; i < 16; i += 4)
        {
            e[i] = -e[i]; // zmiana znaku X
            f = e[i + 1];
            e[i + 1] = e[i + 2];
            e[i + 2] = f; // zamiana Y i Z
        }
    };
    inline bool IdentityIs()
    { // sprawdzenie jednostkowoœci
        for (int i = 0; i < 16; ++i)
            if (e[i] != ((i % 5) ? 0.0 : 1.0)) // jedynki tylko na 0, 5, 10 i 15
                return false;
        return true;
    }

  private:
    scalar_t e[16];
};

// Scalar operations

// Returns false if there are 0 solutions
inline bool SolveQuadratic(scalar_t a, scalar_t b, scalar_t c, scalar_t *x1, scalar_t *x2);

// Vector operations
inline bool operator==(const vector3 &, const vector3 &);
inline bool operator<(const vector3 &, const vector3 &);

inline vector3 operator-(const vector3 &);
inline vector3 operator*(const vector3 &, scalar_t);
inline vector3 operator*(scalar_t, const vector3 &);
inline vector3 &operator*=(vector3 &, scalar_t);
inline vector3 operator/(const vector3 &, scalar_t);
inline vector3 &operator/=(vector3 &, scalar_t);

inline vector3 operator+(const vector3 &, const vector3 &);
inline vector3 &operator+=(vector3 &, const vector3 &);
inline vector3 operator-(const vector3 &, const vector3 &);
inline vector3 &operator-=(vector3 &, const vector3 &);

// X3 is the 3 element version of a function, X4 is four element
inline scalar_t LengthSquared3(const vector3 &);
inline scalar_t LengthSquared4(const vector3 &);
inline scalar_t Length3(const vector3 &);
inline scalar_t Length4(const vector3 &);
inline vector3 Normalize(const vector3 &);
inline vector3 Normalize4(const vector3 &);
inline scalar_t DotProduct(const vector3 &, const vector3 &);
inline scalar_t DotProduct4(const vector3 &, const vector3 &);
// Cross product is only defined for 3 elements
inline vector3 CrossProduct(const vector3 &, const vector3 &);

inline vector3 operator*(const matrix4x4 &, const vector3 &);

// Matrix operations
inline bool operator==(const matrix4x4 &, const matrix4x4 &);
inline bool operator<(const matrix4x4 &, const matrix4x4 &);

inline matrix4x4 operator*(const matrix4x4 &, const matrix4x4 &);

inline matrix4x4 Transpose(const matrix4x4 &);
scalar_t Determinant(const matrix4x4 &);
matrix4x4 Adjoint(const matrix4x4 &);
matrix4x4 Inverse(const matrix4x4 &);

// Inline implementations follow
inline bool SolveQuadratic(scalar_t a, scalar_t b, scalar_t c, scalar_t *x1, scalar_t *x2)
{
    // If a == 0, solve a linear equation
    if (a == 0)
    {
        if (b == 0)
            return false;
        *x1 = c / b;
        *x2 = *x1;
        return true;
    }
    else
    {
        scalar_t det = b * b - 4 * a * c;
        if (det < 0)
            return false;
        det = SQRT_FUNCTION(det) / (2 * a);
        scalar_t prefix = -b / (2 * a);
        *x1 = prefix + det;
        *x2 = prefix - det;
        return true;
    }
}

inline bool operator==(const vector3 &v1, const vector3 &v2)
{
    return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}

inline bool operator<(const vector3 &v1, const vector3 &v2)
{
    //	for (int i=0;i<4;++i)
    //		if (v1[i] < v2[i]) return true;
    //	else if (v1[i] > v2[i]) return false;

    return false;
}

inline vector3 operator-(const vector3 &v)
{
    return vector3(-v.x, -v.y, -v.z);
}

inline vector3 operator*(const vector3 &v, scalar_t k)
{
    return vector3(k * v.x, k * v.y, k * v.z);
}

inline vector3 operator*(scalar_t k, const vector3 &v)
{
    return v * k;
}

inline vector3 &operator*=(vector3 &v, scalar_t k)
{
    v.x *= k;
    v.y *= k;
    v.z *= k;
    return v;
};

inline vector3 operator/(const vector3 &v, scalar_t k)
{
    return vector3(v.x / k, v.y / k, v.z / k);
}

inline vector3 &operator/=(vector3 &v, scalar_t k)
{
    v.x /= k;
    v.y /= k;
    v.z /= k;
    return v;
}

inline scalar_t LengthSquared3(const vector3 &v)
{
    return DotProduct(v, v);
}
inline scalar_t LengthSquared4(const vector3 &v)
{
    return DotProduct4(v, v);
}

inline scalar_t Length3(const vector3 &v)
{
    return SQRT_FUNCTION(LengthSquared3(v));
}
inline scalar_t Length4(const vector3 &v)
{
    return SQRT_FUNCTION(LengthSquared4(v));
}

inline vector3 Normalize(const vector3 &v)
{
    vector3 retVal = v / Length3(v);
    return retVal;
}
inline vector3 SafeNormalize(const vector3 &v)
{
    double l = Length3(v);
    vector3 retVal;
    if (l == 0)
        retVal.x = retVal.y = retVal.z = 0;
    else
        retVal = v / l;
    return retVal;
}
inline vector3 Normalize4(const vector3 &v)
{
    return v / Length4(v);
}

inline vector3 operator+(const vector3 &v1, const vector3 &v2)
{
    return vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

inline vector3 &operator+=(vector3 &v1, const vector3 &v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    return v1;
}

inline vector3 operator-(const vector3 &v1, const vector3 &v2)
{
    return vector3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

inline vector3 &operator-=(vector3 &v1, const vector3 &v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    return v1;
}

inline scalar_t DotProduct(const vector3 &v1, const vector3 &v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline scalar_t DotProduct4(const vector3 &v1, const vector3 &v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

inline vector3 CrossProduct(const vector3 &v1, const vector3 &v2)
{
    return vector3(v1.y * v2.z - v1.z * v2.y, v2.x * v1.z - v2.z * v1.x, v1.x * v2.y - v1.y * v2.x);
}

inline vector3 operator*(const matrix4x4 &m, const vector3 &v)
{
    return vector3(v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0],
                   v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1],
                   v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2]);
}

void inline vector3::Normalize()
{
    double il = 1 / Length();
    x *= il;
    y *= il;
    z *= il;
}

double inline vector3::Length()
{
    return SQRT_FUNCTION(x * x + y * y + z * z);
}

inline bool operator==(const matrix4x4 &m1, const matrix4x4 &m2)
{
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
            if (m1[x][y] != m2[x][y])
                return false;
    return true;
}

inline bool operator<(const matrix4x4 &m1, const matrix4x4 &m2)
{
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
            if (m1[x][y] < m2[x][y])
                return true;
            else if (m1[x][y] > m2[x][y])
                return false;
    return false;
}

inline matrix4x4 operator*(const matrix4x4 &m1, const matrix4x4 &m2)
{
    matrix4x4 retVal;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
        {
            retVal(x)[y] = 0;
            for (int i = 0; i < 4; ++i)
                retVal(x)[y] += m1[i][y] * m2[x][i];
        }
    return retVal;
}

inline matrix4x4 Transpose(const matrix4x4 &m)
{
    matrix4x4 retVal;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
            retVal(x)[y] = m[y][x];
    return retVal;
}

inline matrix4x4 &matrix4x4::Rotation(scalar_t angle, vector3 axis)
{
    scalar_t c = COSINE_FUNCTION(angle);
    scalar_t s = SINE_FUNCTION(angle);
    // One minus c (short name for legibility of formulai)
    scalar_t omc = (1 - c);

    if (LengthSquared3(axis) != 1)
        axis = Normalize(axis);

    scalar_t x = axis.x;
    scalar_t y = axis.y;
    scalar_t z = axis.z;
    scalar_t xs = x * s;
    scalar_t ys = y * s;
    scalar_t zs = z * s;
    scalar_t xyomc = x * y * omc;
    scalar_t xzomc = x * z * omc;
    scalar_t yzomc = y * z * omc;

    e[0] = x * x * omc + c;
    e[1] = xyomc + zs;
    e[2] = xzomc - ys;
    e[3] = 0;

    e[4] = xyomc - zs;
    e[5] = y * y * omc + c;
    e[6] = yzomc + xs;
    e[7] = 0;

    e[8] = xzomc + ys;
    e[9] = yzomc - xs;
    e[10] = z * z * omc + c;
    e[11] = 0;

    e[12] = 0;
    e[13] = 0;
    e[14] = 0;
    e[15] = 1;

    return *this;
}

inline matrix4x4 &matrix4x4::Translation(const vector3 &translation)
{
    Identity();
    e[12] = translation.x;
    e[13] = translation.y;
    e[14] = translation.z;
    return *this;
}

inline matrix4x4 &matrix4x4::Scale(scalar_t x, scalar_t y, scalar_t z)
{
    Identity();
    e[0] = x;
    e[5] = y;
    e[10] = z;
    return *this;
}

inline matrix4x4 &matrix4x4::BasisChange(const vector3 &u, const vector3 &v, const vector3 &n)
{
    e[0] = u.x;
    e[1] = v.x;
    e[2] = n.x;
    e[3] = 0;

    e[4] = u.y;
    e[5] = v.y;
    e[6] = n.y;
    e[7] = 0;

    e[8] = u.z;
    e[9] = v.z;
    e[10] = n.z;
    e[11] = 0;

    e[12] = 0;
    e[13] = 0;
    e[14] = 0;
    e[15] = 1;

    return *this;
}

inline matrix4x4 &matrix4x4::BasisChange(const vector3 &v, const vector3 &n)
{
    vector3 u = CrossProduct(v, n);
    return BasisChange(u, v, n);
}

inline matrix4x4 &matrix4x4::ProjectionMatrix(bool perspective, scalar_t left_plane,
                                              scalar_t right_plane, scalar_t top_plane,
                                              scalar_t bottom_plane, scalar_t near_plane,
                                              scalar_t far_plane)
{
    scalar_t A = (right_plane + left_plane) / (right_plane - left_plane);
    scalar_t B = (top_plane + bottom_plane) / (top_plane - bottom_plane);
    scalar_t C = (far_plane + near_plane) / (far_plane - near_plane);

    Identity();
    if (perspective)
    {
        e[0] = 2 * near_plane / (right_plane - left_plane);
        e[5] = 2 * near_plane / (top_plane - bottom_plane);
        e[8] = A;
        e[9] = B;
        e[10] = C;
        e[11] = -1;
        e[14] = 2 * far_plane * near_plane / (far_plane - near_plane);
    }
    else
    {
        e[0] = 2 / (right_plane - left_plane);
        e[5] = 2 / (top_plane - bottom_plane);
        e[10] = -2 / (far_plane - near_plane);
        e[12] = A;
        e[13] = B;
        e[14] = C;
    }
    return *this;
}

double inline SquareMagnitude(const vector3 &v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

} // close namespace

// If we're testing, then we need OSTREAM support
#ifdef TEST_MATH3D
#define OSTREAM_MATH3D
#endif

#ifdef OSTREAM_MATH3D
#include <ostream>
// Streaming support
std::ostream &operator<<(std::ostream &os, const Math3D::vector3 &v)
{
    os << '[';
    for (int i = 0; i < 4; ++i)
        os << ' ' << v[i];
    return os << ']';
}

std::ostream &operator<<(std::ostream &os, const Math3D::matrix4x4 &m)
{
    for (int y = 0; y < 4; ++y)
    {
        os << '[';
        for (int x = 0; x < 4; ++x)
            os << ' ' << m[x][y];
        os << " ]" << std::endl;
    }
    return os;
}
#endif // OSTREAM_MATH3D

#endif
