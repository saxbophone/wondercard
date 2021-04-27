#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/Public.hpp>

using namespace com::saxbophone::ps1_memcard_protocol;
// test case to check network construction and train movement
TEST_CASE("Library works") {
    REQUIRE(library_works());
}
