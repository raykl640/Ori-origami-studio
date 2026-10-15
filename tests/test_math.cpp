// ============================================================================
//  ORI - Procedural Origami Folding Studio
//  Tests : Vec2, Mat3 (homogeneous transforms and reflections)
//  Owner : Mohamed Bile (Fold transformation engine)
// ============================================================================
#include "TestFramework.h"
#include "../src/math/Mat3.h"
#include "../src/math/Vec2.h"
#include <cmath>
#include <cstdlib>

using namespace ori;

TEST(vec2_arithmetic_and_products) {
    const Vec2 a{3, 4}, b{-2, 1};
    CHECK(a + b == Vec2(1, 5));
    CHECK(a - b == Vec2(5, 3));
    CHECK(a * 2 == Vec2(6, 8) && 2 * a == Vec2(6, 8));
    CHECK_NEAR(a.length(), 5.0, 1e-12);
    CHECK_NEAR(dot(a, b), -2.0, 1e-12);
    CHECK_NEAR(cross(a, b), 11.0, 1e-12);
    CHECK_NEAR(dot(a, a.perp()), 0.0, 1e-12);          // perp is perpendicular
    CHECK_NEAR(a.normalized().length(), 1.0, 1e-12);
    CHECK(Vec2{}.normalized() == Vec2{});               // zero stays zero
    CHECK(nearlyEqual(lerp(a, b, 0.5), Vec2(0.5, 2.5)));
    CHECK(nearlyEqual(Vec2(1, 0).rotated(kPi / 2), Vec2(0, 1), 1e-12));
}

TEST(mat3_basic_transforms) {
    const Vec2 p{2, 3};
    CHECK(nearlyEqual(Mat3::translate(5, -1).apply(p), Vec2(7, 2)));
    CHECK(nearlyEqual(Mat3::scale(2, 3).apply(p), Vec2(4, 9)));
    CHECK(nearlyEqual(Mat3::rotate(kPi / 2).apply(Vec2(1, 0)), Vec2(0, 1), 1e-12));
    // Rotation about a pivot keeps the pivot fixed.
    const Vec2 pivot{10, 10};
    CHECK(nearlyEqual(Mat3::rotateAbout(1.234, pivot).apply(pivot), pivot, 1e-9));
    // Directions ignore translation.
    CHECK(nearlyEqual(Mat3::translate(100, 100).applyVector(Vec2(1, 2)), Vec2(1, 2)));
    // Matrices apply right to left: (T * S) p = T(S(p)).
    const Mat3 ts = Mat3::translate(1, 1) * Mat3::scale(2, 2);
    CHECK(nearlyEqual(ts.apply(p), Vec2(5, 7)));
}

TEST(mat3_inverse_and_determinant) {
    const Mat3 m = Mat3::translate(4, -7) * Mat3::rotate(0.7) * Mat3::scale(2, 0.5) * Mat3::shear(0.3, 0);
    CHECK_NEAR(m.determinant(), 2 * 0.5, 1e-9);        // rotation and shear keep area
    CHECK((m * m.inverse()).isIdentity(1e-9));
    CHECK((m.inverse() * m).isIdentity(1e-9));
    CHECK(!Mat3::scale(0, 1).isInvertible());
    CHECK(Mat3::scale(0, 1).inverse().isIdentity());   // singular -> identity, no NaNs
    CHECK(m.transposed().transposed().nearlyEquals(m));
}

TEST(reflection_composed_equals_closed_form) {
    std::srand(7);
    auto rnd = [] { return (std::rand() % 2001 - 1000) / 10.0; };
    for (int i = 0; i < 300; ++i) {
        const Vec2 a{rnd(), rnd()}, b{rnd(), rnd()};
        if (distance(a, b) < 1e-3) continue;
        const Mat3 composed = Mat3::reflectLine(a, b);
        CHECK(composed.nearlyEquals(Mat3::reflectLineClosedForm(a, b), 1e-9));
        CHECK_NEAR(composed.determinant(), -1.0, 1e-9);   // reflections reverse orientation
        CHECK(composed.flipsOrientation());
        CHECK((composed * composed).isIdentity(1e-9));    // reflecting twice = no change
        CHECK(nearlyEqual(composed.apply(a), a, 1e-9));   // points on the line stay put
        CHECK(nearlyEqual(composed.apply(b), b, 1e-9));
        // A point and its image are equidistant from the line, on opposite sides.
        const Vec2 p{rnd(), rnd()};
        const Vec2 q = composed.apply(p);
        const Vec2 d = (b - a).normalized();
        CHECK_NEAR(cross(d, p - a), -cross(d, q - a), 1e-7);
        CHECK(nearlyEqual(lerp(p, q, 0.5), a + d * dot(p - a, d), 1e-7));   // midpoint on the line
    }
}

TEST(reflection_special_cases) {
    CHECK(nearlyEqual(Mat3::reflectX().apply(Vec2(3, 4)), Vec2(3, -4)));
    CHECK(nearlyEqual(Mat3::reflectY().apply(Vec2(3, 4)), Vec2(-3, 4)));
    CHECK(nearlyEqual(Mat3::reflectOrigin().apply(Vec2(3, 4)), Vec2(-3, -4)));
    // The diagonal y = x swaps coordinates (the classic book-fold of a square).
    CHECK(nearlyEqual(Mat3::reflectLine({0, 0}, {1, 1}).apply(Vec2(3, 4)), Vec2(4, 3), 1e-12));
    // A vertical line x = 5.
    CHECK(nearlyEqual(Mat3::reflectLine({5, 0}, {5, 10}).apply(Vec2(2, 7)), Vec2(8, 7), 1e-12));
}

TEST(partial_fold_interpolates_identity_to_reflection) {
    const Vec2 a{0, 0}, b{10, 5};
    CHECK(Mat3::partialFold(a, b, 0.0).isIdentity(1e-12));
    CHECK(Mat3::partialFold(a, b, 1.0).nearlyEquals(Mat3::reflectLine(a, b), 1e-12));
    CHECK_NEAR(Mat3::partialFold(a, b, 0.5).determinant(), 0.0, 1e-12);   // flap edge-on
    // Points on the crease never move during the fold.
    for (double t = 0; t <= 1.0; t += 0.1)
        CHECK(nearlyEqual(Mat3::partialFold(a, b, t).apply(b), b, 1e-9));
    // The flap's projected distance from the crease shrinks as |cos(pi t)|.
    const Vec2 p{2, 9};
    const Vec2 d = (b - a).normalized();
    const double h0 = cross(d, p - a);
    for (double t = 0; t <= 1.0; t += 0.125)
        CHECK_NEAR(cross(d, Mat3::partialFold(a, b, t).apply(p) - a), h0 * std::cos(kPi * t), 1e-9);
}
