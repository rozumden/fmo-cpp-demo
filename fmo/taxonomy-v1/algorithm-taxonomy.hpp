#ifndef FMO_ALGORITHM_TAXONOMY_V1_HPP
#define FMO_ALGORITHM_TAXONOMY_V1_HPP

#include <fmo/agglomerator.hpp>
#include <fmo/algebra.hpp>
#include <fmo/algorithm.hpp>
#include <fmo/subsampler.hpp>
#include <fmo/stats.hpp>
#include <fmo/strip.hpp>
#include <fmo/processing.hpp>
#include "../include-opencv.hpp"

namespace fmo {
    struct TaxonomyV1 final : public Algorithm {
        virtual ~TaxonomyV1() override = default;

        /// Initializes all caches. Creates as many decimation levels as needed to process images
        /// with specified dimensions. The following calls to setInputSwap() will require that the
        /// format and dimensions match the format and dimensions provided here.
        TaxonomyV1(const Config& cfg, Format format, Dims dims);

        /// To be called every frame, providing the next image for processing. The processing will
        /// take place during the call and might take a long time. The input is received by swapping
        /// the contents of the provided input image with an internal buffer.
        virtual void setInputSwap(Image&) override;

        /// To be called every frame, obtaining a list of fast-moving objects that have been
        /// detected this frame. The returned objects (i.e. instances of class Detection) may be
        /// used only before the next call to setInputSwap().
        virtual void getOutput(Output &, bool smoothTrajecotry) override;

        /// Provide the offset of the frame number in which the detected objects are being reported
        /// relative to the current input frame.
        virtual int getOutputOffset() const override { return 0; }

        /// Visualizes the result of detection, returning an image that is useful for debugging
        /// algorithm behavior. The returned image will have BGR format and the same dimensions as
        /// the input image.
        virtual const Image& getDebugImage() override;

        virtual const Image& getDebugImage(int level, bool showIm, bool showLM, int add) override;
        
    private:
        // structures

        /// Special values used instead of indices.
        enum Special : int16_t {
            UNTOUCHED = 0, ///< not processed
            TOUCHED = 1,   ///< processed
            END = -1,      ///< not an index, e.g. a strip is the last in its component
        };

        /// Connected component data.
        struct Component final {
            enum Status : int16_t {
                NOT_PROCESSED,
                TOO_LARGE,
                TOO_SMALL,
                NOT_STROKE,
                LATERAL,
                SHADOW,
                FMO,
                FMO_NOT_CONFIRMED,
            };

            Component() : status(NOT_PROCESSED) {}

            int id = 0;
            int area = 0;
            float len = 0;
            float iou = 0.f;
            Status status; ///< describes the reason why a component was discarded.
            float radius = 0;
            std::vector<cv::Point2f> pixels;  
            std::vector<cv::Point2f> traj;
            std::vector<cv::Point2f> trajFinal;
            std::vector<cv::Point2f> otherLM;
            std::vector<float> dist;
            SCurve * curve = nullptr;
            SCurve * curveSmooth = nullptr;
            SCircle circle;
            SLine line;
            float maxDist = 0.f;

            cv::Vec2d center = {0, 0};
            cv::Point2f start = {0, 0};
            Dims size = {0, 0};
        public:
            virtual const void draw(cv::Mat& img) const { if(curve != nullptr) curve->draw(img); }
            virtual const void drawSmooth(cv::Mat& img) const { if(curve != nullptr) curve->drawSmooth(img); }
            void calcIoU();
        };

        /// Object data.
        struct Object {
            Pos center = {0, 0};         ///< midpoint
            NormVector direction;        ///< principal direction
            float length = 0;            ///< length in principal direction
            float radius = 0;
            float velocity = 0;             ///< in radii per exposure
            SCurve * curve = nullptr;
            SCurve * curveSmooth = nullptr;
        };

        struct MyDetection : public Detection {
            virtual ~MyDetection() override = default;
            MyDetection(const Detection::Object& detObj, 
                        const TaxonomyV1::Object* obj, TaxonomyV1* aMe);
            virtual void getPoints(PointSet& out) const override;

        private:
            TaxonomyV1* me;
            const TaxonomyV1::Object* mObj;
        };

        // methods

        /// Subsamples the input image until it is below a set height; saves the source image and the
        /// subsampled image.
        void swapAndSubsampleInput(Image& in);

        /// Calculates the per-pixel median of the last three frames to obtain the background.
        /// Creates a binary difference image of background vs. the latest image.
        void computeBinDiff();

        /// Find connected components
        void findComponents();

        /// Process components 
        void processComponents();

        /// Tests whether a triplet of objects from consecutive frames should be considered as a
        /// detection of a fast-moving object.
        bool selectable(Object& o0, Object& o1, Object& o2) const;

        /// Find the bounding box enclosing the object in source image coordinates.
        Bounds getBounds(const Object& obj) const;

        // data

        const Config mCfg; ///< configuration received upon construction

        struct {
            Image image;  ///< latest source image
            int frameNum; ///< the number of images received so far
        } mSourceLevel;

        struct {
            Image inputs[4];       ///< input images subsampled to processing resolution, 0 - newest
            Image background;      ///< median of the last three inputs
            Image diff;
            Image binDiff;         ///< binary difference image, latest image vs. background
            Image diffAcc;
            Image binDiffPrev;
            Image labels;          ///< label image with connected components
            Image distTran;
            Image localMaxima;
            cv::Mat stats;
            cv::Mat centroids;
            int objectCounter = 0; ///< used to generate unique identifiers for detections
            int objectsNow = 0;
            float scale = 0;
            Dims dims;
            Dims newDims;
        } mProcessingLevel;

        struct {
            Image image;                ///< cached decimation step
            Image visualized;           ///< debug visualization
            Image visualizedFull;       ///< debug visualization full size
            Image pointsRaster;         ///< for rasterization when generating pixel coords
            Image objectsMask;
            Image distTranReverse;
            Image distTranGray;
            Image distTranBGR;
            Image ones;
            Image binDiffInv;
        } mCache;

        Differentiator mDiff;               ///< for creating the binary difference image
        std::vector<Component> mComponents; ///< connected components
        std::vector<Component> mPrevComponents; ///< connected components
        std::vector<Object> mObjects;       ///< objects
    };
}

#endif // FMO_ALGORITHM_TAXONOMY_V1_HPP
