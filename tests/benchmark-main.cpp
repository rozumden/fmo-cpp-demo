#include <fmo/benchmark.hpp>
#include <iostream>

int main() {
    fmo::Registry::get().runAll([](const char* cStr) { std::cout << cStr; },
                                []() { return false; });
}
