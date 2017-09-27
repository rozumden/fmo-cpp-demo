#include "image-util.hpp"
#include "include-simd.hpp"
#include <fmo/processing.hpp>
#include <iostream>

namespace fmo {
    struct Median5Job : public cv::ParallelLoopBody {
#if defined(FMO_HAVE_AVX2)
        using batch_t = __m256i;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, 
                  const uint8_t* src4, const uint8_t* src5, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a1 = _mm256_load_si256((const batch_t*)(src1 + i));
                batch_t a2 = _mm256_load_si256((const batch_t*)(src2 + i));
                batch_t a3 = _mm256_load_si256((const batch_t*)(src3 + i));
                batch_t a4 = _mm256_load_si256((const batch_t*)(src4 + i));
                batch_t a5 = _mm256_load_si256((const batch_t*)(src5 + i));

                batch_t max1 = _mm256_max_epu8(a1, a2);
                batch_t min1 = _mm256_min_epu8(a1, a2);
                batch_t max2 = _mm256_max_epu8(a3, a4);
                batch_t min2 = _mm256_min_epu8(a3, a4);
                min1 = _mm256_max_epu8(min1,min2);
                max1 = _mm256_min_epu8(max1,max2);

                min2 = _mm256_max_epu8(min1, max1);
                max2 = _mm256_min_epu8(min1, max1);
                min2 = _mm256_min_epu8(min2, a5);
                max2 = _mm256_max_epu8(max2, min2);
                _mm256_stream_si256((batch_t*)(dst + i), max2);
            }
        }
#elif defined(FMO_HAVE_SSE2)
        using batch_t = __m128i;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, 
                  const uint8_t* src4, const uint8_t* src5, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a1 = _mm_load_si128((const batch_t*)(src1 + i));
                batch_t a2 = _mm_load_si128((const batch_t*)(src2 + i));
                batch_t a3 = _mm_load_si128((const batch_t*)(src3 + i));
                batch_t a4 = _mm_load_si128((const batch_t*)(src4 + i));
                batch_t a5 = _mm_load_si128((const batch_t*)(src5 + i));

                batch_t max1 = _mm_max_epu8(a1, a2);
                batch_t min1 = _mm_min_epu8(a1, a2);
                batch_t max2 = _mm_max_epu8(a3, a4);
                batch_t min2 = _mm_min_epu8(a3, a4);
                min1 = _mm_max_epu8(min1,min2);
                max1 = _mm_min_epu8(max1,max2);

                min2 = _mm_max_epu8(min1, max1);
                max2 = _mm_min_epu8(min1, max1);
                min2 = _mm_min_epu8(min2, a5);
                max2 = _mm_max_epu8(max2, min2);
                _mm_stream_si128((batch_t*)(dst + i), max2);
            }
        }
#elif defined(FMO_HAVE_NEON)
        using batch_t = uint8x16_t;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, 
                  const uint8_t* src4, const uint8_t* src5, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i += sizeof(batch_t)) {
                batch_t a1 = vld1q_u8(src1 + i);
                batch_t a2 = vld1q_u8(src2 + i);
                batch_t a3 = vld1q_u8(src3 + i);
                batch_t a4 = vld1q_u8(src4 + i);
                batch_t a5 = vld1q_u8(src5 + i);

                batch_t max1 = vmaxq_u8(a1, a2);
                batch_t min1 = vminq_u8(a1, a2);
                batch_t max2 = vmaxq_u8(a3, a4);
                batch_t min2 = vminq_u8(a3, a4);
                min1 = vmaxq_u8(min1,min2);
                max1 = vminq_u8(max1,max2);

                min2 = vmaxq_u8(min1, max1);
                max2 = vminq_u8(min1, max1);
                min2 = vminq_u8(min2, a5);
                max2 = vmaxq_u8(max2, min2);
                vst1q_u8(dst + i, max2);
            }
        }
#else
        using batch_t = uint8_t;

        void impl(const uint8_t* src1, const uint8_t* src2, const uint8_t* src3, 
                  const uint8_t* src4, const uint8_t* src5, uint8_t* dst,
                  size_t iEnd) const {
            for (size_t i = 0; i < iEnd; i++) {
                uint8_t max1 = std::max(src1[i], src2[i]);
                uint8_t min1 = std::min(src1[i], src2[i]);
                uint8_t max2 = std::max(src3[i], src4[i]);
                uint8_t min2 = std::min(src3[i], src4[i]);
                min1 = std::max(min1,min2);
                max1 = std::min(max1,max2);

                min2 = std::max(min1, max1);
                max2 = std::min(min1, max1);
                min2 = std::min(min2, src5[i]);
                dst[i] = std::max(max2, min2);
            }
        }
#endif

        Median5Job(const Mat& src1, const Mat& src2, const Mat& src3, 
                  const Mat& src4, const Mat& src5, Mat& dst)
            : mSrc1(src1.data()), mSrc2(src2.data()), mSrc3(src3.data()), 
              mSrc4(src4.data()), mSrc5(src5.data()), mDst(dst.data()) {}

        virtual void operator()(const cv::Range& pieces) const override {
            size_t first = size_t(pieces.start) * sizeof(batch_t);
            size_t last = size_t(pieces.end) * sizeof(batch_t);
            const uint8_t* const src1 = mSrc1 + first;
            const uint8_t* const src2 = mSrc2 + first;
            const uint8_t* const src3 = mSrc3 + first;
            const uint8_t* const src4 = mSrc4 + first;
            const uint8_t* const src5 = mSrc5 + first;
            uint8_t* const dst = mDst + first;
            const size_t iEnd = last - first;
            impl(src1, src2, src3, src4, src5, dst, iEnd);
        }

    private:
        const uint8_t* const mSrc1;
        const uint8_t* const mSrc2;
        const uint8_t* const mSrc3;
        const uint8_t* const mSrc4;
        const uint8_t* const mSrc5;
        uint8_t* const mDst;
    };

    void median5(const Image& src1, const Image& src2, const Image& src3, 
                  const Image& src4, const Image& src5, Image& dst) {
        const Format format = src1.format();
        const Dims dims = src1.dims();
        const cv::Size size = getCvSize(format, dims);
        const size_t bytes = size_t(size.width) * size_t(size.height) * getPixelStep(format);
        const size_t pieces = bytes / sizeof(Median5Job::batch_t);

        if (format != src2.format() || dims != src2.dims() 
         || format != src3.format() || dims != src3.dims()
         || format != src4.format() || dims != src4.dims()
         || format != src5.format() || dims != src5.dims()) {
            throw std::runtime_error("median5: format/dimensions mismatch of inputs");
        }

        // run the job in parallel
        dst.resize(format, dims);
        Median5Job job{src1, src2, src3, src4, src5, dst};
        cv::parallel_for_(cv::Range{0, int(pieces)}, job, cv::getNumThreads());

        // process the last few bytes inidividually
        for (size_t i = pieces * sizeof(Median5Job::batch_t); i < bytes; i++) {
            uint8_t t = std::max(src1.data()[i], src2.data()[i]);
            uint8_t s = std::min(src1.data()[i], src2.data()[i]);
            t = std::min(t, src3.data()[i]);
            dst.data()[i] = std::max(s, t);
        }
    }
}
