#include "loop.hpp"
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

int main(int argc, char** argv) try
{
    Status s{argc, argv};
    
    // add path or find all sequences
    if (!s.args.inputDir.empty()) { 
        if (s.args.names.empty()) {
            std::string path = s.args.inputDir.substr(0, s.args.inputDir.find("*", 0));
            path.append("list.txt");
            std::fstream file;
            file.open(path,std::ios_base::in);
            while (file) {
                getline(file,path);
                s.args.names.emplace_back(path);
            }
            s.args.names.pop_back(); /// dirty hack
        }
        for (auto& str : s.args.names) {
            std::string paths = s.args.inputDir;
            replace( paths, "*", str); 
            s.args.inputs.emplace_back(paths);
        }
    }
    
    if (!s.args.baseline.empty()) { s.baseline.load(s.args.baseline); }
    if (s.haveCamera()) { s.args.inputs.emplace_back(); }
    if (!s.args.detectDir.empty()) { s.rpt.reset(new DetectionReport(s.args.detectDir, s.date)); }

    // select visualizer
    {
        bool demo = s.haveCamera();
        if (s.args.demo) demo = true;
        if (s.args.debug) demo = false;
        if (s.args.removal) {
            s.visualizer = std::unique_ptr<Visualizer>(new RemovalVisualizer{s});
        } else if (s.args.tutdemo) {
            s.visualizer = std::unique_ptr<Visualizer>(new TUTDemoVisualizer{s});
        } else if (s.args.utiademo) {
            s.visualizer = std::unique_ptr<Visualizer>(new UTIADemoVisualizer{s});
        } else
        s.visualizer = demo ? std::unique_ptr<Visualizer>(new DemoVisualizer{s})
                            : std::unique_ptr<Visualizer>(new DebugVisualizer{s});
    }

    std::vector<Statistics> stats(s.args.inputs.size());
    for (size_t i = 0; !s.quit && i < s.args.inputs.size(); i++) {
//        try {
            do {
                s.reload = false;
                stats[i] = processVideo(s, i);
            } while (s.reload);
//        } catch (std::exception& e) {
//            std::cerr << "while playing '" << s.args.inputs.at(i) << "'\n";
//            throw e;
//        }
    }

    EvaluationReport report(s.results, s.baseline, s.args, s.date,
                            s.timer.toc<fmo::TimeUnit::SEC, float>());
    report.write(std::cout);
    printStatistics(stats);

    if (!s.args.evalDir.empty()) { report.save(s.args.evalDir); }
    if (!s.args.scoreFile.empty()) { report.saveScore(s.args.scoreFile); }
} catch (std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    std::cerr << "tip: use --help to see a list of available commands\n";
    return -1;
}


void printStatistics(std::vector<Statistics> &stats) {
    std::cout << std::endl;
    std::cout << "Statistics together: " << std::endl;
    long long totalSum = 0;
    long long totalFrames = 0;
    std::vector<int> nDets;
    nDets.reserve(stats.size());
    for(auto &stat : stats) {
        nDets.push_back(stat.totalDetections);
        totalSum += stat.totalDetections;
        totalFrames += stat.nFrames;
    }
    float meanFrames = (float)totalSum / totalFrames;
    float meanSeq = (float)totalSum / stats.size();
    float meanNFrames = (float) totalFrames / stats.size();

    const auto median_it = nDets.begin() + nDets.size() / 2;
    std::nth_element(nDets.begin(), median_it , nDets.end());
    auto median = *median_it;

    std::cout << "Number of sequences - " << stats.size() << std::endl;
    std::cout << "Average number of frames - " << meanNFrames << std::endl;
    std::cout << "Detections: total - " << totalSum <<
              ", average per frame - " << meanFrames <<
              ", average per sequence - " << meanSeq <<
              ", median per sequence - " << median << std::endl;
}