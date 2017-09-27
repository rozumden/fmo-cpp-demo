#ifndef FMO_DESKTOP_RECORDER_HPP
#define FMO_DESKTOP_RECORDER_HPP

#include "video.hpp"
#include <atomic>
#include <fmo/exchange.hpp>
#include <fmo/image.hpp>
#include <memory>
#include <thread>

struct RecordingThread {
    RecordingThread(const std::string& dir, fmo::Format format, fmo::Dims dims, float fps);
    ~RecordingThread();
    void swapSend(fmo::Image& input);

private:
    static void threadImpl(RecordingThread* self);

    const fmo::Format mFormat;
    const fmo::Dims mDims;
    std::unique_ptr<VideoOutput> mVideoOutput;
    std::atomic<bool> mStop;
    fmo::Exchange<fmo::Image> mExchange;
    std::thread mThread;
};

struct AutomaticRecorder {
    ~AutomaticRecorder();
    AutomaticRecorder(std::string dir, fmo::Format format, fmo::Dims dims, float fps);
    void frame(const fmo::Mat& input, bool event);
    bool isRecording() const { return bool(mThread); }

private:
    using FrameIterator = std::vector<fmo::Image>::iterator;
    static constexpr int NUM_FRAMES = 60;     ///< number of frames stored
    const std::string mDir;                   ///< directory to save videos to
    const fmo::Format mFormat;                ///< image format
    const fmo::Dims mDims;                    ///< image dimensions
    const float mFps;                         ///< frames per second setting
    std::vector<fmo::Image> mImages;          ///< frame headers
    FrameIterator mHead;                      ///< last written frame
    FrameIterator mStopAt;                    ///< frame to stop recording at
    std::unique_ptr<RecordingThread> mThread; ///< video encoding in a separate thread
    int mFrameNum = 0;                        ///< frame number, incremented in frame()
};

struct ManualRecorder {
    ~ManualRecorder();
    ManualRecorder(std::string dir, fmo::Format format, fmo::Dims dims, float fps);
    void frame(const fmo::Mat& input);

private:
    fmo::Image mImage;
    RecordingThread mThread;
};

#endif // FMO_DESKTOP_RECORDER_HPP
