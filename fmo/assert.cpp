#if defined(ANDROID)

#include <android/log.h>

namespace fmo {
    void assertFail(const char* what, const char* file, int line) {
        __android_log_print(ANDROID_LOG_ERROR, "fmo-android", "Assert fail \"%s\" at %s:%d", what,
                            file, line);
    }

    template <typename T>
    void assertInfo(T* what, const char* file, int line) {

    }
}

#else

#include <iostream>

namespace fmo {
    void assertFail(const char* what, const char* file, int line) {
        std::cerr << "Assert fail \"" << what << "\" at " << file << ':' << line << std::endl;
    }

    void assertInfo(const char* what, const char* file, int line) {
        std::cout << what << " at " << file << ':' << line << std::endl;
    }

    void assertInfo(int what, const char* file, int line) {
        std::cout << what << " at " << file << ':' << line << std::endl;
    }

    void assertInfo(float what, const char* file, int line) {
        std::cout << what << " at " << file << ':' << line << std::endl;
    }

    void assertInfo(double what, const char* file, int line) {
        std::cout << what << " at " << file << ':' << line << std::endl;
    }

}

#endif
