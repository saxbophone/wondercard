#include <optional>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>

using namespace com::saxbophone::ps1_memcard_protocol;

SCENARIO("MemoryCard can be powered on when off and off when on") {
    GIVEN("A MemoryCard that is powered off") {
        MemoryCard card;
        REQUIRE_FALSE(card.powered_on);
        THEN("The MemoryCard can be powered on successfully") {
            REQUIRE(card.power_on());
            AND_WHEN("The MemoryCard is powered on") {
                REQUIRE(card.powered_on);
                THEN("The MemoryCard can be powered off successfully") {
                    REQUIRE(card.power_off());
                    AND_WHEN("The MemoryCard is powered off") {
                        REQUIRE_FALSE(card.powered_on);
                        THEN("The MemoryCard cannot be powered off successfully") {
                            CHECK_FALSE(card.power_off());
                        }
                    }
                }
                THEN("The MemoryCard cannot be powered on successfully") {
                    CHECK_FALSE(card.power_on());
                }
            }
        }
    }
}

SCENARIO("MemoryCard ignores commands sent to it when powered off") {
    GIVEN("A MemoryCard that is powered off") {
        MemoryCard card;
        WHEN("A command is sent for the card") {
            std::uint8_t command = 0x81; // generic: "hey memory card!" message
            std::optional<std::uint8_t> response = std::nullopt;
            bool ack = card.send(command, response);
            THEN("The card does not acknowledge the command or send return data") {
                CHECK_FALSE(ack);
                CHECK(response == std::nullopt);
            }
        }
    }
}

SCENARIO("MemoryCard ignores commands that are not memory card commands") {
    GIVEN("A MemoryCard that is powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A command byte that is not 0x81 (memory card command)") {
            std::uint8_t command = GENERATE(
                take(
                    100,
                    filter(
                        [](std::uint8_t i) { return i != 0x81; },
                        random(0x00, 0xFF)
                    )
                )
            );
            INFO("command is:" << command);
            WHEN("The command byte is sent to the MemoryCard") {
                std::optional<std::uint8_t> response = std::nullopt;
                bool ack = card.send(command, response);
                THEN("The card does not acknowledge the command or send return data") {
                    CHECK_FALSE(ack);
                    CHECK(response == std::nullopt);
                }
            }
        }
    }
}

SCENARIO("Reading Data from Memory Card") {
    GIVEN("A MemoryCard that is zero-initialised and powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A sequence of command bytes to read an invalid sector") {
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0400, 0xFFFF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[] = {
                0x81, 0x52, 0x00, 0x00, msb, lsb, 0x00, 0x00, 0x00, 0x00,
            };
            AND_GIVEN("A sqeuence of expected response bytes indicating read failure") {
                std::optional<std::uint8_t> expected_outputs[] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00, 0x5C, 0x5D, 0xFF, 0xFF,
                };
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected read failure response") {
                        for (std::size_t i = 0; i < 10; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            CHECK(card.send(inputs[i], output)); // check card ACK
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        AND_GIVEN("A sequence of command bytes to read a valid sector") {
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000, 0x03FF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[140] = {
                0x81, 0x52, 0x00, 0x00, msb, lsb, 0x00, 0x00, 0x00, 0x00,
                // remaining members set to zero, which is what we want
            };
            AND_GIVEN("A sqeuence of expected response bytes indicating read data") {
                std::optional<std::uint8_t> expected_outputs[140] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00, 0x5C, 0x5D, msb, lsb,
                    // 128 zero bytes now expected to follow (card zero-initialised)
                    // checksum, "good read" magic end byte
                    [138] = msb ^ lsb, 0x47,
                };
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected read success response") {
                        for (std::size_t i = 0; i < 140; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            CHECK(card.send(inputs[i], output)); // check card ACK
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
    }
    // TODO: tests for MemoryCards that contain non-zero data!
}

SCENARIO("Writing Data to Memory Card") {
    GIVEN("A MemoryCard that is zero-initialised and powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A sequence of command bytes to write an invalid sector") {
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0400, 0xFFFF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[138] = {
                0x81, 0x57, 0x00, 0x00, msb, lsb,
                // 128 zero bytes now expected to follow (writing blanks)
                // checksum and remaining blank bytes to read responses
                [134] = msb ^ lsb, 0x00, 0x00, 0x00,
            };
            AND_GIVEN("A sqeuence of expected response bytes indicating bad sector") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                    // checksum receive, acknowledge, end status byte = bad sector
                    [134] = 0x00, 0x5C, 0x5D, 0xFF,
                };
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write failure response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            CHECK(card.send(inputs[i], output)); // check card ACK
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        AND_GIVEN("A sequence of command bytes to write a valid sector") {
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000, 0x03FF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[138] = {
                0x81, 0x57, 0x00, 0x00, msb, lsb,
                // 128 zero bytes now expected to follow (writing blanks)
                // checksum and remaining blank bytes to read responses
                [134] = msb ^ lsb, 0x00, 0x00, 0x00,
            };
            AND_GIVEN("A sqeuence of expected response bytes indicating write success") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                    // checksum receive, acknowledge, end status byte = Good Write
                    [134] = 0x00, 0x5C, 0x5D, 0x47,
                };
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write success response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            CHECK(card.send(inputs[i], output)); // check card ACK
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        // TODO: Bad Checksum test
    }
}

SCENARIO("Get Memory Card ID Command") {
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
    }
}
