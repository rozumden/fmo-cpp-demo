#include "../include-opencv.hpp"
#include "explorer.hpp"
#include <fmo/processing.hpp>

namespace fmo {
    namespace {
        // inline cv::Point toCv(Pos p) { return {p.x, p.y}; }
        const cv::Scalar inactiveStripsColor{0x20, 0x20, 0x20};
        const cv::Scalar oldStripsColor{0xC0, 0x60, 0x00};
        const cv::Scalar stripsColor{0xC0, 0xC0, 0x00};
        const cv::Scalar newStripsColor{0x60, 0xC0, 0x00};
        const cv::Scalar tooFewStripsColor{0x00, 0x00, 0xC0};
        const cv::Scalar notAnObjectColor{0x00, 0x60, 0xC0};
        const cv::Scalar clusterConnectionColor{0x00, 0xC0, 0xC0};
    }

    void ExplorerV3::visualize() {
        // cover the visualization image with the latest input image
        copy(mSourceLevel.image1, mCache.visColor, Format::BGR);
        cv::Mat result = mCache.visColor.wrap();

        // scale the current diff to source size
        {
            mCache.visDiffGray.resize(Format::GRAY, mSourceLevel.dims);
            cv::Size cvSize{mSourceLevel.dims.width, mSourceLevel.dims.height};
            cv::resize(mLevel.preprocessed.wrap(), mCache.visDiffGray.wrap(), cvSize, 0, 0,
                       cv::INTER_NEAREST);
            copy(mCache.visDiffGray, mCache.visDiffColor, Format::BGR);
        }

        // blend diff with input image
        cv::addWeighted(mCache.visDiffColor.wrap(), 0.5, result, 0.5, 0, result);

        // draw meta-strips
        for (auto& strip : mLevel.metaStrips) {
            cv::Point p1{strip.pos.x - strip.halfDims.width, strip.pos.y - strip.halfDims.height};
            cv::Point p2{strip.pos.x + strip.halfDims.width, strip.pos.y + strip.halfDims.height};

            auto* color = &stripsColor;
            if (!strip.newer) color = &oldStripsColor;
            if (!strip.older) color = &newStripsColor;
            cv::rectangle(result, p1, p2, *color);
        }

        // draw clusters
        for (auto& cluster : mClusters) {
            const cv::Scalar* color = nullptr;

            if (cluster.isInvalid()) {
                if (cluster.whyInvalid() == Cluster::TOO_FEW_STRIPS ||
                    cluster.whyInvalid() == Cluster::TOO_SHORT) {
                    color = &tooFewStripsColor;
                } else if (cluster.whyInvalid() == Cluster::NOT_AN_OBJECT) {
                    color = &notAnObjectColor;
                }
            }

            auto* strip = &mLevel.metaStrips[cluster.l.strip];
            while (true) {
                // draw strip
                if (color != nullptr) {
                    cv::Point p1{strip->pos.x - strip->halfDims.width,
                                 strip->pos.y - strip->halfDims.height};
                    cv::Point p2{strip->pos.x + strip->halfDims.width,
                                 strip->pos.y + strip->halfDims.height};
                    cv::rectangle(result, p1, p2, *color);
                }

                if (strip->next == MetaStrip::END) {
                    break;
                } else {
                    auto* next = &mLevel.metaStrips[strip->next];

                    // draw an interconnection if needed
                    if (next->pos.x - strip->pos.x > mLevel.step ||
                        !Strip::overlapY(*strip, *next)) {
                        cv::Point p1{strip->pos.x, strip->pos.y};
                        cv::Point p2{next->pos.x, next->pos.y};
                        cv::line(result, p1, p2, clusterConnectionColor);
                    }

                    strip = next;
                }
            }
        }
    }
}
