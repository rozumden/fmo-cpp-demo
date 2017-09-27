#include "include-opencv.hpp"
#include "include-simd.hpp"
#include <cstring>
#include <fmo/algorithm.hpp>
#include <fmo/benchmark.hpp>
#include <fmo/subsampler.hpp>
#include <fmo/differentiator.hpp>
#include <fmo/image.hpp>
#include <fmo/processing.hpp>
#include <fmo/stats.hpp>
#include <fmo/strip.hpp>
#include <random>

namespace fmo {
    namespace {
        void log(log_t logFunc, const char* cStr) { logFunc(cStr); }

        template <typename Arg1, typename... Args>
        void log(log_t logFunc, const char* format, Arg1 arg1, Args... args) {
            char buf[81];
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
            snprintf(buf, sizeof(buf), format, arg1, args...);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
            logFunc(buf);
        }
    }

    Registry& Registry::get() {
        static Registry instance;
        return instance;
    }

    void techInfo(log_t logFunc) {
#if defined(__thumb__) || defined(_M_ARM)
        log(logFunc, "Arch: ARM");
#elif defined(__arm__) || defined(_M_ARMT)
        log(logFunc, "Arch: ARM + Thumb");
#elif defined(__aarch64__)
        log(logFunc, "Arch: ARM64");
#elif defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
        log(logFunc, "Arch: AMD64");
#elif defined(__i386__) || defined(__i386) || defined(_M_IX86)
        log(logFunc, "Arch: i386");
#else
        log(logFunc, "Arch: unknown");
#endif

#if defined(FMO_HAVE_SSE2)
        log(logFunc, " + SSE2");
#endif
#if defined(FMO_HAVE_AVX2)
        log(logFunc, " + AVX2");
#endif
#if defined(FMO_HAVE_NEON)
        log(logFunc, " + NEON");
#endif

        log(logFunc, "\nCores: %d / Threads: %d\n", cv::getNumberOfCPUs(), cv::getNumThreads());
    }

    void Registry::runAll(log_t logFunc, stop_t stopFunc) const {
        fmo::SectionStats stats;

        try {
            techInfo(logFunc);
            log(logFunc, "Benchmark started.\n");

            for (auto func : mFuncs) {
                stats.reset();
                bool updated = false;

                while (!updated && !stopFunc()) {
                    stats.start();
                    func.second();
                    updated = stats.stop();
                }

                if (stopFunc()) { throw std::runtime_error("stopped"); }

                auto q = stats.quantilesMs();
                log(logFunc, "%s: %.2f / %.1f / %.0f\n", func.first, q.q50, q.q95, q.q99);
            }

            log(logFunc, "Benchmark finished.\n\n");
        } catch (std::exception& e) { log(logFunc, "Benchmark interrupted: %s.\n\n", e.what()); }
    }

    Benchmark::Benchmark(const char* name, bench_t func) {
        auto& reg = Registry::get();
        reg.add(name, func);
    }

    namespace {
        struct {
            cv::Mat grayNoise;
            cv::Mat grayCircles;
            cv::Mat grayBlack;
            cv::Mat rect;

            cv::Mat out1;
            cv::Mat out2;
            cv::Mat out3;

            fmo::Image grayNoiseImage;
            fmo::Image grayCirclesImage;
            fmo::Image grayBlackImage;
            fmo::Image yuv420SpNoiseImage;
            fmo::Image yuv420SpNoiseImage2;
            fmo::Image yuv420SpNoiseImage3;
            fmo::Image yuvNoiseImage;
            fmo::Image yuvNoiseImage2;
            fmo::Image outImage;
            std::vector<fmo::Image> outImageVec;

            std::mt19937 re{5489};
            using limits = std::numeric_limits<int>;
            std::uniform_int_distribution<int> uniform{limits::min(), limits::max()};
            std::uniform_int_distribution<int> randomGray{2, 254};
            std::unique_ptr<fmo::Algorithm> algorithmGray;
            std::unique_ptr<fmo::Algorithm> algorithmYuv420Sp;
            fmo::Subsampler subsampler;
            fmo::Differentiator::Config diffCfg;
            fmo::Differentiator diff{diffCfg};
            fmo::StripGen stripGen;
            std::vector<fmo::Pos16> pos16Vec;
            std::vector<fmo::Strip> stripVec;
        } global;

        struct Init {
            static const int W = 1920;
            static const int H = 1080;

            static cv::Mat newGrayMat() { return {cv::Size{W, H}, CV_8UC1}; }

            Init() {

                {
                    global.grayNoise = newGrayMat();
                    auto* data = global.grayNoise.data;
                    auto* end = data + (W * H);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }

                    global.grayNoiseImage.assign(fmo::Format::GRAY, {W, H}, global.grayNoise.data);
                }

                {
                    global.yuv420SpNoiseImage.resize(fmo::Format::YUV420SP, {W, H});
                    auto* data = global.yuv420SpNoiseImage.data();
                    auto* end = data + (3 * W * H / 2);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }
                }

                {
                    global.yuv420SpNoiseImage2.resize(fmo::Format::YUV420SP, {W, H});
                    auto* data = global.yuv420SpNoiseImage2.data();
                    auto* end = data + (3 * W * H / 2);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }
                }

                {
                    global.yuv420SpNoiseImage3.resize(fmo::Format::YUV420SP, {W, H});
                    auto* data = global.yuv420SpNoiseImage3.data();
                    auto* end = data + (3 * W * H / 2);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }
                }

                {
                    global.yuvNoiseImage.resize(fmo::Format::YUV, {W, H});
                    auto* data = global.yuvNoiseImage.data();
                    auto* end = data + (3 * W * H);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }
                }

                {
                    global.yuvNoiseImage2.resize(fmo::Format::YUV, {W, H});
                    auto* data = global.yuvNoiseImage2.data();
                    auto* end = data + (3 * W * H);

                    for (; data < end; data += sizeof(int)) {
                        *(int*)data = global.uniform(global.re);
                    }
                }

                {
                    global.grayCircles = newGrayMat();
                    auto* data = global.grayCircles.data;

                    for (int r = 0; r < H; r++) {
                        int rmod = ((r + 128) % 256);
                        int dy = std::min(rmod, 256 - rmod);
                        int dy2 = dy * dy;
                        for (int c = 0; c < W; c++) {
                            int cmod = ((c + 128) % 256);
                            int dx = std::min(cmod, 256 - cmod);
                            int dx2 = dx * dx;
                            *data++ = (dx2 + dy2 < 10000) ? 0xFF : 0x00;
                        }
                    }

                    global.grayCirclesImage.assign(fmo::Format::GRAY, {W, H},
                                                   global.grayCircles.data);
                }

                {
                    global.grayBlack = newGrayMat();
                    auto* data = global.grayBlack.data;
                    auto len = size_t(W * H);
                    std::memset(data, 0, len);

                    global.grayBlackImage.assign(fmo::Format::GRAY, {W, H}, global.grayBlack.data);
                }

                global.rect = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});

                {
                    fmo::Algorithm::Config cfg;
                    global.algorithmGray = Algorithm::make(cfg, fmo::Format::GRAY, {W, H});
                    global.algorithmYuv420Sp = Algorithm::make(cfg, fmo::Format::YUV420SP, {W, H});
                }
            }
        };

        void init() { static Init once; }

        Benchmark FMO_UNIQUE_NAME{"fmo::Subsampler GRAY", []() {
                                      init();
                                      global.subsampler(global.grayNoiseImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::Subsampler YUV", []() {
                                      init();
                                      global.subsampler(global.yuvNoiseImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::Subsampler YUV420SP", []() {
                                      init();
                                      global.subsampler(global.yuv420SpNoiseImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::StripGen", []() {
                                      init();
                                      int outNoise;
                                      global.stripVec.clear();
                                      global.stripGen(global.grayCirclesImage, 2, 1, 2,
                                                      global.stripVec, outNoise);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::Differentiator", []() {
                                      init();
                                      global.diff(global.yuvNoiseImage, global.yuvNoiseImage2,
                                                  global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::copy + fmo::Algorithm GRAY", []() {
                                      init();
                                      static int i = 0;

                                      switch (i++ % 3) {
                                      case 0:
                                          fmo::copy(global.grayCirclesImage, global.outImage);
                                          break;
                                      case 1:
                                          fmo::copy(global.grayNoiseImage, global.outImage);
                                          break;
                                      case 2:
                                          fmo::copy(global.grayBlackImage, global.outImage);
                                          break;
                                      }

                                      global.algorithmGray->setInputSwap(global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::copy + fmo::Algorithm YUV420SP", []() {
                                      init();
                                      static int i = 0;

                                      switch (i++ % 3) {
                                      case 0:
                                          fmo::copy(global.yuv420SpNoiseImage, global.outImage);
                                          break;
                                      case 1:
                                          fmo::copy(global.yuv420SpNoiseImage2, global.outImage);
                                          break;
                                      case 2:
                                          fmo::copy(global.yuv420SpNoiseImage3, global.outImage);
                                          break;
                                      }

                                      global.algorithmYuv420Sp->setInputSwap(global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::copy GRAY", []() {
                                      init();
                                      fmo::copy(global.grayNoiseImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::copy YUV420SP", []() {
                                      init();
                                      fmo::copy(global.yuv420SpNoiseImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"fmo::median3", []() {
                                      init();
                                      fmo::median3(global.grayNoiseImage, global.grayCirclesImage,
                                                   global.grayBlackImage, global.outImage);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::bitwise_or", []() {
                                      init();
                                      cv::bitwise_or(global.grayNoise, global.grayCircles,
                                                     global.out1);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::resize/NEAREST", []() {
                                      init();
                                      cv::resize(global.grayNoise, global.out1,
                                                 {Init::W / 2, Init::H / 2}, 0, 0,
                                                 cv::INTER_NEAREST);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::resize/AREA", []() {
                                      init();
                                      cv::resize(global.grayNoise, global.out1,
                                                 {Init::W / 2, Init::H / 2}, 0, 0, cv::INTER_AREA);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::threshold", []() {
                                      init();
                                      cv::threshold(global.grayNoise, global.out1, 0x80, 0xFF,
                                                    cv::THRESH_BINARY);
                                  }};

        Benchmark FMO_UNIQUE_NAME{"cv::absdiff", []() {
                                      init();
                                      cv::absdiff(global.grayNoise, global.grayCircles,
                                                  global.out1);
                                  }};
    }
}
