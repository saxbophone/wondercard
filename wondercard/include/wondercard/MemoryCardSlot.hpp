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
        enum class IOResult {
            UNKNOWN_STATUS = 0,
            NO_CARD,
            NO_ACK,
            INVALID_RESPONSE,
            UNEXPECTED_ACK,
            BAD_CHECKSUM,
            BED_SECTOR,
            SUCCESS,
        };

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
         * @returns true/false indicating read sucess/failure
         * @param[out] data destination to write read data to
         * @warning Not Implemented
         */
        bool read_card(std::span<Byte, MemoryCard::CARD_SIZE> data);

        /**
         * @brief Writes data from the given span to the entire card
         * @returns true/false indicating write sucess/failure
         * @param data Data to write to the card
         */
        bool write_card(std::span<Byte, MemoryCard::CARD_SIZE> data);

        /**
         * @brief Reads the specified block of the inserted card
         * @returns true/false indicating read sucess/failure
         * @param index Block to read from
         * @param[out] data destination to write read data to
         * @todo Change return type to an enum or introduce exception throwing
         * so the variety of causes of failure can be determined by the caller.
         */
        bool read_block(std::size_t index, MemoryCard::Block data);

        /**
         * @brief Writes data from the given span to the specified block of the
         * inserted card.
         * @returns true/false indicating write sucess/failure
         * @param index Block to write to
         * @param data Data to write to the block
         */
        bool write_block(std::size_t index, MemoryCard::Block data);

        /**
         * @brief Reads the specified sector of the inserted card
         * @returns true/false indicating read sucess/failure
         * @param index Sector to read from
         * @param[out] data destination to write read data to
         * @todo Change return type to an enum or introduce exception throwing
         * so the variety of causes of failure can be determined by the caller.
         */
        bool read_sector(std::size_t index, MemoryCard::Sector data);

        /**
         * @brief Writes data from the given span to the specified sector of
         * the inserted card.
         * @returns true/false indicating write sucess/failure
         * @param index Sector to write to
         * @param data Data to write to the sector
         */
        bool write_sector(std::size_t index, MemoryCard::Sector data);

    private:
        template <std::size_t sector_index>
        bool _read_block_sector(std::size_t block_sector, MemoryCard::Block data);

        template <std::size_t sector_index>
        bool _write_block_sector(std::size_t block_sector, MemoryCard::Block data);

        template <std::size_t block_index>
        bool _read_card_block(std::span<Byte, MemoryCard::CARD_SIZE> data);

        template <std::size_t block_index>
        bool _write_card_block(std::span<Byte, MemoryCard::CARD_SIZE> data);

        MemoryCard* _inserted_card;
    };
}

#endif // include guard
