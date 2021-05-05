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

    bool MemoryCardSlot::read_sector(std::size_t index, std::span<Byte, MemoryCard::SECTOR_SIZE> data) {
        // guard against reading when no card in slot
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // scratchpad variable for card responses
        TriState output = std::nullopt;
        // get MSB and LSB of sector index
        Byte msb = (index & 0x300u) >> 8;
        Byte lsb = index & 0x0FFu;
        // command sequence to send to the card until sector data is received
        Byte commands[] = {
            0x81, 0x52, 0x00, 0x00, msb, lsb, 0x00, 0x00, 0x00, 0x00,
        };
        // expected valid responses (std::nullopt indicates don't-cares)
        TriState valid_responses[] = {
            {},   {},   0x5A, 0x5D, {},  {},  0x5C, 0x5D, msb,  lsb,
        };
        // send each command in commands sequence and bail if no ACK or response wrong
        for (std::size_t i = 0; i < 10; i++) {
            if (!this->_inserted_card->send(commands[i], output)) {
                return false; // no ACK, oh dear!
            }
            // validate response unless response is don't-care
            if (
                valid_responses[i] != std::nullopt and
                output != valid_responses[i]
            ) {
                return false; // invalid response
            }
        }
        // calculate checksum value so far (MSB XOR LSB)
        Byte checksum = msb ^ lsb;
        // if this point is reached, we are ready to read sector data
        for (std::size_t i = 0; i < MemoryCard::SECTOR_SIZE; i++) {
            if (!this->_inserted_card->send(0x00, output)) {
                return false; // no ACK, oh dear!
            }
            // if output is high-z, bail immediately
            if (output == std::nullopt) {
                return false;
            }
            // store output (sector data) into return param
            data[i] = output.value(); // guaranteed not high-Z due to guard clause
            // update checksum
            checksum ^= data[i];
        }
        TriState card_checksum = std::nullopt;
        // receive card-calculated checksum
        if (!this->_inserted_card->send(0x00, card_checksum)) {
            return false; // no ACK
        }
        // end byte should always be 0x47 and never ACK
        bool end_ack = this->_inserted_card->send(0x00, output);
        return end_ack == false and output == 0x47;
    }

    void MemoryCardSlot::write_sector(std::size_t index, MemoryCard::Sector data) {
        return;
    }
}
