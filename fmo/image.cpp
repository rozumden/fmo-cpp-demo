#include "image-util.hpp"
#include <fmo/assert.hpp>
#include <fmo/region.hpp>

namespace fmo {
    Image::Image(const std::string& filename, Format format) {
        cv::Mat mat;

        switch (format) {
        case Format::BGR:
        case Format::YUV:
            mat = cv::imread(filename, cv::IMREAD_COLOR);
            break;
        default:
            mat = cv::imread(filename, cv::IMREAD_GRAYSCALE);
            break;
        }

        if (mat.data == nullptr) { throw std::runtime_error("failed to open image"); }

        FMO_ASSERT(mat.isContinuous(), "reading image: not continuous");
        FMO_ASSERT(mat.type() == getCvType(format), "reading image: unexpected mat type");
        Dims dims = getDims(format, mat.size());
        size_t bytes = mat.elemSize() * mat.total();
        FMO_ASSERT(getNumBytes(format, dims) == bytes, "reading image: unexpected size");
        mData.resize(bytes);
        std::copy(mat.data, mat.data + mData.size(), mData.data());
        mFormat = format;
        mDims = dims;
    }

    void Image::assign(Format format, Dims dims, const uint8_t* data) {
        size_t bytes = getNumBytes(format, dims);
        mData.resize(bytes);
        mDims = dims;
        mFormat = format;
        std::copy(data, data + bytes, mData.data());
    }

    Region Image::region(Pos pos, Dims dims) {
        if (pos.x < 0 || pos.y < 0 || pos.x + dims.width > mDims.width ||
            pos.y + dims.height > mDims.height) {
            throw std::runtime_error("region outside image");
        }

        auto rowStep = static_cast<size_t>(mDims.width);

        if (mFormat == Format::YUV420SP) {
            if (pos.x % 2 != 0 || pos.y % 2 != 0 || dims.width % 2 != 0 || dims.height % 2 != 0) {
                throw std::runtime_error("region: YUV420SP regions must be aligned to 2px");
            }

            uint8_t* start = data();
            start += static_cast<size_t>(pos.x);
            start += rowStep * static_cast<size_t>(pos.y);

            uint8_t* uvStart = uvData();
            uvStart += static_cast<size_t>(pos.x);
            uvStart += rowStep * static_cast<size_t>(pos.y / 2);

            return {Format::YUV420SP, pos, dims, start, uvStart, rowStep};
        } else {
            size_t pixelStep = getPixelStep(mFormat);
            rowStep *= pixelStep;

            uint8_t* start = mData.data();
            start += pixelStep * static_cast<size_t>(pos.x);
            start += rowStep * static_cast<size_t>(pos.y);

            return {mFormat, pos, dims, start, nullptr, rowStep};
        }
    }

    void Image::resize(Format format, Dims dims) {
        size_t bytes = getNumBytes(format, dims);
        mData.resize(bytes);
        mDims = dims;
        mFormat = format;
    }

    cv::Mat Image::wrap() { return {getCvSize(mFormat, mDims), getCvType(mFormat), mData.data()}; }

    cv::Mat Image::wrap() const {
        auto* ptr = const_cast<uint8_t*>(mData.data());
        return {getCvSize(mFormat, mDims), getCvType(mFormat), ptr};
    }
}
