#ifndef FMO_AGGLOMERATOR_IMPL_HPP
#define FMO_AGGLOMERATOR_IMPL_HPP

#include <algorithm>
#include <fmo/agglomerator.hpp>

namespace fmo {
    template <typename DistanceFunc, typename MergeFunc>
    void Agglomerator::operator()(DistanceFunc distanceFunc, MergeFunc mergeFunc,
                                  Id_t numClusters) {
        numClusters = std::min(numClusters, safetyMaxNumClusters);
        mIds.clear();
        mPairs.clear();

        auto addCluster = [&distanceFunc, this](Id_t i) {
            // calculate distances to all existing clusters
            for (auto j : mIds) {
                Dist_t d = distanceFunc(i, j);
                if (d != infDist) { mPairs.emplace_back(d, i, j); }
            }
            // add cluster
            mIds.push_back(i);
        };

        auto mergeClusters = [&mergeFunc, &addCluster, this](Id_t i, Id_t j) {
            // remove i, j from mIds
            {
                auto last = std::remove_if(begin(mIds), end(mIds),
                                           [i, j](Id_t id) { return id == i || id == j; });
                mIds.erase(last, end(mIds));
            }
            // remove i, j from mPairs
            {
                auto last = std::remove_if(begin(mPairs), end(mPairs), [i, j](const Pair& r) {
                    return r.i == i || r.i == j || r.j == j || r.j == i;
                });
                mPairs.erase(last, end(mPairs));
            }
            // merge into i
            mergeFunc(i, j);
            // re-insert i
            addCluster(i);
        };

        auto findBestPair = [this]() {
            Dist_t bestDist = infDist;
            Pair* bestRel = nullptr;

            for (auto& pair : mPairs) {
                if (pair.d < bestDist) {
                    bestDist = pair.d;
                    bestRel = &pair;
                }
            }

            return bestRel;
        };

        // add initial clusters
        for (Id_t i = 0; i < numClusters; i++) { addCluster(i); }

        // merge until there's no viable pairs left
        while (!mPairs.empty()) {
            auto* pair = findBestPair();
            mergeClusters(pair->i, pair->j);
        }
    }
}

#endif // FMO_AGGLOMERATOR_IMPL_HPP
