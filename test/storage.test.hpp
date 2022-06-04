#pragma once


#include <filesystem>
#include <system_error>

#include "doctest.h"



#include <persia/storage.hpp>


struct item {
    int key;
    int data;
    
    static key_of(item const& item) noexcept {
        return item.key;
    }
};

using storage = persia::storage<int, item>;

TEST_SUITE("storage") {
    
    SCENARIO("open non existing storage") {
        auto ec = std::error_code{};
        std::filesystem::remove("test.pmap", ec);
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!expected_target);
    }

    
    SCENARIO("create storage") {
        auto expected_target = storage::create("test.pmap", 1);
        REQUIRE(!!expected_target);
        REQUIRE_EQ(expected_target->capacity(), 1);
        REQUIRE_EQ(expected_target->size(), 0);
        REQUIRE(expected_target->empty());
        REQUIRE(!expected_target->fully_occupied());
    }
    
    
    SCENARIO("open existing storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        REQUIRE_EQ(expected_target->capacity(), 1);
        REQUIRE_EQ(expected_target->size(), 0);
        REQUIRE(expected_target->empty());
        REQUIRE(!expected_target->fully_occupied());
    }
    
    SCENARIO("inserting to storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        auto const inserted = expected_target->insert(item{1, 1});
        REQUIRE(inserted);
        REQUIRE_EQ(expected_target->capacity(), 1);
        REQUIRE_EQ(expected_target->size(), 1);
        REQUIRE(!expected_target->empty());
        REQUIRE(expected_target->fully_occupied());
    }
    
    
    SCENARIO("updating in storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        auto* item = expected_target->find(1);
        REQUIRE(!!item);
        item->data = 2;
    }
    
    
    SCENARIO("reading updated value from storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        auto const* item = expected_target->find(1);
        REQUIRE(!!item);
        REQUIRE_EQ(item->data, 2);
    }
    
    
    SCENARIO("insertinf into fully occupied storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        auto const inserted = expected_target->insert(item{2, 2});
        REQUIRE(!inserted);
        REQUIRE_EQ(expected_target->capacity(), 1);
        REQUIRE_EQ(expected_target->size(), 1);
    }
    
    
    SCENARIO("finding non existing key") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        auto const* item = expected_target->find(2);
        REQUIRE(!item);
    }
    
    
    SCENARIO("expanding storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        REQUIRE_EQ(expected_target->capacity(), 2);
        REQUIRE_EQ(expected_target->size(), 1);
        auto const inserted = expected_target->insert(item{2, 2});
        REQUIRE(inserted);
        REQUIRE_EQ(expected_target->size(), 2);
        REQUIRE(expected_target->fully_occupied());
    }
    
    
    SCENARIO("opening expanded storage") {
        auto expected_target = storage::open("test.pmap", 1);
        REQUIRE(!!expected_target);
        REQUIRE_EQ(expected_target->capacity(), 2);
        REQUIRE_EQ(expected_target->size(), 2);
    }
    
    
    SCENARIO("inserting or assigning to storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        auto const inserted = expected_target->insert_or_assign(item{1, 1});
        REQUIRE(inserted);
    }
    
    
    SCENARIO("iterating through storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        auto sum = 0;
        for(auto it = expected_target->begin(); it != expected_target->end(); ++it)
            sum += it->data;
        REQUIRE_EQ(sum, 3);
    }
    
    
    SCENARIO("removing non existing key from storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        auto const erased = expected_target->erase(3);
        REQUIRE(!erased);
        REQUIRE_EQ(expected_target->size(), 2);
    }
    
    
    SCENARIO("removing existing key from storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        auto const erased = expected_target->erase(1);
        REQUIRE(erased);
        REQUIRE_EQ(expected_target->size(), 1);
    }
    
    SCENARIO("clearing storage") {
        auto expected_target = storage::open("test.pmap", 2);
        REQUIRE(!!expected_target);
        expected_target->insert(item{1, 1});
        expected_target->clear();
        REQUIRE_EQ(expected_target->size(), 0);
        REQUIRE(expected_target->empty());
    }
    
}
