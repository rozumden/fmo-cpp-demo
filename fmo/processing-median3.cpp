#include "image-util.hpp"
#include "include-simd.hpp"
#include <fmo/processing.hpp>

namespace fmo {
    struct Median3Job : public cv::ParallelLoopBody {
#if defined(FMO_HAVE_AVX2)
        using batch_t = __m256i;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a = _mm256_load_si256((const batch_t*)(src1 + i));
                batch_t b = _mm256_load_si256((const batch_t*)(src2 + i));
                batch_t t = _mm256_max_epu8(a, b);
                b = _mm256_min_epu8(a, b);
                t = _mm256_min_epu8(t, _mm256_load_si256((const batch_t*)(src3 + i)));
                t = _mm256_max_epu8(b, t);
                _mm256_stream_si256((batch_t*)(dst + i), t);
            }
        }
#elif defined(FMO_HAVE_SSE2)
        using batch_t = __m128i;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a = _mm_load_si128((const batch_t*)(src1 + i));
                batch_t b = _mm_load_si128((const batch_t*)(src2 + i));
                batch_t t = _mm_max_epu8(a, b);
                b = _mm_min_epu8(a, b);
                t = _mm_min_epu8(t, _mm_load_si128((const batch_t*)(src3 + i)));
                t = _mm_max_epu8(b, t);
                _mm_stream_si128((batch_t*)(dst + i), t);
            }
        }
#elif defined(FMO_HAVE_NEON)
        using batch_t = uint8x16_t;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a = vld1q_u8(src1 + i);
                batch_t b = vld1q_u8(src2 + i);
                batch_t t = vmaxq_u8(a, b);
                b = vminq_u8(a, b);
                t = vminq_u8(t, vld1q_u8(src3 + i));
                t = vmaxq_u8(b, t);
                vst1q_u8(dst + i, t);
            }
        }
#else
        using batch_t = uint8_t;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i++) {
                uint8_t t = std::max(src1[i], src2[i]);
                uint8_t s = std::min(src1[i], src2[i]);
                t = std::min(t, src3[i]);
                dst[i] = std::max(s, t);
            }
        }
#endif

        Median3Job(const Mat& src1, const Mat& src2, const Mat& src3, Mat& dst)
            : mSrc1(src1.data()), mSrc2(src2.data()), mSrc3(src3.data()), mDst(dst.data()) {}

        virtual void operator()(const cv::Range& pieces) const override {
            size_t first = size_t(pieces.start) * sizeof(batch_t);
            size_t last = size_t(pieces.end) * sizeof(batch_t);
            const uint8_t* const src1 = mSrc1 + first;
            const uint8_t* const src2 = mSrc2 + first;
            const uint8_t* const src3 = mSrc3 + first;
            uint8_t* const dst = mDst + first;
            const size_t iEnd = last - first;
            impl(src1, src2, src3, dst, iEnd);
        }

    private:
        const uint8_t* const mSrc1;
        const uint8_t* const mSrc2;
        const uint8_t* const mSrc3;
        uint8_t* const mDst;
    };

    void median3(const Image& src1, const Image& src2, const Image& src3, Image& dst) {
        const Format format = src1.format();
        const Dims dims = src1.dims();
        const cv::Size size = getCvSize(format, dims);
        const size_t bytes = size_t(size.width) * size_t(size.height) * getPixelStep(format);
        const size_t pieces = bytes / sizeof(Median3Job::batch_t);

        if (format != src2.format() || dims != src2.dims() || format != src3.format() ||
            dims != src3.dims()) {
            throw std::runtime_error("median3: format/dimensions mismatch of inputs");
        }

        // run the job in parallel
        dst.resize(format, dims);
        Median3Job job{src1, src2, src3, dst};
        cv::parallel_for_(cv::Range{0, int(pieces)}, job, cv::getNumThreads());

        // process the last few bytes inidividually
        for (size_t i = pieces * sizeof(Median3Job::batch_t); i < bytes; i++) {
            uint8_t t = std::max(src1.data()[i], src2.data()[i]);
            uint8_t s = std::min(src1.data()[i], src2.data()[i]);
            t = std::min(t, src3.data()[i]);
            dst.data()[i] = std::max(s, t);
        }
    }
}
