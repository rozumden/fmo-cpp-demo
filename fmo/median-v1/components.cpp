#include "algorithm-median.hpp"
#include <limits>

namespace fmo {
    void MedianV1::findComponents() {
        auto& input = mProcessingLevel.binDiff;
        const int minHeight = mCfg.minStripHeight;
        const int minGapY = std::max(1, int(mCfg.minGapY * input.dims().height));
        const int step = 1 << mProcessingLevel.pixelSizeLog2;
        int outNoise;
        mStripGen(input, minHeight, minGapY, step, mStrips, outNoise);
        mDiff.reportAmountOfNoise(outNoise);

        // sort strips by x coordinate
        std::sort(begin(mStrips), end(mStrips), [](const Strip& l, const Strip& r) {
            if (l.pos.x == r.pos.x) {
                return l.pos.y < r.pos.y;
            } else {
                return l.pos.x < r.pos.x;
            }
        });

        // sanity check: strips must be addressable with int16_t
        constexpr size_t int16Max = size_t(std::numeric_limits<int16_t>::max());
        if (mStrips.size() > int16Max) {
            mStrips.erase(mStrips.begin() + int16Max, mStrips.end());
        }

        // reset components
        mNextStrip.clear();
        mNextStrip.resize(mStrips.size(), Special::UNTOUCHED);
        mComponents.clear();

        const int maxGapX = step * std::max(1, int(mCfg.maxGapX * input.dims().height));
        const int iEnd = int(mStrips.size());

        for (int i = 0; i < iEnd; i++) {
            Strip& me = mStrips[i];
            auto& meNext = mNextStrip[i];

            // create new components for previously untouched strips
            if (meNext == Special::UNTOUCHED) { mComponents.emplace_back(int16_t(i)); }

            // find the next strip in component
            meNext = Special::END;
            for (int j = i + 1; j < iEnd; j++) {
                Strip& them = mStrips[j];

                if (them.pos.x > me.pos.x + maxGapX) break;
                if (Strip::overlapY(me, them)) {
                    auto& themNext = mNextStrip[j];

                    if (themNext == Special::UNTOUCHED) {
                        meNext = int16_t(j);
                        themNext = Special::TOUCHED;
                    }

                    // only one overlapping candidate allowed
                    break;
                }
            }
        }
    }
}
