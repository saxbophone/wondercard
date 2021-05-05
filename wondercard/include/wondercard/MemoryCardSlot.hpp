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

#ifndef COM_SAXBOPHONE_WONDERCARD_MEMORY_CARD_SLOT_HPP
#define COM_SAXBOPHONE_WONDERCARD_MEMORY_CARD_SLOT_HPP

#include <optional>
#include <span>

#include <cstddef>
#include <cstdint>

#include <wondercard/common.hpp>
#include <wondercard/MemoryCard.hpp>


namespace com::saxbophone::wondercard {
    /**
     * @brief A MemoryCardSlot is a device which a MemoryCard can be inserted
     * into and read/written from.
     */
    class MemoryCardSlot {
    public:
        MemoryCardSlot();

        /**
         * @brief Sends the given command byte to the inserted MemoryCard
         * @returns `false` when there is no MemoryCard inserted
         * @returns Response value from inserted MemoryCard when one is inserted
         * @param command Command byte to send (pass `std::nullopt` for High-Z)
         * @param[out] data Destination to write response data to
         */
        bool send(
            TriState command,
            TriState& data
        );

        /**
         * @brief Attempts to insert the given MemoryCard into this MemoryCardSlot
         * @returns `true` when card inserted successfully
         * @returns `false` when another card is already inserted
         * @param card MemoryCard to attempt to insert
         */
        bool insert_card(MemoryCard& card);

        /**
         * @brief Attempts to remove a MemoryCard from this MemoryCardSlot
         * @returns `true` when a card was removed successfully
         * @returns `false` when there was no card in the slot to remove
         */
        bool remove_card();

        /**
         * @brief Reads the entire contents of the inserted card
         * @returns Card data contents as array of bytes
         * @warning Not Implemented
         */
        std::array<Byte, MemoryCard::CARD_SIZE> read_card();

        /**
         * @brief Writes data from the given span to the entire card
         * @param data Data to write to the card
         * @warning Not Implemented
         */
        void write_card(std::span<Byte, MemoryCard::CARD_SIZE> data);

        /**
         * @brief Reads the specified block of the inserted card
         * @returns Block data contents as array of bytes
         * @param index Block to read from
         * @warning Not Implemented
         */
        std::array<Byte, MemoryCard::BLOCK_SIZE> read_block(std::size_t index);

        /**
         * @brief Writes data from the given span to the specified block of the
         * inserted card.
         * @param index Block to write to
         * @param data Data to write to the block
         * @warning Not Implemented
         */
        void write_block(std::size_t index, MemoryCard::Block data);

        /**
         * @brief Reads the specified sector of the inserted card
         * @returns false if failed to read data
         * @returns true if succeeded to read data
         * @param index Sector to read from
         * @param[out] data Span to write read data to
         */
        bool read_sector(std::size_t index, std::span<Byte, MemoryCard::SECTOR_SIZE> data);

        /**
         * @brief Writes data from the given span to the specified sector of
         * the inserted card.
         * @param index Sector to write to
         * @param data Data to write to the sector
         * @warning Not Implemented
         */
        void write_sector(std::size_t index, MemoryCard::Sector data);

    private:
        MemoryCard* _inserted_card;
    };
}

#endif // include guard
