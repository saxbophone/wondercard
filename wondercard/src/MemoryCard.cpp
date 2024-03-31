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
      , _state(MemoryCard::_STARTING_STATE)
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
            this->_state = MemoryCard::_STARTING_STATE;
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
            switch (this->_state) {
            case MemoryCard::State::IDLE:
                if (command == 0x81) { // a Memory Card command
                    this->_state = MemoryCard::State::AWAITING_COMMAND;
                    return true;
                } else { // ignore commands that aren't for Memory Cards
                    return false;
                }
            case MemoryCard::State::AWAITING_COMMAND:
                // always send FLAG in response
                data = this->_flag;
                switch (command.value_or(0x00)) { // decode memory card command
                case 0x52:
                    this->_state = MemoryCard::State::READ_DATA_COMMAND;
                    this->_sub_state.read_state = MemoryCard::ReadState::RECV_MEMCARD_ID_1;
                    break;
                case 0x57:
                    this->_state = MemoryCard::State::WRITE_DATA_COMMAND;
                    this->_sub_state.write_state = MemoryCard::WriteState::RECV_MEMCARD_ID_1;
                    break;
                case 0x53:
                    this->_state = MemoryCard::State::GET_MEMCARD_ID_COMMAND;
                    this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_MEMCARD_ID_1;
                    break;
                default:
                    this->_state = MemoryCard::State::IDLE;
                    return false; // No ACK (last byte)
                }
                return true; // ACK
            // otherwise, use sub-state-machines
            case MemoryCard::State::READ_DATA_COMMAND:
                return read_data_command(command, data);
            case MemoryCard::State::WRITE_DATA_COMMAND:
                return write_data_command(command, data);
            case MemoryCard::State::GET_MEMCARD_ID_COMMAND:
                return get_memcard_id_command(command, data);
            default:
                return false; // NACK
            }
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

    Generator<std::pair<bool, TriState>> MemoryCard::_state_machine(const TriState& data_in) {
        while (data_in != 0x81) co_yield std::make_pair(false, std::nullopt); // ignore commands that aren't for Memory Cards
        co_yield std::make_pair(true, std::nullopt); // received a Memory Card command --reply with ACK 
        // always send FLAG in response to a Memory Card command
        co_yield std::make_pair(true, this->_flag);
        switch (data_in.value_or(0x00)) { // decode memory card command
        case 0x52: {// READ_DATA_COMMAND
            // reply with memcard ID
            co_yield std::make_pair(true, 0x5A);
            co_yield std::make_pair(true, 0x5D);
            Byte checksum = data_in.value_or(0xFF); // reset checksum
            std::uint16_t address = (std::uint16_t)checksum << 8; // rx address MSB
            co_yield std::make_pair(true, 0x00);
            address |= data_in.value_or(0xFF);
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
            break;
        }
        case 0x57:
            // this->_state = MemoryCard::State::WRITE_DATA_COMMAND;
            // this->_sub_state.write_state = MemoryCard::WriteState::RECV_MEMCARD_ID_1;
            break;
        case 0x53:
            // this->_state = MemoryCard::State::GET_MEMCARD_ID_COMMAND;
            // this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_MEMCARD_ID_1;
            break;
        default:
            co_yield std::make_pair(false, std::nullopt); // No ACK (last byte)
            co_return;
        }
        // return true; // ACK
    }

    bool MemoryCard::read_data_command(
        TriState command,
        TriState& data
    ) {
        switch (this->_sub_state.read_state) {
        // for these two states, command is supposed to be 0x00 but what can we do if it's not?
        case MemoryCard::ReadState::RECV_MEMCARD_ID_1:
            data = 0x5A;
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_MEMCARD_ID_2;
            break;
        case MemoryCard::ReadState::RECV_MEMCARD_ID_2:
            data = 0x5D;
            this->_sub_state.read_state = MemoryCard::ReadState::SEND_ADDRESS_MSB;
            break;
        case MemoryCard::ReadState::SEND_ADDRESS_MSB:
            this->_checksum = command.value_or(0xFF); // reset checksum
            this->_address = (std::uint16_t)this->_checksum << 8;
            data = 0x00;
            this->_sub_state.read_state = MemoryCard::ReadState::SEND_ADDRESS_LSB;
            break;
        case MemoryCard::ReadState::SEND_ADDRESS_LSB:
            this->_address |= command.value_or(0xFF);
            this->_checksum ^= (Byte)(this->_address & 0x00FF);
            // detect invalid sectors (out of bounds)
            if (this->_address > MemoryCard::_LAST_SECTOR) {
                this->_address = 0xFFFF; // poison value
            }
            data = 0x00;
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_COMMAND_ACK_1;
            break;
        // for these two states, command is supposed to be 0x00 but what can we do if it's not?
        case MemoryCard::ReadState::RECV_COMMAND_ACK_1:
            data = 0x5C;
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_COMMAND_ACK_2;
            break;
        case MemoryCard::ReadState::RECV_COMMAND_ACK_2:
            data = 0x5D;
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_MSB;
            break;
        case MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_MSB:
            data = (Byte)(this->_address >> 8);
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_LSB;
            break;
        case MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_LSB:
            data = (Byte)(this->_address & 0x00FF);
            // we'll only continue if sector address is not a poison value
            if (this->_address == 0xFFFF) {
                this->_state = MemoryCard::State::IDLE;
                return false;
            } else {
                this->_sub_state.read_state = MemoryCard::ReadState::RECV_DATA_SECTOR;
                this->_byte_counter = 0x00; // init counter
                break;
            }
        case MemoryCard::ReadState::RECV_DATA_SECTOR:
            // reply with current byte from the correct sector
            data = this->get_sector(this->_address)[this->_byte_counter];
            // update checksum
            this->_checksum ^= data.value();
            this->_byte_counter++;
            if (this->_byte_counter == 128u) {
                this->_sub_state.read_state = MemoryCard::ReadState::RECV_CHECKSUM;
            }
            break;
        case MemoryCard::ReadState::RECV_CHECKSUM:
            data = this->_checksum;
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_END_BYTE;
            break;
        case MemoryCard::ReadState::RECV_END_BYTE:
            data = 0x47; // should always be 0x47 for "Good read"
            this->_state = MemoryCard::State::IDLE;
            return false;
        }
        return true;
    }

    bool MemoryCard::write_data_command(
        TriState command,
        TriState& data
    ) {
        switch (this->_sub_state.write_state) {
        // for these two states, command is supposed to be 0x00 but what can we do if it's not?
        case MemoryCard::WriteState::RECV_MEMCARD_ID_1:
            data = 0x5A;
            this->_sub_state.write_state = MemoryCard::WriteState::RECV_MEMCARD_ID_2;
            break;
        case MemoryCard::WriteState::RECV_MEMCARD_ID_2:
            data = 0x5D;
            this->_sub_state.write_state = MemoryCard::WriteState::SEND_ADDRESS_MSB;
            break;
        case MemoryCard::WriteState::SEND_ADDRESS_MSB:
            this->_checksum = command.value_or(0xFF); // reset checksum
            this->_address = (std::uint16_t)this->_checksum << 8;
            data = 0x00;
            this->_sub_state.write_state = MemoryCard::WriteState::SEND_ADDRESS_LSB;
            break;
        case MemoryCard::WriteState::SEND_ADDRESS_LSB:
            this->_address |= command.value_or(0xFF);
            this->_checksum ^= (Byte)(this->_address & 0x00FF);
            // detect invalid sectors (out of bounds)
            if (this->_address > MemoryCard::_LAST_SECTOR) {
                this->_address = 0xFFFF; // poison value
            }
            data = 0x00;
            this->_byte_counter = 0x00; // init counter
            this->_sub_state.write_state = MemoryCard::WriteState::SEND_DATA_SECTOR;
            break;
        case MemoryCard::WriteState::SEND_DATA_SECTOR:{
            // grab byte, converting Z-state to 0xFF if encountered (shouldn't, but...)
            Byte write_byte = command.value_or(0xFF);
            // so long as the sector address is valid, write the sector
            if (this->_address != 0xFFFF) {
                this->get_sector(this->_address)[this->_byte_counter] = write_byte;
            }
            // update the checksum
            this->_checksum ^= write_byte;
            this->_byte_counter++;
            data = 0x00;
            if (this->_byte_counter == 128u) {
                this->_sub_state.write_state = MemoryCard::WriteState::SEND_CHECKSUM;
            }
            break;
        }
        case MemoryCard::WriteState::SEND_CHECKSUM:{
            // set to inverted calculated checksum if no value, to force a bad checksum in that case
            Byte sent_checksum = command.value_or(~this->_checksum);
            /*
             * take checksum sent in command and validate against calculated
             * checksum
             * for brevity, store the result of comparison in the checksum
             */
            this->_checksum = sent_checksum == this->_checksum ? 0x00 : 0xFF;
            data = 0x00;
            this->_sub_state.write_state = MemoryCard::WriteState::RECV_COMMAND_ACK_1;
            break;
        }
        case MemoryCard::WriteState::RECV_COMMAND_ACK_1:
            data = 0x5C;
            this->_sub_state.write_state = MemoryCard::WriteState::RECV_COMMAND_ACK_2;
            break;
        case MemoryCard::WriteState::RECV_COMMAND_ACK_2:
            data = 0x5D;
            this->_sub_state.write_state = MemoryCard::WriteState::RECV_END_BYTE;
            break;
        case MemoryCard::WriteState::RECV_END_BYTE:
            /*
             * status end byte:
             * 0x47 = Good, 0x4E = Bad Checksum, 0xFF = Bad Sector
             */
            if (this->_address == 0xFFFF) {       // Bad Sector
                data = 0xFF;
            } else if (this->_checksum == 0xFF) { // Bad Checksum
                data = 0x4E;
            } else {                              // Good
                data = 0x47;
            }
            this->_state = MemoryCard::State::IDLE;
            return false;
        }
        return true;
    }

    bool MemoryCard::get_memcard_id_command(
        TriState,
        TriState& data
    ) {
        // XXX: This function is hell please refactor it
        switch (this->_sub_state.get_id_state) {
        // for these two states, command is supposed to be 0x00 but what can we do if it's not?
        case MemoryCard::GetIdState::RECV_MEMCARD_ID_1:
            data = 0x5A;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_MEMCARD_ID_2;
            break;
        case MemoryCard::GetIdState::RECV_MEMCARD_ID_2:
            data = 0x5D;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_COMMAND_ACK_1;
            break;
        // for these two states, command is supposed to be 0x00 but what can we do if it's not?
        case MemoryCard::GetIdState::RECV_COMMAND_ACK_1:
            data = 0x5C;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_COMMAND_ACK_2;
            break;
        case MemoryCard::GetIdState::RECV_COMMAND_ACK_2:
            data = 0x5D;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_INFO_1;
            break;
        case MemoryCard::GetIdState::RECV_INFO_1:
            data = 0x04;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_INFO_2;
            break;
        case MemoryCard::GetIdState::RECV_INFO_2:
            data = 0x00;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_INFO_3;
            break;
        case MemoryCard::GetIdState::RECV_INFO_3:
            data = 0x00;
            this->_sub_state.get_id_state = MemoryCard::GetIdState::RECV_INFO_4;
            break;
        case MemoryCard::GetIdState::RECV_INFO_4:
            data = 0x80;
            this->_state = MemoryCard::State::IDLE;
            return false;
        }
        return true;
    }

    const Byte MemoryCard::_FLAG_INIT_VALUE = 0x08;
    const MemoryCard::State MemoryCard::_STARTING_STATE = MemoryCard::State::IDLE;
    const std::uint16_t MemoryCard::_LAST_SECTOR = 0x03FF;
}
