#ifndef FMO_DESKTOP_EVALUATOR_HPP
#define FMO_DESKTOP_EVALUATOR_HPP

#include "objectset.hpp"
#include <array>
#include <fmo/algorithm.hpp>
#include <fmo/assert.hpp>
#include <fmo/pointset.hpp>
#include <iosfwd>
#include <list>
#include <map>

enum class Event { TP = 0, TN = 1, FP = 2, FN = 3 };
enum class Comparison { NONE, SAME, IMPROVEMENT, REGRESSION, BUFFERING };

const std::string& eventName(Event e);

/// Event count (TP, TN, FP, FN).
struct Evaluation {
    Evaluation() : mEvents{{0, 0, 0, 0}} {}
    int& operator[](Event e) { return mEvents[int(e)]; }
    int operator[](Event e) const { return mEvents[int(e)]; }
    void clear() { mEvents.fill(0); }

    void operator+=(const Evaluation& rhs) {
        for (int i = 0; i < 4; i++) { mEvents[i] += rhs.mEvents[i]; }
    }

private:
    std::array<int, 4> mEvents;
};

/// Evaluation results for a single frame, including event counts and IoU.
struct EvalResult {
    Evaluation eval;           ///< event counts
    Comparison comp;           ///< decision about improvement/regression
    std::vector<double> iouDt; ///< maximum IOUs of detected objects
    std::vector<double> iouGt; ///< maximum IOUs of GT objects

    void clear() {
        eval.clear();
        comp = Comparison::NONE;
        iouDt.clear();
        iouGt.clear();
    }

    std::string str() const;
};

inline bool good(Evaluation r) { return r[Event::FN] + r[Event::FP] == 0; }
inline bool bad(Evaluation r) { return r[Event::TN] + r[Event::TP] == 0; }

/// Results of evaluation of a particular sequence.
struct FileResults {
    static constexpr double IOU_STORAGE_FACTOR = 1e3;

    FileResults(const std::string& aName) : name(aName) {}

    /// Clears all data except the name.
    void clear() {
        frames.clear();
        iou.clear();
    }

    // data
    const std::string name;         ///< name of the sequence
    std::vector<Evaluation> frames; ///< evaluation for each frame
    std::vector<int> iou;           ///< non-zero intersection-over-union values
};

/// Responsible for storing and loading evaluation statistics.
struct Results {
    using list_t = std::list<FileResults>;
    using const_iterator = list_t::const_iterator;
    using size_type = list_t::size_type;
    Results() = default;

    /// Provides access to data regarding a specific file. A new data structure is created. If a
    /// structure with the given name already exists, an exception is thrown.
    FileResults& newFile(const std::string& name);

    /// Provides access to data regatding a specific file. If there is no data, a reference to an
    /// empty data structure is returned.
    const FileResults& getFile(const std::string& name) const;

    /// Loads results from a stream.
    void load(std::istream& in);

    /// Saves results to a stream.
    void save(std::ostream& out) const;

    /// Loads results from a file.
    void load(const std::string& file);

    /// Iterates over files in the results.
    const_iterator begin() const { return mList.begin(); }

    /// Iterates over files in the results.
    const_iterator end() const { return mList.end(); }

    /// Provides the number of files.
    size_type size() const { return mList.size(); }

    /// Checks whether there are any files.
    bool empty() const { return mList.empty(); }

    /// Calculate the histogram of intersection-over-union values.
    std::vector<int> makeIOUHistogram(int bins) const;

    /// Calculate average intersection-over-union value for all objects with non-zero overlap.
    double getAverageIOU() const;

private:
    // data
    std::list<FileResults> mList;
    std::map<std::string, FileResults*> mMap;
};

/// Responsible for calculating frame statistics for a single input file.
struct Evaluator {
    static constexpr int FRAME_OFFSET = -1;

    ~Evaluator();

    Evaluator(const std::string& gtFilename, fmo::Dims dims, Results& results,
              const Results& baseline);

    /// Decides whether the algorithm has been successful by comparing the objects it has provided
    /// with the ground truth. The frames must be provided in an increasing order, starting with
    /// frame number 1.
    void evaluateFrame(const fmo::Algorithm::Output& dt, int frameNum, EvalResult& out, float iouThreshold);

    /// Provides the ground truth for this sequence.
    const ObjectSet& gt() const { return mGt; }

private:
    // data
    int mFrameNum = 0;
    FileResults* mFile;
    const FileResults* mBaseline;
    ObjectSet mGt;
    std::string mName;
    fmo::PointSet mPointsCache;
};

/// Extracts filename from path.
std::string extractFilename(const std::string& path);

/// Extracts name of sequence from path.
std::string extractSequenceName(const std::string& path);

#endif // FMO_DESKTOP_EVALUATOR_HPP
