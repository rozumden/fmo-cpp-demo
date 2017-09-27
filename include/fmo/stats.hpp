#ifndef FMO_STATS_HPP
#define FMO_STATS_HPP

#include <cstdint>
#include <vector>

namespace fmo {
    /**
     * Provides current time in the form of the number of nanoseconds relative to a specific a point
     * in time (the origin). The origin does not change during the execution of the program, which
     * makes this function viable for execution time measurements. A system clock with the best
     * precision is used. Monotonicity of the returned times is not guaranteed.
     */
    int64_t nanoTime();

    /**
     * Lists all supported units for display.
     */
    enum class TimeUnit { NS, MS, SEC, HZ };

    /**
     * Allows to convert nanoseconds to another time unit. The conversion involves a double
     * division, therefore beware of a performance hit if used too often.
     */
    template <TimeUnit TU, typename T>
    struct NanoCast;
    template <typename T>
    struct NanoCast<TimeUnit::NS, T> {
        T operator()(int64_t ns) { return static_cast<T>(ns); }
    };
    template <typename T>
    struct NanoCast<TimeUnit::MS, T> {
        T operator()(int64_t ns) { return static_cast<T>(ns / 1e6); }
    };
    template <typename T>
    struct NanoCast<TimeUnit::SEC, T> {
        T operator()(int64_t ns) { return static_cast<T>(ns / 1e9); }
    };
    template <typename T>
    struct NanoCast<TimeUnit::HZ, T> {
        T operator()(int64_t ns) { return static_cast<T>(1e9 / ns); }
    };

    /**
     * Allows to measure time in seconds.
     */
    struct Timer {
        Timer() : mTicNs(nanoTime()) {}

        /**
         * Call at the start of the measured segment.
         */
        void tic() { mTicNs = nanoTime(); }

        /**
         * Call at the end of the measured segment. Returns measured time since the last call to
         * tic() or the construction, whichever happened most recently.
         */
        template <TimeUnit TU, typename T>
        T toc() {
            int64_t delta = nanoTime() - mTicNs;
            return NanoCast<TU, T>{}(delta);
        }

    private:
        int64_t mTicNs;
    };

    /**
     * Statistic measurements of a random variable, consisting of the 50% quantile (the median),
     * the 95% quantile, and the 99% quantile.
     */
    template <typename T>
    struct Quantiles {
        Quantiles(T in50, T in95, T in99) : q50(in50), q95(in95), q99(in99) {}
        T q50, q95, q99;
    };

    /**
     * Provides robust statistic measurements of fuzzy quantities, such as execution time. The data
     * type of the measurements is fixed to 64-bit signed integer. Use the add() method to add new
     * samples. The add() method may trigger calculation of quantiles, which are afterwards
     * retrievable using the quantiles() method.
     */
    struct Stats {
        static constexpr int DEFAULT_SORT_PERIOD = 1000;
        static constexpr int DEFAULT_WARM_UP = 10;

        Stats(const Stats&) = delete;

        Stats& operator=(const Stats&) = delete;

        /**
         * @param sortPeriod The calculation of quantiles is triggered periodically, after
         * sortPeriod samples are added. A maximum of 2 * sortPeriod samples will be retained over
         * time.
         * @param warmUpFrames The number of initial samples that will be ignored.
         */
        Stats(int sortPeriod = DEFAULT_SORT_PERIOD, int warmUp = DEFAULT_WARM_UP);

        /**
         * Restores the object to the initial state. All samples will be removed.
         *
         * @param defVal The value to set all quantiles to.
         */
        void reset(int64_t defVal);

        /**
         * Inserts a sample to an internal vector. Each time sortPeriod (see constructor) samples
         * are added, new quantiles are calculated. This can be detected using the return value of
         * this method.
         *
         * @return True if the quantiles have just been updated. Use the quantiles() method to
         * retrieve them.
         */
        bool add(int64_t val);

        /**
         * @return The statistic measurements (quantiles) as previously calculated by the add()
         * method, or specified using the reset() method, whichever happened last.
         */
        const Quantiles<int64_t>& quantiles() const { return mQuantiles; }

    private:
        /**
         * Discard half of the elements in the underlying sorted or partially sorted vector so that
         * the statistics are not affected.
         */
        void subsample();

        const int mStorageSize;
        const int mSortPeriod;
        const int mWarmUpFrames;
        std::vector<int64_t> mVec;
        int mWarmUpCounter;
        Quantiles<int64_t> mQuantiles;
    };

    /**
     * A class that measures frame time and robustly estimates the frame rate in Hertz. Use the
     * tick() method to perform the measurements.
     */
    struct FrameStats {
        FrameStats(int sortPeriod = Stats::DEFAULT_SORT_PERIOD,
                   int warmUp = Stats::DEFAULT_WARM_UP);

        /**
         * Removes all previously measured values and resets all the quantiles to a given value.
         */
        void reset(float defaultHz);

        /**
         * Performs the frame time measurement. Call this method once per frame. Check the return
         * value to detect whether the quantiles have been updated during the call to this method.
         *
         * @return True if the quantiles have just been updated. Retrieve the quantiles using the
         * quantilesHz() method.
         */
        bool tick();

        /**
         * @return Quantiles, calculated previously by the tick() method, or specified by the
         * reset() method, whichever happened last.
         */
        const Quantiles<float>& quantilesHz() const { return mQuantilesHz; }

        /**
         * @return last fps, calculated previously by the tick() method, or specified by the
         * reset() method, whichever happened last.
         */
        float getLastFps() const { return (1 / (mLastTimeDiffNs*1e-9)); }

    private:
        void updateMyQuantiles();

        Stats mStats;
        int64_t mLastTimeNs;
        int64_t mLastTimeDiffNs;
        Quantiles<float> mQuantilesHz;
    };

    /**
     * A class that robustly estimates execution time of a repeatedly performed code section. Use
     * the start() and stop() methods to mark the beginning and the end of the evaluated code.
     */
    struct SectionStats {
        SectionStats(int sortPeriod = Stats::DEFAULT_SORT_PERIOD,
                     int warmUp = Stats::DEFAULT_WARM_UP);

        /**
         * Removes all previously measured values and resets all the quantiles to zero.
         */
        void reset();

        /**
         * To be called just before the measured section of code starts.
         */
        void start();

        /**
         * To be called as soon as the measured section of code ends. Check the return value to
         * detect whether the quantiles have been updated during the call to this method.
         *
         * @return True if the quantiles have just been updated. Retrieve the quantiles using the
         * quantilesMs() method.
         */
        bool stop();

        /**
         * @return Quantiles, calculated previously by the stop() method, or specified by the
         * reset() method, whichever happened last.
         */
        const Quantiles<float>& quantilesMs() const { return mQuantilesMs; }

    private:
        void updateMyQuantiles();

        Stats mStats;
        int64_t mStartTimeNs;
        Quantiles<float> mQuantilesMs;
    };
}

#endif // FMO_STATS_HPP
