#include "../include-opencv.hpp"
#include "algorithm-median.hpp"
#include <fmo/processing.hpp>

namespace fmo {
    namespace {
        const cv::Scalar colorDiscarded{0x00, 0x00, 0xC0};
        const cv::Scalar colorDiscardedTooFewStrips{0x00, 0x00, 0x60};
        const cv::Scalar colorDiscardedBadHull{0x00, 0x60, 0xC0};
        const cv::Scalar colorDiscardedSmallAspect{0x60, 0x00, 0xC0};
        const cv::Scalar colorGood{0x00, 0xC0, 0x00};
        const cv::Scalar colorObjects[3] = {
            {0x00, 0xC0, 0x00}, {0x00, 0x80, 0x00}, {0x00, 0x40, 0x00},
        };
        const cv::Scalar colorSelected{0x00, 0xC0, 0xC0};
    }

    const Image& MedianV1::getDebugImage() {
        // convert to BGR
        fmo::copy(mProcessingLevel.binDiff, mCache.diffConverted, Format::BGR);
        fmo::convert(mProcessingLevel.inputs[0], mCache.inputConverted, Format::BGR);

        // scale to source size
        cv::Mat cvDiff;
        cv::Mat cvVis;
        {
            mCache.diffScaled.resize(Format::BGR, mSourceLevel.image.dims());
            mCache.visualized.resize(Format::BGR, mSourceLevel.image.dims());
            cvDiff = mCache.diffScaled.wrap();
            cvVis = mCache.visualized.wrap();
            cv::resize(mCache.diffConverted.wrap(), cvDiff, cvDiff.size(), 0, 0, cv::INTER_NEAREST);
            cv::resize(mCache.inputConverted.wrap(), cvVis, cvVis.size(), 0, 0, cv::INTER_NEAREST);
        }

        // blend diff and input
        cv::addWeighted(cvDiff, 0.5, cvVis, 0.5, 0, cvVis);

        // draw components
        int step = 1 << mProcessingLevel.pixelSizeLog2;
        for (auto& comp : mComponents) {
            const cv::Scalar* color = &colorDiscarded;
            if (comp.status == Component::GOOD) { color = &colorGood; }
            if (comp.status == Component::TOO_FEW_STRIPS) { color = &colorDiscardedTooFewStrips; }
            if (comp.status == Component::SMALL_STRIP_AREA) { color = &colorDiscardedBadHull; }
            if (comp.status == Component::SMALL_ASPECT) { color = &colorDiscardedSmallAspect; }

            for (int16_t i = comp.first; i != Special::END; i = mNextStrip[i]) {
                Strip& l = mStrips[i];
                {
                    // draw the strip as a rectangle
                    cv::Point p1{l.pos.x - l.halfDims.width, l.pos.y - l.halfDims.height};
                    cv::Point p2{l.pos.x + l.halfDims.width, l.pos.y + l.halfDims.height};
                    cv::rectangle(cvVis, p1, p2, *color);
                }

                int16_t j = mNextStrip[i];
                if (j != Special::END) {
                    Strip& r = mStrips[j];

                    if (!Strip::inContact(l, r, step)) {
                        // draw an interconnection if in the same component but not touching
                        int x1 = l.pos.x + l.halfDims.width;
                        int x2 = r.pos.x - r.halfDims.width;
                        {
                            cv::Point p1{x1, l.pos.y - l.halfDims.height};
                            cv::Point p2{x2, r.pos.y - r.halfDims.height};
                            cv::line(cvVis, p1, p2, *color);
                        }
                        {
                            cv::Point p1{x1, l.pos.y + l.halfDims.height};
                            cv::Point p2{x2, r.pos.y + r.halfDims.height};
                            cv::line(cvVis, p1, p2, *color);
                        }
                    }
                }
            }
        }

        auto toCvPoint2f = [](NormVector nv) { return cv::Point2f{nv.x, nv.y}; };

        // draw objects
        for (int i = 2; i >= 0; i--) {
            for (auto& o : mObjects[i]) {
                const cv::Scalar* color = &colorObjects[i];
                if (o.selected) color = &colorSelected;

                cv::Point2f cnt{float(o.center.x), float(o.center.y)};
                cv::Point2f a1 = toCvPoint2f(o.direction);
                cv::Point2f a2 = toCvPoint2f(perpendicular(o.direction));
                a1 *= o.halfLen[0];
                a2 *= o.halfLen[1];
                cv::Point2f p1 = cnt + a1 - a2;
                cv::Point2f p2 = cnt + a1 + a2;
                cv::Point2f p3 = cnt - a1 + a2;
                cv::Point2f p4 = cnt - a1 - a2;
                cv::line(cvVis, p1, p2, *color);
                cv::line(cvVis, p2, p3, *color);
                cv::line(cvVis, p3, p4, *color);
                cv::line(cvVis, p4, p1, *color);

                if (i != 2 && o.prev != Special::END) {
                    auto& o2 = mObjects[i + 1][o.prev];
                    cv::Point2f cnt2{float(o2.center.x), float(o2.center.y)};
                    cv::line(cvVis, cnt, cnt2, *color);
                }
            }
        }

        return mCache.visualized;
    }
}
