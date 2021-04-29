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

// TODO: test case for invalid memory card commands!

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
                            bool ack = card.send(inputs[i], output);
                            if (i != 9) { // unless last byte, check card ACK
                                CHECK(ack);
                            } else {
                                CHECK_FALSE(ack);
                            }
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
            };
            // set remaining members to zero (can't default init because of optional)
            for (std::size_t i = 10; i < 140; i++) {
                inputs[i] = 0x00;
            }
            AND_GIVEN("A sqeuence of expected response bytes indicating read data") {
                std::optional<std::uint8_t> expected_outputs[140] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00, 0x5C, 0x5D, msb, lsb,
                };
                // 128 zero bytes are expected in output sector data
                for (std::size_t i = 10; i < 138; i++) {
                    expected_outputs[i] = 0x00;
                }
                expected_outputs[138] = msb ^ lsb; // checksum
                expected_outputs[139] = 0x47;      // "good read" magic end byte
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected read success response") {
                        for (std::size_t i = 0; i < 140; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 139) { // unless last byte, check card ACK
                                CHECK(ack);
                            } else {
                                CHECK_FALSE(ack);
                            }
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
                // all other bytes should be zero, except checksum
            };
            inputs[134] = msb ^ lsb; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating bad sector") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                expected_outputs[134] = 0x00; // checksum received
                expected_outputs[135] = 0x5C; // ACK 1
                expected_outputs[136] = 0x5D; // ACK 2
                expected_outputs[137] = 0xFF; // end status = bad sector!
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
            };
            inputs[134] = msb ^ lsb; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating write success") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                expected_outputs[134] = 0x00; // checksum received
                expected_outputs[135] = 0x5C; // ACK 1
                expected_outputs[136] = 0x5D; // ACK 2
                expected_outputs[137] = 0x47; // end status = good write!
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
                            bool ack = card.send(inputs[i], output);
                            if (i != 9) { // unless last byte, check card ACK
                                CHECK(ack);
                            } else {
                                CHECK_FALSE(ack);
                            }
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
    }
}
