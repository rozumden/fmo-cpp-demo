#include <algorithm>
#include <chrono>
#include <fmo/stats.hpp>

namespace {
    const int64_t FRAME_STATS_MIN_DELTA = 500000; // 0.5 ms

    int64_t toNs(float hz) { return static_cast<uint64_t>(1e9 / hz); }

    float toHz(int64_t ns) { return static_cast<float>(1e9 / ns); }

    float toMs(int64_t ns) { return static_cast<float>(ns / 1e6); }

    using Clock = std::chrono::high_resolution_clock;

    struct {
        Clock::time_point origin = Clock::now();
    } global;
}

namespace fmo {
    int64_t nanoTime() {
        auto duration = Clock::now() - global.origin;
        return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    }

    Stats::Stats(int sortPeriod, int warmUp)
        : mStorageSize(2 * sortPeriod),
          mSortPeriod(sortPeriod),
          mWarmUpFrames(warmUp),
          mVec(),
          mWarmUpCounter(0),
          mQuantiles(0, 0, 0) {
        mVec.reserve(size_t(mStorageSize));
    }

    void Stats::reset(int64_t defVal) {
        mVec.clear();
        mWarmUpCounter = 0;
        mQuantiles.q50 = mQuantiles.q95 = mQuantiles.q99 = defVal;
    }

    bool Stats::add(int64_t val) {
        if (mWarmUpCounter++ < mWarmUpFrames) return false;
        mVec.push_back(val);
        if (mVec.size() % mSortPeriod != 0) return false;

        auto iter50 = begin(mVec) + ((50 * mVec.size()) / 100);
        auto iter95 = begin(mVec) + ((95 * mVec.size()) / 100);
        auto iter99 = begin(mVec) + ((99 * mVec.size()) / 100);

        std::nth_element(begin(mVec), iter50, end(mVec));
        if (iter50 != iter95) std::nth_element(iter50 + 1, iter95, end(mVec));
        if (iter95 != iter99) std::nth_element(iter95 + 1, iter99, end(mVec));

        mQuantiles.q50 = *iter50;
        mQuantiles.q95 = *iter95;
        mQuantiles.q99 = *iter99;

        if (mVec.size() >= size_t(mStorageSize)) subsample();
        return true;
    }

    void Stats::subsample() {
        auto halfSize = mVec.size() / 2;
        auto iter1 = begin(mVec);
        auto iter2 = iter1 + halfSize;
        auto iterEnd = end(mVec);

        while (iter2 != iterEnd) {
            iter1++;
            iter2++;
            if (iter2 == iterEnd) break;
            *iter1 = *iter2;
            iter1++;
            iter2++;
        }

        mVec.resize(halfSize);
    }

    FrameStats::FrameStats(int sortPeriod, int warmUp)
        : mStats(sortPeriod, warmUp), mLastTimeNs(0), mQuantilesHz(0, 0, 0) {}

    void FrameStats::reset(float defaultHz) {
        mStats.reset(toNs(defaultHz));
        mLastTimeNs = nanoTime();
        updateMyQuantiles();
    }

    bool FrameStats::tick() {
        auto timeNs = nanoTime();
        auto deltaNs = timeNs - mLastTimeNs;
        mLastTimeNs = timeNs;
        mLastTimeDiffNs = deltaNs;
        if (deltaNs < FRAME_STATS_MIN_DELTA) return false;
        bool updated = mStats.add(deltaNs);
        if (updated) updateMyQuantiles();
        return updated;
    }

    void FrameStats::updateMyQuantiles() {
        auto& quantiles = mStats.quantiles();
        mQuantilesHz.q50 = toHz(quantiles.q50);
        mQuantilesHz.q95 = toHz(quantiles.q95);
        mQuantilesHz.q99 = toHz(quantiles.q99);
    }

    SectionStats::SectionStats(int sortPeriod, int warmUp)
        : mStats(sortPeriod, warmUp), mStartTimeNs(0), mQuantilesMs(0, 0, 0) {}

    void SectionStats::reset() {
        mStats.reset(0);
        updateMyQuantiles();
    }

    void SectionStats::start() { mStartTimeNs = nanoTime(); }

    bool SectionStats::stop() {
        auto deltaNs = nanoTime() - mStartTimeNs;
        bool updated = mStats.add(deltaNs);
        if (updated) updateMyQuantiles();
        return updated;
    }

    void SectionStats::updateMyQuantiles() {
        auto& quantiles = mStats.quantiles();
        mQuantilesMs.q50 = toMs(quantiles.q50);
        mQuantilesMs.q95 = toMs(quantiles.q95);
        mQuantilesMs.q99 = toMs(quantiles.q99);
    }
}
