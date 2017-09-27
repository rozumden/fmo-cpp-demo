#include "explorer.hpp"

namespace fmo {
    void ExplorerV1::findStrips() {
        if (mFrameNum >= 3) {
            mStrips.clear();
            findStrips(mLevel);
        }
    }

    void ExplorerV1::findStrips(ProcessedLevel& level) {
        level.numStrips = 0;
        Dims dims = level.preprocessed.dims();
        uint8_t* colData = level.preprocessed.data();

        int black2Prev = 0;
        int blackPrev = 0;
        int black = 0;
        int whitePrev = 0;
        int white = 0;
        int col = 0;
        int row = 0;
        int step = level.step;
        int halfStep = level.step / 2;
        int minGap = int(mCfg.minGapY * dims.height);

        // Called after a white strip has ended. Stores a strip if the previous two black and one
        // white strip satisfy all conditions.
        auto check = [&, this]() {
            if (black2Prev >= minGap && blackPrev >= minGap && whitePrev > 0) {
                int halfHeight = whitePrev * halfStep;
                int x = (col * step) + halfStep;
                int y = ((row - white - blackPrev) * step) - halfHeight;
                mStrips.emplace_back(int16_t(x), int16_t(y), int16_t(halfHeight));
                level.numStrips++;
            }
        };

        for (col = 0; col < dims.width; col++, colData++) {
            uint8_t* data = colData;
            black2Prev = 0;
            blackPrev = 0;
            black = minGap; // manipulate: don't limit by top edge
            whitePrev = 0;
            white = 0;

            for (row = 0; row < dims.height; row++, data += dims.width) {
                if (*data != 0) {
                    if (white++ == 0) {
                        black2Prev = blackPrev;
                        blackPrev = black;
                        black = 0;
                    }
                } else {
                    if (black++ == 0) {
                        if (white >= mCfg.minStripsInComponent) {
                            check();
                            whitePrev = white;
                            white = 0;
                        }
                        else {
                            black = blackPrev + white + black;
                            blackPrev = black2Prev;
                            black2Prev = 0;
                            white = 0;
                        }
                    }
                }
            }
            // manipulate: don't limit by bottom edge
            if (white != 0) {
                check();
            } else {
                black2Prev = blackPrev;
                blackPrev = black + minGap;
                white = 0;
                row = row + minGap;
                check();
            }
        }
    }
}
