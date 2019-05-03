#ifndef FMO_DESKTOP_VIDEO_HPP
#define FMO_DESKTOP_VIDEO_HPP

#include <fmo/common.hpp>
#include <fmo/region.hpp>
#include <memory>
#include <string>
#include "desktop-opencv.hpp"

namespace cv {
    class Mat;
    class VideoCapture;
    class VideoWriter;
}

/// For reading image data from a video file or a camera device.
struct VideoInput {
    ~VideoInput();
    VideoInput() = delete;
    VideoInput(const VideoInput&) = delete;
    VideoInput& operator=(const VideoInput&) = delete;
    VideoInput(VideoInput&&);
    VideoInput& operator=(VideoInput&&);
    VideoInput(std::unique_ptr<cv::VideoCapture>&& cap);

    /// Opens a camera device for reading. Throws on error.
    static std::unique_ptr<VideoInput> makeFromCamera(int camId);

    /// Opens a video file for reading. Throws on error.
    static std::unique_ptr<VideoInput> makeFromFile(const std::string& filename);

    /// Provides the next frame from the file or device. May block until the next frame is
    /// available. If there are no more frames, the returned region will point to nullptr.
    fmo::Region receiveFrame();

    void restart() {
        mCap->set(CV_CAP_PROP_POS_AVI_RATIO,0);
    }

    void set_fps(float fpsVal) {
        mCap->set(CV_CAP_PROP_FPS, fpsVal); 
        mFps = (float)mCap->get(CV_CAP_PROP_FPS);;
    }

    double get_exposure() {
        return mCap->get(CV_CAP_PROP_EXPOSURE);
    }

    void set_exposure(float expVal) {
        mCap->set(CV_CAP_PROP_AUTO_EXPOSURE, 0.25); 
        mCap->set(CV_CAP_PROP_EXPOSURE, expVal); 
    }

    void default_camera() {
        mCap->set(CV_CAP_PROP_AUTO_EXPOSURE, 0.75); 
        mCap->set(CV_CAP_PROP_FPS, 30); 
    }

    fmo::Dims dims() const { return mDims; }
    float fps() const { return mFps; }

private:
    // data
    std::unique_ptr<cv::Mat> mMat;
    std::unique_ptr<cv::VideoCapture> mCap;
    fmo::Dims mDims;
    float mFps;
};

/// For writing data into a video file.
struct VideoOutput {
    ~VideoOutput();
    VideoOutput() = delete;
    VideoOutput(const VideoOutput&) = delete;
    VideoOutput& operator=(const VideoOutput&) = delete;
    VideoOutput(VideoOutput&&);
    VideoOutput& operator=(VideoOutput&&);
    VideoOutput(std::unique_ptr<cv::VideoWriter>&& writer, fmo::Dims dims);

    /// Creates a new video file. Throws on error.
    static std::unique_ptr<VideoOutput> makeFile(const std::string& filename, fmo::Dims dims,
                                                 float fps);

    /// Creates a new file in the specified directory. The directory must exist. The file name is
    /// generated based on current system time with second precision. Throws on error.
    static std::unique_ptr<VideoOutput> makeInDirectory(const std::string& dir, fmo::Dims dims,
                                                        float fps);

    /// Saves a frame to the video file.
    void sendFrame(const fmo::Mat& frame);

private:
    // data
    std::unique_ptr<cv::VideoWriter> mWriter;
    fmo::Dims mDims;
};

#endif // FMO_DESKTOP_VIDEO_HPP
