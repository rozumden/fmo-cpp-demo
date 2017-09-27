#include "../catch/catch.hpp"
#include "test-data.hpp"
#include "test-tools.hpp"

SCENARIO("reading images from files", "[image][load]") {
    WHEN("loading and converting a known image to BGR") {
        fmo::Image image{IM_4x2_FILE, fmo::Format::BGR};
        THEN("image has correct dimensions") {
            REQUIRE(image.dims() == IM_4x2_DIMS);
            AND_THEN("image has correct format") {
                REQUIRE(image.format() == fmo::Format::BGR);
                AND_THEN("image matches hard-coded values") {
                    REQUIRE(exact_match(image, IM_4x2_BGR));
                }
            }
        }
    }
    WHEN("loading and converting a known image to GRAY") {
        fmo::Image image{IM_4x2_FILE, fmo::Format::GRAY};
        THEN("image has correct dimensions") {
            REQUIRE(image.dims() == IM_4x2_DIMS);
            AND_THEN("image has correct format") {
                REQUIRE(image.format() == fmo::Format::GRAY);
                AND_THEN("image matches hard-coded values") {
                    REQUIRE(exact_match(image, IM_4x2_GRAY));
                }
            }
        }
    }
    WHEN("loading into an unsupported format") {
        THEN("constructor throws") {
            REQUIRE_THROWS(fmo::Image(IM_4x2_FILE, fmo::Format::UNKNOWN));
        }
    }
    WHEN("the image file doesn't exist") {
        THEN("constructor throws") { REQUIRE_THROWS(fmo::Image("Eh3qUrSOFl", fmo::Format::BGR)); }
    }
    GIVEN("a BGR source image") {
        fmo::Image src{fmo::Format::BGR, IM_4x2_DIMS, IM_4x2_BGR.data()};
        WHEN("the image is saved to a file") {
            const std::string file = "temp_bgr.png";
            fmo::save(src, file);
            AND_WHEN("the image is re-loaded") {
                fmo::Image loaded{file, src.format()};
                THEN("the re-loaded image matches the original one") {
                    REQUIRE(src.format() == loaded.format());
                    REQUIRE(src.dims() == loaded.dims());
                    REQUIRE(exact_match(src, loaded));
                }
            }
        }
    }
    GIVEN("a GRAY source image") {
        fmo::Image src{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_GRAY.data()};
        WHEN("the image is saved to a file") {
            const std::string file = "temp_gray.png";
            fmo::save(src, file);
            AND_WHEN("the image is re-loaded") {
                fmo::Image loaded{file, src.format()};
                THEN("the re-loaded image matches the original one") {
                    REQUIRE(src.format() == loaded.format());
                    REQUIRE(src.dims() == loaded.dims());
                    REQUIRE(exact_match(src, loaded));
                }
            }
        }
    }
    GIVEN("a YUV420SP source image") {
        fmo::Image src{fmo::Format::YUV420SP, IM_4x2_DIMS, IM_4x2_YUV420SP.data()};
        WHEN("the image is saved to a file") {
            const std::string file = "temp_yuv420sp.png";
            fmo::save(src, file);
            AND_WHEN("the image is re-loaded") {
                fmo::Image loaded{file, src.format()};
                THEN("the re-loaded image matches the original one") {
                    REQUIRE(src.format() == loaded.format());
                    REQUIRE(src.dims() == loaded.dims());
                    REQUIRE(exact_match(src, loaded));
                }
            }
        }
    }
}
