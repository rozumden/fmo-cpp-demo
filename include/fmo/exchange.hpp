#ifndef FMO_EXCHANGE_HPP
#define FMO_EXCHANGE_HPP

#include <algorithm>
#include <condition_variable>
#include <mutex>

namespace fmo {
    /// Exchanges date in a multi-threaded scenario between a periodical producer and one or more
    /// consumers. Payload gets dropped if it's not consumed before next payload arrives. It is
    /// assumed that the payload is cheap to swap.
    template <typename T>
    struct Exchange {
        Exchange(const Exchange&) = delete;
        Exchange& operator=(const Exchange&) = delete;

        /// Create a new exchange. The constructor arguments are forwarded to the constructor of the
        /// payload.
        template <typename... Args>
        Exchange(Args&&... args) : mPayload(std::forward<Args>(args)...) {}

        /// Sends new data to the consumers. Previous payload is discarded if it hasn't been
        /// processed yet. Data is stored by swapping.
        void swapSend(T& payload) {
            std::lock_guard<std::mutex> lock(mMutex);

            using std::swap;
            swap(mPayload, payload);

            mHave = true;
            mWait.notify_all();
        }

        /// Get the most recent payload deposited using swapSend. If there is no new, previously
        /// unreceived payload available, the method will block until there's new data or the exit()
        /// method is called. Data is received by swapping.
        void swapReceive(T& payload) {
            std::unique_lock<std::mutex> lock(mMutex);
            mWait.wait(lock, [this]() { return mHave || mExit; });
            if (mExit) { return; }

            using std::swap;
            swap(mPayload, payload);

            mHave = false;
        }

        /// Set the internal exit flag and wake up all waiting threads.
        void exit() {
            std::lock_guard<std::mutex> lock(mMutex);
            mExit = true;
            mWait.notify_all();
        }

    private:
        T mPayload;
        std::mutex mMutex;
        std::condition_variable mWait;
        bool mHave = false;
        bool mExit = false;
    };
}

#endif // FMO_EXCHANGE_HPP
