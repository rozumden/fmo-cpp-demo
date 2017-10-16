#include "../include-opencv.hpp"
#include "algorithm-taxonomy.hpp"
#include <fmo/algorithm.hpp>
#include <fmo/assert.hpp>

namespace fmo {
    Bounds TaxonomyV1::getBounds(const Object& o) const {
        float sz = o.length + o.radius + 5;
        sz *= 5;
        cv::Point2f cnt{float(o.center.x), float(o.center.y)};
        cv::Point2f aMax{float(o.center.x) + sz, float(o.center.y) + sz};
        cv::Point2f aMin{float(o.center.x) - sz, float(o.center.y) - sz};

        Bounds b{{int(aMin.x), int(aMin.y)}, {int(aMax.x), int(aMax.y)}};

        b.min.x = std::max(b.min.x, 0);
        b.min.y = std::max(b.min.y, 0);
        b.max.x = std::min(b.max.x, mSourceLevel.image.dims().width - 1);
        b.max.y = std::min(b.max.y, mSourceLevel.image.dims().height - 1);
        return b;
    }

    void TaxonomyV1::getOutput(Output &out, bool smoothTrajecotry) {
        out.clear();
        Detection::Object detObj;
         for (auto& o : mObjects) {
            detObj.center = o.center;
            detObj.direction[0] = o.direction.y;
            detObj.direction[1] = o.direction.x;
            detObj.length = 4.f * o.length;

            if(smoothTrajecotry) {
                detObj.curve = o.curveSmooth->clone();
            } else {
                detObj.curve = o.curve->clone();
            }
            detObj.curve->scale = mProcessingLevel.scale;

            detObj.scale = mProcessingLevel.scale;
            detObj.radius = o.radius;
            detObj.velocity = o.velocity;

            out.detections.emplace_back();
            out.detections.back().reset(new MyDetection(detObj, &o, this));
        }
    }

    TaxonomyV1::MyDetection::MyDetection(const Detection::Object& detObj,
                                       const TaxonomyV1::Object* obj, TaxonomyV1* aMe)
        : Detection(detObj, Detection::Predecessor()), me(aMe), mObj(obj) {}

    void TaxonomyV1::MyDetection::getPoints(PointSet& out) const {
        // adjust rasterized object size
        int thickness = int(roundf(2.f * object.radius));

        // rasterize the object into a temporary buffer
        Bounds b = me->getBounds(*mObj);
        Dims dims{b.max.x - b.min.x + 1, b.max.y - b.min.y + 1};
        auto& temp = me->mCache.pointsRaster;
        temp.resize(Format::GRAY, dims);

        // cv::Point2f center{float(mObj->center.x - b.min.x), float(mObj->center.y - b.min.y)};

        cv::Mat buf = temp.wrap();
        buf.setTo(uint8_t(0x00));
        
        object.curve->shift = {(float)b.min.x,(float)b.min.y};
        object.curve->draw(buf, 0xFF, thickness);
        object.curve->shift = {0,0};

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
