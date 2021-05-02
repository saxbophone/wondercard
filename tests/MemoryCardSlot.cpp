#include <optional>
#include <vector>

#include <cstdint>

#include <catch2/catch.hpp>

#include <wondercard/MemoryCard.hpp>
#include <wondercard/MemoryCardSlot.hpp>


using namespace com::saxbophone::wondercard;

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
