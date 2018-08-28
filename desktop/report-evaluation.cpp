#include "report.hpp"
#include <fmo/assert.hpp>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>

EvaluationReport::EvaluationReport(const Results& results, const Results& baseline,
                                   const Args& args, const Date& date, float seconds)
    : mDate(date), mResults(&results) {
    std::ostringstream out;
    info(out, mStats, *mResults, baseline, args, mDate, seconds);
    mInfo = out.str();
}

void EvaluationReport::write(std::ostream& out) const { out << mInfo; }

void EvaluationReport::save(const std::string& directory) const {
    if (mResults->empty()) return;
    std::string fn = directory + '/' + mDate.fileNameSafeStamp() + ".txt";
    std::ofstream out{fn, std::ios_base::out | std::ios_base::binary};

    if (!out) {
        std::cerr << "failed to open '" << fn << "'\n";
        throw std::runtime_error("failed to open file for writing results");
    }

    write(out);
    out << '\n';
    mResults->save(out);
}

void EvaluationReport::saveScore(const std::string& file) const {
    std::ofstream out{file};
    auto print = [&out](double d) { out << std::fixed << std::setprecision(12) << d << '\n'; };
    for (int i = 0; i < Stats::NUM_STATS; i++) { print(mStats.avg[i]); }
    for (int i = 0; i < Stats::NUM_STATS; i++) { print(mStats.total[i]); }
    print(mStats.iou);
}

void EvaluationReport::info(std::ostream& out, Stats& stats, const Results& results,
                            const Results& baseline, const Args& args, const Date& date,
                            float seconds) {
    std::vector<std::string> fields;
    bool haveBase = false;
    Evaluation count;
    Evaluation countBase;
    double sum[Stats::NUM_STATS] = {0, 0, 0, 0, 0};
    double sumBase[Stats::NUM_STATS] = {0, 0, 0, 0, 0};
    int numFiles = 0;
    int numBaseFiles = 0;

    auto precision = [](Evaluation& count) {
        if (count[Event::FP] == 0) { return 1.; }
        int div = count[Event::TP] + count[Event::FP];
        return count[Event::TP] / double(div);
    };
    auto recall = [](Evaluation& count) {
        if (count[Event::FN] == 0) { return 1.; }
        int div = count[Event::TP] + count[Event::FN];
        return count[Event::TP] / double(div);
    };
    auto fscore = [precision, recall](Evaluation& count, double beta) {
        double p = precision(count);
        double r = recall(count);
        if (p <= 0 || r <= 0) return 0.;
        double betaSqr = beta * beta;
        return ((betaSqr + 1) * p * r) / ((betaSqr * p) + r);
    };
    auto fscore_05 = [fscore](Evaluation& count) { return fscore(count, 0.5); };
    auto fscore_10 = [fscore](Evaluation& count) { return fscore(count, 1.0); };
    auto fscore_20 = [fscore](Evaluation& count) { return fscore(count, 2.0); };
    auto percent = [&args](std::ostream& out, double val) {
        out << std::fixed << std::setprecision(2) << (val * 100) << (args.tex ? "\\%" : "%");
    };

    using stat_func_t = std::function<double(Evaluation&)>;
    std::array<stat_func_t, Stats::NUM_STATS> statFuncs = {{precision, recall, fscore_05, fscore_10,
                                                           fscore_20}};
    std::array<const char*, Stats::NUM_STATS> funcNames = {{
        args.tex ? "Precision" : "precision", args.tex ? "Recall" : "recall",
        args.tex ? "$F_{0.5}$" : "F_0.5",     args.tex ? "$F_1$" : "F_1.0",
        args.tex ? "$F_2$" : "F_2.0",
    }};
    constexpr bool funcDisplayed[] = {true, true, false, true, false};

    auto countStrImpl = [](int val, int valBase) {
        std::ostringstream out;
        out << val;
        int delta = val - valBase;
        if (delta != 0) { out << " (" << std::showpos << delta << std::noshowpos << ')'; }
        return out.str();
    };
    auto percentStrImpl = [&](double val, double valBase) {
        std::ostringstream out;
        percent(out, val);
        double delta = val - valBase;
        if (abs(delta) > 5e-5) {
            out << " (" << std::showpos;
            percent(out, delta);
            out << std::noshowpos << ')';
        }
        return out.str();
    };
    auto countStr = [&](Event event) {
        int val = count[event];
        int valBase = haveBase ? countBase[event] : val;
        return countStrImpl(val, valBase);
    };
    auto percentStr = [&](int i) {
        double val = statFuncs[i](count);
        double valBase = haveBase ? statFuncs[i](countBase) : val;
        return percentStrImpl(val, valBase);
    };
    auto addToAverage = [&](int i) {
        sum[i] += statFuncs[i](count);
        if (haveBase) { sumBase[i] += statFuncs[i](countBase); }
    };
    auto replaceAll = [](std::string& s, const std::string& find, const std::string& replace) {
        size_t pos = s.find(find);
        while (pos != std::string::npos) {
            s.replace(pos, find.length(), replace);
            pos = s.find(find, pos + replace.length());
        }
    };

    fields.push_back(args.tex ? "Sequence name" : "sequence");
    fields.push_back(args.tex ? "$TP$" : "tp");
    fields.push_back(args.tex ? "$TN$" : "tn");
    fields.push_back(args.tex ? "$FP$" : "fp");
    fields.push_back(args.tex ? "$FN$" : "fn");
    for (int i = 0; i < Stats::NUM_STATS; i++) {
        if (funcDisplayed[i]) { fields.push_back(funcNames[i]); }
    }

    for (auto& file : results) {
        if (file.frames.size() == 0) continue;
        auto& baseFile = baseline.getFile(file.name);
        haveBase = baseFile.frames.size() == file.frames.size();

        count.clear();
        for (auto eval : file.frames) { count += eval; }
        if (haveBase) {
            countBase.clear();
            for (auto eval : baseFile.frames) { countBase += eval; }
        }

        std::string name{args.names.empty() ? file.name : args.names.at(numFiles)};
        if (args.tex) { replaceAll(name, "_", "\\_"); }

        fields.push_back(name);
        fields.push_back(countStr(Event::TP));
        fields.push_back(countStr(Event::TN));
        fields.push_back(countStr(Event::FP));
        fields.push_back(countStr(Event::FN));

        for (int i = 0; i < Stats::NUM_STATS; i++) {
            if (funcDisplayed[i]) { fields.push_back(percentStr(i)); }
            addToAverage(i);
        }

        numFiles++;
        if (haveBase) { numBaseFiles++; }
    }

    if (numFiles == 0) {
        // no entries -- quit
        return;
    }

    // calculate totals
    count.clear();
    countBase.clear();
    for (auto& file : results) {
        if (file.frames.size() == 0) continue;
        auto& baseFile = baseline.getFile(file.name);
        haveBase = baseFile.frames.size() == file.frames.size();

        for (auto eval : file.frames) { count += eval; }
        if (haveBase) {
            for (auto eval : baseFile.frames) { countBase += eval; }
        }
    }

    haveBase = numBaseFiles > 0;

    fields.push_back(args.tex ? "Total" : "total");
    fields.push_back(countStr(Event::TP));
    fields.push_back(countStr(Event::TN));
    fields.push_back(countStr(Event::FP));
    fields.push_back(countStr(Event::FN));
    for (int i = 0; i < Stats::NUM_STATS; i++) {
        stats.total[i] = statFuncs[i](count);
        stats.totalBase[i] = haveBase ? statFuncs[i](countBase) : stats.total[i];
        if (funcDisplayed[i]) fields.push_back(percentStrImpl(stats.total[i], stats.totalBase[i]));
    }

    fields.push_back(args.tex ? "Average" : "average");
    fields.push_back("");
    fields.push_back("");
    fields.push_back("");
    fields.push_back("");
    for (int i = 0; i < Stats::NUM_STATS; i++) {
        stats.avg[i] = sum[i] / double(numFiles);
        stats.avgBase[i] = haveBase ? (sumBase[i] / double(numFiles)) : stats.avg[i];
        if (funcDisplayed[i]) fields.push_back(percentStrImpl(stats.avg[i], stats.avgBase[i]));
    }

    int cols = 5;
    for (int i = 0; i < Stats::NUM_STATS; i++) {
        if (funcDisplayed[i]) cols++;
    }
    FMO_ASSERT(fields.size() % cols == 0, "bad number of fields");
    std::vector<int> colSize(cols, 0);

    auto hline = [&]() {
        if (args.tex) {
            out << "\\hline\n";
        } else {
            for (int i = 0; i < colSize[0]; i++) { out << '-'; }
            for (int col = 1; col < cols; col++) {
                out << '|';
                for (int i = 0; i < colSize[col]; i++) { out << '-'; }
            }
            out << '\n';
        }
    };

    for (auto it = fields.begin(); it != fields.end();) {
        for (int col = 0; col < cols; col++, it++) {
            colSize[col] = std::max(colSize[col], int(it->size()) + 1);
        }
    }

    constexpr int numBins = 10;
    auto hist = results.makeIOUHistogram(numBins);
    auto histBase = baseline.makeIOUHistogram(numBins);
    stats.iou = results.getAverageIOU();
    stats.iouBase = haveBase ? baseline.getAverageIOU() : stats.iou;

    out << "parameters: " << std::defaultfloat << std::setprecision(6);
    args.printParameters(out, ' ');
    out << "\n\n";
    out << "generated on: " << date.preciseStamp() << '\n';
    out << "evaluation time: " << std::fixed << std::setprecision(1) << seconds << " s\n";
    out << "iou: ";
    for (int i = 0; i < numBins; i++) {
        int bin = hist[i];
        int binBase = haveBase ? histBase[i] : bin;
        out << countStrImpl(bin, binBase) << " ";
    }
    out << '\n';
    out << "iou avg: " << percentStrImpl(stats.iou, stats.iouBase) << '\n';
    for (int i = 0; i < Stats::NUM_STATS; i++) {
        // display stats of quantities that are not in the table
        if (!funcDisplayed[i]) {
            out << funcNames[i];
            out << " total: " << percentStrImpl(stats.total[i], stats.totalBase[i]);
            out << ", avg: " << percentStrImpl(stats.avg[i], stats.avgBase[i]);
            out << '\n';
        }
    }
    out << '\n';
    int row = 0;
    for (auto it = fields.begin(); it != fields.end(); row++) {
        out << std::setw(colSize[0]) << std::left << *it++ << std::right;
        for (int col = 1; col < cols; col++, it++) {
            out << (args.tex ? " &" : "|");
            out << std::setw(colSize[col]) << *it;
        }
        out << (args.tex ? " \\\\\n" : "\n");
        if (row == 0) hline();
        if (row == numFiles) hline();
    }
}
