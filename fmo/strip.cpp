#include "include-opencv.hpp"
#include <algorithm>
#include <cstdint>
#include <fmo/assert.hpp>
#include <fmo/common.hpp>
#include <fmo/strip.hpp>
#include <mutex>
#include <vector>

namespace fmo {
    struct StripGenImpl : public cv::ParallelLoopBody {
        using batch_t = uint64_t;
        using rle_t = int16_t;

        enum {
            WIDTH = sizeof(batch_t),
        };

        StripGenImpl(const fmo::Mat& img, int minHeight, int minGap, int step,
                     std::vector<rle_t>& rle, std::vector<Strip>& temp, std::vector<Strip>& out,
                     int& noiseOut, int numThreads)
            : mDims(img.dims()),
              mRleStep(mDims.height + 4),
              mRleSz(mRleStep * WIDTH),
              mTempSz((mRleSz + 1) / 2),
              mSkip(int(img.skip()) / WIDTH),
              mStep(step),
              mMinHeight(minHeight),
              mMinGap(minGap),
              mData((const batch_t*)(img.data())),
              mRle(&rle),
              mTemp(&temp),
              mOut(&out),
              mNoiseOut(&noiseOut),
              mNumThreads(numThreads) {
            FMO_ASSERT(int(img.skip()) % WIDTH == 0, "StripGen::operator(): bad skip");
            mRle->resize(mRleSz * mNumThreads);
            mTemp->resize(mTempSz * mNumThreads);
            mOut->clear();
            *mNoiseOut = 0;
        }

        virtual void operator()(const cv::Range& r) const override {
            const int threadNum = r.start;
            rle_t* const rle = mRle->data() + (mRleSz * threadNum);
            Strip* const temp = mTemp->data() + (mTempSz * threadNum);
            const int16_t step = int16_t(mStep);
            const int16_t halfStep = int16_t(mStep / 2);
            const int pad = std::max(0, std::max(mMinHeight, mMinGap));
            const int numBatches = mDims.width / WIDTH;
            const int batchFirst = (threadNum * numBatches) / mNumThreads;
            const int batchLast = ((threadNum + 1) * numBatches) / mNumThreads;
            const int colFirst = batchFirst * WIDTH;
            const int colLast = batchLast * WIDTH;
            const int skip = mSkip;
            const int minHeight = mMinHeight;
            const Dims dims = mDims;
            const int minGap = mMinGap;
            Strip* tempEnd = temp;

            int16_t origX = int16_t(halfStep + (colFirst * step));
            int noise = 0;
            rle_t* front[WIDTH];
            const batch_t* colData = mData + batchFirst;

            for (int w = 0; w < WIDTH; w++) { front[w] = rle + (w * mRleStep); }

            for (int col = colFirst; col < colLast; col += WIDTH, colData++) {
                const batch_t* data = colData;
                rle_t* back[WIDTH];
                int n[WIDTH];

                for (int w = 0; w < WIDTH; w++) {
                    back[w] = front[w];

                    // add top of image
                    *back[w] = rle_t(-pad);
                    n[w] = 1;
                }

                // must start with a black segment
                if (*data != 0) {
                    for (int w = 0; w < WIDTH; w++) {
                        if (((const uint8_t*)(data))[w] != 0) {
                            *++(back[w]) = rle_t(0);
                            n[w]++;
                        }
                    }
                }
                data += skip;

                // store indices of changes
                for (int row = 1; row < dims.height; row++, data += skip) {
                    const batch_t* prev = data - skip;
                    if (*data != *prev) {
                        for (int w = 0; w < WIDTH; w++) {
                            if (((const uint8_t*)(data))[w] != ((const uint8_t*)(prev))[w]) {
                                if ((row - *(back[w])) < minHeight) {
                                    // remove noise
                                    back[w]--;
                                    n[w]--;
                                    noise++;
                                } else {
                                    *++(back[w]) = rle_t(row);
                                    n[w]++;
                                }
                            }
                        }
                    }
                }

                for (int w = 0; w < WIDTH; w++, origX += int16_t(step)) {
                    // must end with a black segment
                    if ((n[w] & 1) == 0) {
                        *++(back[w]) = rle_t(dims.height);
                        n[w]++;
                    }

                    // add bottom of image
                    *++(back[w]) = rle_t(dims.height + pad);
                    n[w]++;

                    // report white segments as strips if all conditions are met
                    rle_t* lastWhite = back[w] - 1;
                    for (rle_t* i = front[w]; i < lastWhite; i += 2) {
                        if (*(i + 1) - *(i + 0) >= minGap && *(i + 3) - *(i + 2) >= minGap) {
                            int halfHeight = (*(i + 2) - *(i + 1)) * halfStep;
                            int origY = (*(i + 2) + *(i + 1)) * halfStep;
                            tempEnd->pos = {int16_t(origX), int16_t(origY)};
                            tempEnd->halfDims = {int16_t(halfStep), int16_t(halfHeight)};
                            tempEnd++;
                        }
                    }
                }

                // move data outside
                {
                    std::lock_guard<std::mutex> lock(mMutex);

                    mOut->insert(mOut->end(), temp, tempEnd);
                    tempEnd = temp;

                    *mNoiseOut += noise;
                    noise = 0;
                }
            }
        }

    private:
        const Dims mDims;
        const int mRleStep;
        const int mRleSz;
        const int mTempSz;
        const int mSkip;
        const int mStep;
        const int mMinHeight;
        const int mMinGap;
        const batch_t* const mData;
        std::vector<rle_t>* const mRle;
        std::vector<Strip>* const mTemp;
        std::vector<Strip>* const mOut;
        int* const mNoiseOut;
        const int mNumThreads;
        mutable std::mutex mMutex;
    };

    void StripGen::operator()(const fmo::Mat& img, int minHeight, int minGap, int step,
                              std::vector<Strip>& out, int& outNoise) {
        int numThreads = cv::getNumThreads();
        StripGenImpl job{img, minHeight, minGap, step, mRle, mTemp, out, outNoise, numThreads};
        cv::parallel_for_(cv::Range{0, numThreads}, job);
    }
}
