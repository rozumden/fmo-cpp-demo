#ifndef FMO_BENCHMARK_HPP
#define FMO_BENCHMARK_HPP

#include <functional>
#include <vector>

namespace fmo {
    using log_t = void(*)(const char*);
    using stop_t = bool(*)();
    using bench_t = void(*)();

    struct Registry {
        Registry(const Registry&) = delete;

        Registry& operator=(const Registry&) = delete;

        static Registry& get();

        void add(const char* name, bench_t func) { mFuncs.emplace_back(name, func); }

        void runAll(log_t logFunc, stop_t stopFunc) const;

    private:
        Registry() = default;

        std::vector<std::pair<const char*, bench_t>> mFuncs;
    };

    struct Benchmark {
        Benchmark() = delete;

        Benchmark(const Benchmark&) = delete;

        Benchmark& operator=(const Benchmark&) = delete;

        Benchmark(const char* name, bench_t);
    };
}

#define FMO_CONCAT_IMPL(symbol1, symbol2) symbol1##symbol2
#define FMO_CONCAT(symbol1, symbol2) FMO_CONCAT_IMPL(symbol1, symbol2)
#define FMO_UNIQUE_NAME FMO_CONCAT(FMO_UNIQUE_, __COUNTER__)

#endif // FMO_BENCHMARK_HPP
