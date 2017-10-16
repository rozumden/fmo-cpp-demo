#include "algorithm-taxonomy.hpp"
#include "../include-opencv.hpp"
#include <fmo/processing.hpp>
#include <iostream>
#include <fmo/assert.hpp>

namespace fmo {
    void registerTaxonomyV1() {
        Algorithm::registerFactory(
            "taxonomy-v1", [](const Algorithm::Config& config, Format format, Dims dims) {
                return std::unique_ptr<Algorithm>(new TaxonomyV1(config, format, dims));
            });
    }

    TaxonomyV1::TaxonomyV1(const Config& cfg, Format format, Dims dims)
        : mCfg(cfg), mSourceLevel{{format, dims}, 0}, mDiff(cfg.diff) {
            auto& level = mProcessingLevel;
            int w = std::round(dims.width * ((float)mCfg.imageHeight / dims.height));
            level.dims = {dims};
            level.newDims = {w, mCfg.imageHeight};
            level.binDiff.resize(Format::GRAY, level.newDims);
            level.binDiff.wrap().setTo(0);

            level.diffAcc.resize(Format::GRAY, level.newDims);
            level.diffAcc.wrap().setTo(0);

            level.diff.resize(Format::BGR, level.newDims);
            level.diff.wrap().setTo(0);

            level.background.resize(Format::BGR, level.newDims);
            level.background.wrap().setTo(0);

            level.labels.resize(Format::INT32, level.newDims);
            level.distTran.resize(Format::FLOAT, level.newDims);
            level.localMaxima.resize(Format::GRAY, level.newDims);

            mCache.distTranReverse.resize(Format::FLOAT, level.newDims);
            mCache.distTranGray.resize(Format::GRAY, level.newDims);
            mCache.distTranBGR.resize(Format::BGR, level.newDims);
            mCache.visualized.resize(Format::BGR, level.newDims);
            mCache.visualizedFull.resize(Format::BGR, dims);
            mCache.objectsMask.resize(Format::BGR, dims);
            mCache.ones.resize(Format::GRAY, level.newDims);
            mCache.ones.wrap().setTo(1);

    }

    void TaxonomyV1::setInputSwap(Image& in) {
        swapAndSubsampleInput(in);
        computeBinDiff();
        findComponents();
        processComponents();
    }

    void TaxonomyV1::swapAndSubsampleInput(Image& in) {
        if (in.format() != mSourceLevel.image.format()) {
            throw std::runtime_error("setInputSwap(): bad format");
        }

        if (in.dims() != mSourceLevel.image.dims()) {
            throw std::runtime_error("setInputSwap(): bad dimensions");
        }

        mSourceLevel.image.swap(in);
        mSourceLevel.frameNum++;

        // resize an image to exact height
        mProcessingLevel.scale = (float) mCfg.imageHeight / in.dims().height; 
        subsample_resize(mSourceLevel.image, mCache.image, mProcessingLevel.scale);

        // swap the product of decimation into the processing level
        mProcessingLevel.inputs[3].swap(mProcessingLevel.inputs[2]);
        mProcessingLevel.inputs[2].swap(mProcessingLevel.inputs[1]);
        mProcessingLevel.inputs[1].swap(mProcessingLevel.inputs[0]);
        mProcessingLevel.inputs[0].swap(mCache.image);

    }

    void TaxonomyV1::computeBinDiff() {
        auto& level = mProcessingLevel;

        if (mSourceLevel.frameNum < 5) {
            // initial frames
            return;
        }

        fmo::median5(level.inputs[0], level.inputs[1], level.inputs[2], 
            level.inputs[3], level.background, level.background);

//        level.diff.wrap() = abs(level.inputs[0].wrap() - level.background.wrap());
//        cv::transform(level.diff.wrap(), level.diffAcc.wrap(), cv::Matx13f(1,1,1));
        level.binDiffPrev.swap(level.binDiff);
        mDiff(level.inputs[0], level.background, level.binDiff);
    }
}
