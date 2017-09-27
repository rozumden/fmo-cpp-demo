#include "explorer.hpp"
#include <fmo/agglomerator-impl.hpp>
#include <math.h>

namespace fmo {
    namespace {
        float distL2(const Pos& p1, const Pos& p2) {
            int dx = p1.x - p2.x;
            int dy = p1.y - p2.y;
            return hypotf(float(dx), float(dy));
        };
    }

    void ExplorerV2::findClusters() {
        // create initial clusters, one for each component that satisfies conditions
        auto makeInitialClusters = [this]() {
            mClusters.clear();
            auto& halfHeights = mCache.halfHeights;

            for (auto& comp : mComponents) {
                halfHeights.clear();
                int index = comp.first;
                Strip* firstStrip = &mStrips[index];
                Strip* strip = firstStrip;
                int numStrips = 0;

                while (true) {
                    numStrips++;
                    halfHeights.push_back(strip->halfDims.height);
                    if (next(*strip) == Special::END) break;
                    index = next(*strip);
                    strip = &mStrips[index];
                }

                // condition: a cluster must have enough strips, otherwise it is ignored
                if (numStrips < mCfg.minStripsInComponent) continue;

                // analyze the half-heights
                std::sort(begin(halfHeights), end(halfHeights));
                auto q20 = begin(halfHeights) + (halfHeights.size() / 5);
                auto q50 = begin(halfHeights) + (halfHeights.size() / 2);
                auto q80 = begin(halfHeights) + (4 * halfHeights.size() / 5);

                // condition: q80/q20 of heights must not exceed a value
                if (float(*q80) / float(*q20) > mCfg.maxHeightRatioInternal) continue;

                // add a new cluster
                mClusters.emplace_back();
                auto& cluster = mClusters.back();
                cluster.l.strip = comp.first;
                cluster.r.strip = index;
                cluster.l.pos = firstStrip->pos;
                cluster.r.pos = strip->pos;
                cluster.numStrips = numStrips;
                cluster.approxHeightMin = float(*q50);
                cluster.approxHeightMax = cluster.approxHeightMin;
                cluster.lengthTotal = distL2(cluster.l.pos, cluster.r.pos);
                cluster.lengthGaps = 0.f;
            }

            if (mClusters.size() > size_t(Agglomerator::safetyMaxNumClusters)) {
                // if there is way too many clusters, ignore this frame entirely
                mClusters.clear();
                return;
            }
        };

        // evaluate the viability of merging clusters i and j
        auto score = [this](int i, int j) {
            const Cluster* l = &mClusters[i];
            const Cluster* r = &mClusters[j];
            if (l->l.pos.x > r->l.pos.x) std::swap(l, r);

            // condition: one cluster must end before the other begins
            if (l->r.pos.x >= r->l.pos.x) { return Agglomerator::infDist; }

            // condition: approximate heights must be consistent for all components
            float maxHeight = std::max(l->approxHeightMax, r->approxHeightMax);
            float minHeight = std::min(l->approxHeightMin, r->approxHeightMin);
            float heightScore = maxHeight / minHeight;
            if (heightScore > mCfg.maxHeightRatioExternal) { return Agglomerator::infDist; }

            // condition: distance must not exceed a given multiple of height
            float dist = distL2(l->r.pos, r->l.pos);
            float distanceScore = dist / minHeight;
            if (distanceScore > mCfg.maxDistance) { return Agglomerator::infDist; }

            // condition: gaps must not constitute more than a given fraction of total length
            float gapLen = l->lengthGaps + r->lengthGaps + dist;
            float totalLen = l->lengthTotal + r->lengthTotal + dist;
            float gapScore = gapLen / totalLen;
            if (gapScore > mCfg.maxGapsLength) { return Agglomerator::infDist; }

            // all conditions passed: calculate a score
            float score = mCfg.heightRatioWeight * (heightScore / mCfg.maxHeightRatioExternal);
            score += mCfg.distanceWeight * (distanceScore / mCfg.maxDistance);
            score += mCfg.gapsWeight * (gapScore / mCfg.maxGapsLength);
            return score;
        };

        // merge clusters i and j into cluster i
        auto mergeClusters = [this](int i, int j) {
            Cluster& cluster = mClusters[i];
            Cluster& other = mClusters[j];
            Cluster* l = &cluster;
            Cluster* r = &other;
            if (l->l.pos.x > r->l.pos.x) std::swap(l, r);
            float dist = distL2(l->r.pos, r->l.pos);
            next(mStrips[l->r.strip]) = int16_t(r->l.strip); // interconnect strips
            cluster.l = l->l;
            cluster.r = r->r;
            cluster.numStrips = l->numStrips + r->numStrips;
            cluster.approxHeightMin = std::min(l->approxHeightMin, r->approxHeightMin);
            cluster.approxHeightMax = std::max(l->approxHeightMax, r->approxHeightMax);
            cluster.lengthTotal = l->lengthTotal + r->lengthTotal + dist;
            cluster.lengthGaps = l->lengthGaps + r->lengthGaps + dist;
            other.setInvalid(Cluster::MERGED);
        };

        // run agglomerator algorithm
        makeInitialClusters();
        mAggl(score, mergeClusters, int16_t(mClusters.size()));

        // invalidate clusters based on additional criteria
        for (auto& cluster : mClusters) {
            // already invalid: ignore
            if (cluster.isInvalid()) continue;

            // too few strips
            if (cluster.numStrips < mCfg.minStripsInCluster) {
                cluster.setInvalid(Cluster::TOO_FEW_STRIPS);
            }
            // too short
            float len = cluster.lengthTotal / (2 * cluster.approxHeightMax);
            if (len < mCfg.minClusterLength) { cluster.setInvalid(Cluster::TOO_SHORT); }
        }
    }
}
