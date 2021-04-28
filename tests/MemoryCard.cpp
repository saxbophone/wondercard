#include <optional>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>

using namespace com::saxbophone::ps1_memcard_protocol;
// test case to check network construction and train movement
TEST_CASE("Get Memory Card ID Command") {
    MemoryCard card;
    // sequence of command bytes to get memory card ID
    std::optional<std::uint8_t> inputs[] = {
        0x81, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    // sequence of expected response bytes that card should respond with
    std::optional<std::uint8_t> expected_outputs[] = {
        std::nullopt, 0x08, 0x5A, 0x5D, 0x5C, 0x5D, 0x04, 0x00, 0x00, 0x80,
    };

    // power up the card
    REQUIRE(card.power_on());
    // send each command byte and check the response in turn
    for (std::size_t i = 0; i < 10; i++) {
        std::optional<std::uint8_t> output = std::nullopt;
        CHECK(card.send(inputs[i], output)); // check card ACK
        // now check the response was as expected
        REQUIRE(output == expected_outputs[i]);
    }
    // power down the card
    REQUIRE(card.power_off());
}
