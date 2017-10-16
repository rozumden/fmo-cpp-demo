#include "../include-opencv.hpp"
#include "algorithm-median.hpp"

namespace fmo {
    Bounds MedianV1::getBounds(const Object& o) const {
        NormVector perp = perpendicular(o.direction);
        cv::Point2f a1{o.direction.x, o.direction.y};
        cv::Point2f a2{perp.x, perp.y};
        a1 *= o.halfLen[0];
        a2 *= o.halfLen[1];
        cv::Point2f aMax = {std::max(a1.x, -a1.x) + std::max(a2.x, -a2.x),
                            std::max(a1.y, -a1.y) + std::max(a2.y, -a2.y)};
        cv::Point2f aMin = {std::min(a1.x, -a1.x) + std::min(a2.x, -a2.x),
                            std::min(a1.y, -a1.y) + std::min(a2.y, -a2.y)};
        cv::Point2f cnt{float(o.center.x), float(o.center.y)};
        aMax += cnt;
        aMin += cnt;
        Bounds b{{int(aMin.x), int(aMin.y)}, {int(aMax.x), int(aMax.y)}};
        b.min.x = std::max(b.min.x, 0);
        b.min.y = std::max(b.min.y, 0);
        b.max.x = std::min(b.max.x, mSourceLevel.image.dims().width - 1);
        b.max.y = std::min(b.max.y, mSourceLevel.image.dims().height - 1);
        return b;
    }

    void MedianV1::getOutput(Output &out, bool smoothTrajecotry) {
        out.clear();
        Detection::Predecessor detPrev;
        Detection::Object detObj;
        float radiusCorr = mCfg.outputRadiusCorr * float(1 << (mProcessingLevel.pixelSizeLog2 - 1));
        constexpr int outputLag = 2;

        for (auto& o : mObjects[outputLag]) {
            if (!o.selected) { continue; }

            detObj.id = o.id;
            detObj.center = o.center;
            detObj.direction[0] = o.direction.x;
            detObj.direction[1] = o.direction.y;
            detObj.length = 2.f * o.halfLen[0];

            float radii[3];
            radii[0] = o.halfLen[1];
            int numRadii = 1;

            // fill in information about the predecessor
            float velocityDistance = 0;
            int velocityNumFrames = 0;
            Object* oPrev = nullptr;
            if (o.prev != Special::END) {
                oPrev = &mObjects[outputLag + 1][o.prev];
                detPrev.id = oPrev->id;
                detPrev.center = oPrev->center;
                velocityDistance += length(oPrev->center - o.center);
                velocityNumFrames++;
                radii[numRadii] = oPrev->halfLen[1];
                numRadii++;
            } else {
                detPrev = Detection::Predecessor{};
            }

            // ditto about the successor
            Object* oNext = nullptr;
            if (o.next != Special::END) {
                oNext = &mObjects[outputLag - 1][o.next];
                velocityDistance += length(oNext->center - o.center);
                velocityNumFrames++;
                radii[numRadii] = oNext->halfLen[1];
                numRadii++;
            }

            // estimate radius
            if (mCfg.outputNoRobustRadius || numRadii == 1) {
                detObj.radius = radii[0];
            } else if (numRadii == 2) {
                detObj.radius = 0.5f * (radii[0] + radii[1]);
            } else { // numRadii == 3
                auto med3 = [] (float a, float b, float c) {
                    if (a > b) std::swap(a, b);
                    b = std::min(b, c);
                    return std::max(a, b);
                };
                detObj.radius = med3(radii[0], radii[1], radii[2]);
            }

            // apply radius correction
            detObj.radius -= radiusCorr;
            detObj.radius = std::max(detObj.radius, mCfg.outputRadiusMin);

            // calculate velocity, average over two frames if there are both neighbors
            detObj.velocity = velocityDistance / float(velocityNumFrames);

            out.detections.emplace_back();
            out.detections.back().reset(new MyDetection(detObj, detPrev, &o, this));
        }
    }

    MedianV1::MyDetection::MyDetection(const Detection::Object& detObj,
                                       const Detection::Predecessor& detPrev,
                                       const MedianV1::Object* obj, MedianV1* aMe)
        : Detection(detObj, detPrev), me(aMe), mObj(obj) {}

    void MedianV1::MyDetection::getPoints(PointSet& out) const {
        // adjust rasterized object size
        float rasterSize = object.radius - me->mCfg.outputRasterCorr;
        rasterSize = std::max(rasterSize, me->mCfg.outputRadiusMin);
        int thickness = int(roundf(2.f * rasterSize));

        // rasterize the object into a temporary buffer
        Bounds b = me->getBounds(*mObj);
        Dims dims{b.max.x - b.min.x + 1, b.max.y - b.min.y + 1};
        auto& temp = me->mCache.pointsRaster;
        temp.resize(Format::GRAY, dims);
        cv::Point2f center{float(mObj->center.x - b.min.x), float(mObj->center.y - b.min.y)};
        cv::Point2f a{mObj->direction.x, mObj->direction.y};
        a *= (mObj->halfLen[0] - mObj->halfLen[1]);
        cv::Point2f p1 = center - a;
        cv::Point2f p2 = center + a;
        cv::Mat buf = temp.wrap();
        buf.setTo(uint8_t(0x00));
        cv::line(buf, p1, p2, 0xFF, thickness);

        // output non-zero points
        out.clear();
        const uint8_t* data = temp.data();
        for (int y = b.min.y; y <= b.max.y; y++) {
            for (int x = b.min.x; x <= b.max.x; x++, data++) {
                if (*data != 0) { out.push_back(Pos{x, y}); }
            }
        }

        // no need to sort the points, they are already sorted according to pointSetCompLt()
    }
}
