#ifndef FMO_REGION_HPP
#define FMO_REGION_HPP

#include <fmo/common.hpp>

namespace fmo {
    /// Refers to a rectangular part of an image. Does not own any data.
    struct Region final : public Mat {
        Region();
        Region(const Region&) = default;
        Region& operator=(const Region&) = default;
        Region(Format format, Pos pos, Dims dims, uint8_t* data, uint8_t* uvData, size_t rowStep);

        /// Provides information about the position of the region in the original image.
        Pos pos() const { return mPos; }

        /// Creates a new region that refers to a rectangular area inside this region.
        virtual Region region(Pos pos, Dims dims) override;

        /// The number of bytes to advance if one needs to access the next row.
        virtual size_t skip() const override { return mRowStep; }

        /// Provides access to image data.
        virtual uint8_t* data() override { return mData; }

        /// Provides access to image data.
        virtual const uint8_t* data() const override { return mData; }

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual uint8_t* uvData() override { return mUvData; }

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual const uint8_t* uvData() const override { return mUvData; }

        /// Resizes the region to match the desired format and dimensions. A region cannot mustn't
        /// be resized -- an exception is thrown if the current format and dimensions don't match
        /// the arguments.
        virtual void resize(Format format, Dims dims) override;

        /// Wraps the data pointer in a Mat object.
        virtual cv::Mat wrap() override;

        /// Wraps the data pointer in a Mat object. Be careful with this one -- use the returned Mat
        /// only for reading.
        virtual cv::Mat wrap() const override;

    private:
        Pos mPos;
        uint8_t* mData;
        uint8_t* mUvData;
        size_t mRowStep;
    };
}

#endif // FMO_REGION_HPP
