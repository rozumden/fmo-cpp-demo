#ifndef FMO_COMMON_HPP
#define FMO_COMMON_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cv {
    class Mat;
}

namespace fmo {
    // forward declarations
    struct Image;
    struct Region;

    /// Possible image color formats.
    enum class Format {
        UNKNOWN = 0,
        GRAY,
        BGR,
        YUV,
        INT32,
        YUV420SP,
        FLOAT,
    };

    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
    
    /// Image location.
    struct Pos {
        int x, y;

        friend bool operator==(const Pos& lhs, const Pos& rhs) {
            return lhs.x == rhs.x && lhs.y == rhs.y;
        }

        friend bool operator!=(const Pos& lhs, const Pos& rhs) { return !(lhs == rhs); }
    };

    /// Image dimensions.
    struct Dims {
        int width, height;

        friend bool operator==(const Dims& lhs, const Dims& rhs) {
            return lhs.width == rhs.width && lhs.height == rhs.height;
        }

        friend bool operator!=(const Dims& lhs, const Dims& rhs) { return !(lhs == rhs); }
    };

    /// A rectangular area.
    struct Bounds {
        Pos min; ///< minimum coordinates (inclusive)
        Pos max; ///< maximum coordinates (inclusive)

        friend bool operator==(const Bounds& lhs, const Bounds& rhs) {
            return lhs.min == rhs.min && lhs.max == rhs.max;
        }

        friend bool operator!=(const Bounds& lhs, const Bounds& rhs) { return !(lhs == rhs); }
    };

    /// Image location (16-bit).
    struct Pos16 {
        int16_t x, y;

        Pos16() = default;
        Pos16(const Pos& pos) : x(int16_t(pos.x)), y(int16_t(pos.y)) {}
        Pos16(int16_t aX, int16_t aY) : x(aX), y(aY) {}
        operator Pos() const { return {x, y}; }

        int16_t operator [](int i) const { if (i == 0) return x; else return y;}
        int16_t & operator [](int i) { if (i == 0) return x; else return y; }
    };

    /// Image dimensions (16-bit).
    struct Dims16 {
        int16_t width, height;

        Dims16() = default;
        Dims16(const Dims& dims) : width(int16_t(dims.width)), height(int16_t(dims.height)) {}
        Dims16(int16_t aWidth, int16_t aHeight) : width(aWidth), height(aHeight) {}
        operator Dims() const { return {width, height}; }
    };

    /// A rectangular area (16-bit).
    struct Bounds16 {
        Pos16 min, max;

        Bounds16() = default;
        Bounds16(const Bounds& bounds) : min(bounds.min), max(bounds.max) {}
        Bounds16(Pos16 aMin, Pos16 aMax) : min(aMin), max(aMax) {}
        operator Bounds() const { return {min, max}; }
    };

    /// An object that represents the OpenCV Mat class. Use the wrap() method to create an instance
    /// of cv::Mat.
    struct Mat {
        ~Mat() = default;
        Mat() = default;
        Mat(const Mat&) = default;
        Mat& operator=(const Mat&) = default;
        Mat(Format format, Dims dims) : mFormat(format), mDims(dims) {}

        /// Provides current image format.
        Format format() const { return mFormat; }

        /// Provides current image dimensions.
        Dims dims() const { return mDims; }

        /// Creates a region that refers to a rectangular area inside the image.
        virtual Region region(Pos pos, Dims dims) = 0;

        /// The number of bytes to advance if one needs to access the next row.
        virtual size_t skip() const = 0;

        /// Provides access to image data.
        virtual uint8_t* data() = 0;

        /// Provides access to image data.
        virtual const uint8_t* data() const = 0;

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual uint8_t* uvData() = 0;

        /// Provides access to the part of image data where UVis stored. Only for YUV420SP images.
        virtual const uint8_t* uvData() const = 0;

        /// Resizes the image to match the desired format and dimensions.
        virtual void resize(Format format, Dims dims) = 0;

        /// Wraps the data pointer in a Mat object.
        virtual cv::Mat wrap() = 0;

        /// Wraps the data pointer in a Mat object. Be careful with this one -- use the returned Mat
        /// only for reading.
        virtual cv::Mat wrap() const = 0;

    protected:
        Format mFormat = Format::UNKNOWN;
        Dims mDims = {0, 0};
    };
}

#endif // FMO_COMMON_HPP
