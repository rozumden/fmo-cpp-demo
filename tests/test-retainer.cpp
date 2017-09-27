#include "../catch/catch.hpp"
#include <fmo/retainer.hpp>

namespace {
    struct Dummy {
        int id = -1;
        int clears = 0;
        void clear() { clears++; }
    };
}

TEST_CASE("Retainer", "[retainer]") {
    fmo::Retainer<Dummy, 2> r;
    REQUIRE(r.size() == 0);
    REQUIRE(r.empty() == true);

    r.emplace_back();
    r.back().id = 1;
    r.emplace_back();
    r.back().id = 2;
    r.emplace_back();
    r.back().id = 3;
    r.emplace_back();
    r.back().id = 4;

    REQUIRE(r.size() == 4);
    REQUIRE(r.empty() == false);

    REQUIRE(r.back().id == 4);
    r.pop_back();
    REQUIRE(r.back().id == 3);
    r.pop_back();
    REQUIRE(r.back().id == 2);
    r.pop_back();
    REQUIRE(r.back().id == 1);
    REQUIRE(r.size() == 1);
    REQUIRE(r.empty() == false);
    r.pop_back();
    REQUIRE(r.size() == 0);
    REQUIRE(r.empty() == true);

    r.emplace_back();
    REQUIRE(r.back().id == 1);
    REQUIRE(r.back().clears == 1);
    r.emplace_back();
    REQUIRE(r.back().id == 2);
    REQUIRE(r.back().clears == 1);
    r.emplace_back();
    REQUIRE(r.back().id == -1);
    REQUIRE(r.back().clears == 0);

    r.clear();
    REQUIRE(r.size() == 0);
    REQUIRE(r.empty() == true);

    r.emplace_back();
    REQUIRE(r.back().id == 1);
    REQUIRE(r.back().clears == 2);
    r.emplace_back();
    REQUIRE(r.back().id == 2);
    REQUIRE(r.back().clears == 2);
    r.emplace_back();
    REQUIRE(r.back().id == -1);
    REQUIRE(r.back().clears == 0);

    auto i = r.begin();
    REQUIRE(i == begin(r));
    REQUIRE(i->id == 1);
    REQUIRE((*i).clears == 2);
    auto j = i++;
    REQUIRE(i->id == 2);
    REQUIRE((*i).clears == 2);
    REQUIRE(i != j);
    auto k = ++i;
    REQUIRE(i->id == -1);
    REQUIRE((*i).clears == 0);
    REQUIRE(i != r.end());
    REQUIRE(i != end(r));
    REQUIRE(i == k);
    i++;
    REQUIRE(i == r.end());
    REQUIRE(i == end(r));

    REQUIRE(r[0].id == 1);
    REQUIRE(r[0].clears == 2);
    REQUIRE(r[1].id == 2);
    REQUIRE(r[1].clears == 2);
    REQUIRE(r[2].id == -1);
    REQUIRE(r[2].clears == 0);
}
