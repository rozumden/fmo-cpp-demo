#ifndef FMO_ALGORITHM_HPP
#define FMO_ALGORITHM_HPP

#include <fmo/differentiator.hpp>
#include <fmo/image.hpp>
#include <fmo/pointset.hpp>
#include <functional>
#include <memory>
#include <string>
#include <fmo/processing.hpp>

namespace fmo {
    /// Represents a detection algorithm. Specific algorithms are implemented in different
    /// compilation units as subclasses of this class, and then registered using the
    /// registerFactory() static method. To create a new instance of an algorithm, call the make()
    /// static method.
    struct Algorithm {
        Algorithm() = default;
        Algorithm(const Algorithm&) = delete;
        Algorithm& operator=(const Algorithm&) = delete;
        virtual ~Algorithm() {}
        struct Config;
        struct Detection;

        /// Type of a function that produces an Algorithm instance.
        using Factory = std::function<std::unique_ptr<Algorithm>(const Config&, Format, Dims)>;

        /// Configuration settings determining the properties of a new Algorithm instance. Pass the
        /// object into the make() static method.
        struct Config {
            // parameters pertinent to "median" algorithms

            /// Name of the algorithm.
            std::string name;
            /// Configuration regarding creation of difference images.
            Differentiator::Config diff;
            /// Minimum IOU to accept a detection as TP during evaluation.
            float iouThreshold;
            /// Strips that are close to each other will be considered as part of the same connected
            /// component. This value is relative to image height. Used only in "median" algorithms.
            float maxGapX;
            /// Strips will be ignored unless they are at a distance from other strips in the image.
            /// This value is relative to image height.
            float minGapY;
            /// Maximum image height for processing. The input image will be downscaled by a factor
            /// of 2 until its height is less or equal to the specified value.
            int maxImageHeight;
            /// Strips that have less than this number of pixels in the downscaled image will be
            /// ignored.
            int minStripHeight;
            /// Candidate objects that have less than this number of strips will be ignored. Used
            /// only in "median" algorithms.
            int minStripsInObject;
            /// Candidate objects will be discarded if the area of strips contained within is small.
            /// This value is relative to the area of the objects convex hull. Used only in "median"
            /// algorithms.
            float minStripArea;
            /// Candidate objects will be discarded if their shape is too round. The aspect ratio
            /// must be greater than the specified value. Used only in "median" algorithms.
            float minAspect;
            /// The angle of the object is considered indistinguishable if its aspect ratio is below
            /// this value.
            float minAspectForRelevantAngle;
            /// Candidate object will be discarded if its distance to an object found in frame T-2
            /// is below this value, weighted by the length of the older object.
            float minDistToTMinus2;
            /// Objects cannot be matched if the ratio of their aspect ratios exceeds this value.
            float matchAspectMax;
            /// Objects cannot be matched if the ratio of their areas exceeds this value.
            float matchAreaMax;
            /// Objects cannot be matched if their distance (weighted by their average length)
            /// is below this value.
            float matchDistanceMin;
            /// Objects cannot be matched if their distance (weighted by their average length)
            /// exceeds this value.
            float matchDistanceMax;
            /// Objects cannot be matched if they do not lie on a line. The sine of the greater
            /// angle must not exceed this value.
            float matchAngleMax;
            /// The weight of aspect ratio comparison when matching objects.
            float matchAspectWeight;
            /// The weight of area comparison when matching objects.
            float matchAreaWeight;
            /// The weight of mutual distance when matching objects.
            float matchDistanceWeight;
            /// The weight of mutual angle when matching objects.
            float matchAngleWeight;
            /// A triplet of objects will be discarded if the an object is too far from the
            /// expected location based on the linear motion assumption. This value is relative to
            /// object size.
            float selectMaxDistance;
            /// Enables radius correction.
            float outputRadiusCorr;
            /// Reported radius cannot be lower than this value.
            float outputRadiusMin;
            /// Additional radius correction for rasterization (i.e. outputting points) only.
            float outputRasterCorr;
            /// Disables robust radius estimation.
            bool outputNoRobustRadius;

            // parameters pertinent to "taxonomy" algorithms
            /// Exact image height for processing. The input image will be downscaled by a factor
            /// of 2 until its height is less or equal to the specified value.
            int imageHeight;

            // legacy parameters

            /// Connected components that have less than this number of strips will be ignored.
            int minStripsInComponent;
            /// Clusters of connected components that have less than this number of strips will be
            /// ignored.
            int minStripsInCluster;
            /// Minimum length of a cluster relative to its height.
            float minClusterLength;
            /// When forming clusters from connected components, this value determines the
            /// importance of strip height ratio while considering to merge to clusters.
            float heightRatioWeight;
            /// When forming clusters from connected components, this value determines the
            /// importance of mutual distance while considering to merge two clusters.
            float distanceWeight;
            /// When forming clusters from connected components, this value determines the
            /// importance of minimizing gaps while considering to merge two clusters.
            float gapsWeight;
            /// Maximum ratio of strip heights allowed when merging strips from consecutive frames.
            float maxHeightRatioStrips;
            /// This value is compared to the 0.8-quantile divided by the 0.2 quantile of strip
            /// heights in a single connected component. If the fraction exceeds this value, the
            /// component is deemed inconsistent with the constant-height assumption and is
            /// discarded.
            float maxHeightRatioInternal;
            /// When forming clusters from connected components, this is the maximum acceptable
            /// fraction of approximate strip heights. If the approximate strip heights of the two
            /// components being joined differ more than specified by this value, the components
            /// cannot be joined together.
            float maxHeightRatioExternal;
            /// When forming clusters from connected components, this is the maximum acceptable
            /// distance between the component endpoints. This value is relative to approximate
            /// strip height of the smallest component in the clusters being merged.
            float maxDistance;
            /// The maximum fraction of a cluster length that may be covered by gaps between
            /// components.
            float maxGapsLength;
            /// Minimum distance that an object must travel in a single frame. This value is
            /// relative to the length of the path travelled in three frames.
            float minMotion;
            /// Maximum distance that an object may travel in a single frame. This value is relative
            /// to the length of the path travelled in three frames.
            float maxMotion;
            /// When outputting object point set, specifies what resolution should be used. When
            /// using source resolution, additional heavy-weight calculations need to be performed.
            bool pointSetSourceResolution;

            /// Creates a new config instance with default settings.
            Config();
        };

        /// A structure that contains all relevant information about a detected object. The user of
        /// the algorithm receives subclasses of this structure using the getOutput() method.
        /// Instances of this class are invalidated by the next call to setInputSwap().
        struct Detection {
            virtual ~Detection() = default;

            /// Detailed information about the detected object.
            struct Object {
                int id = -1;                     ///< object identifier
                Pos center{-1, -1};              ///< object midpoint
                float direction[2] = {0.f, 0.f}; ///< object unit direction (0 - x, 1 - y)
                float length = -1.f;             ///< length of the object in input frame pixels
                float radius = -1.f;             ///< radius of the object in input frame pixels
                float velocity = -1.f;           ///< velocity in input frame pixels per frame
                SCurve * curve = nullptr;
                float scale = 1;

                bool haveId() const { return id != -1; }
                bool haveCenter() const { return center.x != -1; }
                bool haveDirection() const { return direction[0] != 0.f || direction[1] != 0.f; }
                bool haveLength() const { return length >= 0.f; }
                bool haveRadius() const { return radius >= 0.f; }
                bool haveVelocity() const { return velocity >= 0.f; }
            };

            /// Information about the associated object in the previous frame.
            struct Predecessor {
                int id = -1;        ///< predecessor identifier
                Pos center{-1, -1}; ///< predecessor midpoint

                bool haveId() const { return id != -1; }
                bool haveCenter() const { return center.x != -1; }
            };

            Detection(const Object& anObject, const Predecessor& aPredecessor)
                : object(anObject), predecessor(aPredecessor) {}

            /// Generates coordinates of object pixels and sorts them according to pointSetCompLt().
            virtual void getPoints(PointSet& out) const = 0;

            const Object object;           ///< info about the detected object
            const Predecessor predecessor; ///< info about the matched object in the previous frame
        };

        /// Type of result produced by an algorithm instance every frame, reporting the detected
        /// objects.
        struct Output {
            /// Information about the detected objects.
            std::vector<std::unique_ptr<Detection>> detections;

            /// Resets all data.
            void clear() { detections.clear(); }
        };

        /// Creates a new instance of an Algorithm. The field config.name is used to determine which
        /// algorithm factory will be used. The factory must have been previously added with the
        /// registerFactory() static method.
        static std::unique_ptr<Algorithm> make(const Config& config, Format format, Dims dims);

        /// Registers an algorithm factory so that it is available via a call to make(). The
        /// provided name will be used as the search key and must be unique.
        static void registerFactory(const std::string& name, const Factory& factory);

        /// Provides the list of currently available factory names, previously added using
        /// registerFactory().
        static std::vector<std::string> listFactories();

        /// To be called every frame, providing the next image for processing. The processing will
        /// take place during the call and might take a long time. The input is received by swapping
        /// the contents of the provided input image with an internal buffer.
        virtual void setInputSwap(Image& input) = 0;

        /// To be called every frame, obtaining a list of fast-moving objects that have been
        /// detected this frame. The returned objects (i.e. instances of class Detection) may be
        /// used only before the next call to setInputSwap().
        virtual void getOutput(Output &output, bool smoothTrajecotry = false) { output.clear(); }

        /// Provide the offset of the frame number in which the detected objects are being reported
        /// relative to the current input frame.
        virtual int getOutputOffset() const = 0;

        /// Visualizes the result of detection, returning an image that is useful for debugging
        /// algorithm behavior. The returned image will have BGR format and the same dimensions as
        /// the input image.
        virtual const Image& getDebugImage() = 0;

        virtual const Image& getDebugImage(int level, bool showIm, bool showLM, int add) {
            return getDebugImage();
        }

    };
}

#endif // FMO_ALGORITHM_HPP
