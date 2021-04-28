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
                    break;
                default:
                    this->_state = MemoryCard::State::IDLE;
                    break;
                }
                // always send FLAG in response
                data = this->_flag;
                return true; // ACK
            // otherwise, use sub-state-machines
            case MemoryCard::State::READ_DATA_COMMAND:
                return read_data_command(command, data);
            case MemoryCard::State::WRITE_DATA_COMMAND:
                return write_data_command(command, data);
            case MemoryCard::State::GET_MEMCARD_ID_COMMAND:
                return get_memcard_id_command(command, data);
            }
        }
    }

    bool MemoryCard::read_data_command(
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
    ) {
        return {};
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
        return {};
    }

    const std::uint8_t MemoryCard::_FLAG_INIT_VALUE = 0x08;
    const MemoryCard::State MemoryCard::_STARTING_STATE = MemoryCard::State::IDLE;
}
