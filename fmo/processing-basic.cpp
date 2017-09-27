#include "image-util.hpp"
#include <fmo/assert.hpp>
#include <fmo/processing.hpp>
#include <fmo/region.hpp>

namespace fmo {
    void save(const Mat& src, const std::string& filename) {
        cv::Mat srcMat = src.wrap();
        cv::imwrite(filename, srcMat);
    }

    void copy(const Mat& src, Mat& dst) {
        dst.resize(src.format(), src.dims());

        if (src.format() == Format::YUV420SP) {
            cv::Mat srcMat1 = yuv420SPWrapGray(src);
            cv::Mat srcMat2 = yuv420SPWrapUV(src);
            cv::Mat dstMat1 = yuv420SPWrapGray(dst);
            cv::Mat dstMat2 = yuv420SPWrapUV(dst);
            srcMat1.copyTo(dstMat1);
            srcMat2.copyTo(dstMat2);
        } else {
            cv::Mat srcMat = src.wrap();
            cv::Mat dstMat = dst.wrap();
            srcMat.copyTo(dstMat);
        }
    }

    void copy(const Mat& src, Mat& dst, Format format) {
        const auto srcFormat = src.format();
        const auto dims = src.dims();
        const auto dstFormat = format;

        if (src.data() == dst.data()) {
            throw std::runtime_error("copy: in-place conversions are not allowed");
        }

        if (srcFormat == dstFormat) {
            // no format change -- just copy
            copy(src, dst);
            return;
        }

        dst.resize(dstFormat, dims);
        auto grayCompatible = [](Format f) { return f == Format::GRAY || f == Format::YUV420SP; };
        auto bgrCompatible = [](Format f) { return f == Format::BGR || f == Format::YUV; };
        auto clearUV = [](Mat& mat) {
            if (mat.format() == Format::YUV420SP) { yuv420SPWrapUV(mat).setTo(0); }
        };

        if (grayCompatible(srcFormat) && grayCompatible(dstFormat)) {
            yuv420SPWrapGray(src).copyTo(yuv420SPWrapGray(dst));
            clearUV(dst);
            return;
        }

        if (grayCompatible(srcFormat) && bgrCompatible(dstFormat)) {
            cv::cvtColor(yuv420SPWrapGray(src), dst.wrap(), cv::COLOR_GRAY2BGR);
            return;
        }

        if (bgrCompatible(srcFormat) && grayCompatible(dstFormat)) {
            int channel = (srcFormat == Format::YUV) ? 0 : 1;
            cv::extractChannel(src.wrap(), yuv420SPWrapGray(src), channel);
            clearUV(dst);
            return;
        }

        if (bgrCompatible(srcFormat) && bgrCompatible(dstFormat)) {
            src.wrap().copyTo(dst.wrap());
            return;
        }

        throw std::runtime_error("copy: unsupported conversion");
    }

    void convert(const Mat& src, Mat& dst, Format format) {
        const auto srcFormat = src.format();
        const auto dstFormat = format;

        if (src.data() == dst.data()) {
            throw std::runtime_error("convert: in-place conversions are not allowed");
        }

        if (srcFormat == dstFormat) {
            // no format change -- just copy
            copy(src, dst);
            return;
        }

        enum { ERROR = -1 };
        int code = ERROR;

        dst.resize(dstFormat, src.dims()); // this is why we check for same instance
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        if (srcFormat == Format::BGR) {
            if (dstFormat == Format::GRAY) {
                code = cv::COLOR_BGR2GRAY;
            } else if (dstFormat == Format::YUV) {
                code = cv::COLOR_BGR2YCrCb;
            }
        } else if (srcFormat == Format::GRAY) {
            if (dstFormat == Format::BGR) { code = cv::COLOR_GRAY2BGR; }
        } else if (srcFormat == Format::YUV) {
            if (dstFormat == Format::BGR) {
                code = cv::COLOR_YCrCb2BGR;
            } else if (dstFormat == Format::GRAY) {
                cv::extractChannel(srcMat, dstMat, 0);
                return;
            }
        } else if (srcFormat == Format::YUV420SP) {
            if (dstFormat == Format::BGR) {
                code = cv::COLOR_YUV420sp2BGR;
            } else if (dstFormat == Format::GRAY) {
                code = cv::COLOR_YUV420sp2GRAY;
            }
        }

        if (code == ERROR) {
            throw std::runtime_error("convert: failed to perform color conversion");
        }

        cv::cvtColor(srcMat, dstMat, code);

        FMO_ASSERT(dstMat.data == dst.data(), "convert: dst buffer reallocated");
    }

    void less_than(const Mat& src, Mat& dst, uint8_t value) {
        if (src.format() != Format::GRAY && src.format() != Format::FLOAT) {
            throw std::runtime_error("less_than: input must be GRAY");
        }

        dst.resize(src.format(), src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        cv::threshold(srcMat, dstMat, value - 1, 0xFF, cv::THRESH_BINARY_INV);
        FMO_ASSERT(dstMat.data == dst.data(), "less_than: dst buffer reallocated");
    }

    void greater_than(const Mat& src, Mat& dst, uint8_t value) {
        if (src.format() != Format::GRAY && src.format() != Format::FLOAT) {
            throw std::runtime_error("greater_than: input must be GRAY or FLOAT");
        }

        dst.resize(src.format(), src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        cv::threshold(srcMat, dstMat, value, 0xFF, cv::THRESH_BINARY);
        FMO_ASSERT(dstMat.data == dst.data(), "greater_than: dst buffer reallocated");
    }

    void distance_transform(const Mat& src, Mat& dst) {
        dst.resize(Format::FLOAT, src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();
        
        cv::distanceTransform(srcMat, dstMat, cv::DIST_L2, cv::DIST_MASK_3);
        FMO_ASSERT(dstMat.data == dst.data(), "distance_transform: dst buffer reallocated");
    }

    void local_maxima(const Mat& src, Mat& dst) {
        dst.resize(Format::GRAY, src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();
        
        // non maxima suppression
        cv::Mat temp;
        cv::dilate(srcMat, temp, cv::Mat());
        dstMat = srcMat == temp;
//        cv::bitwise_and(dstMat, srcMat >= 1.5, dstMat);

        FMO_ASSERT(dstMat.data == dst.data(), "local_maxima: dst buffer reallocated");
    }

    void imfill(const Mat& src, Mat& dst) {
        dst.resize(src.format(), src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();
        
        int morph_size = 2;
        cv::Mat element = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 2*morph_size + 1, 2*morph_size+1 ), cv::Point( morph_size, morph_size ) );
        cv::morphologyEx( srcMat, dstMat, cv::MORPH_CLOSE, element);   
    
        FMO_ASSERT(dstMat.data == dst.data(), "imfill: dst buffer reallocated");
    }

    void imgrid(const std::vector<cv::Mat> &src, cv::Mat &dst, int grid_x, int grid_y) {
        // patch size
        int width  = dst.cols/grid_x;
        int height = dst.rows/grid_y;
        // iterate through grid
        int k = 0;
        for(int i = 0; i < grid_y; i++) {
            for(int j = 0; j < grid_x; j++) {
                cv::Mat s = src[k++];
                resize(s,s,cv::Size(width,height));
                s.copyTo(dst(cv::Rect(j*width,i*height,width,height)));
            }
        }
    }

    void imgridfull(const std::vector<cv::Mat> &src, cv::Mat &dst, int grid_x, int grid_y) {
        // patch size
        int width  = src[0].cols;
        int height = src[0].rows;
        // iterate through grid
        int k = 0;
        for(int i = 0; i < grid_y; i++) {
            for(int j = 0; j < grid_x; j++) {
                cv::Mat s = src[k++];
                s.copyTo(dst(cv::Rect(j*width,i*height,width,height)));
            }
        }
    }

    void putcorner(const cv::Mat &src, cv::Mat &dst) {
        int width  = src.cols/4;
        int height = src.rows/4;
        cv::Mat temp;
        resize(src,temp,cv::Size(width,height));
        temp.copyTo(dst(cv::Rect(3*width,0,width,height)));
    }

    void absdiff(const Mat& src1, const Mat& src2, Mat& dst) {
        if (src1.format() != src2.format()) {
            throw std::runtime_error("diff: inputs must have same format");
        }
        if (src1.dims() != src2.dims()) {
            throw std::runtime_error("diff: inputs must have same size");
        }

        dst.resize(src1.format(), src1.dims());
        cv::Mat src1Mat = src1.wrap();
        cv::Mat src2Mat = src2.wrap();
        cv::Mat dstMat = dst.wrap();

        cv::absdiff(src1Mat, src2Mat, dstMat);
        FMO_ASSERT(dstMat.data == dst.data(), "absdiff: dst buffer reallocated");
    }

    void flip(const Mat& src, Mat& dst) {
        dst.resize(src.format(), src.dims());
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();
        cv::flip(srcMat, dstMat, 1);
        FMO_ASSERT(dstMat.data == dst.data(), "flip: dst buffer reallocated");
    }

    void subsample(const Mat& src, Mat& dst) {
        if (src.format() == Format::YUV420SP) {
            throw std::runtime_error("downscale: source cannot be YUV420SP");
        }

        Dims srcDims = src.dims();
        Dims dstDims = {srcDims.width / 2, srcDims.height / 2};

        if (dstDims.width == 0 || dstDims.height == 0) {
            throw std::runtime_error("downscale: source is too small");
        }

        dst.resize(src.format(), dstDims);
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        if (srcDims.width % 2 != 0) { srcMat.flags &= ~cv::Mat::CONTINUOUS_FLAG; }

        srcMat.cols &= ~1;
        srcMat.rows &= ~1;

        cv::resize(srcMat, dstMat, cv::Size(dstDims.width, dstDims.height), 0, 0, cv::INTER_AREA);
    }

    void subsample_resize(const Mat& src, Mat& dst, float scale) {
        if (src.format() == Format::YUV420SP) {
            throw std::runtime_error("downscale: source cannot be YUV420SP");
        }

        Dims srcDims = src.dims();
        int w = std::round(srcDims.width * scale);
        int h = std::round(srcDims.height * scale);
        Dims dstDims = {w, h};

        if (dstDims.width == 0 || dstDims.height == 0) {
            throw std::runtime_error("downscale: source is too small");
        }

        dst.resize(src.format(), dstDims);
        cv::Mat srcMat = src.wrap();
        cv::Mat dstMat = dst.wrap();

        if (srcDims.width % 2 != 0) { srcMat.flags &= ~cv::Mat::CONTINUOUS_FLAG; }

        srcMat.cols &= ~1;
        srcMat.rows &= ~1;
       
        cv::resize(srcMat, dstMat, cv::Size(dstDims.width, dstDims.height), 0, 0, cv::INTER_LINEAR);
    }
}
