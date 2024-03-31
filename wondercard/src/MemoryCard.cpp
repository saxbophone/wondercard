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
#include <span>

#include <cstddef>
#include <cstdint>

#include <wondercard/common.hpp>
#include <wondercard/MemoryCard.hpp>


namespace com::saxbophone::wondercard {
    MemoryCard::MemoryCard()
      : powered_on(this->_powered_on)
      , bytes(this->_bytes)
      , _powered_on(false)
      , _flag(MemoryCard::_FLAG_INIT_VALUE)
      {}

    MemoryCard::MemoryCard(
        std::span<Byte, MemoryCard::CARD_SIZE> data
    )
      : MemoryCard()
      {
        std::copy(data.begin(), data.end(), this->_bytes.begin());
    }

    bool MemoryCard::power_on() {
        if (!this->powered_on) { // card is currently off, okay to power on
            // set powered on and reset flag value to default
            this->_powered_on = true;
            this->_flag = MemoryCard::_FLAG_INIT_VALUE;
            return true;
        } else { // card is already powered on! no-op
            return false;
        }
    }

    bool MemoryCard::power_off() {
        // set powered_on to false if not already and return true, else false
        return std::exchange(this->_powered_on, false);
    }

    bool MemoryCard::send(
        TriState command,
        TriState& data
    ) {
        // don't do anything, including ACK, if card isn't powered on
        if (!this->powered_on) {
            return false;
        } else {
            this->_data_in = command;
            auto [ack, data_out] = this->_state_machine();
            data = data_out;
            return ack;
        }
    }

    MemoryCard::Block MemoryCard::get_block(std::size_t i) {
        // TODO: validate Block number
        return MemoryCard::Block(
            this->_bytes.begin() + i * MemoryCard::BLOCK_SIZE,
            MemoryCard::BLOCK_SIZE
        );
    }

    MemoryCard::Sector MemoryCard::get_sector(std::size_t i) {
        // TODO: validate Sector number
        return MemoryCard::Sector(
            this->_bytes.begin() + i * MemoryCard::SECTOR_SIZE,
            MemoryCard::SECTOR_SIZE
        );
    }

    Generator<std::pair<bool, TriState>> MemoryCard::_process_command() {
        while (true) {
            while (_data_in != 0x81) co_yield std::make_pair(false, std::nullopt); // ignore commands that aren't for Memory Cards
            co_yield std::make_pair(true, std::nullopt); // received a Memory Card command --reply with ACK 
            switch (_data_in.value_or(0x00)) { // decode memory card command
            case 0x52: { // READ_DATA_COMMAND
                co_yield std::make_pair(true, this->_flag);
                auto read_gen = _read_data_command();
                while (read_gen) {
                    co_yield read_gen();
                }
                break;
            }
            case 0x57: { // WRITE_DATA_COMMAND
                co_yield std::make_pair(true, this->_flag);
                auto write_gen = _write_data_command();
                while (write_gen) {
                    co_yield write_gen();
                }
                break;
            }
            case 0x53: // GET_MEMCARD_ID_COMMAND
                co_yield std::make_pair(true, this->_flag);
                for (Byte out : {0x5A, 0x5D, 0x5C, 0x5D, 0x04, 0x00, 0x00}) {
                    co_yield std::make_pair(true, out);
                }
                co_yield std::make_pair(false, 0x80);
                break;
            default:
                co_yield std::make_pair(false, this->_flag);
            }
        }
    }

    Generator<std::pair<bool, TriState>> MemoryCard::_read_data_command() {
        // reply with two ACK bytes
        co_yield std::make_pair(true, 0x5A);
        co_yield std::make_pair(true, 0x5D);
        Byte checksum = _data_in.value_or(0xFF); // reset checksum
        std::uint16_t address = (std::uint16_t)checksum << 8; // rx address MSB
        co_yield std::make_pair(true, 0x00);
        address |= _data_in.value_or(0xFF); // address LSB
        checksum ^= (Byte)(address & 0x00FF);
        // detect invalid sectors (out of bounds)
        if (address > MemoryCard::_LAST_SECTOR) {
            address = 0xFFFF; // poison value
        }
        co_yield std::make_pair(true, 0x00);
        // send two ACK bytes
        co_yield std::make_pair(true, 0x5C);
        co_yield std::make_pair(true, 0x5D);
        // send confirmation of address MSB and LSB
        co_yield std::make_pair(true, (Byte)(address >> 8));
        Byte lsb = (Byte)(address & 0x00FF);
        // we'll only continue if sector address is not a poison value
        if (address == 0xFFFF) {
            co_yield std::make_pair(false, lsb);
            co_return;
        }
        co_yield std::make_pair(true, lsb);
        // send the requested data sector
        for (Byte counter = 0; counter < 128u; ++counter) {
            Byte data = this->get_sector(address)[counter];
            // update checksum
            checksum ^= data;
            co_yield std::make_pair(true, data);
        }
        // send checksum
        co_yield std::make_pair(true, checksum);
        co_yield std::make_pair(false, 0x47); // 0x47: "Good Read"
        co_return;
    }

    Generator<std::pair<bool, TriState>> MemoryCard::_write_data_command() {
        // reply with two ACK bytes
        co_yield std::make_pair(true, 0x5A);
        co_yield std::make_pair(true, 0x5D);
        Byte checksum = _data_in.value_or(0xFF); // reset checksum
        std::uint16_t address = (std::uint16_t)checksum << 8; // rx address MSB
        co_yield std::make_pair(true, 0x00);
        address |= _data_in.value_or(0xFF); // address LSB
        checksum ^= (Byte)(address & 0x00FF);
        // detect invalid sectors (out of bounds)
        if (address > MemoryCard::_LAST_SECTOR) {
            address = 0xFFFF; // poison value
        }
        co_yield std::make_pair(true, 0x00);
        // write the requested data sector
        for (Byte counter = 0; counter < 128u; ++counter) {
            // grab byte, converting Z-state to 0xFF if encountered (shouldn't, but...)
            Byte write_byte = _data_in.value_or(0xFF);
            // so long as the sector address is valid, write the sector
            if (address != 0xFFFF) {
                this->get_sector(address)[counter] = write_byte;
            }
            // update the checksum
            checksum ^= write_byte;
            co_yield std::make_pair(true, 0x00);
        }
        // checksum
        // set to inverted calculated checksum if no value, to force a bad checksum in that case
        Byte rx_checksum = _data_in.value_or(checksum);
        /*
         * take checksum received in command and validate against calculated
         * checksum
         * for brevity, store the result of comparison in the checksum
         */
        checksum = rx_checksum == checksum ? 0x00 : 0xFF;
        co_yield std::make_pair(true, 0x00);
        // send two ACK bytes
        co_yield std::make_pair(true, 0x5C);
        co_yield std::make_pair(true, 0x5D);
        /*
         * status end byte:
         * 0x47 = Good, 0x4E = Bad Checksum, 0xFF = Bad Sector
         */
        if (address == 0xFFFF) {       // Bad Sector
            co_yield std::make_pair(false, 0xFF);
        } else if (checksum == 0xFF) { // Bad Checksum
            co_yield std::make_pair(false, 0x4E);
        } else {                       // Good
            co_yield std::make_pair(false, 0x47);
        }
        co_return;
    }

    const Byte MemoryCard::_FLAG_INIT_VALUE = 0x08;
    const std::uint16_t MemoryCard::_LAST_SECTOR = 0x03FF;
}
