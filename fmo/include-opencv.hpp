#ifndef FMO_OPENCV_HPP
#define FMO_OPENCV_HPP

#include <opencv2/core/version.hpp>

#if CV_MAJOR_VERSION == 2
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#elif CV_MAJOR_VERSION == 3
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/video.hpp"

#endif

#endif // FMO_OPENCV_HPP
