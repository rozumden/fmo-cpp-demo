#ifndef FMO_DESKTOP_OBJECTSET_HPP
#define FMO_DESKTOP_OBJECTSET_HPP

#include <fmo/pointset.hpp>

/// Contains objects for each frame in a sequence. Holds the ground truth.
struct ObjectSet {
    ObjectSet() = default;

    /// Loads points from a file.
    void loadGroundTruth(const std::string& filename, fmo::Dims dims);

    /// Acquires the point sets corresponding to all objects at a given frame. If there are no
    /// objects a reference to an empty vector is returned. The frame numbering is one-based, that
    /// is, the first frame is frame number 1. This is consistent with what is stored in ground
    /// truth files.
    const std::vector<fmo::PointSet>& get(int frameNum) const;

    fmo::Dims dims() const { return mDims; }
    int numFrames() const { return (int)mFrames.size(); }

private:
    using frame_t = std::unique_ptr<std::vector<fmo::PointSet>>;

    /// Acquires the point set at a given frame. The argument must be in range 1 to numFrames()
    /// inclusive.
    frame_t& at(int frameNum) { return mFrames.at(frameNum - 1); }

    /// Acquires the point set at a given frame. The argument must be in range 1 to numFrames()
    /// inclusive.
    const frame_t& at(int frameNum) const { return mFrames.at(frameNum - 1); }

    // data
    fmo::Dims mDims = {0, 0}; ///< video dimensions
    int mOffset = 0;          ///< frame number offset when using get()
    std::vector<frame_t> mFrames;
};

#endif // FMO_DESKTOP_OBJECTSET_HPP
