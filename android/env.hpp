#ifndef FMO_ANDROID_ENV_HPP
#define FMO_ANDROID_ENV_HPP

#include "java_interface.hpp"
#include <fmo/assert.hpp>

struct Env {
    Env(const Env&) = delete;

    Env& operator=(const Env&) = delete;

    Env(JavaVM* vm, const char* threadName) : mVM(vm) {
        JavaVMAttachArgs args = {JNI_VERSION_1_6, threadName, nullptr};
        jint result = mVM->AttachCurrentThread(&mPtr, &args);
        FMO_ASSERT(result == JNI_OK, "AttachCurrentThread failed");
    }

    ~Env() { mVM->DetachCurrentThread(); }

    JNIEnv* get() { return mPtr; }

    JNIEnv& operator->() { return *mPtr; }

private:
    JavaVM* mVM;
    JNIEnv* mPtr = nullptr;
};

#endif //FMO_ANDROID_ENV_HPP
