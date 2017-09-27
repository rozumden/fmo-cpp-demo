#include "evaluator.hpp"
#include "desktop-opencv.hpp"
#include <fmo/image.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

const std::string& eventName(Event e) {
    static const std::string tp = "TP";
    static const std::string tn = "TN";
    static const std::string fp = "FP";
    static const std::string fn = "FN";
    static const std::string questionMarks = "??";

    switch (e) {
    case Event::TP:
        return tp;
    case Event::TN:
        return tn;
    case Event::FP:
        return fp;
    case Event::FN:
        return fn;
    default:
        return questionMarks;
    }
}

// EvalResult

std::string EvalResult::str() const {
    std::string result;
    result.reserve(80);

    for (int i = 0; i < 4; i++) {
        Event event = Event(i);
        if (eval[event]) {
            if (eval[event] > 1) {
                result += std::to_string(eval[event]);
                result.push_back('x');
            }
            result += eventName(event);
            result.push_back(' ');
        }
    }

    if (comp == Comparison::IMPROVEMENT) result += "(improvement) ";
    if (comp == Comparison::REGRESSION) result += "(regression) ";
    if (comp == Comparison::SAME) result += "(no change) ";
    if (comp == Comparison::NONE) result += "(no baseline) ";
    if (comp == Comparison::BUFFERING) result += "(buffering)";
    return result;
}

// Results

FileResults& Results::newFile(const std::string& name) {
    auto found = mMap.find(name);

    if (found != mMap.end()) {
        found->second->clear();
        return *found->second;
    } else {
        mList.emplace_back(name);
        mMap.emplace(name, &mList.back());
        return mList.back();
    }
}

const FileResults& Results::getFile(const std::string& name) const {
    auto found = mMap.find(name);

    if (found == mMap.end()) {
        static FileResults empty{"(no results)"};
        return empty;
    }

    return *found->second;
}

namespace {
    std::string introToken{"/FMO/EVALUATION/V3/"};
    constexpr Event eventOrder[4] = {Event::FN, Event::FP, Event::TN, Event::TP};
}

void Results::load(std::istream& in) {
    std::string token;
    bool found = false;
    while (!found && in >> token) { found = token == introToken; }
    if (!found) { throw std::runtime_error("failed to find data start token"); }

    int numFiles;
    in >> numFiles;

    for (int i = 0; i < numFiles; i++) {
        in >> token;
        auto& file = newFile(token);
        int numFrames;
        in >> numFrames;
        int numIOUs;
        in >> numIOUs;
        file.frames.resize(numFrames);

        for (int e = 0; e < 4; e++) {
            Event event = eventOrder[e];
            in >> token;
            if (token != eventName(event)) {
                std::cerr << "expected " << eventName(event) << " but got " << token << '\n';
                throw std::runtime_error("expected token not found");
            }
            for (int f = 0; f < numFrames; f++) {
                int n;
                in >> n;
                file.frames[f][event] = n;
            }
        }

        if (numIOUs > 0) {
            file.iou.resize(numIOUs);

            in >> token;
            if (token != "IOU") {
                std::cerr << "expected 'IOU' but got '" << token << "'\n";
                throw std::runtime_error("expected token not found");
            }

            for (int t = 0; t < numIOUs; t++) {
                int n;
                in >> n;
                file.iou[t] = n;
            }
        }

        if (!in) { throw std::runtime_error("error while parsing"); }
    }
}

void Results::save(std::ostream& out) const {
    out << introToken << '\n';
    out << mMap.size() << '\n';

    for (auto& entry : mMap) {
        auto& name = entry.first;
        auto& file = *entry.second;
        size_t numIOUs = file.iou.size();
        out << name << ' ' << file.frames.size() << ' ' << numIOUs << '\n';

        for (int e = 0; e < 4; e++) {
            Event event = eventOrder[e];
            out << eventName(event);
            for (auto eval : file.frames) { out << ' ' << eval[event]; }
            out << '\n';
        }

        if (numIOUs > 0) {
            out << "IOU";
            for (auto value : file.iou) { out << ' ' << value; }
            out << '\n';
        }
    }
}

void Results::load(const std::string& fn) try {
    mMap.clear();
    mList.clear();
    std::ifstream in{fn, std::ios_base::in | std::ios_base::binary};
    if (!in) { throw std::runtime_error("failed to open file"); }
    load(in);
} catch (std::exception& e) {
    std::cerr << "while reading '" << fn << "'\n";
    throw e;
}

std::vector<int> Results::makeIOUHistogram(int bins) const {
    int divisor = int(std::round(FileResults::IOU_STORAGE_FACTOR / double(bins)));
    std::vector<int> hist(bins, 0);

    for (auto& file : mList) {
        for (auto val : file.iou) {
            int bin = std::min(bins - 1, val / divisor);
            hist[bin]++;
        }
    }

    return hist;
}

double Results::getAverageIOU() const {
    int sum = 0;
    int count = 0;
    for (auto& file : mList) {
        for (auto val : file.iou) {
            sum += val;
            count++;
        }
    }

    if (count == 0) return 0.;
    return double(sum) / (double(count) * FileResults::IOU_STORAGE_FACTOR);
}

// Evaluator

Evaluator::~Evaluator() {
    // if ended prematurely, remove the results
    if (size_t(mGt.numFrames()) != mFile->frames.size()) { mFile->clear(); }
}

Evaluator::Evaluator(const std::string& gtFilename, fmo::Dims dims, Results& results,
                     const Results& baseline) {
    mGt.loadGroundTruth(gtFilename, dims);
    mName = extractSequenceName(gtFilename);
    mFile = &results.newFile(mName);
    mFile->frames.reserve(mGt.numFrames());

    mBaseline = &baseline.getFile(mName);
    if (mBaseline->frames.empty()) mBaseline = nullptr;
    if (mBaseline != nullptr && mBaseline->frames.size() != size_t(mGt.numFrames())) {
        std::cerr << "baseline has " << mBaseline->frames.size() << " frames, expecting "
                  << mGt.numFrames() << '\n';
        throw std::runtime_error("bad baseline number of frames");
    }
}

void Evaluator::evaluateFrame(const fmo::Algorithm::Output& dt, int frameNum, EvalResult& out, float iouThreshold) {
    if (++mFrameNum != frameNum) {
        std::cerr << "got frame: " << frameNum << " expected: " << mFrameNum << '\n';
        throw std::runtime_error("bad number of frames");
    }

    if (mFrameNum > mGt.numFrames()) {
        std::cerr << "GT has " << mGt.numFrames() << " frames, now at " << mFrameNum << '\n';
        throw std::runtime_error("movie length inconsistent with GT");
    }

    auto iou = [this](const fmo::PointSet& ps1, const fmo::PointSet& ps2) {
        int intersection = 0;
        int union_ = 0;
        auto mismatch = [&](fmo::Pos) { union_++; };
        auto match = [&](fmo::Pos) {
            intersection++;
            union_++;
        };
        fmo::pointSetCompare(ps1, ps2, mismatch, mismatch, match);
        return double(intersection) / double(union_);
    };

    auto& gt = mGt.get(mFrameNum);

    // try each GT object with each detected object, store max IOU
    out.clear();
    out.iouDt.resize(dt.detections.size(), 0.);
    out.iouGt.resize(gt.size(), 0.);
    for (size_t i = 0; i < out.iouDt.size(); i++) {
        auto& dtScore = out.iouDt[i];
        dt.detections[i]->getPoints(mPointsCache);
        for (size_t j = 0; j < out.iouGt.size(); j++) {
            auto& gtScore = out.iouGt[j];
            auto& gtSet = gt[j];
            auto score = iou(gtSet, mPointsCache);
            gtScore = std::max(gtScore, score);
            dtScore = std::max(dtScore, score);
        }
    }

    // emit events based on best IOU of each object
    for (auto score : out.iouGt) {
        if (score > iouThreshold) {
            out.eval[Event::TP]++;
        } else {
            out.eval[Event::FN]++;
        }
    }
    for (auto score : out.iouDt) {
        if (score > iouThreshold) {
            // ignore, TPs are added in the GT loop above
        } else {
            out.eval[Event::FP]++;
        }
    }

    // store non-zero IOUs of GT objects
    for (auto score : out.iouGt) {
        if (score > 0) {
            mFile->iou.push_back(int(std::round(score * FileResults::IOU_STORAGE_FACTOR)));
        }
    }

    if (dt.detections.empty() && gt.empty()) {
        // no objects at all: add a single TN
        out.eval[Event::TN]++;
    }

    if (mBaseline != nullptr) {
        auto baseline = mBaseline->frames.at(mFile->frames.size());

        if (bad(baseline) && good(out.eval)) {
            out.comp = Comparison::IMPROVEMENT;
        } else if (good(baseline) && bad(out.eval)) {
            out.comp = Comparison::REGRESSION;
        } else {
            out.comp = Comparison::SAME;
        }
    } else {
        out.comp = Comparison::NONE;
    }

    mFile->frames.emplace_back(out.eval);
}

std::string extractFilename(const std::string& path) {
    auto findLast = [&](char c) {
        auto re = rend(path);
        auto it = std::find(rbegin(path), re, c);
        if (it == re) return 0;
        return int(&*it - path.data()) + 1;
    };
    int skip = std::max(findLast('\\'), findLast('/'));
    return std::string(begin(path) + skip, end(path));
}

std::string extractSequenceName(const std::string& path) {
    auto stripSuffix = [](std::string& str, const std::string& suffix) {
        int offset = int(str.size()) - int(suffix.size());
        if (offset < 0) return;
        for (int i = 0; i < int(suffix.size()); i++) {
            if (str[offset + i] != suffix[i]) return;
        }
        str.resize(str.size() - suffix.size());
    };
    std::string str = extractFilename(path);
    stripSuffix(str, ".mat");
    stripSuffix(str, ".txt");
    stripSuffix(str, "_gt");
    stripSuffix(str, ".avi");
    stripSuffix(str, ".mp4");
    stripSuffix(str, ".mov");

    for (auto& c : str) {
        if (c == ' ') c = '_';
    }

    return str;
}
