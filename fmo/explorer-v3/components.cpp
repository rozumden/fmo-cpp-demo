#include "../include-opencv.hpp"
#include "explorer.hpp"
#include <algorithm>
#include <fmo/algebra.hpp>
#include <fmo/assert.hpp>

namespace fmo {
    namespace {
        constexpr int int16_max = std::numeric_limits<int16_t>::max();
    }

    void ExplorerV3::findComponents() {
        // reset
        mComponents.clear();
        int step = mLevel.step;
        auto& strips = mLevel.metaStrips;

        // sanity check: strips must be addressable with int16_t
        if (strips.size() > size_t(int16_max)) {
            strips.erase(strips.begin() + int16_max, strips.end());
        }

        // assumption: strips are sorted
        int end = int(strips.size());
        for (int i = 0; i < end; i++) {
            MetaStrip& me = strips[i];

            // create new components for previously untouched strips
            if (me.next == MetaStrip::UNTOUCHED) { mComponents.emplace_back(int16_t(i)); }

            // find next strip
            me.next = MetaStrip::END;
            for (int j = i + 1; j < end; j++) {
                MetaStrip& them = strips[j];

                if (them.pos.x == me.pos.x) continue;
                if (them.pos.x > me.pos.x + step) break;
                if (Strip::overlapY(me, them) && them.next == MetaStrip::UNTOUCHED) {
                    me.next = int16_t(j);
                    them.next = MetaStrip::TOUCHED;
                    break;
                }
            }
        }
    }
}
