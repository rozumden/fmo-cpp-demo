#ifndef FMO_IMAGE_UTIL_HPP
#define FMO_IMAGE_UTIL_HPP

#include "include-opencv.hpp"
#include <fmo/image.hpp>

namespace fmo {
    /// Get the number of bytes of data that an image requires, given its format and dimensions.
    size_t getNumBytes(Format format, Dims dims);

    /// Convert the actual dimensions to the size that is used by OpenCV. OpenCV considers YUV
    /// 4:2:0 SP images 1.5x taller.
    cv::Size getCvSize(Format format, Dims dims);

    /// Convert the size used by OpenCV to the actual dimensions. OpenCV considers YUV 4:2:0 SP
    /// images 1.5x taller.
    Dims getDims(Format format, cv::Size size);

    /// Get the Mat data type used by OpenCV that corresponds to the format.
    int getCvType(Format format);

    /// Get the number of bytes between a color value and the next one. This makes sense only
    /// for interleaved formats, such as GRAY, BGR, or INT32.
    size_t getPixelStep(Format format);

    /// Access the gray channel of a YUV420SP mat.
    cv::Mat yuv420SPWrapGray(const Mat& mat);

    /// Access the FLOAT channel of a YUV420SP mat.
    cv::Mat yuv420SPWrapFloat(const Mat& mat);

    /// Access the UV channel of a YUV420SP mat.
    cv::Mat yuv420SPWrapUV(const Mat& mat);
}

#endif
