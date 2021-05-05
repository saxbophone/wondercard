#include <array>
#include <optional>
#include <vector>

#include <cstdint>

#include <catch2/catch.hpp>

#include <wondercard/common.hpp>
#include <wondercard/MemoryCard.hpp>
#include <wondercard/MemoryCardSlot.hpp>

#include "test_helpers.hpp"


using namespace com::saxbophone::wondercard;
using namespace com::saxbophone::wondercard::PRIVATE::test_helpers;

SCENARIO("MemoryCards can be inserted and removed from MemoryCardSlot") {
    GIVEN("An empty MemoryCardSlot") {
        MemoryCardSlot slot;
        THEN("Attempting to remove a card from the empty slot fails") {
            CHECK_FALSE(slot.remove_card());
        }
        AND_GIVEN("A MemoryCard") {
            MemoryCard card;
            THEN("The card can be inserted into the slot successfully") {
                REQUIRE(slot.insert_card(card));
                AND_WHEN("The slot has had a card inserted into it") {
                    THEN("The card should be powered on") {
                        CHECK(card.powered_on);
                    }
                    THEN("Attempting to insert a card into the slot fails") {
                        CHECK_FALSE(slot.insert_card(card)); // doesn't matter that it's the same card
                    }
                    THEN("The card can be removed from the slot successfully") {
                        REQUIRE(slot.remove_card());
                        AND_THEN("The card should be powered off") {
                            CHECK_FALSE(card.powered_on);
                        }
                    }
                }
            }
        }
    }
    GIVEN("Two empty MemoryCardSlots") {
        MemoryCardSlot slots[2];
        AND_GIVEN("A MemoryCard") {
            MemoryCard card;
            AND_GIVEN("The MemoryCard is inserted into the first slot") {
                REQUIRE(slots[0].insert_card(card));
                THEN("Attempting to insert the same card into the second slot fails") {
                    CHECK_FALSE(slots[1].insert_card(card));
                }
            }
        }
    }
}

SCENARIO("Calling MemoryCardSlot.send() with no MemoryCard inserted") {
    GIVEN("An empty MemoryCardSlot") {
        MemoryCardSlot slot;
        THEN("Calling send() method of the slot always returns false") {
            Byte command = GENERATE(take(100, random(0x00, 0xFF)));
            TriState result;
            REQUIRE_FALSE(slot.send(command, result));
        }
    }
}

SCENARIO("Calling MemoryCardSlot.send() with a MemoryCard inserted behaves same as MemoryCard.send()") {
    GIVEN("A list of test command sequences") {
        // array of variable-sized vectors
        std::vector<std::vector<TriState>> inputs = {
            {0x01,}, // command for game controller (should be ignored)
            {std::nullopt,}, // high-impedance input
            {0x81,}, // incomplete command for memory card
            {0x81, 0x33,}, // memory card command mode, invalid memory card command
            {0x81, 0x52, 0x00, 0x00, 0x01, 0x33, 0x00, 0x00, 0x00, 0x00,}, // read
            {0x81, 0x57, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00,}, // incomplete write command
            {0x81, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}, // get card ID
        };
        std::vector<TriState> sequence = GENERATE_COPY(from_range(inputs));
        AND_GIVEN("A MemoryCardSlot with a MemoryCard inserted") {
            MemoryCardSlot slot;
            MemoryCard card;
            slot.insert_card(card);
            AND_GIVEN("Another identical 'control' MemoryCard that is powered on") {
                MemoryCard control;
                REQUIRE(control.power_on());
                WHEN("A test command sequence is passed to MemoryCardSlot.send()") {
                    THEN("The response data and retrn value are the same as those received from the control card") {
                        for (TriState command : sequence) {
                            TriState response = std::nullopt, control_response = std::nullopt;
                            bool slot_ack = slot.send(command, response);
                            bool control_ack = control.send(command, control_response);
                            REQUIRE(slot_ack == control_ack);
                            REQUIRE(response == control_response);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to read entire MemoryCard") {
    GIVEN("An entire MemoryCard's-worth of data") {
        std::array<
            Byte,
            MemoryCard::CARD_SIZE
        > data = generate_random_bytes<MemoryCard::CARD_SIZE>();
        AND_GIVEN("A MemoryCard initialised with that data") {
            MemoryCard card(data);
            auto bytes = card.bytes;
            // verify card data is correct --test is invalid if not
            for (std::size_t i = 0; i < MemoryCard::CARD_SIZE; i++) {
                REQUIRE(bytes[i] == data[i]);
            }
            AND_GIVEN("A MemoryCardSlot that the card is successfully inserted into") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                THEN("Calling MemoryCardSlot.read_card() returns an array identical to the data") {
                    auto card_data = slot.read_card();
                    for (std::size_t i = 0; i < MemoryCard::CARD_SIZE; i++) {
                        REQUIRE(card_data[i] == data[i]);
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to write entire MemoryCard") {
    GIVEN("An entire MemoryCard's-worth of data") {
        std::array<
            Byte,
            MemoryCard::CARD_SIZE
        > data = generate_random_bytes<MemoryCard::CARD_SIZE>();
        AND_GIVEN("An empty MemoryCard") {
            MemoryCard card;
            AND_GIVEN("A MemoryCardSlot that the card is successfully inserted into") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                WHEN("MemoryCardSlot.write_card() is called with the data") {
                    slot.write_card(data);
                    THEN("The card data bytes should equal those of the data") {
                        auto bytes = card.bytes;
                        for (std::size_t i = 0; i < MemoryCard::CARD_SIZE; i++) {
                            REQUIRE(bytes[i] == data[i]);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to read a MemoryCard Block") {
    GIVEN("A Block's-worth of random data") {
        std::array<
            Byte,
            MemoryCard::BLOCK_SIZE
        > data = generate_random_bytes<MemoryCard::BLOCK_SIZE>();
        // generate valid block numbers to try reading
        std::size_t block_number = GENERATE(range(0u, 15u));
        AND_GIVEN("A MemoryCard with a specific block filled with the data") {
            MemoryCard card;
            auto block = card.get_block(block_number);
            for (std::size_t i = 0; i < MemoryCard::BLOCK_SIZE; i++) {
                block[i] = data[i];
            }
            AND_GIVEN("A MemoryCardSlot with the card inserted into it") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                WHEN("MemoryCardSlot.read_block() is called with the block number") {
                    auto block_data = slot.read_block(block_number);
                    THEN("The returned data is equal to the generated data") {
                        for (std::size_t i = 0; i < MemoryCard::BLOCK_SIZE; i++) {
                            REQUIRE(block_data[i] == data[i]);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to write a MemoryCard Block") {
    GIVEN("A Block's-worth of random data") {
        std::array<
            Byte,
            MemoryCard::BLOCK_SIZE
        > data = generate_random_bytes<MemoryCard::BLOCK_SIZE>();
        // generate valid block numbers to try reading
        std::size_t block_number = GENERATE(range(0u, 15u));
        AND_GIVEN("A blank MemoryCard") {
            MemoryCard card;
            AND_GIVEN("A MemoryCardSlot with the card inserted into it") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                WHEN("MemoryCardSlot.write_block() is called with the block number and generated data") {
                    slot.write_block(block_number, data);
                    THEN("The corresponding block of the card is equal to generated data") {
                        auto block = card.get_block(block_number);
                        for (std::size_t i = 0; i < MemoryCard::BLOCK_SIZE; i++) {
                            REQUIRE(block[i] == data[i]);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to read a MemoryCard Sector") {
    GIVEN("A Sector's-worth of random data") {
        std::array<
            Byte,
            MemoryCard::SECTOR_SIZE
        > data = generate_random_bytes<MemoryCard::SECTOR_SIZE>();
        // generate valid sector numbers to try reading
        std::size_t sector_number = GENERATE(range(0x000u, 0x3FFu)); // 0..1023
        AND_GIVEN("A MemoryCard with a specific sector filled with the data") {
            MemoryCard card;
            auto sector = card.get_sector(sector_number);
            for (std::size_t i = 0; i < MemoryCard::SECTOR_SIZE; i++) {
                sector[i] = data[i];
            }
            AND_GIVEN("A MemoryCardSlot with the card inserted into it") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                WHEN("MemoryCardSlot.read_sector() is called with the sector number and a destination array") {
                    std::array<Byte, MemoryCard::SECTOR_SIZE> output;
                    REQUIRE(slot.read_sector(sector_number, output));
                    THEN("The output data is equal to the generated data") {
                        for (std::size_t i = 0; i < MemoryCard::SECTOR_SIZE; i++) {
                            REQUIRE(output[i] == data[i]);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Using higher level I/O API to write a MemoryCard Sector") {
    GIVEN("A Sector's-worth of random data") {
        std::array<
            Byte,
            MemoryCard::SECTOR_SIZE
        > data = generate_random_bytes<MemoryCard::SECTOR_SIZE>();
        // generate valid sector numbers to try reading
        std::size_t sector_number = GENERATE(range(0x000u, 0x3FFu)); // 0..1023
        AND_GIVEN("A blank MemoryCard") {
            MemoryCard card;
            AND_GIVEN("A MemoryCardSlot with the card inserted into it") {
                MemoryCardSlot slot;
                REQUIRE(slot.insert_card(card));
                WHEN("MemoryCardSlot.write_sector() is called with the sector number and generated data") {
                    slot.write_sector(sector_number, data);
                    THEN("The corresponding sector of the card is equal to generated data") {
                        auto sector = card.get_sector(sector_number);
                        for (std::size_t i = 0; i < MemoryCard::SECTOR_SIZE; i++) {
                            REQUIRE(sector[i] == data[i]);
                        }
                    }
                }
            }
        }
    }
}
