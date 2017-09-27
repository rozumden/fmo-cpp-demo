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

    void ExplorerV2::findObjects() {
        mObjects.clear();

        // sort valid clusters descending by total length
        auto& sortClusters = mCache.sortClusters;
        sortClusters.clear();

        for (auto& cluster : mClusters) {
            if (cluster.isInvalid()) continue;
            sortClusters.emplace_back(cluster.lengthTotal, &cluster);
        }

        std::sort(begin(sortClusters), end(sortClusters),
                  [](auto& l, auto& r) { return l.first > r.first; });

        // find the longest cluster that is considered an object
        for (auto& sortCluster : sortClusters) {
            if (isObject(*sortCluster.second)) {
                mObjects.push_back(sortCluster.second);
                break;
            } else {
                sortCluster.second->setInvalid(Cluster::NOT_AN_OBJECT);
            }
        }
    }

    bool ExplorerV2::isObject(Cluster& cluster) const {
        // find the bounding box enclosing strips present in the difference images
        cluster.bounds1 = findClusterBoundsInDiff(cluster, mLevel.diff1, mLevel.step);
        cluster.bounds2 = findClusterBoundsInDiff(cluster, mLevel.diff2, mLevel.step);

        // condition: both diffs must have *some* strips present
        if (cluster.bounds1.min.x == BOUNDS_MAX || cluster.bounds2.min.x == BOUNDS_MAX)
            return false;

        // force left-to-right direction
        int xMin = cluster.l.pos.x;
        if (cluster.bounds1.min.x != xMin) {
            // force left-to-right by swapping ranges
            std::swap(cluster.bounds1, cluster.bounds2);
        }

        // condition: leftmost strip must be in range1
        if (cluster.bounds1.min.x != xMin) return false;

        // condition: rightmost strip must be in range2
        int xMax = cluster.r.pos.x;
        if (cluster.bounds2.max.x != xMax) return false;

        // condition: range1 must end a significant distance away from rightmost strip
        int minMotion = int(mCfg.minMotion * (xMax - xMin));
        int maxMotion = int(mCfg.maxMotion * (xMax - xMin));
        if (xMax - cluster.bounds1.max.x < minMotion) return false;
        if (xMax - cluster.bounds1.max.x > maxMotion) return false;

        // condition: range2 must start a significant distance away from lefmost strip
        if (cluster.bounds2.min.x - xMin < minMotion) return false;
        if (cluster.bounds2.min.x - xMin > maxMotion) return false;

        return true;
    }

    Bounds ExplorerV2::findClusterBoundsInDiff(const Cluster& cluster, const Mat& diff,
                                               int step) const {
        int halfStep = step / 2;
        const uint8_t* data = diff.data();
        int skip = int(diff.skip());
        Bounds result{{BOUNDS_MAX, BOUNDS_MAX}, {BOUNDS_MIN, BOUNDS_MIN}};

        // iterate over all strips in cluster
        int index = cluster.l.strip;
        while (index != Special::END) {
            auto& strip = mStrips[index];
            int col = (strip.pos.x - halfStep) / step;
            int row = (strip.pos.y - halfStep) / step;
            uint8_t val = *(data + (row * skip + col));

            // if the center of the strip is in the difference image
            if (val != 0) {
                // update bounds
                result.min.x = std::min(result.min.x, int(strip.pos.x));
                result.min.y = std::min(result.min.y, int(strip.pos.y));
                result.max.x = std::max(result.max.x, int(strip.pos.x));
                result.max.y = std::max(result.max.y, int(strip.pos.y));
            }
            index = next(strip);
        }

        return result;
    }

    ExplorerV2::MyDetection::MyDetection(const Detection::Object& detObj,
                                         const Detection::Predecessor& detPrev,
                                         const Cluster* cluster, const ExplorerV2* aMe)
        : Detection(detObj, detPrev), me(aMe), mCluster(cluster) {}

    void ExplorerV2::MyDetection::getPoints(PointSet& out) const {
        out.clear();
        auto& obj = *mCluster;
        int halfStep = me->mLevel.step / 2;
        int minX = std::max(obj.bounds1.min.x, obj.bounds2.min.x);
        int maxX = std::min(obj.bounds1.max.x, obj.bounds2.max.x);

        // iterate over all strips in cluster
        int index = obj.l.strip;
        while (index != Special::END) {
            auto& strip = me->mStrips[index];

            // if the center of the strip is in both bounding boxes
            if (strip.pos.x >= minX && strip.pos.x <= maxX) {
                // put all pixels in the strip as object pixels
                int ye = strip.pos.y + strip.halfDims.height;
                int xe = strip.pos.x + halfStep;

                for (int y = strip.pos.y - strip.halfDims.height; y < ye; y++) {
                    for (int x = strip.pos.x - halfStep; x < xe; x++) { out.push_back({x, y}); }
                }
            }

            index = next(strip);
        }

        // sort to enable fast comparion with other point lists
        std::sort(begin(out), end(out), pointSetCompLt);
    }

    namespace {
        Pos center(const fmo::Bounds& b) {
            return {(b.max.x + b.min.x) / 2, (b.max.y + b.min.y) / 2};
        }

        float average(float v1, float v2) { return (v1 + v2) / 2; }
    }

    void ExplorerV2::getOutput(Output& out) {
        out.clear();
        Detection::Object detObj;
        Detection::Predecessor detPrev;

        for (auto* cluster : mObjects) {
            detObj.center = center(cluster->bounds1);
            detObj.radius = average(cluster->approxHeightMin, cluster->approxHeightMax);
            detPrev.center = center(cluster->bounds2);
            out.detections.emplace_back();
            out.detections.back().reset(new MyDetection(detObj, detPrev, cluster, this));
        }
    }
}
