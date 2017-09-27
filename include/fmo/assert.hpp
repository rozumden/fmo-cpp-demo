#ifndef FMO_ASSERT_HPP
#define FMO_ASSERT_HPP

namespace fmo {
    void assertFail(const char* what, const char* file, int line);
    
    void assertInfo(const char* what, const char* file, int line);
    void assertInfo(int what, const char* file, int line);
    void assertInfo(float what, const char* file, int line);
    void assertInfo(double what, const char* file, int line);
}

#define FMO_ASSERT(expr, reason)                                                                   \
    if (!(expr)) { fmo::assertFail(reason, __FILE__, __LINE__); }

#define FMO_INFO(reason)                                                                   \
    if (true) { fmo::assertInfo(reason, __FILE__, __LINE__); }

#endif // FMO_ASSERT_HPP
