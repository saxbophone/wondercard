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

#include <optional>

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
        bool _powered_on;
    };
}

#endif // include guard
