#include "explorer.hpp"
#include <algorithm>
#include <cmath>
#include <fmo/algebra.hpp>

namespace fmo {
    void ExplorerV1::findTrajectories() {
        mTrajectories.clear();

        int numComponents = int(mComponents.size());
        // not tested: it is assumed that the components are sorted by the x coordinate of the
        // leftmost strip

        for (int i = 0; i < numComponents; i++) {
            Component& me = mComponents[i];

            if (me.trajectory == Component::NO_TRAJECTORY) {
                // if the component does not have a trajectory yet, add a new one
                me.trajectory = int16_t(mTrajectories.size());
                mTrajectories.emplace_back(i);
            }

            Trajectory& myTrajectory = mTrajectories[me.trajectory];
            Strip& myFirst = mStrips[me.first];
            Strip& myLast = mStrips[me.last];
            int myWidth = myLast.x - myFirst.x;
            myTrajectory.maxWidth = int16_t(std::max(int(myTrajectory.maxWidth), myWidth));

            me.next = Component::NO_COMPONENT;
            for (int j = i + 1; j < numComponents; j++) {
                Component& candidate = mComponents[j];
                Strip& candFirst = mStrips[candidate.first];

                // condition: candidate must not be farther than max component width so far
                int dx = candFirst.x - myLast.x;
                if (dx > myTrajectory.maxWidth) break; // sorted by x => may end loop

                // condition: candidate must not be part of another trajectory
                if (candidate.trajectory != Component::NO_TRAJECTORY) continue;

                // condition: candidate must begin after this component has ended
                // condition: angle must not exceed ~63 degrees
                int dy = (candFirst.y > myLast.y) ? (candFirst.y - myLast.y) : (myLast.y - candFirst.y);
                if (dy > 2 * dx) continue;

                // condition: candidate must have a consistent approximate height
                if (me.approxHalfHeight > 2 * candidate.approxHalfHeight ||
                    candidate.approxHalfHeight > 2 * me.approxHalfHeight)
                    continue;

                candidate.trajectory = me.trajectory;
                me.next = int16_t(j);
                break;
            }
        }
    }

    void ExplorerV1::analyzeTrajectories() {
        for (auto& traj : mTrajectories) {
            // iterate over components, sum strips, find last component
            int numStrips = 0;
            Component* firstComp = &mComponents[traj.first];
            int index = traj.first;
            Component* comp = firstComp;

            while (true) {
                numStrips += comp->numStrips;
                if (comp->next == Component::NO_COMPONENT) break;
                index = comp->next;
                comp = &mComponents[index];
            }

            traj.last = int16_t(index);
            traj.numStrips = int16_t(numStrips);
        }
    }
}
