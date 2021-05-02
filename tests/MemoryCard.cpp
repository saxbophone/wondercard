#include <optional>
#include <random>

#include <cstdint>

#include <catch2/catch.hpp>

#include <wondercard/MemoryCard.hpp>


using namespace com::saxbophone::wondercard;

template<std::size_t SIZE>
static std::array<Byte, SIZE> generate_random_bytes() {
    std::array<Byte, SIZE> data;
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
            Byte command = 0x81_u8; // generic: "hey memory card!" message
            TriState response = std::nullopt;
            bool ack = card.send(command, response);
            THEN("The card does not acknowledge the command or send return data") {
                REQUIRE_FALSE(ack);
                REQUIRE(response == std::nullopt);
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
            Byte command = GENERATE(
                take(
                    100,
                    filter(
                        [](Byte i) { return i != 0x81_u8; },
                        random(0x00_u8, 0xFF_u8)
                    )
                )
            );
            WHEN("The command byte is sent to the MemoryCard") {
                TriState response = std::nullopt;
                bool ack = card.send(command, response);
                THEN("The card does not acknowledge the command or send return data") {
                    REQUIRE_FALSE(ack);
                    REQUIRE(response == std::nullopt);
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
        TriState response = std::nullopt;
        REQUIRE(card.send(0x81_u8, response)); // we need ACK otherwise can't test
        WHEN("An invalid memory card command byte is sent to the card") {
            Byte wrong_command = (Byte)GENERATE(
                take(
                    100,
                    filter(
                        [](std::uint16_t c) {
                            return c != 0x52_u16 and c != 0x53_u16 and c != 0x57_u16;
                        },
                        random(0x00_u16, 0xFF_u16)
                    )
                )
            );
            bool ack = card.send(wrong_command, response);
            THEN("The card responds with FLAG and NACK") {
                REQUIRE(response == 0x08_u8);
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
            std::uint16_t sector = GENERATE(take(100, random(0x0400_u16, 0xFFFF_u16)));
            // retrieve MSB and LSB of sector number
            Byte msb = sector >> 8;
            Byte lsb = (Byte)(sector & 0x00FF);
            TriState inputs[] = {
                0x81_u8, 0x52_u8, 0x00_u8, 0x00_u8, msb, lsb, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8,
            };
            AND_GIVEN("A sqeuence of expected response bytes indicating read failure") {
                TriState expected_outputs[] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x00_u8, 0x00_u8, 0x5C_u8, 0x5D_u8, 0xFF_u8, 0xFF_u8,
                };
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected read failure response") {
                        for (std::size_t i = 0; i < 10; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 9) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        AND_GIVEN("A sequence of command bytes to read a valid sector") {
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000_u16, 0x03FF_u16)));
            // retrieve MSB and LSB of sector number
            Byte msb = sector >> 8;
            Byte lsb = (Byte)(sector & 0x00FF);
            TriState inputs[140] = {
                0x81_u8, 0x52_u8, 0x00_u8, 0x00_u8, msb, lsb, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8,
            };
            // set remaining members to zero (can't default init because of optional)
            for (std::size_t i = 10; i < 140; i++) {
                inputs[i] = 0x00_u8;
            }
            AND_GIVEN("A sqeuence of expected response bytes indicating read data") {
                TriState expected_outputs[140] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x00_u8, 0x00_u8, 0x5C_u8, 0x5D_u8, msb, lsb,
                };
                // set expected sector data to read from card
                auto sec = card.get_sector(sector);
                Byte data_checksum = 0x00_u8;
                for (std::size_t i = 0; i < 128; i++) {
                    expected_outputs[10 + i] = sec[i];
                    data_checksum ^= sec[i];
                }
                expected_outputs[138] = msb ^ lsb ^ data_checksum; // checksum
                expected_outputs[139] = 0x47_u8;      // "good read" magic end byte
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected read success response") {
                        for (std::size_t i = 0; i < 140; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 139) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
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
            Byte data_checksum = 0x00_u8;
            for (auto byte : data) {
                data_checksum ^= byte;
            }
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0400_u16, 0xFFFF_u16)));
            // retrieve MSB and LSB of sector number
            Byte msb = sector >> 8;
            Byte lsb = (Byte)(sector & 0x00FF);
            TriState inputs[138] = {
                0x81_u8, 0x57_u8, 0x00_u8, 0x00_u8, msb, lsb,
                // all other bytes should be zero, except data and checksum
            };
            // set sector data to write to card
            for (std::size_t i = 0; i < 128; i++) {
                inputs[6 + i] = data[i];
            }
            inputs[134] = msb ^ lsb ^ data_checksum; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating bad sector") {
                TriState expected_outputs[138] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x00_u8, 0x00_u8,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                for (std::size_t i = 6; i < 134; i++) {
                    expected_outputs[i] = 0x00_u8;
                }
                expected_outputs[134] = 0x00_u8; // checksum received
                expected_outputs[135] = 0x5C_u8; // ACK 1
                expected_outputs[136] = 0x5D_u8; // ACK 2
                expected_outputs[137] = 0xFF_u8; // end status = bad sector!
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write failure response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 137) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
        AND_GIVEN("A sequence of command bytes to write a valid sector") {
            constexpr std::size_t SECTOR_SIZE = 128u;
            auto data = generate_random_bytes<SECTOR_SIZE>();
            // calculate data checksum for later use
            Byte data_checksum = 0x00_u8;
            for (auto byte : data) {
                data_checksum ^= byte;
            }
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000_u16, 0x03FF_u16)));
            // retrieve MSB and LSB of sector number
            Byte msb = sector >> 8;
            Byte lsb = (Byte)(sector & 0x00FF);
            TriState inputs[138] = {
                0x81_u8, 0x57_u8, 0x00_u8, 0x00_u8, msb, lsb,
                // 128 zero bytes now expected to follow (writing blanks)
            };
            // set sector data to write to card
            for (std::size_t i = 0; i < 128; i++) {
                inputs[6 + i] = data[i];
            }
            inputs[134] = msb ^ lsb ^ data_checksum; // checksum
            AND_GIVEN("A sqeuence of expected response bytes indicating write success") {
                TriState expected_outputs[138] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x00_u8, 0x00_u8,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                for (std::size_t i = 6; i < 134; i++) {
                    expected_outputs[i] = 0x00_u8;
                }
                expected_outputs[134] = 0x00_u8; // checksum received
                expected_outputs[135] = 0x5C_u8; // ACK 1
                expected_outputs[136] = 0x5D_u8; // ACK 2
                expected_outputs[137] = 0x47_u8; // end status = good write!
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected write success response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 137) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
                        }
                        AND_THEN("The correct card sector should contain the written data") {
                            auto sec = card.get_sector(sector);
                            for (std::size_t i = 0; i < SECTOR_SIZE; i++) {
                                REQUIRE(sec[i] == data[i]);
                            }
                        }
                    }
                }
            }
        }
        AND_GIVEN("A sequence of command bytes to write a sector with an invalid checksum") {
            constexpr std::size_t SECTOR_SIZE = 128u;
            auto data = generate_random_bytes<SECTOR_SIZE>();
            // calculate data checksum for later use
            Byte data_checksum = 0x00_u8;
            for (auto byte : data) {
                data_checksum ^= byte;
            }
            // sector numbers are 16-bit
            std::uint16_t sector = GENERATE(take(100, random(0x0000_u16, 0x03FF_u16)));
            // retrieve MSB and LSB of sector number
            Byte msb = sector >> 8;
            Byte lsb = (Byte)(sector & 0x00FF);
            TriState inputs[138] = {
                0x81_u8, 0x57_u8, 0x00_u8, 0x00_u8, msb, lsb,
                // 128 zero bytes now expected to follow (writing blanks)
            };
            // set sector data to write to card
            for (std::size_t i = 0; i < 128; i++) {
                inputs[6 + i] = data[i];
            }
            // easiest way to corrupt the checksum is to invert it
            inputs[134] = ~(msb ^ lsb ^ data_checksum);
            AND_GIVEN("A sqeuence of expected response bytes indicating bad checksum") {
                TriState expected_outputs[138] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x00_u8, 0x00_u8,
                    // next 128 bytes are 0x00 as data being written (no response)
                };
                for (std::size_t i = 6; i < 134; i++) {
                    expected_outputs[i] = 0x00;
                }
                expected_outputs[134] = 0x00_u8; // checksum received
                expected_outputs[135] = 0x5C_u8; // ACK 1
                expected_outputs[136] = 0x5D_u8; // ACK 2
                expected_outputs[137] = 0x4E_u8; // end status = bad checksum!
                WHEN("The sequence of command bytes is sent to the card") {
                    THEN("The card should respond with the expected bad checksum response") {
                        for (std::size_t i = 0; i < 138; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 137) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Get Memory Card ID Command") {
    GIVEN("A MemoryCard that is powered on") {
        MemoryCard card;
        // power up the card
        REQUIRE(card.power_on());
        AND_GIVEN("A sequence of command bytes to get memory card ID") {
            TriState inputs[] = {
                0x81_u8, 0x53_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8, 0x00_u8,
            };
            AND_GIVEN("A sequence of data bytes the card is expected to respond with") {
                TriState expected_outputs[] = {
                    std::nullopt, 0x08_u8, 0x5A_u8, 0x5D_u8, 0x5C_u8, 0x5D_u8, 0x04_u8, 0x00_u8, 0x00_u8, 0x80_u8,
                };
                WHEN("The sqeuence of command bytes is sent to the card") {
                    THEN("The card responds with the expected response byte") {
                        for (std::size_t i = 0; i < 10; i++) {
                            TriState output = std::nullopt;
                            bool ack = card.send(inputs[i], output);
                            if (i != 9) { // unless last byte, check card ACK
                                REQUIRE(ack);
                            } else {
                                REQUIRE_FALSE(ack);
                            }
                            REQUIRE(output == expected_outputs[i]);
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
            Byte,
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
                Byte,
                BLOCK_SIZE
            > data = generate_random_bytes<BLOCK_SIZE>();
            Byte b = GENERATE(range(0_u8, 16_u8));
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
                Byte,
                SECTOR_SIZE
            > data = generate_random_bytes<SECTOR_SIZE>();
            std::uint16_t s = GENERATE(take(100, random(0_u16, 1024_u16)));
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
