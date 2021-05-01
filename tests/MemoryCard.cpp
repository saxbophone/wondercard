#include <optional>
#include <random>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>


using namespace com::saxbophone::ps1_memcard_protocol;

template<std::size_t SIZE>
static std::array<std::uint8_t, SIZE> generate_random_bytes() {
    std::array<std::uint8_t, SIZE> data;
    std::default_random_engine engine;
    // N.B: Can't use uint8_t for type as generator doesn't support char types
    std::uniform_int_distribution<std::uint16_t> prng(0x0000, 0x00FF);
    for (auto& d : data) {
        d = prng(engine);
    }
    return data;
}

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

SCENARIO("MemoryCard properly handles invalid memory card commands") {
    GIVEN("A MemoryCard that is powered on and in memory card command mode") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        // send "memory card command mode" byte (0x81)
        std::optional<std::uint8_t> response = std::nullopt;
        REQUIRE(card.send(0x81, response)); // we need ACK otherwise can't test
        WHEN("An invalid memory card command byte is sent to the card") {
            std::uint8_t wrong_command = GENERATE(
                take(
                    100,
                    filter(
                        [](std::uint8_t c) {
                            return c != 0x52 and c != 0x53 and c != 0x57;
                        },
                        random(0x00, 0xFF)
                    )
                )
            );
            bool ack = card.send(wrong_command, response);
            THEN("The card responds with FLAG and NACK") {
                REQUIRE(response == 0x08);
                REQUIRE_FALSE(ack);
            }
        }
    }
}

SCENARIO("Reading Data from Memory Card") {
    GIVEN("A MemoryCard that is initialised with random data and powered on") {
        constexpr std::size_t CARD_SIZE = 128u * 1024u;
        auto data = generate_random_bytes<CARD_SIZE>();
        MemoryCard card(data);
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
                // set expected sector data to read from card
                auto sec = card.get_sector(sector);
                std::uint8_t data_checksum = 0x00;
                for (std::size_t i = 0; i < 128; i++) {
                    expected_outputs[10 + i] = sec[i];
                    data_checksum ^= sec[i];
                }
                expected_outputs[138] = msb ^ lsb ^ data_checksum; // checksum
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
}

SCENARIO("Writing Data to Memory Card") {
    GIVEN("A MemoryCard that is zero-initialised and powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A sequence of command bytes to write an invalid sector") {
            constexpr std::size_t SECTOR_SIZE = 128u;
            auto data = generate_random_bytes<SECTOR_SIZE>();
            // calculate data checksum for later use
            std::uint8_t data_checksum = 0x00;
            for (auto byte : data) {
                data_checksum ^= byte;
            }
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0400, 0xFFFF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[138] = {
                0x81, 0x57, 0x00, 0x00, msb, lsb,
                // all other bytes should be zero, except data and checksum
            };
            // set sector data to write to card
            for (std::size_t i = 0; i < 128; i++) {
                inputs[6 + i] = data[i];
            }
            inputs[134] = msb ^ lsb ^ data_checksum; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating bad sector") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                for (std::size_t i = 6; i < 134; i++) {
                    expected_outputs[i] = 0x00;
                }
                expected_outputs[134] = 0x00; // checksum received
                expected_outputs[135] = 0x5C; // ACK 1
                expected_outputs[136] = 0x5D; // ACK 2
                expected_outputs[137] = 0xFF; // end status = bad sector!
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write failure response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            INFO(i);
                            bool ack = card.send(inputs[i], output);
                            if (i != 137) { // unless last byte, check card ACK
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
        AND_GIVEN("A sequence of command bytes to write a valid sector") {
            constexpr std::size_t SECTOR_SIZE = 128u;
            auto data = generate_random_bytes<SECTOR_SIZE>();
            // calculate data checksum for later use
            std::uint8_t data_checksum = 0x00;
            for (auto byte : data) {
                data_checksum ^= byte;
            }
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000, 0x03FF)));
            // retrieve MSB and LSB of sector number
            std::uint8_t msb = sector >> 8;
            std::uint8_t lsb = (std::uint8_t)(sector & 0x00FF);
            std::optional<std::uint8_t> inputs[138] = {
                0x81, 0x57, 0x00, 0x00, msb, lsb,
                // 128 zero bytes now expected to follow (writing blanks)
            };
            // set sector data to write to card
            for (std::size_t i = 0; i < 128; i++) {
                inputs[6 + i] = data[i];
            }
            inputs[134] = msb ^ lsb ^ data_checksum; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating write success") {
                std::optional<std::uint8_t> expected_outputs[138] = {
                    std::nullopt, 0x08, 0x5A, 0x5D, 0x00, 0x00,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                for (std::size_t i = 6; i < 134; i++) {
                    expected_outputs[i] = 0x00;
                }
                expected_outputs[134] = 0x00; // checksum received
                expected_outputs[135] = 0x5C; // ACK 1
                expected_outputs[136] = 0x5D; // ACK 2
                expected_outputs[137] = 0x47; // end status = good write!
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write success response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            std::optional<std::uint8_t> output = std::nullopt;
                            INFO(i);
                            bool ack = card.send(inputs[i], output);
                            if (i != 137) { // unless last byte, check card ACK
                                CHECK(ack);
                            } else {
                                CHECK_FALSE(ack);
                            }
                            CHECK(output == expected_outputs[i]);
                        }
                    }
                    THEN("The correct card sector should contain the written data") {
                        auto sec = card.get_sector(sector);
                        for (std::size_t i = 0; i < SECTOR_SIZE; i++) {
                            CHECK(sec[i] == data[i]);
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

SCENARIO("Populate Memory Card data") {
    GIVEN("Exactly 128KiB of data") {
        constexpr std::size_t CARD_SIZE = 128u * 1024u;
        std::array<
            std::uint8_t,
            CARD_SIZE
        > data = generate_random_bytes<CARD_SIZE>();
        WHEN("A MemoryCard is constructed with a span of the data passed to it") {
            MemoryCard card(data);
            THEN("The MemoryCard bytes should be identical to those of the data") {
                for (std::size_t i = 0; i < CARD_SIZE; i++) {
                    REQUIRE(card.bytes[i] == data[i]);
                }
            }
            THEN("Data can be accessed correctly by Block") {
                constexpr std::size_t BLOCK_SIZE = 8u * 1024u; // 8KiB blocks
                for (std::size_t b = 0; b < 16; b++) {
                    auto block = card.get_block(b);
                    for (std::size_t i = 0; i < BLOCK_SIZE; i++) {
                        REQUIRE(block[i] == data[b * BLOCK_SIZE + i]);
                    }
                }
            }
            THEN("Data can be accessed correctly by Sector") {
                constexpr std::size_t SECTOR_SIZE = 128u;
                for (std::size_t s = 0; s < 1024; s++) {
                    auto sector = card.get_sector(s);
                    for (std::size_t i = 0; i < SECTOR_SIZE; i++) {
                        REQUIRE(sector[i] == data[s * SECTOR_SIZE + i]);
                    }
                }
            }
        }
    }
    GIVEN("A blank MemoryCard") {
        MemoryCard card;
        AND_GIVEN("Exactly 8KiB (one Block) of data") {
            constexpr std::size_t BLOCK_SIZE = 8u * 1024u;
            std::array<
                std::uint8_t,
                BLOCK_SIZE
            > data = generate_random_bytes<BLOCK_SIZE>();
            std::uint8_t b = GENERATE(range(0, 16));
            WHEN("The data is copied to a specific Block") {
                std::copy(data.begin(), data.end(), card.get_block(b).begin());
                THEN("The correct range of MemoryCard data is written to") {
                    for (std::size_t i = 0; i < BLOCK_SIZE; i++) {
                        REQUIRE(data[i] == card.bytes[b * BLOCK_SIZE + i]);
                    }
                }
            }
        }
        AND_GIVEN("Exactly 128 Bytes (one Sector) of data") {
            constexpr std::size_t SECTOR_SIZE = 128u;
            std::array<
                std::uint8_t,
                SECTOR_SIZE
            > data = generate_random_bytes<SECTOR_SIZE>();
            std::uint8_t s = GENERATE(take(100, random(0, 1024)));
            WHEN("The data is copied to a specific Sector") {
                std::copy(data.begin(), data.end(), card.get_sector(s).begin());
                THEN("The correct range of MemoryCard data is written to") {
                    for (std::size_t i = 0; i < SECTOR_SIZE; i++) {
                        REQUIRE(data[i] == card.bytes[s * SECTOR_SIZE + i]);
                    }
                }
            }
        }
    }
}
