#include "image-util.hpp"

namespace fmo {
    size_t getNumBytes(Format format, Dims dims) {
        size_t result = static_cast<size_t>(dims.width) * static_cast<size_t>(dims.height);

        switch (format) {
        case Format::GRAY:
            break;
        case Format::BGR:
        case Format::YUV:
            result *= 3;
            break;
        case Format::INT32:
        case Format::FLOAT:
            result *= 4;
            break;
        case Format::YUV420SP:
            result = (result * 3) / 2;
            break;
        default:
            throw std::runtime_error("getNumBytes: unsupported format");
        }

        return result;
    }

    cv::Size getCvSize(Format format, Dims dims) {
        cv::Size result{dims.width, dims.height};
        if (format == Format::YUV420SP) { result.height = (result.height * 3) / 2; }
        return result;
    }

    Dims getDims(Format format, cv::Size size) {
        Dims result{size.width, size.height};
        if (format == Format::YUV420SP) { result.height = (result.height * 2) / 3; }
        return result;
    }

    int getCvType(Format format) {
        switch (format) {
        case Format::GRAY:
            return CV_8UC1;
        case Format::BGR:
        case Format::YUV:
            return CV_8UC3;
        case Format::INT32:
            return CV_32SC1;
        case Format::YUV420SP:
            return CV_8UC1;
        case Format::FLOAT:
            return CV_32FC1;
        default:
            throw std::runtime_error("getCvType: unsupported format");
        }
    }

    size_t getPixelStep(Format format) {
        switch (format) {
        case Format::GRAY:
            return 1;
        case Format::BGR:
        case Format::YUV:
            return 3;
        case Format::FLOAT:
        case Format::INT32:
            return 4;
        case Format::YUV420SP:
            throw std::runtime_error("getPixelStep: not applicable to YUV420SP");
        default:
            throw std::runtime_error("getPixelStep: unsupported format");
        }
    }

    cv::Mat yuv420SPWrapGray(const Mat& mat) {
        Dims dims = mat.dims();
        uint8_t* data = const_cast<uint8_t*>(mat.data());
        return {cv::Size(dims.width, dims.height), CV_8UC1, data};
    }

    cv::Mat yuv420SPWrapFloat(const Mat& mat) {
        Dims dims = mat.dims();
        uint8_t* data = const_cast<uint8_t*>(mat.data());
        return {cv::Size(dims.width, dims.height), CV_32FC1, data};
    }

    cv::Mat yuv420SPWrapUV(const Mat& mat) {
        Dims dims = mat.dims();
        uint8_t* data = const_cast<uint8_t*>(mat.uvData());
        return {cv::Size(dims.width, dims.height / 2), CV_8UC1, data};
    }
}
