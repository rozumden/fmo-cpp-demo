#include "algorithm-median.hpp"
#include <limits>

namespace fmo {
    namespace {
        constexpr float inf = std::numeric_limits<float>::infinity();
    }

    void MedianV2::matchObjects() {
        auto score = [this](const Object& o1, const Object& o2) {
            float aspect = std::max(o1.aspect, o2.aspect) / std::min(o1.aspect, o2.aspect);
            if (aspect > mCfg.matchAspectMax) {
                // maximum aspect difference: discard if the shape differs
                return inf;
            }

            float area = std::max(o1.area, o2.area) / std::min(o1.area, o2.area);
            if (area > mCfg.matchAreaMax) {
                // maximum area difference: discard if size differs
                return inf;
            }

            float sumOfHalfLengths = o1.halfLen[0] + o2.halfLen[0];
            Vector motion = o2.center - o1.center;
            float distance = length(motion) / sumOfHalfLengths;
            if (distance < mCfg.matchDistanceMin || distance > mCfg.matchDistanceMax) {
                // bad distance
                return inf;
            }

            bool angleGood1 = o1.aspect > mCfg.minAspectForRelevantAngle;
            bool angleGood2 = o2.aspect > mCfg.minAspectForRelevantAngle;
            float angle = 1.f;

            if (angleGood1 && angleGood2) {
                NormVector motionDirection{motion};
                float sin1 = abs(cross(motionDirection, o1.direction));
                float sin2 = abs(cross(motionDirection, o2.direction));
                angle = std::max(sin1, sin2);

                if (angle > mCfg.matchAngleMax) {
                    // maximum angle: discard if not on a line
                    return inf;
                }
            }

            float result = 0;
            result += mCfg.matchAspectWeight * aspect;
            result += mCfg.matchAreaWeight * area;
            result += mCfg.matchDistanceWeight * distance;
            result += mCfg.matchAngleWeight * angle;
            return result;
        };

        int ends[2] = {int(mObjects[0].size()), int(mObjects[1].size())};
        mCache.matches.clear();
        mCache.matches.reserve(ends[0] * ends[1]);

        for (int i = 0; i < ends[0]; i++) {
            for (int j = 0; j < ends[1]; j++) {
                float aScore = score(mObjects[0][i], mObjects[1][j]);
                if (aScore < inf) {
                    Match m{aScore, {int16_t(i), int16_t(j)}};
                    mCache.matches.push_back(m);
                }
            }
        }

        while (!mCache.matches.empty()) {
            // select the best match
            float bestScore = inf;
            Match* bestMatch = nullptr;
            for (auto& match : mCache.matches) {
                if (match.score < bestScore) {
                    bestScore = match.score;
                    bestMatch = &match;
                }
            }
            Match selected = *bestMatch;

            // remove matches that involve the newly matched objects
            auto last = std::remove_if(begin(mCache.matches), end(mCache.matches),
                                       [selected](const Match& m) {
                                           return m.objects[0] == selected.objects[0] ||
                                                  m.objects[1] == selected.objects[1];
                                       });
            mCache.matches.erase(last, end(mCache.matches));

            // save the match
            mObjects[0][selected.objects[0]].prev = selected.objects[1];
            mObjects[1][selected.objects[1]].next = selected.objects[0];
        }
    }
}
