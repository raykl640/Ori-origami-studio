// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : math            Owner : Mohamed Bile (Fold transformation engine)
//  File   : Mat3.h
//  3x3 matrices in homogeneous coordinates. A point (x, y) is the column
//  (x, y, 1); a direction is (x, y, 0), so translations do not move it.
//
//  Every fold in ORI is a reflection across the crease line. A reflection
//  across an arbitrary line is built by COMPOSITION of the basic transforms:
//
//      M = T(p) . R(theta) . S(1, -1) . R(-theta) . T(-p)
//
//  (move the line onto the x-axis, mirror in y, move it back). Matrices are
//  applied right to left, so A * B means "first B, then A".
// ============================================================================
#pragma once
#include "Vec2.h"
#include <string>

namespace ori {

struct Mat3 {
    // Row-major: m[row][col].
    double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    Mat3() = default;
    Mat3(double a, double b, double c,
         double d, double e, double f,
         double g = 0, double h = 0, double i = 1);

    // ---- basic transforms -------------------------------------------------
    static Mat3 identity();
    static Mat3 translate(double tx, double ty);
    static Mat3 translate(const Vec2& t);
    static Mat3 rotate(double theta);                         // about the origin (radians)
    static Mat3 rotateAbout(double theta, const Vec2& pivot);
    static Mat3 scale(double sx, double sy);                  // about the origin
    static Mat3 scaleAbout(double sx, double sy, const Vec2& pivot);
    static Mat3 shear(double shx, double shy);

    // ---- reflections ------------------------------------------------------
    static Mat3 reflectX();                                   // mirror in the x-axis: (x, y) -> (x, -y)
    static Mat3 reflectY();                                   // mirror in the y-axis: (x, y) -> (-x, y)
    static Mat3 reflectOrigin();                              // point reflection:    (x, y) -> (-x, -y)
    // Reflection across the line through a and b, built by composition
    // T(a) R(theta) S(1,-1) R(-theta) T(-a). This is the fold transform.
    static Mat3 reflectLine(const Vec2& a, const Vec2& b);
    // The same reflection from the closed form
    //   [ cos2t  sin2t ] about the line's point a, theta = line angle.
    //   [ sin2t -cos2t ]
    // Used to cross-check the composed version.
    static Mat3 reflectLineClosedForm(const Vec2& a, const Vec2& b);

    // ---- folding in progress ----------------------------------------------
    // A fold that is a fraction t in [0,1] complete, as seen from above.
    // The flap rotates through 3D about the crease; projected onto the table
    // its distance to the crease shrinks by cos(pi t):
    //   M(t) = T(a) R(theta) S(1, cos(pi t)) R(-theta) T(-a)
    // t = 0 is the identity, t = 0.5 is the flap standing upright (edge on),
    // t = 1 equals reflectLine(a, b).
    static Mat3 partialFold(const Vec2& a, const Vec2& b, double t);

    // ---- operations ---------------------------------------------------------
    Mat3 operator*(const Mat3& o) const;
    Mat3& operator*=(const Mat3& o);

    Vec2 apply(const Vec2& p) const;          // point: uses the translation part
    Vec2 applyVector(const Vec2& v) const;    // direction: ignores the translation
    Vec2 operator*(const Vec2& p) const { return apply(p); }

    double determinant() const;
    // Inverse by the adjugate. Returns the identity if the matrix is singular
    // (check isInvertible() first when that matters).
    Mat3 inverse() const;
    bool isInvertible(double eps = 1e-12) const;
    Mat3 transposed() const;

    // A reflection (or any orientation-reversing transform) has det < 0:
    // a folded panel is turned face-down exactly when this is true.
    bool flipsOrientation() const { return determinant() < 0.0; }

    bool nearlyEquals(const Mat3& o, double eps = 1e-9) const;
    bool isIdentity(double eps = 1e-9) const { return nearlyEquals(Mat3{}, eps); }

    std::string toString(int decimals = 3) const;
};

} // namespace ori
