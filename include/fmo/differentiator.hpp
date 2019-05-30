#ifndef FMO_DIFFERENTIATOR_HPP
#define FMO_DIFFERENTIATOR_HPP

#include <fmo/common.hpp>
#include <fmo/image.hpp>

namespace fmo {

    /// Computes first-order absolute difference images in various formats.
    struct Differentiator {
        struct Config {
            /// Initial value for $\Delta$.
            uint8_t thresh;
            /// Threshold factor.
            float diffThFactor;
            /// If the noise level is below this value, $\Delta$ is decreased to make
            /// differentiation more sensitive.
            float noiseMin;
            /// If the noise level is above this value, $\Delta$ is increased to make
            /// differentiation less sensitive.
            float noiseMax;
            /// The period of adjusting $\Delta$.
            int adjustPeriod;

            Config();
        };

        Differentiator(const Config& config);

        /// Computes first-order absolute difference image in various formats. The inputs must have
        /// the same format and size. The output is resized to match the size of the inputs and its
        /// format is set to GRAY. The output image is binary -- the values are either 0x00 or 0xFF.
        void operator()(const Mat& src1, const Mat& src2, Image& dst);

        /// Adjusts the threshold. The provided value is weighted by the number of pixels in the
        /// image to obtain a noise fraction. Threshold is adjusted appropriately in order to keep
        /// the noise fraction in the range mCfg.noiseMin to mCfg.noiseMax.
        void reportAmountOfNoise(int noise);

    private:
        const Config mCfg;       ///< configuration object, received upon construction
        Image mAbsDiff;          ///< cached absolute difference image
        uint8_t mThresh;         ///< current threshold
        std::vector<int> mNoise; ///< recent noise amounts
    };
}

#endif // FMO_DIFFERENTIATOR_HPP
