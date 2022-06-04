#pragma once


#include "doctest.h"

#include <cstdio>
#include <filesystem>
#include <system_error>

#include <persia/mapped_file.hpp>


TEST_SUITE("mapped_file") {
    
    SCENARIO("mapping non existent file") {
        auto target = persia::mapped_file::create("42");
        REQUIRE(!target);
    }
    
    
    SCENARIO("mapping empty file") {
        auto* file = std::fopen("dummy", "w+b");
        REQUIRE(!!file);
        std::fclose(file);
        auto target = persia::mapped_file::create("dummy");
        REQUIRE(!target);
        auto ec = std::error_code{};
        std::filesystem::remove("dummy", ec);
    }
    
    SCENARIO("mapping non empty file") {
        auto* file = std::fopen("dummy", "w+b");
        REQUIRE(!!file);
        char buffer[4096];
        std::fwrite(buffer, sizeof(char), sizeof(buffer), file);
        std::fclose(file);
        auto target = persia::mapped_file::create("dummy");
        REQUIRE(!!target);
        REQUIRE_EQ(target->size(), 4096);
        target = persia::mapped_file{};
        auto ec = std::error_code{};
        std::filesystem::remove("dummy", ec);
    }
    
}
