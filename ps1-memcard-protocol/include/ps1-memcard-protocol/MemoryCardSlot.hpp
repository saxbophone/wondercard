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

#ifndef COM_SAXBOPHONE_PS1_MEMCARD_PROTOCOL_MEMORY_CARD_SLOT_HPP
#define COM_SAXBOPHONE_PS1_MEMCARD_PROTOCOL_MEMORY_CARD_SLOT_HPP

#include <optional>

#include <cstdint>

#include <ps1-memcard-protocol/MemoryCard.hpp>


namespace com::saxbophone::ps1_memcard_protocol {
    /**
     * @brief Represents a virtual PS1 Memory Card
     * @todo Add parametrised constructors!
     */
    class MemoryCardSlot {
    public:
        bool send(
            std::optional<std::uint8_t> command,
            std::optional<std::uint8_t>& data
        );

        bool insert_card(MemoryCard& card);

        bool remove_card();

    private:
        //
    };
}

#endif // include guard
