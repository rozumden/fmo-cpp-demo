#include "explorer.hpp"

namespace fmo {
    void ExplorerV3::findProtoStrips() {
        if (mFrameNum < 2) return;
        mLevel.strips1.swap(mLevel.strips2);
        mLevel.strips1.clear();

        Dims dims = mLevel.diff1.dims();
        int minHeight = mCfg.minStripHeight;
        int minGap = int(mCfg.minGapY * dims.height);
        int step = mLevel.step;
        int outNoise = 0;
        mStripGen(mLevel.diff1, minHeight, minGap, step, mLevel.strips1, outNoise);
        mDiff.reportAmountOfNoise(outNoise);
    }
}
