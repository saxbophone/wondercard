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

#include <optional>

#include <cstddef>
#include <cstdint>

#include <ps1-memcard-protocol/MemoryCard.hpp>


namespace com::saxbophone::ps1_memcard_protocol {
    MemoryCard::MemoryCard()
      : powered_on(this->_powered_on)
      , _powered_on(false)
      , _flag(MemoryCard::_FLAG_INIT_VALUE)
      , _state(MemoryCard::_STARTING_STATE)
      {}

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
        return !this->powered_on ? false : !(this->_powered_on = false);
    }

    bool MemoryCard::send(
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
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

    bool MemoryCard::read_data_command(
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
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
            this->_checksum ^= (std::uint8_t)(this->_address & 0x00FF);
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
            data = (std::uint8_t)(this->_address >> 8);
            this->_sub_state.read_state = MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_LSB;
            break;
        case MemoryCard::ReadState::RECV_CONFIRM_ADDRESS_LSB:
            data = (std::uint8_t)(this->_address & 0x00FF);
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
            // XXX: only all-zero data is implemented for now
            data = 0x00;
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
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
    ) {
        return {};
    }

    bool MemoryCard::get_memcard_id_command(
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
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

    const std::uint8_t MemoryCard::_FLAG_INIT_VALUE = 0x08;
    const MemoryCard::State MemoryCard::_STARTING_STATE = MemoryCard::State::IDLE;
    const std::uint16_t MemoryCard::_LAST_SECTOR = 0x03FF;
}
