/**
 * @file
 * This file forms part of PS1 Memory Card Protocol, a fully virtual emulation
 * of the protocol used by Memory Cards for the PlayStation.
 *
 * @author Joshua Saxby <joshua.a.saxby@gmail.com>
 * @date 2021-04-28
 *
 * @copyright Joshua Saxby 2021. All rights reserved.
 *
 * @copyright
 * This is closed-source software and may not be produced in either part or in
 * full, in either source or binary form, without the express written consent
 * of the copyright holder.
 *
 */

#ifndef COM_SAXBOPHONE_PS1_MEMCARD_PROTOCOL_MEMORY_CARD_HPP
#define COM_SAXBOPHONE_PS1_MEMCARD_PROTOCOL_MEMORY_CARD_HPP

#include <array>
#include <optional>
#include <span>

#include <cstddef>
#include <cstdint>


namespace com::saxbophone::ps1_memcard_protocol {
    /**
     * @brief Represents a virtual PS1 Memory Card
     * @todo Add parametrised constructors!
     */
    class MemoryCard {
    public:
        /**
         * @brief Default constructor
         * @details Initialises card data to all zeroes
         * @warning Default card data may change in future versions of the software
         */
        MemoryCard();

        /**
         * @brief Simulates powering up the card, e.g. when inserted into slot
         * @details Cards know when they have been re-inserted, so they have
         * some way of tracking when they are plugged in and out, presumably
         * this must be done by tracking when the card is powered on and off.
         * @returns `false` if card is already powered on.
         * @returns `true` if card has been powered on by the method call.
         */
        bool power_on();

        /**
         * @brief Simulates powering down the card, e.g. when removed from slot
         * @details Cards know when they have been re-inserted, so they have
         * some way of tracking when they are plugged in and out, presumably
         * this must be done by tracking when the card is powered on and off.
         * @returns `false` if card is already powered down.
         * @returns `true` if card has been powered off by the method call.
         */
        bool power_off();

        /**
         * @brief Attempts to send a command and receive data from the card
         * @param command Command to send to the card
         * (set empty for High Impedance/Z-state, indicating don't care)
         * @param data Destination to store response data from card in
         * (set empty for High Impedance/Z-state, indicating no data)
         * @returns `true` if the card responds with an ACK
         * @returns `false` if we time out waiting for an ACK from the card
         */
        bool send(
            std::optional<std::uint8_t> command,
            std::optional<std::uint8_t>& data
        );

        /**
         * @brief Read-only flag indicating whether the card is powered on or not
         */
        const bool& powered_on;

    private:
        enum class State {
            IDLE,                   /**< Not currently in a communication transaction */
            AWAITING_COMMAND,       /**< Which Memory Card Command mode? */
            READ_DATA_COMMAND,      /**< Reading Memory Card Mode */
            WRITE_DATA_COMMAND,     /**< Writing Memory Card Mode */
            GET_MEMCARD_ID_COMMAND, /**< Get Memory Card ID Mode */
        };

        enum class ReadState {
            RECV_MEMCARD_ID_1,
            RECV_MEMCARD_ID_2,
            SEND_ADDRESS_MSB,
            SEND_ADDRESS_LSB,
            RECV_COMMAND_ACK_1,
            RECV_COMMAND_ACK_2,
            RECV_CONFIRM_ADDRESS_MSB,
            RECV_CONFIRM_ADDRESS_LSB,
            RECV_DATA_SECTOR,
            RECV_CHECKSUM,
            RECV_END_BYTE,
        };

        enum class WriteState {
            RECV_MEMCARD_ID_1,
            RECV_MEMCARD_ID_2,
            SEND_ADDRESS_MSB,
            SEND_ADDRESS_LSB,
            SEND_DATA_SECTOR,
            SEND_CHECKSUM,
            RECV_COMMAND_ACK_1,
            RECV_COMMAND_ACK_2,
            RECV_END_BYTE,
        };

        enum class GetIdState {
            RECV_MEMCARD_ID_1,
            RECV_MEMCARD_ID_2,
            RECV_COMMAND_ACK_1,
            RECV_COMMAND_ACK_2,
            RECV_INFO_1,
            RECV_INFO_2,
            RECV_INFO_3,
            RECV_INFO_4,
        };

        union SubState {
            ReadState read_state;
            WriteState write_state;
            GetIdState get_id_state;
        };

        bool read_data_command(
            std::optional<std::uint8_t> command,
            std::optional<std::uint8_t>& data
        );

        bool write_data_command(
            std::optional<std::uint8_t> command,
            std::optional<std::uint8_t>& data
        );

        bool get_memcard_id_command(
            std::optional<std::uint8_t> command,
            std::optional<std::uint8_t>& data
        );

        const static std::uint8_t _FLAG_INIT_VALUE;
        const static State _STARTING_STATE;
        const static std::uint16_t _LAST_SECTOR;

        bool _powered_on;
        std::uint8_t _flag;  // special FLAG value, a kind of status register on card
        State _state;        // state machine state
        SubState _sub_state; // sub-machine state
        std::uint16_t _address; // sector of address to read/write
        std::uint8_t _byte_counter; // index for tracking how many bytes read/written
        std::uint8_t _checksum; // scratchpad value for calculating checksums
    };
}

#endif // include guard
