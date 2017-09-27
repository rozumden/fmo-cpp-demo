#include "explorer.hpp"

namespace fmo {
    void ExplorerV3::findMetaStrips() {
        auto& out = mLevel.metaStrips;
        out.clear();

        // orders strips by their top edge
        auto stripComp = [](const ProtoStrip& l, const ProtoStrip& r) {
            if (l.pos.x == r.pos.x) {
                return (l.pos.y - l.halfDims.height) < (r.pos.y - r.halfDims.height);
            } else {
                return l.pos.x < r.pos.x;
            }
        };

        enum Situation { UNRELATED, INTERFERING, OVERLAPPING };

        // finds out the relationship between the two strips: unrelated if completely separate,
        // interfering if close but not overlapping, or overlapping
        int minGap = int(mCfg.minGapY * mLevel.diff1.dims().height);
        auto situation = [minGap](const ProtoStrip& l, const ProtoStrip& r) {
            if (l.pos.x != r.pos.x) return UNRELATED;
            int dy = (r.pos.y > l.pos.y) ? (r.pos.y - l.pos.y) : (l.pos.y - r.pos.y);
            int h = l.halfDims.height + r.halfDims.height;
            if (dy < h) return OVERLAPPING;
            if (dy < h + minGap) return INTERFERING;
            return UNRELATED;
        };

        // sort strips before merging
        std::sort(begin(mLevel.strips1), end(mLevel.strips1), stripComp);

        float maxRatio = mCfg.maxHeightRatioStrips;
        float minRatio = 1.f / maxRatio;
        using It = decltype(mLevel.strips1)::iterator;
        It newer = mLevel.strips1.begin();
        It older = mLevel.strips2.begin();

        while (newer != mLevel.strips1.end() && older != mLevel.strips2.end()) {
            Situation sit = situation(*newer, *older);

            if (sit == UNRELATED) {
                // no overlap: add the earlier proto-strip as a separate meta-strip
                if (stripComp(*newer, *older)) {
                    out.emplace_back(*newer, true);
                    newer++;
                } else {
                    out.emplace_back(*older, false);
                    older++;
                }
            } else if (sit == INTERFERING) {
                // no overlap but too close: ignore both
                newer++;
                older++;
            } else { // sit == OVERLAPPING
                // some overlap: compare heights
                float heightRatio = float(newer->halfDims.height) / float(older->halfDims.height);
                if (heightRatio < minRatio) {
                    newer++;
                } else if (heightRatio > maxRatio) {
                    older++;
                } else {
                    // heights are compatible: merge two proto-strips into one meta-strip
                    out.emplace_back(*newer, *older);
                    newer++;
                    older++;
                }
            }
        }

        while (newer != mLevel.strips1.end()) {
            out.emplace_back(*newer, true);
            newer++;
        }

        while (older != mLevel.strips2.end()) {
            out.emplace_back(*older, false);
            older++;
        }
    }
}
