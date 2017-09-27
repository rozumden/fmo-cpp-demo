#ifndef FMO_EXPLORER_IMPL_HPP
#define FMO_EXPLORER_IMPL_HPP

#include <fmo/algorithm.hpp>
#include <fmo/subsampler.hpp>

namespace fmo {
    /// Implementation details of class Explorer.
    struct ExplorerV1 final : public Algorithm {
        virtual ~ExplorerV1() override;

        /// Initializes all caches. Creates as many decimation levels as needed to process images
        /// with specified dimensions. The following calls to setInputSwap() will require that the
        /// format and dimensions match the format and dimensions provided here.
        ExplorerV1(const Config& cfg, Format format, Dims dims);

        /// To be called every frame, providing the next image for processing. The processing will
        /// take place during the call and might take a long time. The input is received by swapping
        /// the contents of the provided input image with an internal buffer.
        virtual void setInputSwap(Image& input) override;

        /// To be called every frame, obtaining a list of fast-moving objects that have been
        /// detected this frame. The returned objects (i.e. instances of class Detection) may be
        /// used only before the next call to setInputSwap().
        virtual void getOutput(Output&) override;

        /// Provide the offset of the frame number in which the detected objects are being reported
        /// relative to the current input frame.
        virtual int getOutputOffset() const override { return -1; }

        /// Visualizes the result of detection, returning an image that is useful for debugging
        /// algorithm behavior. The returned image will have BGR format and the same dimensions as
        /// the input image.
        virtual const Image& getDebugImage() override {
            visualize();
            return mCache.visColor;
        }

    private:
        /// Pos using a small data type for coordinates.
        struct MiniPos {
            int16_t x, y;
            MiniPos() = default;
            MiniPos(const Pos& pos) : x(int16_t(pos.x)), y(int16_t(pos.y)) {}
            MiniPos(int16_t aX, int16_t aY) : x(aX), y(aY) {}
            operator Pos() const { return {x, y}; }
        };

        /// Bounds using a small data type for coordinates.
        struct MiniBounds {
            MiniPos min, max;
            MiniBounds() = default;
            MiniBounds(const Bounds& bounds) : min(bounds.min), max(bounds.max) {}
            MiniBounds(MiniPos aMin, MiniPos aMax) : min(aMin), max(aMax) {}
            operator Bounds() const { return {min, max}; }
        };

        static MiniBounds grow(const MiniBounds& l, const MiniBounds& r) {
            return MiniBounds{MiniPos{std::min(l.min.x, r.min.x), std::min(l.min.y, r.min.y)},
                              MiniPos{std::max(l.max.x, r.max.x), std::max(l.max.y, r.max.y)}};
        }

        /// Data related to source images.
        struct SourceLevel {
            Format format; ///< source format
            Dims dims;     ///< source dimensions
            Image image1;  ///< newest source image
            Image image2;  ///< source image from previous frame
            Image image3;  ///< source image from two frames before
        };

        /// Data related to decimation levels that will not be processed processed any further.
        /// Serves as a cache during decimation.
        struct IgnoredLevel {
            Image image;
        };

        /// Data related to decimation levels that will be processed. Holds all data required to
        /// detect strips in this frame, as well as some detection results.
        struct ProcessedLevel {
            Image image1;       ///< newest source image
            Image image2;       ///< source image from previous frame
            Image image3;       ///< source image from two frames before
            Image diff1;        ///< newest difference image
            Image diff2;        ///< difference image from previous frame
            Image preprocessed; ///< image ready for strip detection
            int step;           ///< relative pixel width (due to downscaling)
            int numStrips = 0;  ///< number of strips detected this frame
        };

        /// Strip data.
        struct Strip {
            enum : int16_t {
                UNTOUCHED = 0,
                TOUCHED = 1,
                END = -1,
            };

            Strip(int16_t aX, int16_t aY, int16_t aHalfHeight)
                : x(aX), y(aY), halfHeight(aHalfHeight), special(UNTOUCHED) {}

            // data
            int16_t x, y;       ///< strip coordinates in the source image
            int16_t halfHeight; ///< strip height in the source image, divided by 2
            int16_t special;    ///< special value, status or index of next strip in stroke
        };

        /// Connected component data.
        struct Component {
            enum : int16_t {
                NO_TRAJECTORY = -1,
                NO_COMPONENT = -1,
            };

            Component(int16_t aFirst) : first(aFirst), trajectory(NO_TRAJECTORY) {}

            int16_t first;            ///< index of the first strip
            int16_t last;             ///< index of the last strip
            int16_t numStrips;        ///< the number of strips in component
            int16_t approxHalfHeight; ///< median of strip half heights
            int16_t next;             ///< index of next component in trajectory
            int16_t trajectory;       ///< index of assigned trajectory
        };

        /// Trajectory data.
        struct Trajectory {
            Trajectory(int16_t aFirst) : first(aFirst), maxWidth(0) {}

            int16_t first;      ///< index of the first component
            int16_t last;       ///< index of the last component
            int16_t maxWidth;   ///< width of the largest component
            int16_t numStrips;  ///< number of strips in trajectory
            MiniBounds bounds1; ///< bounding box enclosing object locations in T and T-1
            MiniBounds bounds2; ///< bounding box enclosing object locations in T-1 and T-2
        };

        struct MyDetection : public Detection {
            virtual ~MyDetection() override = default;
            MyDetection(const Detection::Object& detObj, const Detection::Predecessor& detPrev,
                        const Trajectory* traj, const ExplorerV1* aMe);
            virtual void getPoints(PointSet& out) const override;

        private:
            const ExplorerV1* const me;
            const Trajectory* const mTraj;
        };

        /// Miscellaneous cached objects, typically accessed by a single method.
        struct Cache {
            Image visDiffGray;
            Image visDiffColor;
            Image visColor;
        };

        /// Creates low-resolution versions of the source image using decimation.
        void createLevelPyramid(Image& input);

        /// Applies image-wide operations before strips are detected.
        void preprocess();

        /// Applies image-wide operations before strips are detected.
        void preprocess(ProcessedLevel& level);

        /// Detects strips by iterating over the pixels in the image.
        void findStrips();

        /// Detects strips by iterating over the pixels in the image.
        void findStrips(ProcessedLevel& level);

        /// Creates connected components by joining strips together.
        void findComponents();

        /// Finds properties of previously found components before trajectory search.
        void analyzeComponents();

        /// Creates trajectories by joining components together.
        void findTrajectories();

        /// Finds properties of previously found trajectories before picking the best one.
        void analyzeTrajectories();

        /// Locates an object by selecting the best trajectory.
        void findObjects();

        /// Determines whether the given trajectory should be considered a fast-moving object.
        bool isObject(Trajectory&) const;

        /// Finds a bounding box enclosing strips which are present in a given difference image. To
        /// convert coordinates to image space, a step value has to be specified which denotes the
        /// ratio of original image pixels to image pixels in the difference image.
        Bounds findTrajectoryBoundsInDiff(const Trajectory& traj, const Mat& diff, int step) const;

        /// Finds the bounding box that encloses a given trajectory.
        Bounds findBounds(const Trajectory&) const;

        /// Visualizes the results into the visualization image.
        void visualize();

        // data
        SourceLevel mSourceLevel;                 ///< the level with original images
        Subsampler mSubsampler;                     ///< for reducing input image resolution
        mutable Differentiator mDiff;             ///< for creating difference images
        std::vector<IgnoredLevel> mIgnoredLevels; ///< levels that will not be processed
        ProcessedLevel mLevel;                    ///< the level that will be processed
        std::vector<Strip> mStrips;               ///< detected strips, ordered by x coordinate
        std::vector<Component> mComponents;       ///< detected components, ordered by x coordinate
        std::vector<Trajectory> mTrajectories;    ///< detected trajectories
        std::vector<int> mSortCache;              ///< for storing and sorting integers
        std::vector<const Trajectory*> mRejected; ///< objects that have been rejected this frame
        std::vector<const Trajectory*> mObjects;  ///< objects that have been accepted this frame
        int mFrameNum = 0; ///< frame number, 1 when processing the first frame
        Cache mCache;      ///< miscellaneous cached objects
        const Config mCfg; ///< configuration settings
    };
}

#endif // FMO_EXPLORER_IMPL_HPP
