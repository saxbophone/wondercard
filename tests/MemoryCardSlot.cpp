#include <optional>
#include <vector>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>
#include <ps1-memcard-protocol/MemoryCardSlot.hpp>


using namespace com::saxbophone::ps1_memcard_protocol;

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
}

SCENARIO("Calling MemoryCardSlot.send() with no MemoryCard inserted") {
    GIVEN("An empty MemoryCardSlot") {
        MemoryCardSlot slot;
        THEN("Calling send() method of the slot always returns false") {
            std::uint8_t command = GENERATE(take(100, random(0x00, 0xFF)));
            std::optional<std::uint8_t> result;
            REQUIRE_FALSE(slot.send(command, result));
        }
    }
}

SCENARIO("Calling MemoryCardSlot.send() with a MemoryCard inserted behaves same as MemoryCard.send()") {
    GIVEN("A MemoryCardSlot with a MemoryCard inserted") {
        MemoryCardSlot slot;
        MemoryCard card;
        slot.insert_card(card);
        AND_GIVEN("Another identical 'control' MemoryCard that is powered on") {
            MemoryCard control;
            REQUIRE(control.power_on());
            AND_GIVEN("A list of test command sequences") {
                // array of variable-sized vectors
                std::vector<std::vector<std::optional<std::uint8_t>>> inputs = {
                    {0x01,},
                    {std::nullopt,},
                    {0x81, 0x53, 0x00,},
                };
                std::vector<std::optional<std::uint8_t>> sequence = GENERATE_COPY(from_range(inputs));
                WHEN("A test command sequence is passed to MemoryCardSlot.send()") {
                    THEN("The response data and retrn value are the same as those received from the control card") {
                        for (std::optional<std::uint8_t> command : sequence) {
                            std::optional<std::uint8_t> response = std::nullopt, control_response = std::nullopt;
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
