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

#ifndef COM_SAXBOPHONE_WONDERCARD_MEMORY_CARD_HPP
#define COM_SAXBOPHONE_WONDERCARD_MEMORY_CARD_HPP

#include <array>
#include <optional>
#include <span>
#include <utility>

#include <cstddef>
#include <cstdint>

#include <wondercard/common.hpp>
#include <wondercard/Generator.hpp>


namespace com::saxbophone::wondercard {
    /**
     * @brief Represents a virtual PS1 Memory Card
     */
    class MemoryCard {
    public:
        static constexpr std::size_t CARD_BLOCK_COUNT = 16u; /**< Number of Blocks on the card */
        static constexpr std::size_t BLOCK_SECTOR_COUNT = 64u; /**< Number of Sectors in a Block */
        static constexpr std::size_t SECTOR_SIZE = 128u; /**< Number of bytes in a Sector */
        static constexpr std::size_t BLOCK_SIZE = BLOCK_SECTOR_COUNT * SECTOR_SIZE; /**< Number of bytes in a Block */
        static constexpr std::size_t CARD_SIZE = CARD_BLOCK_COUNT * BLOCK_SIZE; /**< Number of bytes in a MemoryCard */

        /**
         * @brief A non-owning view of an entire save Block on the MemoryCard
         */
        typedef std::span<Byte, BLOCK_SIZE> Block;

        /**
         * @brief A non-owning view of a Sector on the MemoryCard
         */
        typedef std::span<Byte, SECTOR_SIZE> Sector;

        /**
         * @brief Initialises card data to all zeroes
         * @warning Default card data may change in future versions of the software
         */
        MemoryCard();

        /**
         * @brief Populates card data with that of the supplied span
         * @param data The data to initialise the card data with
         */
        MemoryCard(std::span<Byte, MemoryCard::CARD_SIZE> data);

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
            TriState command,
            TriState& data
        );

        /**
         * @returns The Block on this MemoryCard with the given index
         * @param index The index of the Block to retrieve (`{0..15}`)
         * @warning `index` is not currently validated
         */
        Block get_block(std::size_t index);

        /**
         * @returns The Sector on this MemoryCard with the given index
         * @param index The index of the Sector to retrieve (`{0..1023}`)
         * @warning `index` is not currently validated
         */
        Sector get_sector(std::size_t index);

        /**
         * @brief Read-only flag indicating whether the card is powered on or not
         */
        const bool& powered_on;

        /**
         * @brief writable accessor for the MemoryCard data bytes
         */
        std::span<Byte, CARD_SIZE> bytes;

    private:
        Generator<std::pair<bool, TriState>> _process_command(const TriState& data_in);

        Generator<std::pair<bool, TriState>> _read_data_command(const TriState& data_in);

        Generator<std::pair<bool, TriState>> _write_data_command(const TriState& data_in);

        // for exposition of Generator type only
        static Generator<std::pair<bool, TriState>> _typeof_state_machine(const TriState& data_in);

        using StateMachineType = decltype(_typeof_state_machine(TriState{}));
        StateMachineType _state_machine = _process_command(_data_in);
        TriState _data_in;

        const static Byte _FLAG_INIT_VALUE;
        const static std::uint16_t _LAST_SECTOR;

        bool _powered_on;
        Byte _flag;  // special FLAG value, a kind of status register on card
        // raw card data bytes
        std::array<Byte, CARD_SIZE> _bytes;
    };
}

#endif // include guard
