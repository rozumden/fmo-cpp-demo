#include "window.hpp"
#include "desktop-opencv.hpp"
#include <chrono>
#include <fmo/assert.hpp>
#include <fmo/pointset.hpp>
#include <fmo/stats.hpp>
#include <thread>

#define TOSTR_INNER(x) #x
#define TOSTR(x) TOSTR_INNER(x)

namespace {
    const char* const windowName = "FMO";

    constexpr Colour colourFp = Colour::lightMagenta();
    constexpr Colour colourFn = Colour::lightRed();
    constexpr Colour colourTp = Colour::lightGreen();

    constexpr int downscaleThresh = 800;

    cv::Vec3b toCv(Colour c) { return {c.b, c.g, c.r}; }

    struct PutColor {
        cv::Vec3b color;
        cv::Mat mat;

        void operator()(fmo::Pos pt) { mat.at<cv::Vec3b>({pt.x, pt.y}) = color; }
    };
}

Window::~Window() { close(); }

void Window::open(fmo::Dims dims) {
    if (mOpen) return;
    mOpen = true;
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);

    if (dims.height > downscaleThresh) {
        dims.width /= 2;
        dims.height /= 2;
    }

    cv::resizeWindow(windowName, dims.width, dims.height);
}

void Window::close() {
    if (!mOpen) return;
    mOpen = false;
    cv::destroyWindow(windowName);
}

void Window::display(fmo::Mat& image) {
    open(image.dims());
    cv::Mat mat = image.wrap();
    printText(mat);
    cv::imshow(windowName, mat);
    cv::setWindowProperty(windowName, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
}

void Window::printText(cv::Mat& mat) {
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = mat.rows / 1000.;
    int thick = (mat.rows > downscaleThresh) ? 2 : 1;
    int lineWidth = 0;
    int lineHeight = 0;
    int baseline = 0;

    {
        auto lineSize = cv::getTextSize("ABC", fontFace, fontScale, thick, &baseline);
        lineHeight = lineSize.height;
    }

    int above = (9 * lineHeight / 14) + (lineHeight / 2);
    int below = 5 * lineHeight / 14;
    int pad = lineHeight / 2;
    cv::Scalar color(mColour.b, mColour.g, mColour.r);

    // render text in top left corner
    int yMaxLines = 0;
    if (!mLines.empty()) {
        for (auto& line : mLines) {
            auto lineSize = cv::getTextSize(line, fontFace, fontScale, thick, &baseline);
            lineWidth = std::max(lineWidth, lineSize.width);
        }

        // darken the area for the text
        int xMax = 2 * pad + lineWidth;
        int yMax = 2 * pad + int(mLines.size()) * (above + below);
        yMaxLines = yMax;
        cv::Rect rect{0, 0, xMax, yMax};
        mat(rect) = 0.3 * mat(rect);

        // render the text
        int y = pad;
        int clri = 0;
        for (auto& line : mLines) {
            y += above;
            cv::Point origin = {pad, y};
            auto &mclr = mLineClrs[clri];
            cv::Scalar clr(mclr.b, mclr.g, mclr.r);
            cv::putText(mat, line, origin, fontFace, fontScale, clr, thick);
            y += below;
            clri++;
        }
        mLines.clear();
        mLineClrs.clear();
    }

    // render bottom line
    if (mBottomLine != "") {
        int helpRectHeight = (above + below) + 2 * pad;
        cv::Rect helpRect{0, mat.rows - helpRectHeight, mat.cols, helpRectHeight};
        mat(helpRect) = 0.3 * mat(helpRect);
        cv::Point helpOrigin{pad, mat.rows - pad - below};
        cv::putText(mat, mBottomLine, helpOrigin, fontFace, fontScale, color, thick);
    }

    // render top line
    if (mTopLine != "") {
        auto lineSize = cv::getTextSize(mTopLine, fontFace, fontScale, thick, &baseline);
        int helpRectHeight = (above + below) + 2 * pad;
        int lineWidth = lineSize.width+2*pad;
        int offset = (mat.cols - lineWidth)/2;
        cv::Rect helpRect{offset, 0, lineWidth, helpRectHeight};
        mat(helpRect) = 0.3 * mat(helpRect);
        cv::Point helpOrigin{pad+offset, above+pad/2};
        cv::putText(mat, mTopLine, helpOrigin, fontFace, fontScale, color, thick);
    }

    // render center
    if (mCenterLine != "" || mCenterUnderLine != "") {
        double fontScaleCenter = 3*fontScale;
        int thickCenter = 3*thick;
        auto lineSize = cv::getTextSize(mCenterLine, fontFace, fontScaleCenter, thickCenter, &baseline);
        // auto lineSizeUnder = cv::getTextSize(mCenterUnderLine, fontFace, fontScaleCenter, thickCenter, &baseline);
        // int lineUnderWidth = lineSizeUnder.width+2*pad;
        // int offsetWUnder = (mat.cols - lineUnderWidth)/2;
        int lineWidth = lineSize.width+2*pad;
        int lineHeight = lineSize.height+2*pad;
        int offsetW = (mat.cols - lineWidth)/2 + 4*pad;
        int offsetH = (mat.rows - lineHeight)/2 + 2*lineHeight;
        cv::Rect helpRect{offsetW, offsetH, lineWidth, lineHeight};
        // mat(helpRect) = 0.3 * mat(helpRect);
        mat(helpRect) = 0;
        cv::Point helpOrigin{offsetW+pad, offsetH-2*pad};
        cv::Point helpOriginUnder{offsetW+pad, offsetH+lineHeight-2*pad};
        cv::putText(mat, mCenterLine, helpOrigin, fontFace, fontScaleCenter, color, thickCenter);
        cv::putText(mat, mCenterUnderLine, helpOriginUnder, fontFace, fontScaleCenter, color, thickCenter);
    }


    // render table 
    if (visTable && !mTable.empty()) {
        auto lineSize1 = cv::getTextSize("10. ", fontFace, fontScale, thick, &baseline);
        auto lineSize2 = cv::getTextSize("wwwwwwwwww  ", fontFace, fontScale, thick, &baseline);
        auto lineSize3 = cv::getTextSize("444.44", fontFace, fontScale, thick, &baseline);
        lineWidth = lineSize1.width + lineSize2.width + lineSize3.width;

        // darken the area for the text
        int xMax = pad + lineWidth;
        int yMax = yMaxLines + 2*pad + int(mTable.size()) * (above + below);
        cv::Rect rect{0, yMaxLines, xMax, yMax-yMaxLines};
        mat(rect) = 0.3 * mat(rect);

        // render the text
        std::string pref;
        int y = yMaxLines + pad;
        for (int ik = 0; ik < (int)mTable.size(); ++ik) {
            auto &el = mTable[ik];
            if(ik < 9)
               pref = " ";
            else
               pref = "";
            y += above;
            
            cv::Point origin1 = {pad, y};
            cv::Point origin2 = {lineSize1.width + pad, y};
            cv::Point origin3 = {lineSize1.width + lineSize2.width +pad, y};
            cv::putText(mat, pref+std::to_string(ik+1)+".", origin1, fontFace, fontScale, color, thick);
            cv::putText(mat, el.second, origin2, fontFace, fontScale, color, thick);
            cv::putText(mat, std::to_string(el.first).substr(0,4), origin3, fontFace, fontScale, color, thick);
            y += below;
        }
    }
}

Command Window::getCommand(bool block) {
    int keyCode = cv::waitKey(block ? 0 : 1);
    int64_t sinceLastNs = fmo::nanoTime() - mLastNs;
    int64_t waitNs = mFrameNs - sinceLastNs - 1'000'000;

    if (waitNs > 0) {
        std::chrono::nanoseconds chronoNs{waitNs};
        std::this_thread::sleep_for(chronoNs);
    }

    mLastNs = fmo::nanoTime();
    return encodeKey(keyCode);
}

Command Window::encodeKey(int keyCode) {
    switch (keyCode) {
    case 27: // escape
        return Command::QUIT;
    case 13: // cr
    case 10: // lf
        return Command::STEP;
    case ' ':
        return Command::PAUSE;
    case 'f':
        return Command::PAUSE_FIRST;
    case ',':
        return Command::JUMP_BACKWARD;
    case '.':
        return Command::JUMP_FORWARD;
    case 'n':
        return Command::INPUT;
    case 'r':
    case 'R':
        return Command::RECORD;
    case 'g':
    case 'G':
        return Command::RECORD_GRAPHICS;
    case 'a':
    case 'A':
        return Command::AUTOMATIC_MODE;
    case 'm':
    case 'M':
        return Command::MANUAL_MODE;
    case 'e':
    case 'E':
        return Command::FORCED_EVENT;
    case '?':
    case 'h':
    case 'H':
        return Command::SHOW_HELP;
    case 's':
    case 'S':
        return Command::PLAY_SOUNDS;
    case 'p':
    case 'P':
        return Command::SCREENSHOT;
    case '0':
        return Command::LEVEL0;
    case '1':
        return Command::LEVEL1;
    case '2':
        return Command::LEVEL2;
    case '3':
        return Command::LEVEL3;
    case '4':
        return Command::LEVEL4;
    case '5':
        return Command::LEVEL5;
    case 'l':
        return Command::LOCAL_MAXIMA;
    case 'd':
        return Command::DIFF;
    case 'b':
        return Command::BIN_DIFF;
    case 'i':
        return Command::SHOW_IM;
    case 't':
        return Command::DIST_TRAN;
    case 'o':
        return Command::SHOW_NONE;
    default:
        return Command::NONE;
    };
}

void drawPoints(const fmo::PointSet& points, fmo::Mat& target, Colour colour) {
    FMO_ASSERT(target.format() == fmo::Format::BGR, "bad format");
    cv::Mat mat = target.wrap();
    cv::Vec3b vec = {colour.b, colour.g, colour.r};

    for (auto& pt : points) { mat.at<cv::Vec3b>({pt.x, pt.y}) = vec; }
}

void drawPointsGt(const fmo::PointSet& ps, const fmo::PointSet& gt, fmo::Mat& target) {
    FMO_ASSERT(target.format() == fmo::Format::BGR, "bad format");
    cv::Mat mat = target.wrap();
    PutColor c1{toCv(colourFp), mat};
    PutColor c2{toCv(colourFn), mat};
    PutColor c3{toCv(colourTp), mat};
    fmo::pointSetCompare(ps, gt, c1, c2, c3);
}

void removePoints(const fmo::PointSet& points, fmo::Mat& target, fmo::Mat& bg) {
    FMO_ASSERT(target.format() == bg.format(), "bad format");
    cv::Mat mat = target.wrap();
    cv::Mat b = bg.wrap();
    for (auto& pt : points) { mat.at<cv::Vec3b>({pt.x, pt.y}) = b.at<cv::Vec3b>({pt.x, pt.y}); }
}