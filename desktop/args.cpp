#include "args.hpp"
#include <iostream>
#include <fmo/assert.hpp>

namespace {
    using doc_t = const char* const;
    doc_t helpDoc = "Display help.";
    doc_t expDoc = "Set exposure value. Should be between 0 and 1. Usually between 0.03 and 0.1.";
    doc_t fpsDoc = "Set number of frames per second.";
    doc_t radiusDoc = "Set object radius in cm. Used for speed estimation. Used if --p2cm is not specified. By default used for tennis/floorball: 3.6 cm.";
    doc_t p2cmDoc = "Set how many cm are in one pixel on object. Used for speed estimation. More dominant than --radius.";
    doc_t diffthDoc = "Differential image threshold factor. Default 1.0.";
    doc_t defaultsDoc = "Display default values for all parameters.";
    doc_t algorithmDoc = "<name> Specifies the name of the algorithm variant. Use --list to list "
                         "available algorithm names.";
    doc_t listDoc = "Display available algorithm names. Use --algorithm to select an algorithm.";
    doc_t headlessDoc = "Don't draw any GUI unless the playback is paused. Must not be used with "
                        "--wait, --fast.";
    doc_t demoDoc = "Force demo visualization method. This visualization method is preferred when "
                    "--camera is used.";
    doc_t debugDoc = "Force debug visualization method. This visualization method is preferred "
                     "when --input is used.";
    doc_t removalDoc = "Force removal visualization method. This visualization method has "
                     "the highest priority.";
    doc_t noRecordDoc = "Switch off record all frames.";
    doc_t includeDoc = "<path> File with additional command-line arguments. The format is the same "
                       "as when specifying parameters on the command line. Whitespace such as tabs "
                       "and endlines is allowed.";
    doc_t inputDoc = "<path> Path to an input video file. Can be used multiple times. Must not be "
                     "used with --camera.";
    doc_t inputDirDoc = "<path> Path to an input videos directory. Must not be used with --camera."
                     "Provides a template for input videos. Asterisk (*) will be replaced with input"
                     "In case of no input all sequences in list.txt (in the directory) will be used";
    doc_t gtDoc = "<path> Text file containing ground truth data. Using this option enables "
                  "quality evaluation. If used at all, this option must be used as many times as "
                  "--input. Use --eval-dir to specify the directory for evaluation results.";
    doc_t gtDirDoc = "<path> Directory with text files containing ground truth data. Using this option enables "
                  "quality evaluation. Use --eval-dir to specify the directory for evaluation results.";
    doc_t nameDoc = "<string> Name of the input file to be displayed in the evaluation report. If "
                    "used at all, this option must be used as many times as --input.";
    doc_t baselineDoc = "<path> File with previously saved results (via --eval-dir) for "
                        "comparison. When used, the playback will pause to demonstrate where the "
                        "results differ. Must be used with --gt.";
    doc_t cameraDoc = "<int> Input camera device ID. When this option is used, stream from the "
                      "specified camera will be used as input. Using ID 0 selects the default "
                      "camera, if available. Must not be used with --input, --wait, --fast, "
                      "--frame, --pause.";
    doc_t yuvDoc = "Feed image data into the algorithm in YCbCr color space.";
    doc_t recordDirDoc = "<dir> Output directory to save video to. A new video file will be "
                         "created, storing the input video with optionally overlaid detections. The name of the video file "
                         "will be determined by system time. The directory must exist.";
    doc_t evalDirDoc = "<dir> Directory to save evaluation report to. A single file text file will "
                       "be created there with a unique name based on timestamp. Must be used with "
                       "--gt.";
    doc_t texDoc = "Format tables in the evaluation report so that they can be used in the TeX "
                   "typesetting system. Must be used with --eval-dir.";
    doc_t detectDirDoc = "<dir> Directory to save detection output to. A single XML file will be "
                         "created there with a unique name based on timestamp.";
    doc_t scoreFileDoc = "<file> File to write a numeric evaluation score to.";
    doc_t pauseFpDoc = "Playback will pause whenever a detection is deemed a false positive. Must "
                       "be used with --gt.";
    doc_t pauseFnDoc = "Playback will pause whenever a detection is deemed a false negative. Must "
                       "be used with --gt.";
    doc_t pauseRgDoc = "Playback will pause whenever a regression is detected, i.e. whenever a "
                       "frame is evaluated as false and baseline is true. Must be used with "
                       "--baseline.";
    doc_t pauseImDoc = "Playback will pause whenever an improvement is detected, i.e. whenever a "
                       "frame is evaluated as true and baseline is false. Must be used with "
                       "--baseline.";
    doc_t pausedDoc = "Playback will be paused on the first frame. Shorthand for --frame 1. Must "
                      "not be used with --camera.";
    doc_t frameDoc = "<frame> Playback will be paused on the specified frame number. If there are "
                     "multiple input files, playback will be paused in each file that contains a "
                     "frame with the specified frame number. Must not be used with --camera.";
    doc_t fastDoc = "Sets the maximum playback speed. Shorthand for --wait 0. Must not be used "
                    "with --camera, --headless.";
    doc_t waitDoc = "<ms> Specifies the frame time in milliseconds, allowing for slow playback. "
                    "Must not be used with --camera, --headless.";
    doc_t paramDocI = "<int>";
    doc_t paramDocB = "<flag>";
    doc_t paramDocF = "<float>";
    doc_t paramDocUint8 = "<uint8>";
}

Args::Args(int argc, char** argv)
    : inputs(),
      gts(),
      names(),
      camera(-1),
      yuv(false),
      recordDir("."),
      pauseFn(false),
      pauseFp(false),
      pauseRg(false),
      pauseIm(false),
      evalDir(),
      inputDir(),
      gtDir(),
      baseline(),
      frame(-1),
      wait(-1),
      tex(false),
      headless(false),
      demo(false),
      tutdemo(false),
      utiademo(false),
      debug(false),
      removal(false),
      noRecord(false),
      exposure(100),
      fps(-1),
      radius(3.6),
      p2cm(-1),
      params(),
      mParser(),
      mHelp(false),
      mDefaults(false),
      mList(false) {

    // add commands
    mParser.add("\nHelp:");
    mParser.add("--help", helpDoc, mHelp);
    mParser.add("--defaults", defaultsDoc, mDefaults);
    
    mParser.add("\nAlgorithm selection:");
    mParser.add("--algorithm", algorithmDoc, params.name);
    mParser.add("--list", listDoc, mList);

    mParser.add("\nMode selection:");
    mParser.add("--headless", headlessDoc, headless);
    mParser.add("--demo", demoDoc, demo);
    mParser.add("--tutdemo", demoDoc, tutdemo);
    mParser.add("--utiademo", demoDoc, utiademo);
    mParser.add("--debug", debugDoc, debug);
    mParser.add("--removal", removalDoc, removal);
    mParser.add("--no-record", noRecordDoc, noRecord);

    mParser.add("--exposure", expDoc, exposure);
    mParser.add("--fps", fpsDoc, fps);
    mParser.add("--p2cm", p2cmDoc, p2cm);
    mParser.add("--dfactor", diffthDoc, params.diff.diffThFactor);
    mParser.add("--radius", radiusDoc, radius);

    mParser.add("\nInput:");
    mParser.add("--include", includeDoc, [this](const std::string& path) { mParser.parse(path); });
    mParser.add("--input-dir", inputDirDoc, inputDir);
    mParser.add("--input", inputDoc, inputs);
    mParser.add("--gt", gtDoc, gts);
    mParser.add("--gt-dir", gtDirDoc, gtDir);
    mParser.add("--name", nameDoc, names);
    mParser.add("--baseline", baselineDoc, baseline);
    mParser.add("--camera", cameraDoc, camera);
    mParser.add("--yuv", yuvDoc, yuv);

    mParser.add("\nOutput:");
    mParser.add("--record-dir", recordDirDoc, recordDir);
    mParser.add("--eval-dir", evalDirDoc, evalDir);
    mParser.add("--tex", texDoc, tex);
    mParser.add("--detect-dir", detectDirDoc, detectDir);
    mParser.add("--score-file", scoreFileDoc, scoreFile);

    mParser.add("\nPlayback control:");
    mParser.add("--pause-fp", pauseFpDoc, pauseFp);
    mParser.add("--pause-fn", pauseFnDoc, pauseFn);
    mParser.add("--pause-rg", pauseRgDoc, pauseRg);
    mParser.add("--pause-im", pauseImDoc, pauseIm);
    mParser.add("--paused", pausedDoc, [this]() { frame = 1; });
    mParser.add("--frame", frameDoc, frame);
    mParser.add("--fast", fastDoc, [this]() { wait = 0; });
    mParser.add("--wait", waitDoc, wait);

    // add algorithm params

    mParser.add("\nParameters pertaining to the 'median-v1' algorithm:");
    mParser.add("--p-iou-thresh", paramDocF, params.iouThreshold);
    mParser.add("--p-diff-thresh", paramDocUint8, params.diff.thresh);
    mParser.add("--p-diff-adjust-period", paramDocI, params.diff.adjustPeriod);
    mParser.add("--p-diff-min-noise", paramDocF, params.diff.noiseMin);
    mParser.add("--p-diff-max-noise", paramDocF, params.diff.noiseMax);
    mParser.add("--p-max-gap-x", paramDocF, params.maxGapX);
    mParser.add("--p-min-gap-y", paramDocF, params.minGapY);
    mParser.add("--p-max-image-height", paramDocI, params.maxImageHeight);
    mParser.add("--p-image-height", paramDocI, params.imageHeight);
    mParser.add("--p-min-strip-height", paramDocI, params.minStripHeight);
    mParser.add("--p-min-strips-in-object", paramDocI, params.minStripsInObject);
    mParser.add("--p-min-strip-area", paramDocF, params.minStripArea);
    mParser.add("--p-min-aspect", paramDocF, params.minAspect);
    mParser.add("--p-min-aspect-for-relevant-angle", paramDocF, params.minAspectForRelevantAngle);
    mParser.add("--p-min-dist-to-t-minus-2", paramDocF, params.minDistToTMinus2);
    mParser.add("--p-match-aspect-max", paramDocF, params.matchAspectMax);
    mParser.add("--p-match-area-max", paramDocF, params.matchAreaMax);
    mParser.add("--p-match-distance-min", paramDocF, params.matchDistanceMin);
    mParser.add("--p-match-distance-max", paramDocF, params.matchDistanceMax);
    mParser.add("--p-match-angle-max", paramDocF, params.matchAngleMax);
    mParser.add("--p-match-aspect-weight", paramDocF, params.matchAspectWeight);
    mParser.add("--p-match-area-weight", paramDocF, params.matchAreaWeight);
    mParser.add("--p-match-distance-weight", paramDocF, params.matchDistanceWeight);
    mParser.add("--p-match-angle-weight", paramDocF, params.matchAngleWeight);
    mParser.add("--p-select-max-distance", paramDocF, params.selectMaxDistance);
    mParser.add("--p-output-radius-corr", paramDocF, params.outputRadiusCorr);
    mParser.add("--p-output-radius-min", paramDocF, params.outputRadiusMin);
    mParser.add("--p-output-raster-corr", paramDocF, params.outputRasterCorr);
    mParser.add("--p-output-no-robust-radius", paramDocB, params.outputNoRobustRadius);

    mParser.add("\nParameters pertaining only to older versions of the algorithm:");
    mParser.add("--p-min-strips-in-component", paramDocI, params.minStripsInComponent);
    mParser.add("--p-min-strips-in-cluster", paramDocI, params.minStripsInCluster);
    mParser.add("--p-min-cluster-length", paramDocF, params.minClusterLength);
    mParser.add("--p-weight-height-ratio", paramDocF, params.heightRatioWeight);
    mParser.add("--p-weight-distance", paramDocF, params.distanceWeight);
    mParser.add("--p-weight-gaps", paramDocF, params.gapsWeight);
    mParser.add("--p-max-height-ratio-internal", paramDocF, params.maxHeightRatioInternal);
    mParser.add("--p-max-height-ratio-external", paramDocF, params.maxHeightRatioExternal);
    mParser.add("--p-max-distance", paramDocF, params.maxDistance);
    mParser.add("--p-max-gaps-length", paramDocF, params.maxGapsLength);
    mParser.add("--p-min-motion", paramDocF, params.minMotion);

    // parse command-line
    mParser.parse(argc, argv);

    // if requested, display help and exit
    if (mHelp) {
        mParser.printHelp(std::cerr);
        std::exit(-1);
    }

    // if requested, display defaults and exit
    if (mDefaults) {
        mDefaults = false;
        mParser.printValues(std::cerr, '\n');
        std::exit(-1);
    }

    // if requested, display list and exit
    if (mList) {
        auto algoNames = fmo::Algorithm::listFactories();
        for (auto& name : algoNames) { std::cerr << name << '\n'; }
        std::exit(-1);
    }

    // validate parameters, throw if there is trouble
    validate();
}

void Args::validate() const {
    if (inputs.empty() && camera == -1 && inputDir.empty()) {
        throw std::runtime_error("one of --input, --input-dir, --camera must be specified");
    }
    if(!inputDir.empty() && !inputs.empty()) {
        throw std::runtime_error("--input and --input-dir cannot be used together");
    }
    if(!inputs.empty() && !gtDir.empty()) {
        throw std::runtime_error("--input and --gt-dir cannot be used together");
    }
    if (camera != -1) {
        if (!inputs.empty()) { throw std::runtime_error("--camera cannot be used with --input"); }
        if (wait != -1) {
            throw std::runtime_error("--camera cannot be used with --wait or --fast");
        }
        if (frame != -1) {
            throw std::runtime_error("--camera canot be used with --frame or --pause");
        }
    }
    if (!gts.empty()) {
        if (gts.size() != inputs.size()) {
            std::cerr << "have " << inputs.size() << " inputs and " << gts.size() << " gts\n";
            throw std::runtime_error("there must be one --gt for each --input");
        }
    }
    // if (!names.empty()) {
    //     if (names.size() != inputs.size()) {
    //         std::cerr << "have " << inputs.size() << " inputs and " << names.size() << " names\n";
    //         throw std::runtime_error("there must be one --name for each --input");
    //     }
    // }
    if (gts.empty() && gtDir.empty()) {
        if (pauseFn || pauseFp) {
            throw std::runtime_error("--pause-fn|fp must be used with --gt");
        }
        if (!evalDir.empty()) { throw std::runtime_error("--eval-dir must be used with --gt"); }
        if (!baseline.empty()) { throw std::runtime_error("--baseline must be used with --gt"); }
    }
    if (baseline.empty()) {
        if (pauseRg || pauseIm) {
            throw std::runtime_error("--pause-rg|im must be used with --baseline");
        }
    }
    if (removal + demo + debug + tutdemo + utiademo + headless != 1) { throw std::runtime_error("One visualization method should be used."); }
    if (headless && wait != -1) {
        throw std::runtime_error("--headless cannot be used with --wait or --fast");
    }
    if (evalDir.empty() && tex) {
        throw std::runtime_error("--tex cannot be used without --eval-dir");
    }
}
