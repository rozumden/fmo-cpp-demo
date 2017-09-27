#ifndef FMO_STRIPGEN_HPP
#define FMO_STRIPGEN_HPP

#include <fmo/common.hpp>
#include <vector>

namespace fmo {
    /// Strip is a non-empty image region with a width of 1 pixel in the processing resolution.
    /// In the original resolution, strips are wider.
    struct Strip {
        Strip() = default;
        Strip(const Strip&) = default;
        Strip(Pos16 aPos, Dims16 aHalfDims) : pos(aPos), halfDims(aHalfDims) {}
        Strip& operator=(const Strip&) = default;

        /// Finds out if two strips touch each other, i.e. they belong to the same connected
        /// component.
        static bool inContact(const Strip& l, const Strip& r, int step) {
            int dx = r.pos.x - l.pos.x;
            if (dx > step) return false;
            int dy = (r.pos.y > l.pos.y) ? (r.pos.y - l.pos.y) : (l.pos.y - r.pos.y);
            return dy < l.halfDims.height + r.halfDims.height;
        }

        /// Finds out if two strips would overlap if they were in the same column.
        static bool overlapY(const Strip& l, const Strip& r) {
            int dy = (r.pos.y > l.pos.y) ? (r.pos.y - l.pos.y) : (l.pos.y - r.pos.y);
            return dy < l.halfDims.height + r.halfDims.height;
        }

        // data
        Pos16 pos;       ///< coordinates of the center of the strip in the source image
        Dims16 halfDims; ///< dimensions of the strip in the source image, divided by 2
    };

    /// Detects vertical strips by iterating over all pixels in a binary image. Strip is a non-empty
    /// image region with a width of 1 pixel in the processing resolution. In the original
    /// resolution, strips are wider.
    struct StripGen {
        /// Detects vertical strips in the binary image img. Strips shorter than minHeight will be
        /// discarded as noise. The vertical gap between two strips (that are not considered noise)
        /// must be at least minGap, otherwise both strips are discarded.
        ///
        /// @param img image to find strips in
        /// @param minHeight minimum height of strip, otherwise the strip is discarded
        /// @param minGap minimum gap between strips
        /// @param step ratio of original-resolution to processing-resolution pixels
        /// @param out resulting strips, in no particular order
        /// @param outNoise the number of strips discarded due to minHeight
        void operator()(const fmo::Mat& img, int minHeight, int minGap, int step,
                        std::vector<Strip>& out, int& outNoise);

    private:
        std::vector<int16_t> mRle; ///< cache for run-length encodings
        std::vector<Strip> mTemp;  ///< cache for strips
    };
}

#endif // FMO_STRIPGEN_HPP
