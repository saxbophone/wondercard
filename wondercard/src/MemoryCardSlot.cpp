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

    bool MemoryCardSlot::read_card(std::span<Byte, MemoryCard::CARD_SIZE> data) {
        // guard against reading when no card in slot
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // retrieve each block of the card using template recursion
        return this->_read_card_block<0>(data);
    }

    void MemoryCardSlot::write_card(std::span<Byte, MemoryCard::CARD_SIZE> data) {
        return;
    }

    bool MemoryCardSlot::read_block(std::size_t index, MemoryCard::Block data) {
        // guard against reading when no card in slot
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // TODO: Validate index???
        // calculate first sector of block (just shift block number by number of bits of sectors)
        std::size_t block_sector = index << 6;
        // retrieve each sector of the block using template recursion
        return this->_read_block_sector<0>(block_sector, data);
    }

    void MemoryCardSlot::write_block(std::size_t index, MemoryCard::Block data) {
        return;
    }

    bool MemoryCardSlot::read_sector(std::size_t index, MemoryCard::Sector data) {
        // guard against reading when no card in slot
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // TODO: Validate index???
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
        return end_ack == false and output == 0x47 and card_checksum == checksum;
    }

    bool MemoryCardSlot::write_sector(std::size_t index, MemoryCard::Sector data) {
        // guard against reading when no card in slot
        if (this->_inserted_card == nullptr) {
            return false;
        }
        // TODO: Validate index???
        // scratchpad variable for card responses
        TriState output = std::nullopt;
        // get MSB and LSB of sector index
        Byte msb = (index & 0x300u) >> 8;
        Byte lsb = index & 0x0FFu;
        // command sequence to send to the card before sector data is sent
        Byte commands[] = {
            0x81, 0x57, 0x00, 0x00, msb, lsb,
        };
        // expected valid responses (std::nullopt indicates don't-cares)
        TriState valid_responses[] = {
            {},   {},   0x5A, 0x5D, {},  {},
        };
        // send each command in commands sequence and bail if no ACK or response wrong
        for (std::size_t i = 0; i < 6; i++) {
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
        // if this point is reached, we are ready to write sector data
        for (std::size_t i = 0; i < MemoryCard::SECTOR_SIZE; i++) {
            if (!this->_inserted_card->send(data[i], output)) {
                return false; // no ACK, oh dear!
            }
            // update checksum
            checksum ^= data[i];
        }
        // send our calculated checksum value
        if (!this->_inserted_card->send(checksum, output)) {
            return false; // no ACK
        }
        // next two bytes received should be "Command Acknowledge" followed by end byte status
        TriState footer_responses[] = {
            0x5C, 0x5D,   {},
        };
        for (std::size_t i = 0; i < 3; i++) {
            // all remaining commands send 00h
            if (!this->_inserted_card->send(0x00, output) and i != 2) {
                return false; // expect ACK on all but last
            }
            // validate response unless response is don't-care
            if (
                footer_responses[i] != std::nullopt and
                output != footer_responses[i]
            ) {
                return false; // invalid response
            }
        }
        // TODO: We really need a way to tell apart different kinds of fail
        return output == 0x47; // 0x4Eh = Bad Checksum, 0xFFh = Bad Sector
    }

    template <std::size_t sector_index>
    bool MemoryCardSlot::_read_block_sector(std::size_t block_sector, MemoryCard::Block data) {
        // use subspan to write sector data to output
        MemoryCard::Sector sector = data.subspan<sector_index * MemoryCard::SECTOR_SIZE, MemoryCard::SECTOR_SIZE>();
        // base case
        if constexpr (sector_index == (MemoryCard::BLOCK_SECTOR_COUNT - 1)) {
            // last sector
            return this->read_sector(block_sector + sector_index, sector);
        } else {
            // this and next sector (recursive template call)
            return
                this->read_sector(block_sector + sector_index, sector) and
                this->_read_block_sector<sector_index + 1>(block_sector, data);
        }
    }

    template <std::size_t block_index>
    bool MemoryCardSlot::_read_card_block(std::span<Byte, MemoryCard::CARD_SIZE> data) {
        // use subspan to write block data to output
        MemoryCard::Block block = data.subspan<block_index * MemoryCard::BLOCK_SIZE, MemoryCard::BLOCK_SIZE>();
        // base case
        if constexpr (block_index == (MemoryCard::CARD_BLOCK_COUNT - 1)) {
            // last block
            return this->read_block(block_index, block);
        } else {
            // this and next block (recursive template call)
            return
                this->read_block(block_index, block) and
                this->_read_card_block<block_index + 1>(data);
        }
    }
}
