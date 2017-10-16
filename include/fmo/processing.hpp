#ifndef FMO_PROCESSING_HPP
#define FMO_PROCESSING_HPP

#include <fmo/image.hpp>
#include <opencv2/core.hpp>
#include <iostream>

namespace cv {
    template<typename _Tp> class Point_;

    typedef Point_<float> Point2f;
}

namespace fmo {
     /// Fitting curve
    struct SCurve  
    {  
        SCurve() {}
        virtual ~SCurve() {}

    public:
        double scale = 1;
        cv::Point2f shift{0,0};
        float length = 0;
        cv::Point2f start{0, 0};
        cv::Point2f end{0, 0};
        cv::Point2f center{0, 0};

        virtual void draw(cv::Mat& cvVis, cv::Scalar clr = cv::Scalar{0,0,0}, float thickness = 1) const { };
        virtual void drawSmooth(cv::Mat& cvVis, cv::Scalar clr = cv::Scalar{0,0,0}, float thickness = 1) const { };
        virtual float maxDist(const std::vector<cv::Point2f>& pixels) const { return 0; };
        virtual SCurve* clone() const {
            return new SCurve(*this); 
        };
    };

    struct SLine : SCurve 
    {
        SLine() {}
        cv::Vec4f params{0, 0, 0, 0};
        cv::Point2f normal{0, 0};
        cv::Point2f perp{0,0};

        cv::Point2f startSmooth{0, 0};
        cv::Point2f endSmooth{0, 0};

    public:
        virtual void draw(cv::Mat& cvVis, cv::Scalar clr, float thickness) const override;
        virtual void drawSmooth(cv::Mat& cvVis, cv::Scalar clr, float thickness) const override;
        virtual float maxDist(const std::vector<cv::Point2f>& pixels) const override;
        virtual SCurve* clone() const override {
            return new SLine(*this); 
        }
    };

    struct SCircle : SCurve 
    {
        SCircle() { }
        float radius{0};
        float x{0};
        float y{0};
        double startDegree{0};
        double endDegree{360};

        double startDegreeSmooth{0};
        double endDegreeSmooth{180};

        double size{0};

    public:
        virtual void draw(cv::Mat& cvVis, cv::Scalar clr, float thickness) const override;
        virtual void drawSmooth(cv::Mat& cvVis, cv::Scalar clr, float thickness) const override;
        virtual float maxDist(const std::vector<cv::Point2f>& pixels) const override;
        virtual SCurve* clone() const override {
            return new SCircle(*this); 
        }
    };


    /// Saves an image to file.
    void save(const Mat& src, const std::string& filename);

    /// Copies image data. To accomodate the data from "src", resize() is called on "dst".
    void copy(const Mat& src, Mat& dst);

    /// Copies image data. To accomodate the data from "src", resize() is called on "dst".
    /// Regardless of the source format, the destination format is set to "format". Color
    /// conversions performed by this function are not guaranteed to make any sense.
    void copy(const Mat& src, Mat& dst, Format format);

    /// Converts the image "src" to a given color format and saves the result to "dst". One could
    /// pass the same object as both "src" and "dst", but doing so is ineffective, unless the
    /// conversion is YUV420SP to GRAY. Only some conversions are supported, namely: GRAY to BGR,
    /// BGR to GRAY, YUV420SP to BGR, YUV420SP to GRAY.
    void convert(const Mat& src, Mat& dst, Format format);

    /// Selects pixels that have a value less than the specified value; these are set to 0xFF while
    /// others are set to 0x00. Input image must be GRAY.
    void less_than(const Mat& src1, Mat& dst, uint8_t value);

    /// Selects pixels that have a value greater than the specified value; these are set to 0xFF
    /// while others are set to 0x00. Input image must be GRAY.
    void greater_than(const Mat& src1, Mat& dst, uint8_t value);

    void distance_transform(const Mat& src, Mat& dst);

    void local_maxima(const Mat& src, Mat& dst);
    
    void imfill(const Mat& src, Mat& dst);

    void imgrid(const std::vector<cv::Mat> &src, cv::Mat &dst, int grid_x, int grid_y);
    void imgridfull(const std::vector<cv::Mat> &src, cv::Mat &dst, int grid_x, int grid_y);
    void putcorner(const cv::Mat &src, cv::Mat &dst);

    /// Calculates the absolute difference between the two images. Input images must have the same
    /// format and size.
    void absdiff(const Mat& src1, const Mat& src2, Mat& dst);

    /// Resizes an image so that each dimension is divided by two.
    void subsample(const Mat& src, Mat& dst);

    /// Resizes an image exactly.
    void subsample_resize(const Mat& src, Mat& dst, float scale);

    /// Calculates the per-pixel median of three images.
    void median3(const Image& src1, const Image& src2, const Image& src3, Image& dst);

    /// Calculates the per-pixel median of three images.
    void median5(const Image& src1, const Image& src2, const Image& src3, const Image& src4, const Image& src5, Image& dst);

    /// Flips an image in x axis.
    void flip(const Mat& src, Mat& dst);

    float fitline(const std::vector<cv::Point2f>& pixels, float fmoRadius, SLine &line);
    float fitcircle(const std::vector<cv::Point2f>& pixels, float fmoRadius, SCircle &circle);
    float fitcurve(const std::vector<cv::Point2f>& pixels, float fmoRadius, SCurve *&curve,
                   SCircle &circle, SLine &line);
    float fitcurve(const std::vector<cv::Point2f>& pixels, float fmoRadius, SCurve *&curve,
                   SCircle &circle, SLine &line, const cv::Vec2f &c1, const cv::Vec2f &c2);

    float fitcircle(const std::vector<cv::Point2f>& pixels, float fmoRadius, SCircle &circle,
                    const cv::Vec2f &c1, const cv::Vec2f &c2);


    }

#endif // FMO_PROCESSING_HPP
