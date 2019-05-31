#ifndef FMO_DESKTOP_WINDOW_HPP
#define FMO_DESKTOP_WINDOW_HPP

#include <cstdint>
#include <fmo/common.hpp>
#include <fmo/pointset.hpp>

enum class Command {
    NONE,
    STEP,
    PAUSE,
    PAUSE_FIRST,
    JUMP_BACKWARD,
    JUMP_FORWARD,
    RECORD,
    RECORD_GRAPHICS,
    AUTOMATIC_MODE,
    MANUAL_MODE,
    FORCED_EVENT,
    SHOW_HELP,
    PLAY_SOUNDS,
    SCREENSHOT,
    QUIT,
    LEVEL0,
    LEVEL1,
    LEVEL2,
    LEVEL3,
    LEVEL4,
    LEVEL5,
    DIST_TRAN,
    BIN_DIFF,
    DIFF,
    LOCAL_MAXIMA,
    SHOW_IM,
    SHOW_NONE,
    INPUT,
};

struct Colour {
    uint8_t b, g, r;

    static constexpr Colour red() { return {0x40, 0x40, 0x90}; }
    static constexpr Colour green() { return {0x40, 0x80, 0x40}; }
    static constexpr Colour blue() { return {0xA0, 0x40, 0x40}; }
    static constexpr Colour magenta() { return {0xA0, 0x40, 0x90}; }
    static constexpr Colour gray() { return {0x60, 0x60, 0x60}; }
    static constexpr Colour lightRed() { return {0x40, 0x40, 0xFF}; }
    static constexpr Colour lightGreen() { return {0x40, 0xFF, 0x40}; }
    static constexpr Colour lightBlue() { return {0xFF, 0x40, 0x40}; }
    static constexpr Colour lightMagenta() { return {0xFF, 0x40, 0xFF}; }
    static constexpr Colour lightGray() { return {0xC0, 0xC0, 0xC0}; }
};

/// Class of visualization and GUI-related procedures.
struct Window {
    ~Window();

    /// Closes the UI window (if open).
    void close();

    /// Sets the text color. In a given frame, all text will be rendered with the same color.
    void setTextColor(Colour color) { mColour = color; }

    /// Adds text to be rendered when the next image is displayed.
    void print(const std::string& line) { mLines.push_back(line); mLineClrs.push_back(mColour); }
    void print(const std::string& line, Colour clr) { mLines.push_back(line); mLineClrs.push_back(clr); }

    /// Sets the text to display at the bottom of the screen.
    void setBottomLine(const std::string& text) { mBottomLine = text; }

    /// Sets the text to display in the center of the screen.
    void setCenterLine(const std::string& text, const std::string& textunder) { 
        mCenterLine = text; 
        mCenterUnderLine = textunder;
    }

    /// Sets the text to display at the bottom of the screen.
    void setTopLine(const std::string& text) { mTopLine = text; }

    /// Renders the specified image on screen. The image may be modified by text rendering.
    void display(fmo::Mat& image);

    /// Sets the preferred frame duration in seconds.
    void setFrameTime(float sec) { mFrameNs = int64_t(1e9f * sec); }

    /// Receives a command from the user. If the argument is false, blocks for some time between 1
    /// millisecond and the time set by the last call to setFrameTime(). If the argument is true,
    /// block indefinitely.
    Command getCommand(bool block);

private:
    void open(fmo::Dims dims);
    static Command encodeKey(int keyCode);
    void printText(cv::Mat& mat);

    // data
    int64_t mFrameNs = 0;
    int64_t mLastNs = 0;
    std::vector<std::string> mLines;
    std::vector<Colour> mLineClrs;
    Colour mColour = Colour::lightGray();
    bool mOpen = false;
    std::string mBottomLine;
    std::string mTopLine;
    std::string mCenterLine;
    std::string mCenterUnderLine;
public: 
    std::vector<std::pair<float, std::string>> mTable;
    bool visTable = false;
};

/// Visualize a given set of points painting it onto the target image with the specified color.
void drawPoints(const fmo::PointSet& points, fmo::Mat& target, Colour colour);

/// Visualize result point set in comparison with the ground truth point set.
void drawPointsGt(const fmo::PointSet& ps, const fmo::PointSet& gt, fmo::Mat& target);

/// Visualize a given set of points painting it onto the target image with the specified color.
void removePoints(const fmo::PointSet& points, fmo::Mat& target, fmo::Mat& bg);

#endif // FMO_DESKTOP_WINDOW_HPP
