#include "dumb3d.h"
#include "fastmath.h"
#include <cassert>

namespace Math3D
{

void vector3::RotateX(double angle)
{
    double ty = y;
    y = (cos(angle) * y - z * sin(angle));
    z = (z * cos(angle) + sin(angle) * ty);
};
void vector3::RotateY(double angle)
{
    double tx = x;
    x = (cos(angle) * x + z * sin(angle));
    z = (z * cos(angle) - sin(angle) * tx);
};
void vector3::RotateZ(double angle)
{
    double ty = y;
    y = (cos(angle) * y + x * sin(angle));
    x = (x * cos(angle) - sin(angle) * ty);
};

void inline vector3::SafeNormalize()
{
    double l = Length();
    if (l == 0)
    {
        x = y = z = 0;
    }
    else
    {
        x /= l;
        y /= l;
        z /= l;
    }
}

// From code in Graphics Gems; p. 766
inline scalar_t det2x2(scalar_t a, scalar_t b, scalar_t c, scalar_t d) { return a * d - b * c; }

inline scalar_t det3x3(scalar_t a1, scalar_t a2, scalar_t a3, scalar_t b1, scalar_t b2, scalar_t b3,
                       scalar_t c1, scalar_t c2, scalar_t c3)
{
    return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

scalar_t Determinant(const matrix4x4 &m)
{
    scalar_t a1 = m[0][0];
    scalar_t a2 = m[1][0];
    scalar_t a3 = m[2][0];
    scalar_t a4 = m[3][0];
    scalar_t b1 = m[0][1];
    scalar_t b2 = m[1][1];
    scalar_t b3 = m[2][1];
    scalar_t b4 = m[3][1];
    scalar_t c1 = m[0][2];
    scalar_t c2 = m[1][2];
    scalar_t c3 = m[2][2];
    scalar_t c4 = m[3][2];
    scalar_t d1 = m[0][3];
    scalar_t d2 = m[1][3];
    scalar_t d3 = m[2][3];
    scalar_t d4 = m[3][3];

    return a1 * det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4) -
           b1 * det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4) +
           c1 * det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4) -
           d1 * det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);
}

matrix4x4 Adjoint(const matrix4x4 &m)
{
    scalar_t a1 = m[0][0];
    scalar_t a2 = m[0][1];
    scalar_t a3 = m[0][2];
    scalar_t a4 = m[0][3];
    scalar_t b1 = m[1][0];
    scalar_t b2 = m[1][1];
    scalar_t b3 = m[1][2];
    scalar_t b4 = m[1][3];
    scalar_t c1 = m[2][0];
    scalar_t c2 = m[2][1];
    scalar_t c3 = m[2][2];
    scalar_t c4 = m[2][3];
    scalar_t d1 = m[3][0];
    scalar_t d2 = m[3][1];
    scalar_t d3 = m[3][2];
    scalar_t d4 = m[3][3];

    // Adjoint(x,y) = -1^(x+y) * a(y,x)
    // Where a(i,j) is the 3x3 determinant of m with row i and col j removed
    matrix4x4 retVal;
    retVal(0)[0] = det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
    retVal(0)[1] = -det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
    retVal(0)[2] = det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
    retVal(0)[3] = -det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

    retVal(1)[0] = -det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
    retVal(1)[1] = det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
    retVal(1)[2] = -det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
    retVal(1)[3] = det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

    retVal(2)[0] = det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
    retVal(2)[1] = -det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
    retVal(2)[2] = det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
    retVal(2)[3] = -det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

    retVal(3)[0] = -det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
    retVal(3)[1] = det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
    retVal(3)[2] = -det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
    retVal(3)[3] = det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);

    return retVal;
}

matrix4x4 Inverse(const matrix4x4 &m)
{
    matrix4x4 retVal = Adjoint(m);
    scalar_t det = Determinant(m);
    assert(det);

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            retVal(i)[j] /= det;
        }
    }

    return retVal;
}
}

//**************************************
// Testing from here on.
//**************************************
#ifdef TEST_MATH3D
#include <iostream>

using namespace Math3D;
using namespace std;

static int failures = 0;

void ReportFailure(const char *className, const char *testName, bool passed)
{
    cout << className;
    if (passed)
        cout << " passed test ";
    else
    {
        cout << " FAILED test ";
        ++failures;
    }
    cout << testName << "." << endl;
}

const char *vector3Name = "vector3";
const char *matrix4x4Name = "matrix4x4";

void Testvector3Constructors(void)
{
    // Default ctor... just make sure it compiles
    vector3 defaultCtorTest;

    // Initializer ctor test (3 param)
    vector3 initCtorTest(1, 2, 3);
    ReportFailure(vector3Name, "initialized ctor (3 parameter version)",
                  (initCtorTest[0] == 1 && initCtorTest[1] == 2 && initCtorTest[2] == 3 &&
                   initCtorTest[3] == 1));

    // Initializer ctor test (4 param)
    vector3 initCtorTest2(1, 2, 3, 4);
    ReportFailure(vector3Name, "initialized ctor (4 parameter version)",
                  (initCtorTest2[0] == 1 && initCtorTest2[1] == 2 && initCtorTest2[2] == 3 &&
                   initCtorTest2[3] == 4));

    scalar_t initArray[] = {1, 2, 3, 4};
    vector3 initCtorArrayTest3(initArray);
    ReportFailure(vector3Name, "array initialized ctor (3 parameter version)",
                  (initCtorArrayTest3[0] == 1 && initCtorArrayTest3[1] == 2 &&
                   initCtorArrayTest3[2] == 3 && initCtorArrayTest3[3] == 1));

    vector3 initCtorArrayTest4(initArray, 4);
    ReportFailure(vector3Name, "array initialized ctor (4 parameter version)",
                  (initCtorArrayTest4[0] == 1 && initCtorArrayTest4[1] == 2 &&
                   initCtorArrayTest4[2] == 3 && initCtorArrayTest4[3] == 4));

    // Copy ctor test
    vector3 copyCtorTest(initCtorTest2);
    ReportFailure(vector3Name, "copy ctor", (copyCtorTest[0] == 1 && copyCtorTest[1] == 2 &&
                                             copyCtorTest[2] == 3 && copyCtorTest[3] == 4));
}

void Testvector3Comparison(void)
{
    vector3 alpha(1, 1, 1);
    vector3 beta(alpha);
    vector3 gamma(2, 3, 4);

    ReportFailure(vector3Name, "equivalence operator test 1", (alpha == beta));
    ReportFailure(vector3Name, "equivalence operator test 2", (!(alpha == gamma)));
    ReportFailure(vector3Name, "comparison operator test 1", !(alpha < beta));
    ReportFailure(vector3Name, "comparison operator test 2", (alpha < gamma));
    ReportFailure(vector3Name, "comparison operator test 3", !(gamma < beta));
}

void Testvector3Assignment(void)
{
    vector3 alpha(1, 1, 1, 1);
    vector3 beta(10, 10, 10, 10);
    alpha = beta;
    ReportFailure(vector3Name, "assignment operator", (alpha == beta));
}

void Testvector3UnaryOps(void)
{
    vector3 alpha(10, 10, 10, 10);
    vector3 beta(-10, -10, -10, -10);
    alpha = -alpha;
    ReportFailure(vector3Name, "negation operator", (alpha == beta));

    ReportFailure(vector3Name, "length squared 3 element version", LengthSquared3(alpha) == 300);
    ReportFailure(vector3Name, "length 3 element version", Length3(alpha) == SQRT_FUNCTION(300));
    ReportFailure(vector3Name, "length squared 4 element version", LengthSquared4(alpha) == 400);
    ReportFailure(vector3Name, "length 4 element version", Length4(alpha) == SQRT_FUNCTION(400));

    // Manually normalize beta
    // Done without /= on vector3, as we want to be independant of failure of /=
    // Earlier failures should be resolved before later ones (just like C++)
    beta = alpha;
    for (int i = 0; i < 3; ++i)
        beta(i) /= SQRT_FUNCTION(300);
    beta(3) = 1;
    ReportFailure(vector3Name, "normalize 3 element version", Normalize3(alpha) == beta);

    beta = alpha;
    for (int i = 0; i < 4; ++i)
        beta(i) /= SQRT_FUNCTION(400);
    ReportFailure(vector3Name, "normalize 4 element version", Normalize4(alpha) == beta);
}

void Testvector3BinaryOps(void)
{
    // Vector * Matrix is tested in Testmatrix4x4BinaryOps
    vector3 testVec(1, 1, 1, 1);
    vector3 deltaVec(1, 2, 3, 4);
    vector3 crossVec(1, -2, 1, 1); // testVec x deltaVec

    vector3 factorVec(10, 10, 10, 10);
    vector3 sumVec(2, 3, 4, 5);
    vector3 difVec(0, -1, -2, -3);
    vector3 testVec2;

    ReportFailure(vector3Name, "scalar multiply 1", (testVec * 10) == factorVec);
    ReportFailure(vector3Name, "scalar multiply 2", (10 * testVec) == factorVec);
    testVec2 = testVec;
    ReportFailure(vector3Name, "scalar multiply and store", (testVec2 *= 10) == factorVec);

    ReportFailure(vector3Name, "scalar divide", (factorVec / 10) == testVec);
    testVec2 = factorVec;
    ReportFailure(vector3Name, "scalar divide and store", (testVec2 /= 10) == testVec);

    ReportFailure(vector3Name, "vector addition", (testVec + deltaVec) == sumVec);
    testVec2 = testVec;
    ReportFailure(vector3Name, "vector addition and store", (testVec2 += deltaVec) == sumVec);

    ReportFailure(vector3Name, "vector subtraction", (testVec - deltaVec) == difVec);
    testVec2 = testVec;
    ReportFailure(vector3Name, "vector subtraction and store", (testVec2 -= deltaVec) == difVec);

    ReportFailure(vector3Name, "3 element dot product", 6 == DotProduct3(testVec, deltaVec));
    ReportFailure(vector3Name, "4 element dot product", 10 == DotProduct4(testVec, deltaVec));

    ReportFailure(vector3Name, "cross product", crossVec == CrossProduct(testVec, deltaVec));
}

void Testvector3(void)
{
    // Accessors cannot be tested effectively...
    // They are really trivial, and so don't really need testing,
    // but more importantly, how do you test the ctors without assuming
    // the accessors work?  Conversely, how do you test the acccessors
    // without assuming the ctors work?  Chicken and egg problem, and I
    // decided on testing the ctors, not the accessors.
    Testvector3Constructors();
    Testvector3Comparison();
    Testvector3Assignment();
    Testvector3UnaryOps();
    Testvector3BinaryOps();
}

void Testmatrix4x4Constructors(void)
{
    // Check if default ctor compiles
    matrix4x4 defaultTest;

    scalar_t initArray[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    matrix4x4 arrayTest;
    arrayTest.C_Matrix(initArray);
    bool passedTest = true;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
            if (arrayTest[x][y] != initArray[(y << 2) + x])
                passedTest = false;
    ReportFailure(matrix4x4Name, "array constructor", passedTest);

    matrix4x4 copyTest(arrayTest);
    passedTest = true;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
            if (arrayTest[x][y] != copyTest[x][y])
                passedTest = false;
    ReportFailure(matrix4x4Name, "copy constructor", passedTest);
}

void Testmatrix4x4Comparison(void)
{
    scalar_t initArray[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    scalar_t initArray2[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    matrix4x4 alpha, beta, gamma;
    alpha.C_Matrix(initArray);
    beta.C_Matrix(initArray);
    gamma.C_Matrix(initArray2);

    ReportFailure(matrix4x4Name, "equality test 1", alpha == beta);
    ReportFailure(matrix4x4Name, "equality test 2", !(alpha == gamma));
    ReportFailure(matrix4x4Name, "comparison test 1", alpha < gamma);
    ReportFailure(matrix4x4Name, "comparison test 2", !(gamma < alpha));
    ReportFailure(matrix4x4Name, "comparison test 3", !(alpha < beta));
}

void Testmatrix4x4BinaryOps(void)
{
    scalar_t initVector[] = {0, 1, 2, 3};
    scalar_t initMatrix[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    scalar_t resultVector[] = {0 * 0 + 1 * 1 + 2 * 2 + 3 * 3, 0 * 4 + 1 * 5 + 2 * 6 + 3 * 7,
                               0 * 8 + 1 * 9 + 2 * 10 + 3 * 11, 0 * 12 + 1 * 13 + 2 * 14 + 3 * 15};

    vector3 vector1(initVector, 4);
    matrix4x4 matrix1;
    matrix1.C_Matrix(initMatrix);
    vector3 vectorTest(resultVector, 4);
    ReportFailure(matrix4x4Name, "matrix * vector", vectorTest == matrix1 * vector1);

    scalar_t initMatrix2[] = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    matrix4x4 matrix2;
    matrix2.C_Matrix(initMatrix2);
    matrix4x4 resultMatrix;
    for (int x = 0; x < 4; ++x)
        for (int y = 0; y < 4; ++y)
        {
            resultMatrix(x)[y] = 0;
            for (int i = 0; i < 4; ++i)
                resultMatrix(x)[y] += matrix1[i][y] * matrix2[x][i];
        }
    ReportFailure(matrix4x4Name, "matrix * matrix", resultMatrix == matrix1 * matrix2);
}

void Testmatrix4x4(void)
{
    Testmatrix4x4Constructors();
    Testmatrix4x4Comparison();
    Testmatrix4x4BinaryOps();
}

int main(int, char *[])
{
    int vectorFailures = 0;
    int matrixFailures = 0;
    Testvector3();
    vectorFailures = failures;
    failures = 0;

    Testmatrix4x4();
    matrixFailures = failures;

    cout << endl << "****************************************" << endl;
    cout << "*                                      *" << endl;
    if (vectorFailures + matrixFailures == 0)
        cout << "*    No failures detected in Math3D    *" << endl;
    else
    {
        cout << "*     FAILURES DETECTED IN MATH3D!     *" << endl;
        cout << "*     Total vector3 failures: " << vectorFailures << "        *" << endl;
        cout << "*     Total matrix4x4 failures: " << matrixFailures << "        *" << endl;
        cout << "*   Total Failures in Math3D: " << vectorFailures + matrixFailures << "        *"
             << endl;
    }
    cout << "*                                      *" << endl;
    cout << "****************************************" << endl;

    return 0;
}
#endif
