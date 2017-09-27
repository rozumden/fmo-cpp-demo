#include "env.hpp"
#include "java_classes.hpp"
#include <atomic>
#include <fmo/exchange.hpp>
#include <fmo/processing.hpp>
#include <fmo/stats.hpp>
#include <iomanip>
#include <thread>

namespace {
    struct {
        std::mutex mutex;
        JavaVM* javaVM;
        std::atomic<bool> stop;
        std::unique_ptr<fmo::Exchange<fmo::Image>> exchange;
        Reference<Callback> callbackRef;
        fmo::Image image;
        fmo::Dims dims;
        fmo::Format format;
        fmo::Algorithm::Config config;
    } global;

    std::string statsString(const fmo::SectionStats& stats) {
        auto q = stats.quantilesMs();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << q.q50 << " / ";
        oss << std::fixed << std::setprecision(1) << q.q95 << " / ";
        oss << std::fixed << std::setprecision(0) << q.q99;
        return oss.str();
    }

    void threadImpl() {
        Env threadEnv{global.javaVM, "Lib"};
        JNIEnv* env = threadEnv.get();
        fmo::FrameStats frameStats;
        frameStats.reset(30);
        fmo::SectionStats sectionStats;
        fmo::Image input{global.format, global.dims};
        fmo::Algorithm::Output output;
        auto explorer = fmo::Algorithm::make(global.config, global.format, global.dims);
        Callback callback = global.callbackRef.get(env);
        callback.log("Detection started");

        while (!global.stop) {
            global.exchange->swapReceive(input);
            if (global.stop) break;

            frameStats.tick();
            sectionStats.start();
            explorer->setInputSwap(input);
            explorer->getOutput(output);
            bool statsUpdated = sectionStats.stop();

            if (statsUpdated) {
                std::string stats = statsString(sectionStats);
                callback.log(stats.c_str());
            }

            if (!output.detections.empty()) {
                jint numDetections = jint(output.detections.size());
                DetectionArray detections(env, numDetections);

                for (jint i = 0; i < numDetections; i++) {
                    Detection d{env, *output.detections[i]};
                    detections.set(i, d);
                }

                callback.onObjectsDetected(detections);
            }
        }

        global.callbackRef.release(env);
    }

    bool running() { return bool(global.exchange); }
}

void Java_cz_fmo_Lib_detectionStart(JNIEnv* env, jclass, jint width, jint height, jint procRes,
                                    jboolean gray, jobject cbObj) {
    initJavaClasses(env);

    std::unique_lock<std::mutex> lock(global.mutex);
    global.config.maxImageHeight = procRes;
    global.format = (gray != 0) ? fmo::Format::GRAY : fmo::Format::YUV420SP;
    global.dims = {width, height};
    env->GetJavaVM(&global.javaVM);
    global.stop = false;
    global.exchange.reset(new fmo::Exchange<fmo::Image>(global.format, global.dims));
    global.callbackRef = {env, cbObj};

    std::thread thread(threadImpl);
    thread.detach();

    global.image.resize(global.format, global.dims);
}

void Java_cz_fmo_Lib_detectionStop(JNIEnv* env, jclass) {
    std::unique_lock<std::mutex> lock(global.mutex);
    if (!running()) return;
    global.stop = true;
    global.exchange->exit();
}

void Java_cz_fmo_Lib_detectionFrame(JNIEnv* env, jclass, jbyteArray dataYUV420SP) {
    std::unique_lock<std::mutex> lock(global.mutex);
    if (!running()) return;
    jbyte* dataJ = env->GetByteArrayElements(dataYUV420SP, nullptr);
    uint8_t* data = reinterpret_cast<uint8_t*>(dataJ);
    global.image.assign(global.format, global.dims, data);
    global.exchange->swapSend(global.image);
    env->ReleaseByteArrayElements(dataYUV420SP, dataJ, JNI_ABORT);
}
