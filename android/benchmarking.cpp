#include "env.hpp"
#include "java_classes.hpp"
#include <atomic>
#include <fmo/benchmark.hpp>
#include <thread>

namespace {
    struct {
        Reference<Callback> callbackRef;
        JavaVM* javaVM;
        JNIEnv* threadEnv;
        std::atomic<bool> stop;
    } global;
}

void Java_cz_fmo_Lib_benchmarkingStart(JNIEnv* env, jclass, jobject cbObj) {
    initJavaClasses(env);
    global.callbackRef = {env, cbObj};
    global.stop = false;
    env->GetJavaVM(&global.javaVM);

    std::thread thread([]() {
        Env threadEnv{global.javaVM, "benchmarking"};
        global.threadEnv = threadEnv.get();
        fmo::Registry::get().runAll([](const char* cStr) {
                                        Callback cb{global.callbackRef.get(global.threadEnv)};
                                        cb.log(cStr);
                                    },
                                    []() {
                                        return bool(global.stop);
                                    });
        global.callbackRef.release(global.threadEnv);
    });

    thread.detach();
}

void Java_cz_fmo_Lib_benchmarkingStop(JNIEnv* env, jclass) {
    global.stop = true;
}
