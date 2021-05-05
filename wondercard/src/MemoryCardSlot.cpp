/**
 * This file forms part of PS1 Memory Card Protocol, a fully virtual emulation
 * of the protocol used by Memory Cards for the PlayStation.
 *
 * Copyright Joshua Saxby 2021. All rights reserved.
 *
 * This is closed-source software and may not be produced in either part or in
 * full, in either source or binary form, without the express written consent
 * of the copyright holder.
 *
 */

#include <array>
#include <optional>

#include <cstdint>

#include <wondercard/common.hpp>
#include <wondercard/MemoryCardSlot.hpp>


namespace com::saxbophone::wondercard {
    MemoryCardSlot::MemoryCardSlot() : _inserted_card(nullptr) {}

    bool MemoryCardSlot::send(
        TriState command,
        TriState& data
    ) {
        // guard against trying to send to a non-existent card
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // pass on the call to MemoryCard.send()
        return this->_inserted_card->send(command, data);
    }

    bool MemoryCardSlot::insert_card(MemoryCard& card) {
        // guard against card double-insertion
        if (this->_inserted_card != nullptr) {
            return false;
        }
        // if the card can't be powered on, it can't be inserted
        if (!card.power_on()) {
            return false;
        } else {
            // insert the card
            this->_inserted_card = &card;
            return true;
        }
    }

    bool MemoryCardSlot::remove_card() {
        // guard against trying to remove non-existent card
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // power down the card
        this->_inserted_card->power_off();
        // remove the card
        this->_inserted_card = nullptr;
        return true;
    }

    std::array<Byte, MemoryCard::CARD_SIZE> MemoryCardSlot::read_card() {
        return {};
    }

    void MemoryCardSlot::write_card(std::span<Byte, MemoryCard::CARD_SIZE> data) {
        return;
    }

    std::array<Byte, MemoryCard::BLOCK_SIZE> MemoryCardSlot::read_block(std::size_t index) {
        return {};
    }

    void MemoryCardSlot::write_block(std::size_t index, MemoryCard::Block data) {
        return;
    }

    std::array<Byte, MemoryCard::SECTOR_SIZE> MemoryCardSlot::read_sector(std::size_t index) {
        return {};
    }

    void MemoryCardSlot::write_sector(std::size_t index, MemoryCard::Sector data) {
        return;
    }
}
