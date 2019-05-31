#include "desktop-opencv.hpp"
#include "loop.hpp"
#include "recorder.hpp"
#include <algorithm>
#include <fmo/processing.hpp>
#include <fmo/region.hpp>
#include <iostream>
#include <boost/range/adaptor/reversed.hpp>

DebugVisualizer::DebugVisualizer(Status& s) : mStats(60) {
    mStats.reset(15.f);
//    s.window.setBottomLine("[esc] quit | [space] pause | [enter] step | [,][.] jump 10 frames");
}

void DebugVisualizer::process(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                                   const EvalResult& evalResult, fmo::Algorithm& algorithm) {
    mStats.tick();
    // float fpsLast = mStats.getLastFps();

    // draw the debug image provided by the algorithm
    fmo::copy(algorithm.getDebugImage(mLevel, mShowIM, mShowLM, mAdd), mVis);
    // s.window.print(s.inputName);
    // s.window.print("frame: " + std::to_string(s.inFrameNum));
    // s.window.print("fps: " + std::to_string(fpsLast));

    // get pixel coordinates of detected objects
    algorithm.getOutput(mOutputCache, false);
    mObjectPoints.clear();
    for (auto& detection : mOutputCache.detections) {
        mObjectPoints.emplace_back();
        detection->getPoints(mObjectPoints.back());
    }
    mDetections = mOutputCache.detections.size();

    // draw detected points vs. ground truth
    if (evaluator != nullptr) {
        s.window.print(evalResult.str());
        auto& gt = evaluator->gt().get(s.outFrameNum);
        fmo::pointSetMerge(begin(mObjectPoints), end(mObjectPoints), mPointsCache);
        fmo::pointSetMerge(begin(gt), end(gt), mGtPointsCache);
        drawPointsGt(mPointsCache, mGtPointsCache, mVis);
        s.window.setTextColor(good(evalResult.eval) ? Colour::green() : Colour::red());
    } else {
        drawPoints(mPointsCache, mVis, Colour::lightMagenta());
    }
}

void DebugVisualizer::processKeyboard(Status& s, const fmo::Region& frame) {
    // process keyboard input
    bool step = false;
    do {
        auto command = s.window.getCommand(s.paused);
        if (command == Command::PAUSE) s.paused = !s.paused;
        if (command == Command::PAUSE_FIRST) mPauseFirst = !mPauseFirst;
        if (mPauseFirst && mDetections > 0 && mPreviousDet == mDetections) {
            s.paused = !s.paused;
            mPreviousDet = 0;
            mDetections = 0;
        }
        if (command == Command::STEP) step = true;
        if (command == Command::QUIT) s.quit = true;
        if (command == Command::SCREENSHOT) fmo::save(mVis, "screenshot.png");
        if (command == Command::LEVEL0) mLevel = 0;
        if (command == Command::LEVEL1) mLevel = 1;
        if (command == Command::LEVEL2) mLevel = 2;
        if (command == Command::LEVEL3) mLevel = 3;
        if (command == Command::LEVEL4) mLevel = 4;
        if (command == Command::LEVEL5) mLevel = 5;
        if (command == Command::SHOW_IM) mShowIM = !mShowIM;
        if (command == Command::LOCAL_MAXIMA) mShowLM = !mShowLM;
        if (command == Command::SHOW_NONE) mAdd = 0;
        if (command == Command::DIFF) mAdd = 1;
        if (command == Command::BIN_DIFF) mAdd = 2;
        if (command == Command::DIST_TRAN) mAdd = 3;

        if (!s.haveCamera()) {
            if (command == Command::JUMP_BACKWARD) {
                s.paused = false;
                s.args.frame = std::max(1, s.inFrameNum - 10);
                s.reload = true;
            }
            if (command == Command::JUMP_FORWARD) {
                s.paused = false;
                s.args.frame = s.inFrameNum + 10;
            }
        }
    } while (s.paused && !step && !s.quit);
    mPreviousDet = mDetections;
}

void DebugVisualizer::visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                                const EvalResult& evalResult, fmo::Algorithm& algorithm) {
    this->process(s,frame,evaluator,evalResult,algorithm);
    // display
    s.window.display(mVis);
    this->processKeyboard(s,frame);
}

// UTIA Demo
UTIADemoVisualizer::UTIADemoVisualizer(Status &s) : vis1(s) {
    s.window.setTopLine("Fast Moving Objects Detection");
    this->vis1.mode = 2;
}


void UTIADemoVisualizer::visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                                  const EvalResult& evalResult, fmo::Algorithm& algorithm) {
    this->vis1.process(s,frame,algorithm);
    if(mLastDetectedImage.data() == nullptr) fmo::copy(frame,mLastDetectedImage);
    if(mMaxDetectedImage.data() == nullptr) fmo::copy(frame,mMaxDetectedImage);
    if (this->mPreviousDetections*this->vis1.mNumberDetections > 0) {
        fmo::copy(this->vis1.mVis, mLastDetectedImage);
    }
    if(this->vis1.mOffsetFromMaxDetection == 0) {
        fmo::copy(this->vis1.mVis, mMaxDetectedImage);
        this->mOffsetFromMax = 0;
    } else this->mOffsetFromMax++;


    if(this->vis1.mode == 2) 
        s.window.visTable = true;
    else 
        s.window.visTable = false;
    
    // put best players
    auto &playerName = s.inputString;
    int stringSize = 10;
    if((int)playerName.size() < stringSize) 
        playerName = playerName + std::string(stringSize-playerName.size(), ' ');
    if((int)playerName.size() > stringSize) 
        playerName = playerName.substr(0,stringSize);

    // if (vis1.mMaxSpeed > 0) {
    //     float spnow = std::round(vis1.mMaxSpeed*100)/100;

    //     bool putitthere = true;
    //     for(auto &el : s.window.mTable) {
    //         if(std::abs(el.first - spnow) < 0.001 && el.second.compare(playerName) == 0) {
    //             putitthere = false;
    //             break;
    //         }
    //     }

    //     if(putitthere) {
    //         if(s.window.mTable.size() < 10) {
    //             s.window.mTable.push_back(std::make_pair(spnow, playerName));
    //         } else {
    //             auto &lastel = s.window.mTable[s.window.mTable.size()-1];
    //             if(spnow > lastel.first) {
    //                 lastel = std::make_pair(spnow, playerName);
    //             }
    //         }
    //     }
    // }
    // std::sort(s.window.mTable.begin(), s.window.mTable.end(), std::greater <>());

    // if(s.window.visTable) {
    //     std::string pref;
    //     for (int ik = 0; ik < (int)s.window.mTable.size(); ++ik) {
    //         auto &el = s.window.mTable[ik];
    //         if(ik < 9)
    //            pref = " ";
    //         else
    //            pref = "";

    //         s.window.print(pref+std::to_string(ik+1)+". "+el.second+": "+std::to_string(el.first).substr(0,4) + " km/h");
    //     }
    // }
    s.window.display(this->vis1.mVis);

    this->vis1.processKeyboard(s,frame);
    this->mPreviousDetections = this->vis1.mNumberDetections;
    mLastMode = this->vis1.mode;
}



// TUT Demo

TUTDemoVisualizer::TUTDemoVisualizer(Status &s) : vis1(s) {
    try {
        input.emplace_back(VideoInput::makeFromFile("../data/webcam/circle_back_res.avi"));
        input.emplace_back(VideoInput::makeFromFile("../data/webcam/counting_all_res.avi"));
        input.emplace_back(VideoInput::makeFromFile("../data/webcam/floorball_res.avi"));
    } catch(const std::exception& e) {
        input.clear();
        try {
            input.emplace_back(VideoInput::makeFromFile("data/webcam/circle_back_res.avi"));
            input.emplace_back(VideoInput::makeFromFile("data/webcam/counting_all_res.avi"));
            input.emplace_back(VideoInput::makeFromFile("data/webcam/floorball_res.avi"));
        } catch(const std::exception& e) {

        }
    }
    s.window.setTopLine("Fast Moving Objects Detection");
    this->mRecord = !s.args.noRecord;
    if(this->mRecord)
        this->vis1.mRecordAnnotations = false;
//    input.emplace_back(VideoInput::makeFromCamera(0));
}

void TUTDemoVisualizer::visualize(Status& s, const fmo::Region& frame, const Evaluator* evaluator,
                                  const EvalResult& evalResult, fmo::Algorithm& algorithm) {
    if (!this->vis1.mManual && this->mRecord)
        this->vis1.mManual = std::make_unique<ManualRecorder>(s.args.recordDir, frame.format(),
                                                              frame.dims(), 30);
    this->vis1.process(s,frame,algorithm);
    if(mLastDetectedImage.data() == nullptr) fmo::copy(frame,mLastDetectedImage);
    if(mMaxDetectedImage.data() == nullptr) fmo::copy(frame,mMaxDetectedImage);
    if (this->mPreviousDetections*this->vis1.mNumberDetections > 0) {
        fmo::copy(this->vis1.mVis, mLastDetectedImage);
    }
    if(this->vis1.mOffsetFromMaxDetection == 0) {
        fmo::copy(this->vis1.mVis, mMaxDetectedImage);
        this->mOffsetFromMax = 0;
    } else this->mOffsetFromMax++;

    if(this->vis1.mode == 0) {
        mVis.resize(fmo::Format::BGR,fmo::Dims{2*frame.wrap().cols,2*frame.wrap().rows});
        cv::Mat out = mVis.wrap();
        imgs.clear();
        fmo::Region nextframe;
        int cntr = 0;
        for (auto &in : input) {
            nextframe = in->receiveFrame();
            if (nextframe.data() == nullptr) {
                in->restart();
                nextframe = in->receiveFrame();
            }
            imgs.push_back(nextframe.wrap());
            cntr++;
        }
        for (int i = cntr; i < 4; ++i) {
            imgs.push_back(this->vis1.mVis.wrap());
        }
        fmo::imgridfull(imgs, out, 2, 2);
        s.window.display(mVis);
    } else if(this->vis1.mode == 1) {
        s.window.display(this->vis1.mVis);
    } else if(this->vis1.mode == 2) {
        mVis.resize(fmo::Format::BGR,fmo::Dims{2*frame.wrap().cols,2*frame.wrap().rows});
        cv::Mat out = mVis.wrap();
        imgs.clear();
        imgs.push_back(frame.wrap());

        fmo::copy(algorithm.getDebugImage(1, true, false, 1), mTempDebug);
        cv::Mat temp = mTempDebug.wrap();
        cv::resize(temp,temp,frame.wrap().size.operator()());
        imgs.push_back(temp);

        fmo::copy(algorithm.getDebugImage(1, true, true, 3), mTemp2Debug);
        cv::Mat temp2 = mTemp2Debug.wrap();
        cv::resize(temp2,temp2,frame.wrap().size.operator()());
        imgs.push_back(temp2);

        imgs.push_back(this->vis1.mVis.wrap());
        fmo::imgridfull(imgs, out, 2, 2);
        s.window.display(mVis);
    } else if(this->vis1.mode == 3) {
        if (mLastMode != this->vis1.mode) {
            fmo::copy(frame, mVis);
        } else
            fmo::copy(mLastDetectedImage,mVis);
        cv::Mat out = mVis.wrap();
        fmo::putcorner(frame.wrap(), out);
        s.window.display(mVis);
    } else if(this->vis1.mode == 4) {
        if(mLastMode != this->vis1.mode)
            fmo::copy(frame,mVis);
        else
            fmo::copy(mMaxDetectedImage,mVis);
        cv::Mat out = mVis.wrap();
        fmo::putcorner(frame.wrap(), out);
        s.window.display(mVis);
    }
    this->vis1.processKeyboard(s,frame);
    this->mPreviousDetections = this->vis1.mNumberDetections;
    mLastMode = this->vis1.mode;
}

// DEMO

DemoVisualizer::DemoVisualizer(Status& s) : mStats(60) {
    mStats.reset(15.f);
    updateHelp(s);
}

void DemoVisualizer::updateHelp(Status& s) {
    if (!mShowHelp) {
        s.window.setBottomLine("");
    } else {
        std::ostringstream oss;
        oss << "[esc] quit";

        if (mAutomatic) {
            oss << " | [m] manual mode | [e] forced event";
        } else {
            oss << " | [a] automatic mode | [r] start/stop recording";
        }

        if (s.sound) {
            oss << " | [s] disable sound";
        } else {
            oss << " | [s] enable sound";
        }

        s.window.setBottomLine(oss.str());
    }
}

void DemoVisualizer::printStatus(Status& s, int fpsEstimate) const {
    bool recording;
    if (mAutomatic) {
//        s.window.print("automatic mode");
        recording = mAutomatic->isRecording();
    } else {
//        s.window.print("manual mode");
        recording = bool(mManual);
    }

    bool kmh = true;
    std::string meas = kmh ? " km/h" : " mph";
    float fctr = kmh ? 1 : 0.621371;

    // s.window.print(recording ? "recording" : "not recording");
    s.window.print("Detections: " + std::to_string(mMaxDetections));
    for (unsigned int i = 0; i < mSpeeds.size(); ++i) {
        s.window.print("Speed " + std::to_string(i+1) + " : " + std::to_string(std::round(mSpeeds[i]*fctr * 100)/100).substr(0,4) + meas);
    }
    Colour clr = Colour::lightRed();
    s.window.print("Max speed: " + std::to_string(std::round(mMaxSpeed*fctr * 100)/100).substr(0,4) + meas, clr);

    // s.window.print("fps: " + std::to_string(fpsEstimate));
//    s.window.print("[?] for help");

    s.window.setTextColor(recording ? Colour::lightRed() : Colour::lightGray());
}

void DemoVisualizer::onDetection(const Status& s, const fmo::Algorithm::Detection& detection) {
    // register a new event after a time without detections
    if (s.outFrameNum - mLastDetectFrame > EVENT_GAP_FRAMES) {
        if (s.sound) {
            // make some noise
            std::cout << char(7);
        }
        mEventsDetected++;
        mSegments.clear();
        for(auto& c : mCurves) {
            delete c;
        }
        mCurves.clear();
    }
    mLastDetectFrame = s.outFrameNum;

    // don't add a segment if there is no previous center
    if (detection.predecessor.haveCenter()) {  
        fmo::Bounds segment = {detection.predecessor.center, detection.object.center};
        mSegments.push_back(segment);
        // std::cout << "-----------------------------\n";
    } else if (detection.object.curve != nullptr) {
        fmo::SCurve *c = detection.object.curve->clone();
        c->scale = detection.object.scale;
        mCurves.push_back(c);
        float radiusCm = s.args.radius; // floorball = 3.6; tennis = 3.27
        float sp = 0;
        float fpsReal = 29.97;
        if(s.args.fps != -1) fpsReal = s.args.fps;

        if(s.args.p2cm == -1) 
            sp = detection.object.velocity * 3600* fpsReal * radiusCm * 1e-5;
        else {
            float len = detection.object.velocity * (detection.object.radius+1.5);
            // std::cout << len << std::endl;
            sp = len * s.args.p2cm * fpsReal *3600 * 1e-5;
        }
        mSpeeds.push_back(sp);

        if(mLastSpeeds.size() > MAX_SPEED_FRAMES) {
            mLastSpeeds[MAX_SPEED_FRAMES-1].first = MAX_SPEED_TIME;
            mLastSpeeds[MAX_SPEED_FRAMES-1].second = sp;
        } else {
            mLastSpeeds.push_back(std::make_pair(MAX_SPEED_TIME, sp));
        }

        std::sort(mLastSpeeds.begin(), mLastSpeeds.end(), std::greater <>());
    }

    float mSpeedNow = 0;
    for(auto& elv : mLastSpeeds) {
        if(elv.first > 0) {
            elv.first--;
            if(elv.second > mSpeedNow) mSpeedNow = elv.second;
        }
    }

    if(detection.object.curve != nullptr) mMaxSpeed = mSpeedNow;

    // make sure to keep the number of segments bounded in case there's a long event
    if (mSegments.size() > MAX_SEGMENTS) {
        mSegments.erase(begin(mSegments), begin(mSegments) + (mSegments.size() / 2));
    }
    int maxCurves = 20;
    for (int i = 0; i < int(mCurves.size()) - maxCurves; ++i)
    {
        delete mCurves[0];
        mCurves.erase(mCurves.begin());
    }
}

void DemoVisualizer::drawSegments(fmo::Image& im) {
    cv::Mat mat = im.wrap();
    auto color = Colour::magenta();
    float thickness = 8;
    for (auto& segment : mSegments) {
        color.b = std::max(color.b, uint8_t(color.b + 2));
        color.g = std::max(color.g, uint8_t(color.g + 1));
        color.r = std::max(color.r, uint8_t(color.r + 4));
        cv::Scalar cvColor(color.b, color.g, color.r);
        cv::Point pt1{segment.min.x, segment.min.y};
        cv::Point pt2{segment.max.x, segment.max.y};
        cv::line(mat, pt1, pt2, cvColor, thickness);
    }
    for (auto& curve : mCurves) {
        color.b = std::max(color.b, uint8_t(color.b + 2));
        color.g = std::max(color.g, uint8_t(color.g + 1));
        color.r = std::max(color.r, uint8_t(color.r + 4));
        cv::Scalar cvColor(color.b, color.g, color.r);

        curve->drawSmooth(mat, cvColor, thickness);
    }
}

void DemoVisualizer::process(Status& s, const fmo::Region& frame, fmo::Algorithm& algorithm) {
    // estimate FPS
    if (s.outFrameNum < mLastDetectFrame) {
        mEventsDetected = s.outFrameNum;
        mSegments.clear();
        for(auto& c : mCurves) {
            delete c;
        }
        mCurves.clear();
    }

    // decrease last speeds
    for(auto& elv : mLastSpeeds) {
        if(elv.first > 0) {
            elv.first--;
        }
    }
    //

    mStats.tick();
    auto fpsEstimate = [this]() { return std::round(mStats.quantilesHz().q50); };

    // get detections
    algorithm.getOutput(mOutput, true);
    mNumberDetections = mOutput.detections.size();
    if(mNumberDetections > 0)     mSpeeds.clear();

    if (mNumberDetections > mMaxDetections) {
        mMaxDetections = mNumberDetections;
        mOffsetFromMaxDetection = 0;
    } else {
        mOffsetFromMaxDetection++;
    }
    if (mOffsetFromMaxDetection > 10*30 && mNumberDetections > 0) {
        mOffsetFromMaxDetection = 0;
        mMaxDetections = mNumberDetections;
    }

    // draw input image as background
    fmo::copy(frame, mVis);

    // iterate over detected fast-moving objects
    for (auto& detection : mOutput.detections) { onDetection(s, *detection); }

    drawSegments(mVis);
    printStatus(s,fpsEstimate());
}

void DemoVisualizer::processKeyboard(Status& s, const fmo::Region& frame) {
    // record frames
    if (mAutomatic) {
        bool event = mForcedEvent || !mOutput.detections.empty();
        if (mRecordAnnotations) {
            mAutomatic->frame(mVis, event);
        } else {
            mAutomatic->frame(frame, event);
        }
    } else if (mManual) {
        if (mRecordAnnotations) {
            mManual->frame(mVis);
        } else {
            mManual->frame(frame);
        }
    }
    mForcedEvent = false;

    bool step = false;
    // process keyboard input
    do {
        auto command = s.window.getCommand(false);
        if (command == Command::PAUSE) { s.paused = !s.paused; }
        if (command == Command::INPUT) { 
            uint nPlayers = 10;
            int stringSize = 10;
            bool visTable = s.window.visTable;
            s.window.visTable = false;
            float spnow = std::round(mMaxSpeed*100)/100;

            // std::getline(std::cin, s.inputString);
            // system("zenity  --title  \"Gimme some text!\" --entry --text \"Enter your text here\"");
            // s.inputString = "Denis"; 

            if(s.window.mTable.size() >= nPlayers) {
                auto &lastel = s.window.mTable[s.window.mTable.size()-1];
                if(spnow <= lastel.first) {
                    s.window.setCenterLine("Sorry, not in top "+std::to_string(nPlayers), "");
                    s.window.display(mVis);
                    cv::waitKey(500);
                    s.window.visTable = visTable;
                    s.window.setCenterLine("", "");
                    s.window.display(mVis);
                    continue;
                }
            }

            s.window.setCenterLine("Input player's name", "");
            s.window.display(mVis);

            char keyCode = ' '; 
            std::vector<char> vec;
            while(keyCode != 13 && keyCode != '\n' && keyCode != 10) {
                // std::cout << (int) keyCode << std::endl;
                if(keyCode != ' ') {
                    if(keyCode == 127 || keyCode == 8) {
                        if(vec.size() > 0)
                            vec.pop_back();
                    } else {
                        vec.push_back(keyCode);
                    }
                    std::string str(vec.begin(), vec.end());
                    s.window.setCenterLine("Input player's name", str);
                    s.window.display(mVis);
                }
                keyCode = cv::waitKey(0);
            }
            s.window.setCenterLine("","");
            s.window.display(mVis);
            std::string str(vec.begin(), vec.end());
            s.inputString = str;

            auto &playerName = s.inputString;
            if((int)playerName.size() < stringSize) 
                playerName = playerName + std::string(stringSize-playerName.size(), ' ');
            if((int)playerName.size() > stringSize) 
                playerName = playerName.substr(0,stringSize);

            if(s.window.mTable.size() < nPlayers) {
                s.window.mTable.push_back(std::make_pair(spnow, playerName));
            } else {
                auto &lastel = s.window.mTable[s.window.mTable.size()-1];
                if(spnow > lastel.first) {
                    lastel = std::make_pair(spnow, playerName);
                }
            }
            std::sort(s.window.mTable.begin(), s.window.mTable.end(), std::greater <>());
            s.window.visTable = visTable;
        }
        if (command == Command::STEP) step = true;
        if (command == Command::QUIT) { s.quit = true; mManual.reset(nullptr); }
        if (command == Command::SHOW_HELP) {
            mShowHelp = !mShowHelp;
            updateHelp(s);
        }
        if (command == Command::AUTOMATIC_MODE) {
            if (mManual) { mManual.reset(nullptr); }
            if (!mAutomatic) {
                mAutomatic = std::make_unique<AutomaticRecorder>(s.args.recordDir, frame.format(),
                                                                 frame.dims(), 30);
                updateHelp(s);
            }
        }
        if (command == Command::FORCED_EVENT) { mForcedEvent = true; }
        if (command == Command::MANUAL_MODE) {
            if (mAutomatic) {
                mAutomatic.reset(nullptr);
                updateHelp(s);
            }
        }
        if (command == Command::RECORD_GRAPHICS) mRecordAnnotations = !mRecordAnnotations;
        if (command == Command::RECORD) {
            if (mManual) {
                mManual.reset(nullptr);
            } else if (!mAutomatic) {
                mManual = std::make_unique<ManualRecorder>(s.args.recordDir, frame.format(),
                                                           frame.dims(), 30);
            }
        }
        if (command == Command::PLAY_SOUNDS) { s.sound = !s.sound; }
        if (command == Command::LEVEL0) mode = 0;
        if (command == Command::LEVEL1) mode = 1;
        if (command == Command::LEVEL2) mode = 2;
        if (command == Command::LEVEL3) mode = 3;
        if (command == Command::LEVEL4) mode = 4;
    } while (s.paused && !s.quit && !step);
}

void DemoVisualizer::visualize(Status& s, const fmo::Region& frame, const Evaluator*,
                               const EvalResult&, fmo::Algorithm& algorithm) {

    this->process(s,frame,algorithm);
    s.window.display(mVis);
    this->processKeyboard(s,frame);
}


RemovalVisualizer::RemovalVisualizer(Status& s) : mStats(60) {
    mStats.reset(15.f);
    // s.window.setBottomLine("[esc] quit | [space] pause | [enter] step | [,][.] jump 10 frames");
}

void RemovalVisualizer::visualize(Status& s, const fmo::Region& frame, const Evaluator*,
                                const EvalResult&, fmo::Algorithm& algorithm) {
    mEventsDetected++;

    // estimate FPS
    mStats.tick();
    // float fpsLast = mStats.getLastFps();
    auto fpsEstimate = [this]() { return std::round(mStats.quantilesHz().q50); };

    // get the background provided by the algorithm
    fmo::copy(frame, mCurrent);

    // swap frames
    mFrames[4].swap(mFrames[3]);
    mFrames[3].swap(mFrames[2]);
    mFrames[2].swap(mFrames[1]);
    mFrames[1].swap(mFrames[0]);
    mFrames[0].swap(mCurrent);

    fmo::Image &vis = mFrames[std::min(0-algorithm.getOutputOffset(),mEventsDetected-1)];
    fmo::Image &bg  = mFrames[std::min(mEventsDetected-1,4)];

    // s.window.print(s.inputName);
    // s.window.print(mManual ? "recording" : "not recording");
    // s.window.print("frame: " + std::to_string(s.inFrameNum));
    // s.window.print("fps: " + std::to_string(fpsLast));
    s.window.setTextColor(mManual ? Colour::lightRed() : Colour::lightGray());

    // get pixel coordinates of detected objects
    algorithm.getOutput(mOutputCache, false);
    mObjectPoints.clear();
    for (auto& detection : mOutputCache.detections) {
        mObjectPoints.emplace_back();
        detection->getPoints(mObjectPoints.back());
    }
    fmo::pointSetMerge(begin(mObjectPoints), end(mObjectPoints), mPointsCache);

    // replace detected points by the background
    removePoints(mPointsCache, vis, bg);

    // record frames 
    if (mManual) {
        mManual->frame(vis);
    }

    // display
    s.window.display(vis);
    
    // process keyboard input
    bool step = false;
    do {
        auto command = s.window.getCommand(s.paused);
        if (command == Command::PAUSE) s.paused = !s.paused;
        if (command == Command::STEP) step = true;
        if (command == Command::QUIT) s.quit = true;
        if (command == Command::SCREENSHOT) fmo::save(vis, "screenshot.png");

        if (!s.haveCamera()) {
            if (command == Command::JUMP_BACKWARD) {
                s.paused = false;
                s.args.frame = std::max(1, s.inFrameNum - 10);
                s.reload = true;
            }
            if (command == Command::JUMP_FORWARD) {
                s.paused = false;
                s.args.frame = s.inFrameNum + 10;
            }
        }
        if (command == Command::RECORD) {
            if (mManual) {
                mManual.reset(nullptr);
            } else {
                mManual = std::make_unique<ManualRecorder>(s.args.recordDir, frame.format(),
                                                           frame.dims(), fpsEstimate());
            }
        }
    } while (s.paused && !step && !s.quit);
}
