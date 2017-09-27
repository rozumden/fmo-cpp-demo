#ifndef FMO_AGGLOMERATOR_HPP
#define FMO_AGGLOMERATOR_HPP

#include <cstdint>
#include <limits>
#include <vector>

namespace fmo {
    struct Agglomerator {
        using Id_t = int16_t;
        using Dist_t = float;

        static constexpr Id_t safetyMaxNumClusters = 100;
        static constexpr Dist_t infDist = std::numeric_limits<Dist_t>::max();

        /// Performs agglomerative clustering using a naive algorithm with O(n^3) time complexity
        /// and O(n^2) storage complexity. Clusters are merged greedily based on the provided
        /// distance function. Additionally, it is assumed that once a cluster is created by
        /// merging, the distance to all the other clusters cannot be derived from the previously
        /// calculated distances. Consequently, the problem is not an instance of single-linkage
        /// clustering. The merging operation effectively removes two clusters and adds a new one
        /// instead, therefore it is required that the distance function is cheap to calculate, with
        /// complexity O(1) even for non-trivial clusters.
        ///
        /// @param distanceFunc A funtion with signature Dist_t(Id_t i, Id_t j) or compatible that
        /// provides the distance between clusters i, j with time complexity O(1). Return infDist
        /// whenever merging the two clusters is impossible.
        /// @param mergeFunc A function with signature void(Id_t i, Id_t j) that merges clusters i
        /// and j into cluster i and invalidates cluster j.
        /// @param numClusters the initial number of clusters. Clusters are numbered 0 (inclusive)
        /// to numClusters (exclusive).
        template <typename DistanceFunc, typename MergeFunc>
        void operator()(DistanceFunc distanceFunc, MergeFunc mergeFunc, Id_t numClusters);

    private:
        /// For storing the distance between clusters i and j.
        struct Pair {
            Dist_t d;
            Id_t i;
            Id_t j;

            Pair(Dist_t aD, Id_t aI, Id_t aJ) : d(aD), i(aI), j(aJ) {}
        };

        std::vector<Id_t> mIds;
        std::vector<Pair> mPairs;
    };
}

#endif // FMO_AGGLOMERATOR_HPP
