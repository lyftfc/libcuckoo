#include "catch.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include "../../src/cuckoohash_map.hh"
#include "unit_test_util.hh"


TEST_CASE("empty table iteration", "[iterator]") {
    IntIntTable table;
    {
        auto lt = table.lock_table();
        auto it = lt.begin();
        REQUIRE(it == lt.begin());
        REQUIRE(it == lt.end());
        it = lt.cbegin();
        REQUIRE(it == lt.begin());
        REQUIRE(it == lt.end());
        it = lt.end();
        REQUIRE(it == lt.begin());
        REQUIRE(it == lt.end());
        it = lt.cend();
        REQUIRE(it == lt.begin());
        REQUIRE(it == lt.end());
    }
}

template <class Iterator>
void AssertIteratorIsReleased(Iterator& it) {
    REQUIRE_THROWS_AS(*it, std::runtime_error);
    REQUIRE_THROWS_AS((void) it->first, std::runtime_error);

    REQUIRE_THROWS_AS(it++, std::runtime_error);
    REQUIRE_THROWS_AS(++it, std::runtime_error);
    REQUIRE_THROWS_AS(it--, std::runtime_error);
    REQUIRE_THROWS_AS(--it, std::runtime_error);
}

template <class LockedTable>
void AssertLockedTableIsReleased(LockedTable& lt) {
    REQUIRE_THROWS_AS(lt.begin(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.end(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.cbegin(), std::runtime_error);
    REQUIRE_THROWS_AS(lt.cend(), std::runtime_error);
}

TEST_CASE("iterator release", "[iterator]") {
    IntIntTable table;
    table.insert(10, 10);

    SECTION("explicit release") {
        auto lt = table.lock_table();
        auto it = lt.begin();
        lt.release();
        AssertIteratorIsReleased(it);
        AssertLockedTableIsReleased(lt);
    }

    SECTION("release through destructor") {
        auto lt = table.lock_table();
        auto it = lt.begin();
        lt.IntIntTable::locked_table::~locked_table();
        AssertIteratorIsReleased(it);
        AssertLockedTableIsReleased(lt);
        lt.release();
        AssertIteratorIsReleased(it);
        AssertLockedTableIsReleased(lt);
    }
}

TEST_CASE("iterator walkthrough", "[iterator]") {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    SECTION("forward postfix walkthrough") {
        auto lt = table.lock_table();
        auto it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
            auto old_it = it;
            REQUIRE(old_it == it++);
        }
        REQUIRE(it == lt.end());
    }

    SECTION("forward prefix walkthrough") {
        auto lt = table.lock_table();
        auto it = lt.cbegin();
        for (size_t i = 0; i < table.size(); ++i) {
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
            ++it;
        }
        REQUIRE(it == lt.end());
    }

    SECTION("backwards postfix walkthrough") {
        auto lt = table.lock_table();
        auto it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            auto old_it = it;
            REQUIRE(old_it == it--);
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
        }
        REQUIRE(it == lt.begin());
    }

    SECTION("backwards prefix walkthrough") {
        auto lt = table.lock_table();
        auto it = lt.cend();
        for (size_t i = 0; i < table.size(); ++i) {
            --it;
            REQUIRE((*it).first == (*it).second);
            REQUIRE(it->first == it->second);
        }
        REQUIRE(it == lt.begin());
    }
}

TEST_CASE("iterator modification", "[iterator]") {
    IntIntTable table;
    for (int i = 0; i < 10; ++i) {
        table.insert(i, i);
    }

    auto lt = table.lock_table();
    for (auto it = lt.begin(); it != lt.end(); ++it) {
        it->second = it->second + 1;
    }

    auto it = lt.cbegin();
    for (size_t i = 0; i < table.size(); ++i) {
        REQUIRE(it->first == it->second - 1);
        ++it;
    }
    REQUIRE(it == lt.end());
}


TEST_CASE("lock table blocks inserts", "[iterator]") {
    IntIntTable table;
    auto lt = table.lock_table();
    std::thread thread([&table] () {
            for (int i = 0; i < 10; ++i) {
                table.insert(i, i);
            }
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(table.size() == 0);
    lt.release();
    thread.join();

    REQUIRE(table.size() == 10);
}
