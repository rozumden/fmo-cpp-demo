#ifndef FMO_DESKTOP_LOOP_HPP
#define FMO_DESKTOP_LOOP_HPP

#include "args.hpp"
#include "calendar.hpp"
#include "evaluator.hpp"
#include "report.hpp"
#include "window.hpp"
#include "video.hpp"
#include <fmo/algorithm.hpp>
#include <fmo/retainer.hpp>
#include <fmo/stats.hpp>

struct Visualizer;
struct AutomaticRecorder;
struct ManualRecorder;

struct Statistics {
public:
    void nextFrame(int nDetections) {
        totalDetections += nDetections;
        nFrames++;
    }

    float getMean() { return (float)totalDetections/nFrames; }
    void print() {std::cout << "Detections: total - " << totalDetections <<
                            ", average - " << getMean() << std::endl;}
    int totalDetections = 0;
    int nFrames = 0;
};
void printStatistics(std::vector<Statistics> &stats);

struct Status {
    Args args;                              ///< user settings
    Window window;                          ///< GUI handle
    Results results;                        ///< evaluation results
    Results baseline;                       ///< previous evaluation results
    Date date;                              ///< date and time at the start of the evaluation
    fmo::Timer timer;                       ///< timer for the whole run
    std::string inputName;                  ///< name of the currently played back input
    std::unique_ptr<Visualizer> visualizer; ///< visualization method
    std::unique_ptr<DetectionReport> rpt;   ///< detection report file writer
    int inFrameNum;                         ///< frame number of input video (first frame = frame 1)
    int outFrameNum;                        ///< frame number of detected objects and GT
    bool paused = false;                    ///< playback paused
    bool quit = false;                      ///< exit application now
    bool reload = false;                    ///< load the same video again
    bool sound = false;                     ///< play sounds
    std::string inputString = "Baseline";

    Status(int argc, char** argv) : args(argc, argv) {}
    bool haveCamera() const { return args.camera != -1; }
    bool haveWait() const { return args.wait != -1; }
    bool haveFrame() const { return args.frame != -1; }
    void unsetFrame() { args.frame = -1; }
};

Statistics processVideo(Status& s, size_t inputNum);

struct Visualizer {
    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) = 0;

    virtual ~Visualizer() {}
};

struct DebugVisualizer : public Visualizer {
    DebugVisualizer(Status& s);

    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) override;

    void process(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
    const EvalResult& evalResult, fmo::Algorithm& algorithm);

    void processKeyboard(Status& s, const fmo::Region& frame);
public:
    fmo::Image mVis;
private:
    int mDetections = 0;
    bool mPauseFirst = false;
    int mPreviousDet = 0;
    int mLevel = 1;
    bool mShowLM = false;
    bool mShowIM = true;
    int mAdd = 0;
    fmo::FrameStats mStats;  
    fmo::Algorithm::Output mOutputCache;
    fmo::Retainer<fmo::PointSet, 6> mObjectPoints;
    fmo::PointSet mPointsCache;
    fmo::PointSet mGtPointsCache;
};

struct DemoVisualizer : public Visualizer {
    DemoVisualizer(Status& s);

    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) override;

    void process(Status& s, const fmo::Region& frame, fmo::Algorithm& algorithm);

    void processKeyboard(Status& s, const fmo::Region& frame);
public:
    fmo::Image mVis;                               ///< cached image buffer
    int mode = 0;
    int mNumberDetections = 0;                     ///< detection counter
    int mOffsetFromMaxDetection = 0;
    std::unique_ptr<ManualRecorder> mManual;       ///< for manual-mode recording
    bool mRecordAnnotations = true;                ///< add annotations for racording?
    std::vector<float> mSpeeds;
    std::vector<std::pair<int, float>> mLastSpeeds;
    float mMaxSpeed = 0;
private:
    static constexpr int EVENT_GAP_FRAMES = 12;
    static constexpr size_t MAX_SEGMENTS = 200;
    static constexpr int MAX_SPEED_FRAMES = 20;
    const int MAX_SPEED_TIME = 60;

    void updateHelp(Status& s);
    void printStatus(Status& s, int fpsEstimate) const;
    void onDetection(const Status& s, const fmo::Algorithm::Detection& detection);
    void drawSegments(fmo::Image& im);

    fmo::FrameStats mStats;                        ///< frame rate estimator
    bool mShowHelp = false;                        ///< show help on the GUI?
    bool mForcedEvent = false;                     ///< force an event in the next frame?
    std::unique_ptr<AutomaticRecorder> mAutomatic; ///< for automatic-mode recording
    fmo::Algorithm::Output mOutput;                ///< cached output object
    std::vector<fmo::Bounds> mSegments;            ///< object path being visualized
    std::vector<fmo::SCurve*> mCurves;             ///< curves being visualized
    int mEventsDetected = 0;                       ///< event counter
    int mMaxDetections = 0;
    int mLastDetectFrame = -EVENT_GAP_FRAMES;      ///< the frame when the last detection happened
};

struct TUTDemoVisualizer : public Visualizer {
    TUTDemoVisualizer(Status& s);

    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) override;

private:
    bool mRecord = false;
    int mPreviousDetections = 0;
    DemoVisualizer vis1;
    std::vector<std::unique_ptr<VideoInput>> input;
    std::vector<cv::Mat> imgs;
    fmo::Image mVis;                // image collage
    fmo::Image mTempDebug;
    fmo::Image mTemp2Debug;
    int mOffsetFromMax = 0;
    fmo::Image mLastDetectedImage;
    fmo::Image mMaxDetectedImage;

    int mLastMode = -1;
};


struct UTIADemoVisualizer : public Visualizer {
    UTIADemoVisualizer(Status& s);

    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) override;
public:
    bool mRecord = false;
    
private:
    int mPreviousDetections = 0;
    DemoVisualizer vis1;
    std::vector<cv::Mat> imgs;
    fmo::Image mVis;                // image collage
    fmo::Image mTempDebug;
    fmo::Image mTemp2Debug;
    int mOffsetFromMax = 0;
    fmo::Image mLastDetectedImage;
    fmo::Image mMaxDetectedImage;

    int mLastMode = -1;
};

struct RemovalVisualizer : public Visualizer {
    RemovalVisualizer(Status& s);

    virtual void visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                           const EvalResult& evalResult, fmo::Algorithm& algorithm) override;

private:
    fmo::FrameStats mStats;                        ///< frame rate estimator
    fmo::Image mCurrent;
    fmo::Image mFrames[5];
    fmo::Algorithm::Output mOutputCache;
    fmo::Retainer<fmo::PointSet, 6> mObjectPoints;
    fmo::PointSet mPointsCache;
    int mEventsDetected = 0;                       ///< event counter
    std::unique_ptr<ManualRecorder> mManual;       ///< for manual-mode recording
};

#endif // FMO_DESKTOP_LOOP_HPP
