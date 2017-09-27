#include "image-util.hpp"
#include <fmo/subsampler.hpp>
#include <fmo/processing.hpp>

namespace fmo {
    void Subsampler::operator()(const Mat& src, Mat& dst) {
        if (src.format() != Format::YUV420SP) {
            subsample(src, dst);
            return;
        }

        // prepare output buffers
        Dims srcDims = src.dims();
        Dims dstDims = {srcDims.width / 2, srcDims.height / 2};
        cv::Size cvSrcSize{srcDims.width, srcDims.height};
        cv::Size cvDstSize{dstDims.width, dstDims.height};
        dst.resize(Format::YUV, dstDims);
        y.resize(Format::GRAY, dstDims);
        u.resize(Format::GRAY, dstDims);
        v.resize(Format::GRAY, dstDims);
        cv::Mat cvDst[3] = {y.wrap(), u.wrap(), v.wrap()};

        // create Y channel by decimation
        cv::Mat cvSrcY{cvSrcSize, CV_8UC1, const_cast<uint8_t*>(src.data())};
        cv::resize(cvSrcY, cvDst[0], cvDstSize, 0, 0, cv::INTER_AREA);

        // create channels U, V by splitting
        cv::Mat cvSrcUV{cvDstSize, CV_8UC2, const_cast<uint8_t*>(src.uvData())};
        cv::split(cvSrcUV, cvDst + 1);

        // create the result by merging
        cv::merge(cvDst, 3, dst.wrap());
    }

    Dims Subsampler::nextDims(Dims dims) {
        dims.width /= 2;
        dims.height /= 2;
        return dims;
    }

    Format Subsampler::nextFormat(Format before) {
        if (before == Format::YUV420SP) return Format::YUV;
        return before;
    }
}
