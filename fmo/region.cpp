#include "image-util.hpp"
#include <fmo/region.hpp>

namespace fmo {
    Region::Region()
        : Mat(Format::UNKNOWN, {0, 0}), mPos{0, 0}, mData(nullptr), mUvData(nullptr), mRowStep(0) {}

    Region::Region(Format format, Pos pos, Dims dims, uint8_t* data, uint8_t* uvData,
                   size_t rowStep)
        : Mat(format, dims), mPos(pos), mData(data), mUvData(uvData), mRowStep(rowStep) {
        if (pos.x < 0 || pos.y < 0 || dims.width < 0 || dims.height < 0) {
            throw std::runtime_error("region: bad constructor arguments");
        }
    }

    Region Region::region(Pos pos, Dims dims) {
        if (pos.x < 0 || pos.y < 0 || pos.x + dims.width > mDims.width ||
            pos.y + dims.height > mDims.height) {
            throw std::runtime_error("sub-region outside region");
        }

        Pos newPos{mPos.x + pos.x, mPos.y + pos.y};

        if (mFormat == Format::YUV420SP) {
            if (pos.x % 2 != 0 || pos.y % 2 != 0 || dims.width % 2 != 0 || dims.height % 2 != 0) {
                throw std::runtime_error("region: YUV420SP regions must be aligned to 2px");
            }

            uint8_t* start = mData;
            start += static_cast<size_t>(pos.x);
            start += mRowStep * static_cast<size_t>(pos.y);

            uint8_t* uvStart = mUvData;
            uvStart += static_cast<size_t>(pos.x);
            uvStart += mRowStep * static_cast<size_t>(pos.y / 2);

            return {mFormat, newPos, dims, start, uvStart, mRowStep};
        } else {
            uint8_t* start = mData;
            start += getPixelStep(mFormat) * static_cast<size_t>(pos.x);
            start += mRowStep * static_cast<size_t>(pos.y);

            return {mFormat, newPos, dims, start, nullptr, mRowStep};
        }
    }

    void Region::resize(Format format, Dims dims) {
        if (mFormat != format || mDims != dims) {
            throw std::runtime_error("resize: regions mustn't change format or size");
        }
    }

    cv::Mat Region::wrap() {
        if (mFormat == Format::YUV420SP) {
            throw std::runtime_error("wrap: cannot wrap YUV420SP regions");
        }
        return {getCvSize(mFormat, mDims), getCvType(mFormat), mData, mRowStep};
    }

    cv::Mat Region::wrap() const {
        if (mFormat == Format::YUV420SP) {
            throw std::runtime_error("wrap: cannot wrap YUV420SP regions");
        }
        return {getCvSize(mFormat, mDims), getCvType(mFormat), mData, mRowStep};
    }
}
