#include "../catch/catch.hpp"
#include "test-data.hpp"
#include "test-tools.hpp"
#include <fmo/region.hpp>

SCENARIO("extracting regions from images", "[image][region]") {
    GIVEN("a BGR source image") {
        fmo::Image src{fmo::Format::BGR, IM_4x2_DIMS, IM_4x2_BGR.data()};
        GIVEN("an empty destination image") {
            fmo::Image dst{};
            WHEN("a 2x1 region is created from source image") {
                const fmo::Pos pos{1, 1};
                const fmo::Dims dims{2, 1};
                fmo::Region reg = src.region(pos, dims);
                THEN("region has correct position, dimensions and format") {
                    REQUIRE(reg.pos() == pos);
                    REQUIRE(reg.dims() == dims);
                    REQUIRE(reg.format() == fmo::Format::BGR);
                    AND_THEN("region data points to the expected location") {
                        auto* data = src.data();
                        data += 3 * pos.x;
                        data += 3 * src.dims().width * pos.y;
                        REQUIRE(reg.data() == data);
                        WHEN("region is copied to destination image") {
                            fmo::copy(reg, dst);
                            THEN("image has correct dimensions and format") {
                                REQUIRE(dst.dims() == dims);
                                REQUIRE(dst.format() == fmo::Format::BGR);
                                AND_THEN("image contains expected data") {
                                    REQUIRE(exact_match(dst, IM_2x1_BGR));
                                }
                            }
                        }
                        GIVEN("region within region") {
                            const fmo::Pos pos2Rel{1, 0};
                            const fmo::Dims dims2{1, 1};
                            fmo::Region reg2 = reg.region(pos2Rel, dims2);
                            THEN("inner region has correct position, dimensions and format") {
                                const fmo::Pos pos2 = {pos.x + pos2Rel.x, pos.y + pos2Rel.y};
                                REQUIRE(reg2.pos() == pos2);
                                REQUIRE(reg2.dims() == dims2);
                                REQUIRE(reg2.format() == fmo::Format::BGR);
                                AND_THEN("inner region points to the expected location") {
                                    auto* data2 = src.data();
                                    data2 += 3 * pos2.x;
                                    data2 += 3 * src.dims().width * pos2.y;
                                    REQUIRE(reg2.data() == data2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    GIVEN("a GRAY source image") {
        fmo::Image src{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_GRAY.data()};
        GIVEN("an empty destination image") {
            fmo::Image dst{};
            WHEN("a 2x2 region is created from source image") {
                const fmo::Pos pos{1, 0};
                const fmo::Dims dims{2, 2};
                fmo::Region reg = src.region(pos, dims);
                THEN("region has correct position, dimensions and format") {
                    REQUIRE(reg.pos() == pos);
                    REQUIRE(reg.dims() == dims);
                    REQUIRE(reg.format() == fmo::Format::GRAY);
                    AND_THEN("region data points to the expected location") {
                        auto* data = src.data();
                        data += 1 * pos.x;
                        data += 1 * src.dims().width * pos.y;
                        REQUIRE(reg.data() == data);
                        WHEN("region is copied to destination image") {
                            fmo::copy(reg, dst);
                            THEN("image has correct dimensions and format") {
                                REQUIRE(dst.dims() == dims);
                                REQUIRE(dst.format() == fmo::Format::GRAY);
                                AND_THEN("image contains expected data") {
                                    REQUIRE(exact_match(dst, IM_2x2_GRAY));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
