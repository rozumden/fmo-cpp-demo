#ifndef FMO_SUBSAMPLER_HPP
#define FMO_SUBSAMPLER_HPP

#include <fmo/common.hpp>
#include <fmo/image.hpp>

namespace fmo {
    /// Similar to subsample(), but also allows to subsample YUV420SP images, in which case an YUV
    /// image is created.
    struct Subsampler {
        /// Performs decimation, as when subsample() is called, but with additional support for
        /// YUV420SP inputs.
        void operator()(const Mat& src, Mat& dst);

        /// Provides the dimensions of the output, given that the decimation input has dimensions
        /// "dims".
        Dims nextDims(Dims dims);

        /// Provides the format of the output, given that the decimation input has format "before".
        Format nextFormat(Format before);

        /// Provides the pixel size in the output, given that the decimation input has pixel size
        /// "before".
        int nextPixelSize(int before) { return before * 2; }

    private:
        Image y, u, v;
    };
}

#endif // FMO_SUBSAMPLER_HPP
