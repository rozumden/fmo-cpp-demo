#ifndef FMO_DESKTOP_ARGS_HPP
#define FMO_DESKTOP_ARGS_HPP

#include "parser.hpp"
#include <fmo/algorithm.hpp>

/// Processes and verifies command-line arguments.
struct Args {
    /// Read arguments from the command line. Throws exceptions if there are any errors.
    Args(int argc, char** argv);

    std::vector<std::string> inputs; ///< paths to video files to use as inputs
    std::vector<std::string> gts;    ///< paths to ground truth text files, enables evaluation
    std::vector<std::string> names;  ///< names of inputs to be displayed in the report table
    int camera;                      ///< camera ID to use as input
    bool yuv;                        ///< force YCbCr color space
    std::string recordDir;           ///< directory to save recording to
    bool pauseFn;                    ///< pause when a false negative is encountered
    bool pauseFp;                    ///< pause when a false positive is encountered
    bool pauseRg;                    ///< pause when a regression is encountered
    bool pauseIm;                    ///< pause when an improvement is encountered
    std::string evalDir;             ///< path to evaluation results directory
    std::string inputDir;            ///< path to input directory
    std::string gtDir;               ///< path to ground truth directory
    std::string detectDir;           ///< path to detection output directory
    std::string scoreFile;           ///< path to evaluation score file
    std::string baseline;            ///< path to previously saved results file, enables comparison
    int frame;                       ///< frame number to pause at
    int wait;                        ///< frame time in milliseconds
    bool tex;                        ///< format tables in the report as TeX tables
    bool headless;                   ///< don't draw GUI unless the playback is paused
    bool demo;                       ///< force demo visualizer
    bool tutdemo;                    ///< force tutdemo visualizer
    bool utiademo;                   ///< force utiademo visualizer
    bool debug;                      ///< force debug visualizer
    bool removal;                    ///< force removal visualizer (highest priority)
    bool noRecord;                   ///< force removal visualizer (highest priority)
    float exposure;                  ///< set exposure
    float fps;                       ///< set fps
    float radius;                    ///< set radius
    float p2cm;                      ///< set pixel to centimeter
    fmo::Algorithm::Config params;   ///< algorithm parameters

    /// Print all parameters to a stream, separated by the provided character.
    void printParameters(std::ostream& out, char sep) const { mParser.printValues(out, sep); }

private:
    void validate() const;

    Parser mParser;
    bool mHelp;     ///< display help
    bool mDefaults; ///< display defaults for all parameters
    bool mList;     ///< display algorithm list
};

#endif // FMO_DESKTOP_ARGS_HPP
