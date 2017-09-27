#include "objectset.hpp"
#include <fmo/algebra.hpp>
#include <fmo/assert.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

void ObjectSet::loadGroundTruth(const std::string& filename, fmo::Dims dims) try {
    std::ios::sync_with_stdio(false);
    mFrames.clear();

    auto fail = []() { throw std::runtime_error("failed to parse file"); };
    std::ifstream in{filename};
    if (!in) fail();
    int aNumFrames, numObjects;
    in >> mDims.width >> mDims.height >> aNumFrames >> mOffset >> numObjects;
    if (!in) fail();

    if (mDims.width != dims.width || abs(mDims.height - dims.height) > 8) {
        throw std::runtime_error("dimensions inconsistent with video");
    }

    auto addPoints =
        [ npt = dims.width * dims.height, dims ](fmo::PointSet & set, int first, int last) {
        last = std::min(last, npt);
        for (; first < last; first++) {
            int x = first % dims.width;
            int y = first / dims.width;
            set.push_back({x, y});
        }
    };

    mFrames.resize(aNumFrames);
    for (int i = 0; i < numObjects; i++) {
        int frameNum, numRuns;
        in >> frameNum >> numRuns;
        if (!in) fail();

        if (frameNum < 1 || frameNum > aNumFrames) {
            std::cerr << "got frame number " << frameNum << ", want <= " << aNumFrames << '\n';
            throw std::runtime_error("bad frame number");
        }

        auto& ptr = at(frameNum);
        if (!ptr) { ptr = std::make_unique<std::vector<fmo::PointSet>>(); }
        ptr->emplace_back();
        auto& set = ptr->back();

        bool white = false;
        int pos = 0;
        for (int j = 0; j < numRuns; j++) {
            int runLength;
            in >> runLength;
            if (white) { addPoints(set, pos, pos + runLength); }
            pos += runLength;
            white = !white;
        }

        if (white) { addPoints(set, pos, mDims.width * mDims.height); }

        if (!in) fail();
    }
} catch (std::exception& e) {
    std::cerr << "while loading file '" << filename << "'\n";
    throw e;
}

const std::vector<fmo::PointSet>& ObjectSet::get(int frameNum) const {
    static const std::vector<fmo::PointSet> empty;
    frameNum += mOffset;
    if (frameNum < 1 || frameNum > numFrames()) return empty;
    auto& ptr = at(frameNum);
    if (!ptr) return empty;
    return *ptr;
}
