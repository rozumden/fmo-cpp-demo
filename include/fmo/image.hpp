#ifndef FMO_IMAGE_HPP
#define FMO_IMAGE_HPP

#include <fmo/common.hpp>
#include <fmo/allocator.hpp>
#include <memory>
#include <string>
#include <vector>

namespace fmo {
    /// Stores an image in contiguous memory. Has value semantics, i.e. copying aninstance of Image
    /// will perform a copy of the entire image data.
    struct Image final : public Mat {
        using iterator = uint8_t*;
        using const_iterator = const uint8_t*;

        ~Image() = default;
        Image() = default;

        /// Copies the contents from another image.
        Image(const Image& rhs) { assign(rhs.mFormat, rhs.mDims, rhs.mData.data()); }

        /// Copies the contents from another image.
        Image& operator=(const Image& rhs) {
            assign(rhs.mFormat, rhs.mDims, rhs.mData.data());
            return *this;
        }

        /// Swaps the contents of the image with another image.
        Image(Image&& rhs) noexcept { swap(rhs); }

        /// Swaps the contents of the image with another image.
        Image& operator=(Image&& rhs) noexcept {
            swap(rhs);
            return *this;
        }

        /// Reads an image from file and converts it to the desired format.
        Image(const std::string& filename, Format format);

        /// Copies an image from memory.
        Image(Format format, Dims dims, const uint8_t* data) { assign(format, dims, data); }

        /// Copies an image from memory.
        void assign(Format format, Dims dims, const uint8_t* data);

        /// Creates an image with specified format and dimensions.
        Image(Format format, Dims dims) { resize(format, dims); }

        /// The number of bytes in the image.
        size_t size() const { return mData.size(); }

        /// Provides iterator access to the underlying data.
        iterator begin() { return mData.data(); }

        /// Provides iterator access to the underlying data.
        const_iterator begin() const { return mData.data(); }

        /// Provides iterator access to the underlying data.
        friend iterator begin(Image& img) { return img.mData.data(); }

        /// Provides iterator access to the underlying data.
        friend const_iterator begin(const Image& img) { return img.mData.data(); }

        /// Provides iterator access to the underlying data.
        iterator end() { return begin() + mData.size(); }

        /// Provides iterator access to the underlying data.
        const_iterator end() const { return begin() + mData.size(); }

        /// Provides iterator access to the underlying data.
        friend iterator end(Image& img) { return img.end(); }

        /// Provides iterator access to the underlying data.
        friend const_iterator end(const Image& img) { return img.end(); }

        /// Swaps the contents of the two Image instances.
        void swap(Image& rhs) noexcept {
            mData.swap(rhs.mData);
            std::swap(mDims, rhs.mDims);
            std::swap(mFormat, rhs.mFormat);
        }

        /// Removes all data and sets the size to zero. Does not deallocate any memory.
        void clear() noexcept {
            mData.clear();
            mDims = {0, 0};
            mFormat = Format::UNKNOWN;
        }

        /// Swaps the contents of the two Image instances.
        friend void swap(Image& lhs, Image& rhs) noexcept { lhs.swap(rhs); }

        /// Creates a region that refers to a rectangular area in the image.
        virtual Region region(Pos pos, Dims dims) override;

        /// The number of bytes to advance if one needs to access the next row.
        virtual size_t skip() const override { return mDims.width; }

        /// Provides access to image data.
        virtual uint8_t* data() override { return mData.data(); }

        /// Provides access to image data.
        virtual const uint8_t* data() const override { return mData.data(); }

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual uint8_t* uvData() override { return data() + (mDims.width * mDims.height); }

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual const uint8_t* uvData() const override {
            return data() + (mDims.width * mDims.height);
        }

        /// Resizes the image to match the desired format and dimensions. When the size increases,
        /// iterators may get invalidated and all previous contents may be erased.
        virtual void resize(Format format, Dims dims) override;

        /// Wraps the data pointer in a Mat object.
        virtual cv::Mat wrap() override;

        /// Wraps the data pointer in a Mat object. Be careful with this one -- use the returned Mat
        /// only for reading.
        virtual cv::Mat wrap() const override;

    private:
        std::vector<uint8_t, fmo::detail::aligned_allocator<uint8_t, 32>> mData;
    };
}

#endif // FMO_IMAGE_HPP
