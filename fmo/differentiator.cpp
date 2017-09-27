#include "image-util.hpp"
#include "include-opencv.hpp"
#include "include-simd.hpp"
#include <fmo/differentiator.hpp>
#include <fmo/processing.hpp>

namespace fmo {
    namespace {
        constexpr uint8_t threshDecrement = 1;
        constexpr uint8_t threshIncrement = 1;
        constexpr uint8_t threshMin = 10;
        constexpr uint8_t threshMax = 254;
    }

    Differentiator::Config::Config()
        : thresh(24), noiseMin(0.0035), noiseMax(0.0047), adjustPeriod(4) {}

    Differentiator::Differentiator(const Config& cfg)
        : mCfg(cfg), mThresh(std::min(std::max(cfg.thresh, threshMin), threshMax)) {
        mNoise.reserve(mCfg.adjustPeriod);
    }

    struct AddAndThreshJob : public cv::ParallelLoopBody {

#if defined(FMO_HAVE_NEON)
        using batch_t = uint8x16_t;
        using batch3_t = uint8x16x3_t;

        static void impl(const uint8_t* src, const uint8_t* srcEnd, uint8_t* dst, uint8_t thresh) {
            batch_t threshVec = vld1q_dup_u8(&thresh);

            for (; src < srcEnd; src += SRC_BATCH_SIZE, dst += DST_BATCH_SIZE) {
                batch3_t v = vld3q_u8(src);
                batch_t sum = vqaddq_u8(vqaddq_u8(v.val[0], v.val[1]), v.val[2]);
                sum = vcgtq_u8(sum, threshVec);
                vst1q_u8(dst, sum);
            }
        }
#else
        using batch_t = uint8_t;

        static void impl(const uint8_t* src, const uint8_t* srcEnd, uint8_t* dst, uint8_t thresh) {
            int t = int(thresh);
            for (; src < srcEnd; src += SRC_BATCH_SIZE, dst += DST_BATCH_SIZE) {
                *dst = ((src[0] + src[1] + src[2]) > t) ? uint8_t(0xFF) : uint8_t(0);
            }
        }
#endif

        enum : size_t {
            SRC_BATCH_SIZE = sizeof(batch_t) * 3,
            DST_BATCH_SIZE = sizeof(batch_t),
        };

        AddAndThreshJob(const uint8_t* src, uint8_t* dst, uint8_t thresh)
            : mSrc(src), mDst(dst), mThresh(thresh) {}

        virtual void operator()(const cv::Range& pieces) const override {
            size_t firstSrc = size_t(pieces.start) * SRC_BATCH_SIZE;
            size_t lastSrc = size_t(pieces.end) * SRC_BATCH_SIZE;
            size_t firstDst = size_t(pieces.start) * DST_BATCH_SIZE;
            const uint8_t* src = mSrc + firstSrc;
            const uint8_t* srcEnd = mSrc + lastSrc;
            uint8_t* dst = mDst + firstDst;
            impl(src, srcEnd, dst, mThresh);
        }

    private:
        const uint8_t* const mSrc;
        uint8_t* const mDst;
        uint8_t mThresh;
    };

    void addAndThresh(const Image& src, Image& dst, uint8_t thresh) {
        const Format format = src.format();
        const Dims dims = src.dims();
        const size_t pixels = size_t(dims.width) * size_t(dims.height);
        const size_t pieces = pixels / AddAndThreshJob::DST_BATCH_SIZE;

        if (getPixelStep(format) != 3) { throw std::runtime_error("addAndThresh(): bad format"); }

        // run the job in parallel
        dst.resize(Format::GRAY, dims);
        AddAndThreshJob job{src.data(), dst.data(), thresh};
        cv::parallel_for_(cv::Range{0, int(pieces)}, job, cv::getNumThreads());

        // process the last few bytes individually
        size_t lastIndex = pieces * AddAndThreshJob::DST_BATCH_SIZE;
        const uint8_t* data = src.data() + (lastIndex * 3);
        uint8_t* out = dst.data() + lastIndex;
        uint8_t* outEnd = dst.data() + pixels;
        int t = int(thresh);
        for (; out < outEnd; out++, data += 3) {
            *out = ((data[0] + data[1] + data[2]) > t) ? uint8_t(0xFF) : uint8_t(0);
        }
    }

    void Differentiator::operator()(const Mat& src1, const Mat& src2, Image& dst) {
        // calibrate threshold based on measured noise
        if (int(mNoise.size()) >= mCfg.adjustPeriod) {
            std::sort(begin(mNoise), end(mNoise));
            auto median = begin(mNoise) + (mNoise.size() / 2);
            int noiseAmount = *median;
            mNoise.clear();

            int numPixels = src1.dims().width * src1.dims().height;
            double noiseFrac = double(noiseAmount) / double(numPixels);

            if (noiseFrac > mCfg.noiseMax) {
                mThresh += threshIncrement;
                mThresh = std::min(mThresh, threshMax);
            }

            if (noiseFrac < mCfg.noiseMin) {
                mThresh -= threshDecrement;
                mThresh = std::max(mThresh, threshMin);
            }
        }

        // calculate absolute differences
        absdiff(src1, src2, mAbsDiff);

        // threshold
        switch (mAbsDiff.format()) {
        case Format::GRAY: {
            greater_than(mAbsDiff, dst, mThresh);
            return;
        }
        case Format::BGR:
        case Format::YUV: {
            addAndThresh(mAbsDiff, dst, mThresh);
            return;
        }
        default:
            throw std::runtime_error("Differentiator: unsupported format");
        }
    }

    void Differentiator::reportAmountOfNoise(int noise) { mNoise.push_back(noise); }
}
