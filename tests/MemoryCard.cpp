#include <optional>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>

using namespace com::saxbophone::ps1_memcard_protocol;

SCENARIO("MemoryCard can be powered on when off and off when on") {
    GIVEN("A MemoryCard that is powered off (default state)") {
        MemoryCard card;
        THEN("The MemoryCard can be powered on successfully") {
            REQUIRE(card.power_on());
            AND_WHEN("The MemoryCard is powered on") {
                THEN("The MemoryCard can be powered off successfully") {
                    REQUIRE(card.power_off());
                    AND_WHEN("The MemoryCard is powered off") {
                        THEN("The MemoryCard cannot be powered off successfully") {
                            REQUIRE_FALSE(card.power_off());
                        }
                    }
                }
                THEN("The MemoryCard cannot be powered on successfully") {
                    REQUIRE_FALSE(card.power_on());
                }
            }
        }
    }
}

SCENARIO("Get ID Command can be sent to Memory Card with correct response") {
    GIVEN("A MemoryCard that is powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A sequence of command bytes to get memory card ID") {
            std::optional<std::uint8_t> inputs[] = {
                0x81, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            };
            AND_GIVEN("A sequence of data bytes the card is expected to respond with") {
                std::optional<std::uint8_t> expected_outputs[] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x5C, 0x5D, 0x04, 0x00, 0x00, 0x80,
                };
                WHEN("The sqeuence of command bytes is sent to the card") {
                    THEN("The card responds with the expected response byte") {
                        for (std::size_t i = 0; i < 10; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            CHECK(card.send(inputs[i], output)); // check card ACK
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        // power down the card
        REQUIRE(card.power_off());
    }
}
