#include "explorer.hpp"

namespace fmo {
    void ExplorerV2::findStrips() {
        if (mFrameNum >= 3) {
            mStrips.clear();
            findStrips(mLevel);
        }
    }

    void ExplorerV2::findStrips(ProcessedLevel& level) {
        Dims dims = level.preprocessed.dims();
        int minHeight = mCfg.minStripHeight;
        int minGap = int(mCfg.minGapY * dims.height);
        int step = level.step;
        int outNoise;
        mStripGen(level.preprocessed, minHeight, minGap, step, mStrips, outNoise);
        mDiff.reportAmountOfNoise(outNoise);

        // set next strip in component to a special value
        for (auto& strip : mStrips) { next(strip) = Special::UNTOUCHED; }
    }
}
