#include "recorder.hpp"
#include <cstdint>
#include <fmo/assert.hpp>
#include <fmo/processing.hpp>
#include <fmo/region.hpp>

// RecordingThread

RecordingThread::RecordingThread(const std::string& dir, fmo::Format format, fmo::Dims dims,
                                 float fps)
    : mFormat(format),
      mDims(dims),
      mVideoOutput(VideoOutput::makeInDirectory(dir, dims, fps)),
      mStop(false),
      mExchange(format, dims),
      mThread(threadImpl, this) {}

RecordingThread::~RecordingThread() {
    mStop = true;
    mExchange.exit();
    mThread.join();
}

void RecordingThread::threadImpl(RecordingThread* self) {
    fmo::Image input{self->mFormat, self->mDims};

    while (!self->mStop) {
        self->mExchange.swapReceive(input);
        if (self->mStop) return;
        self->mVideoOutput->sendFrame(input);
    }
}

void RecordingThread::swapSend(fmo::Image& input) { mExchange.swapSend(input); }

// AutomaticRecorder

AutomaticRecorder::AutomaticRecorder(std::string dir, fmo::Format format, fmo::Dims dims, float fps)
    : mDir(std::move(dir)), mFormat(format), mDims(dims), mFps(fps) {
    if (format == fmo::Format::YUV420SP) {
        throw std::runtime_error("Recorder: YUV420SP not supported");
    }

    for (int i = 0; i < NUM_FRAMES; i++) { mImages.emplace_back(mFormat, mDims); }
    mHead = begin(mImages);
}

void AutomaticRecorder::frame(const fmo::Mat& input, bool event) {
    mFrameNum++;

    // stop recording if at the mark
    if (mThread && mStopAt == mHead) { mThread.reset(nullptr); }

    // start or extend recording if there was an event
    if (event) {
        if (!mThread) { mThread = std::make_unique<RecordingThread>(mDir, mFormat, mDims, mFps); }
        mStopAt = mHead;
    }

    // advance head
    mHead++;
    if (mHead == end(mImages)) { mHead = begin(mImages); }

    // write the oldest frame to file if recording
    if (mThread && mFrameNum > NUM_FRAMES) { mThread->swapSend(*mHead); }

    // rewrite the oldest frame with the input frame
    fmo::copy(input, *mHead);
}

AutomaticRecorder::~AutomaticRecorder() {
    // duplicate the newest frame until recording ends
    while (mThread) { frame(*mHead, false); }
}

// ManualRecorder

ManualRecorder::~ManualRecorder() = default;

ManualRecorder::ManualRecorder(std::string dir, fmo::Format format, fmo::Dims dims, float fps)
    : mImage{format, dims}, mThread{dir, format, dims, fps} {}

void ManualRecorder::frame(const fmo::Mat& input) {
    fmo::copy(input, mImage);
    mThread.swapSend(mImage);
}
