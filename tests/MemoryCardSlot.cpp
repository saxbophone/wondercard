#include <optional>

#include <cstdint>

#include <catch2/catch.hpp>

#include <ps1-memcard-protocol/MemoryCard.hpp>
#include <ps1-memcard-protocol/MemoryCardSlot.hpp>


using namespace com::saxbophone::ps1_memcard_protocol;

SCENARIO("MemoryCards can be inserted and removed from MemoryCardSlot") {
    GIVEN("A MemoryCardSlot") {
        MemoryCardSlot slot;
        THEN("Attempting to remove a card from the empty slot fails") {
            CHECK_FALSE(slot.remove_card());
        }
        AND_GIVEN("A MemoryCard") {
            MemoryCard card;
            THEN("The card can be inserted into the slot successfully") {
                REQUIRE(slot.insert_card(card));
                AND_WHEN("The slot has had a card inserted into it") {
                    THEN("Attempting to insert a card into the slot fails") {
                        CHECK_FALSE(slot.insert_card(card)); // doesn't matter that it's the same card
                    }
                    THEN("The card can be removed from the slot successfully") {
                        CHECK(slot.remove_card());
                    }
                }
            }
        }
    }
}
