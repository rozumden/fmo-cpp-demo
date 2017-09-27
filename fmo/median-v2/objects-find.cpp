#include "../include-opencv.hpp"
#include "algorithm-median.hpp"

namespace fmo {
    void MedianV2::findObjects() {
        // reset
        mObjects[3].swap(mObjects[2]);
        mObjects[2].swap(mObjects[1]);
        mObjects[1].swap(mObjects[0]);
        mObjects[0].clear();

        const Dims dims = mSourceLevel.image.dims();
        const float imageArea = float(dims.width * dims.height);
        const int step = 1 << mProcessingLevel.pixelSizeLog2;

        // find interesting components
        for (auto& comp : mComponents) {
            int numStrips = 0;
            for (int16_t i = comp.first; i != Special::END; i = mNextStrip[i]) { numStrips++; }

            if (numStrips < mCfg.minStripsInObject) {
                // reject if there are too few strips in the object
                comp.status = Component::TOO_FEW_STRIPS;
                continue;
            }

            // gather points, compute strip area
            int stripArea = 0;
            mCache.lower.clear();
            mCache.upper.clear();
            for (int16_t i = comp.first; i != Special::END; i = mNextStrip[i]) {
                auto& strip = mStrips[i];
                int16_t x1 = strip.pos.x - strip.halfDims.width;
                int16_t x2 = strip.pos.x + strip.halfDims.width;
                int16_t y1 = strip.pos.y - strip.halfDims.height;
                int16_t y2 = strip.pos.y + strip.halfDims.height;
                mCache.lower.emplace_back(x1, y1);
                mCache.lower.emplace_back(x2, y1);
                mCache.upper.emplace_back(x1, y2);
                mCache.upper.emplace_back(x2, y2);
                stripArea += 4 * strip.halfDims.width * strip.halfDims.height;
            }

            // compute convex hull
            auto hullLower = [](const std::vector<Pos16>& src, std::vector<Pos16>& dst) {
                dst.clear();
                for (auto& pos : src) {
                    dst.push_back(pos);
                    size_t sz;
                    while ((sz = dst.size()) >= 3 &&
                           !fmo::left(dst[sz - 2] - dst[sz - 3], dst[sz - 1] - dst[sz - 2])) {
                        dst[sz - 2] = dst[sz - 1];
                        dst.pop_back();
                    }
                }
            };
            auto hullUpper = [] (const std::vector<Pos16>& src, std::vector<Pos16>& dst) {
                dst.clear();
                for (auto& pos : src) {
                    dst.push_back(pos);
                    size_t sz;
                    while ((sz = dst.size()) >= 3 &&
                        !fmo::right(dst[sz - 2] - dst[sz - 3], dst[sz - 1] - dst[sz - 2])) {
                        dst[sz - 2] = dst[sz - 1];
                        dst.pop_back();
                    }
                }
            };

            hullLower(mCache.lower, mCache.temp);
            mCache.temp.swap(mCache.lower);
            hullUpper(mCache.upper, mCache.temp);
            mCache.temp.swap(mCache.upper);

            // compute convex hull area
            auto integrate = [](const std::vector<Pos16>& src) {
                if (src.empty()) return 0;
                int area = 0;
                Pos16 prev = src[0];
                for (Pos16 pos : src) {
                    int dx = pos.x - prev.x;
                    int dy = (pos.y + prev.y) / 2;
                    area += dx * dy;
                    prev = pos;
                }
                return area;
            };

            Object o;
            o.area = float(integrate(mCache.upper) - integrate(mCache.lower));

            if (float(stripArea) / o.area < mCfg.minStripArea) {
                // reject if strips occupy too small a fraction of the total convex hull
                // area
                comp.status = Component::SMALL_STRIP_AREA;
                continue;
            }

            if (o.area / imageArea > (1.f / 8.f)) {
                // reject if the convex hull area is unreasonably large
                comp.status = Component::WAY_TOO_LARGE;
                continue;
            }

            // sample points on the hull boundary
            auto sampleCurve = [step = int16_t(step)](const std::vector<Pos16>& src, std::vector<Pos16>& dst) {
                dst.clear();
                if (src.empty()) return;
                Pos16 prev = src[0];
                int16_t x = src[0].x;

                for (Pos16 pos : src) {
                    while (x < pos.x) {
                        float k = float(pos.y - prev.y) / float(pos.x - prev.x);
                        float y = prev.y + k * float(x - prev.x);
                        dst.emplace_back(x, int16_t(y));
                        x += step;
                    }
                    dst.push_back(pos);
                    x += step;
                    prev = pos;
                }
            };

            sampleCurve(mCache.lower, mCache.temp);
            mCache.temp.swap(mCache.lower);
            sampleCurve(mCache.upper, mCache.temp);
            mCache.temp.swap(mCache.upper);

            // sample points in the hull interior
            auto forEachPoint = [step, this](auto func) {
                size_t iEnd = mCache.lower.size();
                for (size_t i = 0; i < iEnd; i++) {
                    int x = mCache.lower[i].x;
                    int y = ((mCache.lower[i].y + step - 1) / step) * step;
                    int yEnd = mCache.upper[i].y;
                    for (; y <= yEnd; y += step) { func(x, y); }
                }
            };

            mCache.temp.clear();
            forEachPoint(
                [this](int x, int y) { mCache.temp.emplace_back(int16_t(x), int16_t(y)); });

            // find object center
            int N = 0;
            forEachPoint([&](int x, int y) {
                o.center.x += x;
                o.center.y += y;
                N++;
            });
            o.center.x /= N;
            o.center.y /= N;

            // find covariance matrix
            int covXX = 0;
            int covXY = 0;
            int covYY = 0;
            forEachPoint([&](int x, int y) {
                int x_ = x - o.center.x;
                int y_ = y - o.center.y;
                covXX += x_ * x_;
                covXY += x_ * y_;
                covYY += y_ * y_;
            });
            float cov[4] = {covXX / float(N), covXY / float(N), 0, covYY / float(N)};
            cov[2] = cov[1];

            // find eigenvectors and eigenvalues
            float vals[2];
            float vecs[4];
            cv::Mat covMat{cv::Size{2, 2}, CV_32FC1, &cov};
            cv::Mat valsMat{cv::Size{1, 2}, CV_32FC1, &vals};
            cv::Mat vecsMat{cv::Size{2, 2}, CV_32FC1, &vecs};
            cv::eigen(covMat, valsMat, vecsMat);

            // determine object size and aspect
            o.halfLen[0] = sqrtf(3.f * vals[0]);
            o.halfLen[1] = sqrtf(3.f * vals[1]);
            o.aspect = o.halfLen[0] / o.halfLen[1];

            if (o.aspect < mCfg.minAspect) {
                // reject objects that are too round
                comp.status = Component::SMALL_ASPECT;
                continue;
            }

            // find the closest distance to an object in T-2, weighted length of the older object
            float closestTMinus2 = std::numeric_limits<float>::infinity();
            for (auto& o2 : mObjects[2]) {
                float distTMinus2 = length(o.center - o2.center) / (o2.halfLen[0]);
                closestTMinus2 = std::min(closestTMinus2, distTMinus2);
            }
            if (closestTMinus2 < mCfg.minDistToTMinus2) {
                // reject if too close to an object found in T-2
                comp.status = Component::CLOSE_TO_T_MINUS_2;
                continue;
            }

            // determine object direction
            o.direction.x = vecs[0];
            o.direction.y = vecs[1];
            if (o.direction.x < 0) {
                o.direction.x = -o.direction.x;
                o.direction.y = -o.direction.y;
            }

            // no problems encountered: add object
            o.id = mProcessingLevel.objectCounter++;
            mObjects[0].push_back(o);
            comp.status = Component::GOOD;
        }
    }
}
