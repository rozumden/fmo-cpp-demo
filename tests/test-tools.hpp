#ifndef FMO_TEST_TOOLS_HPP
#define FMO_TEST_TOOLS_HPP

#include <fmo/common.hpp>
#include <fmo/image.hpp>
#include <fmo/processing.hpp>

template <typename Lhs, typename Rhs>
bool exact_match(const Lhs& lhs, const Rhs& rhs) {
    auto res = std::mismatch(begin(lhs), end(lhs), begin(rhs), end(rhs));
    return res.first == end(lhs) && res.second == end(rhs);
}

template <typename Lhs, typename Rhs>
bool almost_exact_match(const Lhs& lhs, const Rhs& rhs, uint8_t maxError) {
    auto res = std::mismatch(begin(lhs), end(lhs), begin(rhs), end(rhs), [=] (uint8_t l, uint8_t r) {
        return ((l > r) ? (l - r) : (r - l)) <= maxError;
    });
    return res.first == end(lhs) && res.second == end(rhs);
}

inline bool match_percent(const fmo::Image& lhs, const fmo::Image& rhs, double require) {
    fmo::Image diff;
    fmo::absdiff(lhs, rhs, diff);
    auto numTotal = end(diff) - begin(diff);
    auto numCorrect = std::count(begin(diff), end(diff), 0);
    auto percent = 100. * double(numCorrect) / double(numTotal);
    return percent > require;
}

#endif // FMO_TEST_TOOLS_HPP
