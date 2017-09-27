#include "video.hpp"
#include "calendar.hpp"
#include "desktop-opencv.hpp"
#include <algorithm>
#include <fmo/assert.hpp>
#include <iostream>
#include <stdexcept>

// VideoInput

VideoInput::~VideoInput() = default;
VideoInput::VideoInput(VideoInput&& rhs) = default;
VideoInput& VideoInput::operator=(VideoInput&&) = default;

VideoInput::VideoInput(std::unique_ptr<cv::VideoCapture>&& cap)
    : mMat(std::make_unique<cv::Mat>()), mCap(std::move(cap)) {
    if (!mCap->isOpened()) { throw std::runtime_error("failed to open video"); }

#if CV_MAJOR_VERSION == 2
    mFps = (float)mCap->get(CV_CAP_PROP_FPS);
    mDims.width = (int)mCap->get(CV_CAP_PROP_FRAME_WIDTH);
    mDims.height = (int)mCap->get(CV_CAP_PROP_FRAME_HEIGHT);
#elif CV_MAJOR_VERSION == 3
    mFps = (float)mCap->get(cv::CAP_PROP_FPS);
    mDims.width = (int)mCap->get(cv::CAP_PROP_FRAME_WIDTH);
    mDims.height = (int)mCap->get(cv::CAP_PROP_FRAME_HEIGHT);
#endif
}

std::unique_ptr<VideoInput> VideoInput::makeFromCamera(int camId) {
    try {
        return std::make_unique<VideoInput>(std::make_unique<cv::VideoCapture>(camId));
    } catch (std::exception& e) {
        std::cerr << "while opening camera ID " << camId << "\n";
        throw e;
    }
}

std::unique_ptr<VideoInput> VideoInput::makeFromFile(const std::string& filename) {
    try {
        return std::make_unique<VideoInput>(std::make_unique<cv::VideoCapture>(filename));
    } catch (std::exception& e) {
        std::cerr << "while opening file '" << filename << "'\n";
        throw e;
    }
}

fmo::Region VideoInput::receiveFrame() {
    *mCap >> *mMat;

    if (mMat->empty()) { return {}; }

    FMO_ASSERT(mMat->type() == CV_8UC3, "bad type");
    FMO_ASSERT(mMat->cols == mDims.width, "bad width");
    FMO_ASSERT(mMat->rows == mDims.height, "bad height");

    auto rowStep = size_t(3 * mDims.width);
    return {fmo::Format::BGR, {0, 0}, mDims, mMat->data, nullptr, rowStep};
}

// VideoOutput

VideoOutput::~VideoOutput() = default;
VideoOutput::VideoOutput(VideoOutput&& rhs) = default;
VideoOutput& VideoOutput::operator=(VideoOutput&&) = default;

VideoOutput::VideoOutput(std::unique_ptr<cv::VideoWriter>&& writer, fmo::Dims dims)
    : mWriter(std::move(writer)), mDims(dims) {
    if (!mWriter->isOpened()) { throw std::runtime_error("failed to open file for recording"); }
}

std::unique_ptr<VideoOutput> VideoOutput::makeFile(const std::string& filename, fmo::Dims dims,
                                                   float fps) {
    int fourCC = CV_FOURCC('D', 'I', 'V', 'X');
    fps = std::max(15.f, fps);
    cv::Size size = {dims.width, dims.height};

    try {
        return std::make_unique<VideoOutput>(
            std::make_unique<cv::VideoWriter>(filename, fourCC, fps, size, true), dims);
    } catch (std::exception& e) {
        std::cerr << "while opening file '" << filename << "'\n";
        throw e;
    }
}

std::unique_ptr<VideoOutput> VideoOutput::makeInDirectory(const std::string& dir, fmo::Dims dims,
                                                          float fps) {
    std::string file = dir + '/' + Date{}.fileNameSafeStamp() + ".avi";
    return makeFile(file, dims, fps);
}

void VideoOutput::sendFrame(const fmo::Mat& frame) {
    FMO_ASSERT(frame.format() == fmo::Format::BGR, "bad format");

    // resize if the dimensions of frame do not match the dimensions of the video
    cv::Mat mat;
    if (frame.dims() != mDims) {
        cv::resize(frame.wrap(), mat, {mDims.width, mDims.height});
    } else {
        mat = frame.wrap();
    }

    FMO_ASSERT((mat.flags & cv::Mat::CONTINUOUS_FLAG) != 0, "bad flags");

    *mWriter << mat;
}
