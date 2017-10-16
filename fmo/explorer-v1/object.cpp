#include "../include-opencv.hpp"
#include "explorer.hpp"
#include <algorithm>
#include <fmo/processing.hpp>
#include <fmo/region.hpp>
#include <limits>
#include <type_traits>

namespace fmo {
    namespace {
        constexpr int BOUNDS_MIN = std::numeric_limits<int16_t>::min();
        constexpr int BOUNDS_MAX = std::numeric_limits<int16_t>::max();
    }

    void ExplorerV1::findObjects() {
        // reorder trajectories by the number of strips, largest will be at the front
        std::sort(
            begin(mTrajectories), end(mTrajectories),
            [](const Trajectory& l, const Trajectory& r) { return l.numStrips > r.numStrips; });

        mObjects.clear();
        mRejected.clear();
        for (auto& traj : mTrajectories) {
            // ignore all trajectories with too few strips
            if (traj.numStrips < mCfg.minStripsInCluster) break;

            // test if the trajectory is interesting
            if (isObject(traj)) {
                mObjects.push_back(&traj);
                break; // assume a single interesting trajectory
            } else {
                mRejected.push_back(&traj);
            }
        }
    }

    bool ExplorerV1::isObject(Trajectory& traj) const {
        // find the bounding box enclosing strips present in the difference images
        traj.bounds1 = findTrajectoryBoundsInDiff(traj, mLevel.diff1, mLevel.step);
        traj.bounds2 = findTrajectoryBoundsInDiff(traj, mLevel.diff2, mLevel.step);

        // condition: both diffs must have *some* strips present
        if (traj.bounds1.min.x == BOUNDS_MAX || traj.bounds2.min.x == BOUNDS_MAX) return false;

        // force left-to-right direction
        int xMin = mStrips[mComponents[traj.first].first].x;
        if (traj.bounds1.min.x != xMin) {
            // force left-to-right by swapping ranges
            std::swap(traj.bounds1, traj.bounds2);
        }

        // condition: leftmost strip must be in range1
        if (traj.bounds1.min.x != xMin) return false;

        // condition: rightmost strip must be in range2
        int xMax = mStrips[mComponents[traj.last].last].x;
        if (traj.bounds2.max.x != xMax) return false;

        // condition: range1 must end a significant distance away from rightmost strip
        int minMotion = int(mCfg.minMotion * (xMax - xMin));
        if (xMax - traj.bounds1.max.x < minMotion) return false;

        // condition: range2 must start a significant distance away from lefmost strip
        if (traj.bounds2.min.x - xMin < minMotion) return false;

        return true;
    }

    Bounds ExplorerV1::findTrajectoryBoundsInDiff(const Trajectory& traj, const Mat& diff,
                                                  int step) const {
        int halfStep = step / 2;
        const uint8_t* data = diff.data();
        int skip = int(diff.skip());
        Bounds result{{BOUNDS_MAX, BOUNDS_MAX}, {BOUNDS_MIN, BOUNDS_MIN}};

        // iterate over all strips in trajectory
        int compIdx = traj.first;
        while (compIdx != Component::NO_COMPONENT) {
            const Component& comp = mComponents[compIdx];
            int stripIdx = comp.first;
            while (stripIdx != Strip::END) {
                const Strip& strip = mStrips[stripIdx];
                int col = (strip.x - halfStep) / step;
                int row = (strip.y - halfStep) / step;
                uint8_t val = *(data + (row * skip + col));

                // if the center of the strip is in the difference image
                if (val != 0) {
                    // update bounds
                    result.min.x = std::min(result.min.x, int(strip.x));
                    result.min.y = std::min(result.min.y, int(strip.y));
                    result.max.x = std::max(result.max.x, int(strip.x));
                    result.max.y = std::max(result.max.y, int(strip.y));
                }
                stripIdx = strip.special;
            }
            compIdx = comp.next;
        }

        return result;
    }

    auto ExplorerV1::findBounds(const Trajectory& traj) const -> Bounds {
        Bounds result;
        result.min = {BOUNDS_MAX, BOUNDS_MAX};
        result.max = {BOUNDS_MIN, BOUNDS_MIN};

        const Component* comp = &mComponents[traj.first];
        const Strip* firstStrip = &mStrips[comp->first];
        while (true) {
            const Strip* strip = &mStrips[comp->first];
            while (true) {
                result.min.y = std::min(result.min.y, strip->y - strip->halfHeight);
                result.max.y = std::max(result.max.y, strip->y + strip->halfHeight);
                if (strip->special == Strip::END) break;
                strip = &mStrips[strip->special];
            }
            if (comp->next == Component::NO_COMPONENT) break;
            comp = &mComponents[comp->next];
        }
        const Strip* lastStrip = &mStrips[comp->last];

        int halfWidth = mLevel.step / 2;
        result.min.x = firstStrip->x - halfWidth;
        result.max.x = lastStrip->x + halfWidth;
        return result;
    }

    ExplorerV1::MyDetection::MyDetection(const Detection::Object& detObj,
                                         const Detection::Predecessor& detPrev,
                                         const Trajectory* traj, const ExplorerV1* aMe)
        : Detection(detObj, detPrev), me(aMe), mTraj(traj) {}

    void ExplorerV1::MyDetection::getPoints(PointSet& out) const {
        out.clear();
        const Trajectory& traj = *mTraj;
        int step = me->mLevel.step;
        int halfStep = step / 2;
        const uint8_t* data1 = me->mLevel.diff1.data();
        const uint8_t* data2 = me->mLevel.diff2.data();
        int skip1 = int(me->mLevel.diff1.skip());
        int skip2 = int(me->mLevel.diff2.skip());

        // iterate over all strips in trajectory
        int compIdx = traj.first;
        while (compIdx != Component::NO_COMPONENT) {
            const Component& comp = me->mComponents[compIdx];
            int stripIdx = comp.first;
            while (stripIdx != Strip::END) {
                const Strip& strip = me->mStrips[stripIdx];
                int col = (strip.x - halfStep) / step;
                int row = (strip.y - halfStep) / step;
                uint8_t val1 = *(data1 + (row * skip1 + col));
                uint8_t val2 = *(data2 + (row * skip2 + col));

                // if the center of the strip is in both difference images
                if (val1 != 0 && val2 != 0) {
                    // put all pixels in the strip as object pixels
                    int ye = strip.y + strip.halfHeight;
                    int xe = strip.x + halfStep;

                    for (int y = strip.y - strip.halfHeight; y < ye; y++) {
                        for (int x = strip.x - halfStep; x < xe; x++) { out.push_back({x, y}); }
                    }
                }
                stripIdx = strip.special;
            }
            compIdx = comp.next;
        }

        // sort to enable fast comparion with other point lists
        std::sort(begin(out), end(out), pointSetCompLt);
    }

    namespace {
        Pos center(const fmo::Bounds& b) {
            return{(b.max.x + b.min.x) / 2, (b.max.y + b.min.y) / 2};
        }
    }

    void ExplorerV1::getOutput(Output &out, bool smoothTrajecotry) {
        out.clear();
        Detection::Object detObj;
        Detection::Predecessor detPrev;

        for (auto* traj : mObjects) {
            detObj.center = center(traj->bounds1);
            detObj.radius = mComponents[traj->first].approxHalfHeight;
            detPrev.center = center(traj->bounds2);
            out.detections.emplace_back();
            out.detections.back().reset(new MyDetection(detObj, detPrev, traj, this));
        }
    }
}
