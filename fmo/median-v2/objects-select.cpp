#include "algorithm-median.hpp"

namespace fmo {
    void MedianV2::selectObjects() {
        for (auto& o0 : mObjects[0]) {
            if (o0.prev == Special::END) {
                // no matched object in the previous frame
                continue;
            }
            auto& o1 = mObjects[1][o0.prev];
            if (o1.prev == Special::END) {
                // no matched object in the frame before the previous frame
                continue;
            }
            auto& o2 = mObjects[2][o1.prev];

            // ignore the triplet if it is not deemed a fast-moving object detection
            if (!selectable(o0, o1, o2)) continue;

            // everything is fine: mark objects as selected
            o0.selected = true;
            o1.selected = true;
            o2.selected = true;
        }
    }

    bool MedianV2::selectable(Object& o0, Object& o1, Object& o2) const {
        auto med3 = [](float a, float b, float c) {
            if (a > b) std::swap(a, b);
            b = std::min(b, c);
            return std::max(a, b);
        };

        Vector error{o0.center.x - 2 * o1.center.x + o2.center.x,
                     o0.center.y - 2 * o1.center.y + o2.center.y};
        float distance = length(error) / (2.f * med3(o0.halfLen[0], o1.halfLen[0], o2.halfLen[0]));
        if (distance > mCfg.selectMaxDistance) return false;
        return true;
    }
}
