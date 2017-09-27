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

    void ExplorerV3::findObjects() {
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

    bool ExplorerV3::isObject(Cluster& cluster) const {
        // find the bounding box enclosing strips present in the difference images
        cluster.bounds1 = findClusterBoundsInDiff(cluster, true);
        cluster.bounds2 = findClusterBoundsInDiff(cluster, false);

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

    Bounds ExplorerV3::findClusterBoundsInDiff(const Cluster& cluster, bool newer) const {
        ;
        Bounds result{{BOUNDS_MAX, BOUNDS_MAX}, {BOUNDS_MIN, BOUNDS_MIN}};

        // iterate over all strips in cluster
        int index = cluster.l.strip;
        while (index != MetaStrip::END) {
            auto& strip = mLevel.metaStrips[index];

            // if the center of the strip is in the difference image
            if (newer ? strip.newer : strip.older) {
                // update bounds
                result.min.x = std::min(result.min.x, int(strip.pos.x));
                result.min.y = std::min(result.min.y, int(strip.pos.y));
                result.max.x = std::max(result.max.x, int(strip.pos.x));
                result.max.y = std::max(result.max.y, int(strip.pos.y));
            }
            index = strip.next;
        }

        return result;
    }

    ExplorerV3::MyDetection::MyDetection(const Detection::Object& detObj,
                                         const Detection::Predecessor& detPrev,
                                         const Cluster* cluster, const ExplorerV3* aMe)
        : Detection(detObj, detPrev), me(aMe), mCluster(cluster) {}

    void ExplorerV3::MyDetection::getPoints(PointSet& out) const {
        out.clear();
        auto& obj = *mCluster;

        // iterate over all strips in cluster
        int index = obj.l.strip;
        while (index != MetaStrip::END) {
            auto& strip = me->mLevel.metaStrips[index];

            // if strip is in both diffs
            if (strip.older && strip.newer) {
                // put all pixels in the strip as object pixels
                int ye = strip.pos.y + strip.halfDims.height;
                int xe = strip.pos.x + strip.halfDims.width;

                for (int y = strip.pos.y - strip.halfDims.height; y < ye; y++) {
                    for (int x = strip.pos.x - strip.halfDims.width; x < xe; x++) {
                        out.push_back({x, y});
                    }
                }
            }

            index = strip.next;
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

    void ExplorerV3::getOutput(Output& out) {
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
