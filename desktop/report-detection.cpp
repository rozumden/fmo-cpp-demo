#include "report.hpp"

namespace {
    const char* space[4] = {"  ", "    ", "      ", "        "};
}

// DetectionReport

DetectionReport::DetectionReport(const std::string& directory, const Date& date)
    : mOut(fileName(directory, date), std::ios_base::out | std::ios_base::binary) {

    if (!mOut) {
        std::cerr << "failed to open '" << fileName(directory, date) << "'\n";
        throw std::runtime_error("failed to open detection report file for writing");
    }

    mOut << "<?xml version=\"1.0\" ?>\n";
    mOut << "<run>\n";
    mOut << space[0] << "<date>" << date.preciseStamp() << "</date>\n";
}

DetectionReport::~DetectionReport() { mOut << "</run>\n"; }

std::unique_ptr<DetectionReport::Sequence> DetectionReport::makeSequence(const std::string& input) {
    return std::make_unique<Sequence>(*this, input);
}

std::string DetectionReport::fileName(const std::string& directory, const Date& date) {
    return directory + '/' + date.fileNameSafeStamp() + ".xml";
}

// DetectionReport::Sequence

DetectionReport::Sequence::Sequence(DetectionReport& aMe, const std::string& input) : me(&aMe) {
    me->mOut << space[0] << "<sequence input=\"" << input << "\">\n";
}

DetectionReport::Sequence::~Sequence() { me->mOut << space[0] << "</sequence>\n"; }

void DetectionReport::Sequence::writeFrame(int frameNum, const fmo::Algorithm::Output& algOut,
                                           const EvalResult& evalRes) {
    if (algOut.detections.empty()) return;
    me->mOut << space[1] << "<frame num=\"" << frameNum << "\">\n";

    for (size_t i = 0; i < algOut.detections.size(); i++) {
        auto& detection = *algOut.detections[i];

        if (detection.object.haveId()) {
            me->mOut << space[2] << "<detection id=\"" << detection.object.id << "\">\n";
        } else {
            me->mOut << space[2] << "<detection>\n";
        }

        if (detection.predecessor.haveId()) {
            me->mOut << space[3] << "<predecessor>" << detection.predecessor.id
                     << "</predecessor>\n";
        }

        if (detection.object.haveCenter()) {
            me->mOut << space[3] << "<center x=\"" << detection.object.center.x << "\" y=\""
                     << detection.object.center.y << "\"/>\n";
        }

        if (detection.object.haveDirection()) {
            me->mOut << space[3] << "<direction x=\"" << detection.object.direction[0] << "\" y=\""
                     << detection.object.direction[1] << "\"/>\n";
        }

        if (detection.object.haveLength()) {
            me->mOut << space[3] << "<length unit=\"px\">" << detection.object.length
                     << "</length>\n";
        }

        if (detection.object.haveRadius()) {
            me->mOut << space[3] << "<radius unit=\"px\">" << detection.object.radius
                     << "</radius>\n";
        }

        if (detection.object.haveVelocity()) {
            me->mOut << space[3] << "<velocity unit=\"px/frame\">" << detection.object.velocity
                     << "</velocity>\n";
        }

        if (evalRes.iouDt.size() > i) {
            me->mOut << space[3] << "<iou>" << evalRes.iouDt[i] << "</iou>\n";
        }

        me->mOut << space[3] << "<points>";
        detection.getPoints(me->mPointsCache);
        for (fmo::Pos p : me->mPointsCache) { me->mOut << p.x << ' ' << p.y << ' '; }
        me->mOut << "</points>\n";

        me->mOut << space[2] << "</detection>\n";
    }

    me->mOut << space[1] << "</frame>\n";
}
