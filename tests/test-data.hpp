#ifndef FMO_TEST_DATA_HPP
#define FMO_TEST_DATA_HPP

#include <array>
#include <fmo/common.hpp>

extern const char* const IM_4x2_FILE;
extern const fmo::Dims IM_4x2_DIMS;
extern const std::array<uint8_t, 24> IM_4x2_BGR;
extern const std::array<uint8_t, 6> IM_2x1_BGR;
extern const std::array<uint8_t, 8> IM_4x2_GRAY;
extern const std::array<uint8_t, 8> IM_4x2_LESS_THAN;
extern const std::array<uint8_t, 8> IM_4x2_GREATER_THAN;
extern const std::array<uint8_t, 8> IM_4x2_ABSDIFF;
extern const std::array<uint8_t, 8> IM_4x2_RANDOM_1;
extern const std::array<uint8_t, 8> IM_4x2_RANDOM_2;
extern const std::array<uint8_t, 8> IM_4x2_RANDOM_3;
extern const std::array<uint8_t, 8> IM_4x2_MEDIAN3;
extern const std::array<uint8_t, 4> IM_2x2_GRAY;
extern const std::array<uint8_t, 24> IM_4x2_GRAY_3;
extern const std::array<uint8_t, 12> IM_4x2_YUV420SP;
extern const std::array<uint8_t, 24> IM_4x2_YUV2BGR;
extern const std::array<uint8_t, 12> IM_4x2_YUV420SP_2;
extern const std::array<uint8_t, 6> IM_4x2_SUBSAMPLED;

#endif // FMO_TEST_DATA_HPP
