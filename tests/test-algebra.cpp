#include "../catch/catch.hpp"
#include <fmo/algebra.hpp>

using namespace fmo;

constexpr Vector v1{303, 679};
constexpr Vector v2{453, 501};

static_assert(cross(v1, v1) == 0, "cross()");
static_assert(cross(v1, v2) == (303 * 501 - 453 * 679), "cross()");
static_assert(cross(v2, v1) == -cross(v1, v2), "cross()");
static_assert(cross(v2, v2) == 0, "cross()");
static_assert(dot(v1, v1) == sqr(303) + sqr(679), "dot()");
static_assert(dot(v1, v2) == (303 * 453 + 679 * 501), "dot()");
static_assert(dot(v2, v1) == dot(v1, v2), "dot()");
static_assert(dot(v2, v2) == sqr(453) + sqr(501), "dot()");
static_assert(left(v1, v1) == false, "left()");
static_assert(left(v1, v2) == false, "left()");
static_assert(left(v2, v1) == true, "left()");
static_assert(left(v2, v2) == false, "left()");
static_assert(right(v1, v1) == false, "right()");
static_assert(right(v1, v2) == true, "right()");
static_assert(right(v2, v1) == false, "right()");
static_assert(right(v2, v2) == false, "right()");

TEST_CASE("length()", "[algebra]") {
    REQUIRE(length(v1) == Approx(743.538835).epsilon(1e-6f));
    REQUIRE(length(v2) == Approx(675.433194).epsilon(1e-6f));
}
