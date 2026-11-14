// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Module : math            Owner : Mohamed Bile (Fold transformation engine)
//  File   : Mat3.cpp
// ============================================================================
#include "Mat3.h"
#include <cmath>
#include <cstdio>

namespace ori {

Mat3::Mat3(double a, double b, double c,
           double d, double e, double f,
           double g, double h, double i)
    : m{{a, b, c}, {d, e, f}, {g, h, i}} {}

// ------------------------------------------------------------ basic transforms --
Mat3 Mat3::identity() { return Mat3{}; }

Mat3 Mat3::translate(double tx, double ty) {
    return Mat3(1, 0, tx,
                0, 1, ty);
}

Mat3 Mat3::translate(const Vec2& t) { return translate(t.x, t.y); }

Mat3 Mat3::rotate(double theta) {
    const double c = std::cos(theta), s = std::sin(theta);
    return Mat3(c, -s, 0,
                s,  c, 0);
}

Mat3 Mat3::rotateAbout(double theta, const Vec2& pivot) {
    return translate(pivot) * rotate(theta) * translate(-pivot);
}

Mat3 Mat3::scale(double sx, double sy) {
    return Mat3(sx, 0, 0,
                0, sy, 0);
}

Mat3 Mat3::scaleAbout(double sx, double sy, const Vec2& pivot) {
    return translate(pivot) * scale(sx, sy) * translate(-pivot);
}

Mat3 Mat3::shear(double shx, double shy) {
    return Mat3(1, shx, 0,
                shy, 1, 0);
}

// ----------------------------------------------------------------- reflections --
Mat3 Mat3::reflectX() { return scale(1, -1); }
Mat3 Mat3::reflectY() { return scale(-1, 1); }
Mat3 Mat3::reflectOrigin() { return scale(-1, -1); }

Mat3 Mat3::reflectLine(const Vec2& a, const Vec2& b) {
    const double theta = (b - a).angle();
    // Read right to left: move a to the origin, turn the line onto the x-axis,
    // mirror, turn back, move back.
    return translate(a) * rotate(theta) * reflectX() * rotate(-theta) * translate(-a);
}

Mat3 Mat3::reflectLineClosedForm(const Vec2& a, const Vec2& b) {
    const double theta = (b - a).angle();
    const double c = std::cos(2 * theta), s = std::sin(2 * theta);
    // Linear part L = [c s; s -c]; translation keeps a fixed: t = a - L a.
    const double tx = a.x - (c * a.x + s * a.y);
    const double ty = a.y - (s * a.x - c * a.y);
    return Mat3(c,  s, tx,
                s, -c, ty);
}

Mat3 Mat3::partialFold(const Vec2& a, const Vec2& b, double t) {
    const double theta = (b - a).angle();
    const double k = std::cos(kPi * t);   // 1 -> 0 -> -1 as the flap swings over
    return translate(a) * rotate(theta) * scale(1, k) * rotate(-theta) * translate(-a);
}

// ------------------------------------------------------------------ operations --
Mat3 Mat3::operator*(const Mat3& o) const {
    Mat3 r(0, 0, 0, 0, 0, 0, 0, 0, 0);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                r.m[i][j] += m[i][k] * o.m[k][j];
    return r;
}

Mat3& Mat3::operator*=(const Mat3& o) { return *this = *this * o; }

Vec2 Mat3::apply(const Vec2& p) const {
    const double x = m[0][0] * p.x + m[0][1] * p.y + m[0][2];
    const double y = m[1][0] * p.x + m[1][1] * p.y + m[1][2];
    const double w = m[2][0] * p.x + m[2][1] * p.y + m[2][2];
    // Affine matrices always have w = 1; divide anyway so a projective
    // matrix still gives the right point.
    if (std::fabs(w) > kEps && std::fabs(w - 1.0) > kEps) return {x / w, y / w};
    return {x, y};
}

Vec2 Mat3::applyVector(const Vec2& v) const {
    return {m[0][0] * v.x + m[0][1] * v.y,
            m[1][0] * v.x + m[1][1] * v.y};
}

double Mat3::determinant() const {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

bool Mat3::isInvertible(double eps) const { return std::fabs(determinant()) > eps; }

Mat3 Mat3::inverse() const {
    const double det = determinant();
    if (std::fabs(det) < 1e-12) return Mat3{};
    // Adjugate = transpose of the cofactor matrix.
    Mat3 r;
    r.m[0][0] =  (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
    r.m[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]);
    r.m[0][2] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
    r.m[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    r.m[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
    r.m[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]);
    r.m[2][0] =  (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    r.m[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]);
    r.m[2][2] =  (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
    for (auto& row : r.m)
        for (double& v : row) v /= det;
    return r;
}

Mat3 Mat3::transposed() const {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) r.m[i][j] = m[j][i];
    return r;
}

bool Mat3::nearlyEquals(const Mat3& o, double eps) const {
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (std::fabs(m[i][j] - o.m[i][j]) > eps) return false;
    return true;
}

std::string Mat3::toString(int decimals) const {
    std::string s;
    char buf[64];
    for (int i = 0; i < 3; ++i) {
        s += "[";
        for (int j = 0; j < 3; ++j) {
            std::snprintf(buf, sizeof buf, "%s%.*f", j ? ", " : "", decimals, m[i][j] == 0.0 ? 0.0 : m[i][j]);
            s += buf;
        }
        s += "]\n";
    }
    return s;
}

} // namespace ori
