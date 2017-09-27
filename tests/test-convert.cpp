#include "../catch/catch.hpp"
#include "test-data.hpp"
#include "test-tools.hpp"

SCENARIO("performing color conversions", "[image][convert]") {
    GIVEN("a BGR source image") {
        fmo::Image src{fmo::Format::BGR, IM_4x2_DIMS, IM_4x2_BGR.data()};
        GIVEN("an empty destination image") {
            fmo::Image dst{ };
            WHEN("converting to GRAY") {
                fmo::convert(src, dst, fmo::Format::GRAY);
                THEN("result image has correct dimensions") {
                    REQUIRE(dst.dims() == IM_4x2_DIMS);
                    AND_THEN("result image has correct format") {
                        REQUIRE(dst.format() == fmo::Format::GRAY);
                        AND_THEN("result image matches hard-coded values") {
                            REQUIRE(almost_exact_match(dst, IM_4x2_GRAY, 1));
                        }
                    }
                }
            }
        }
    }
    GIVEN("a GRAY source image") {
        fmo::Image src{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_GRAY.data()};
        GIVEN("an empty destination image") {
            fmo::Image dst{ };
            WHEN("converting to BGR") {
                fmo::convert(src, dst, fmo::Format::BGR);
                THEN("result image has correct dimensions") {
                    REQUIRE(dst.dims() == IM_4x2_DIMS);
                    AND_THEN("result image has correct format") {
                        REQUIRE(dst.format() == fmo::Format::BGR);
                        AND_THEN("result image matches hard-coded values") {
                            REQUIRE(exact_match(dst, IM_4x2_GRAY_3));
                        }
                    }
                }
            }
        }
    }
    GIVEN("a YUV420 source image") {
        fmo::Image src{fmo::Format::YUV420SP, IM_4x2_DIMS, IM_4x2_YUV420SP.data()};
        GIVEN("an empty destination image") {
            fmo::Image dst{ };
            WHEN("converting to GRAY") {
                fmo::convert(src, dst, fmo::Format::GRAY);
                THEN("result image has correct dimensions") {
                    REQUIRE(dst.dims() == IM_4x2_DIMS);
                    AND_THEN("result image has correct format") {
                        REQUIRE(dst.format() == fmo::Format::GRAY);
                        AND_THEN("result image matches hard-coded values") {
                            REQUIRE(exact_match(dst, IM_4x2_GRAY));
                        }
                    }
                }
            }
            WHEN("converting to BGR") {
                fmo::convert(src, dst, fmo::Format::BGR);
                THEN("result image has correct dimensions") {
                    REQUIRE(dst.dims() == IM_4x2_DIMS);
                    AND_THEN("result image has correct format") {
                        REQUIRE(dst.format() == fmo::Format::BGR);
                        AND_THEN("result image matches hard-coded values") {
                            REQUIRE(exact_match(dst, IM_4x2_YUV2BGR));
                        }
                    }
                }
            }
        }
    }
}
