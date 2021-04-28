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
    MemoryCard::MemoryCard() : powered_on(false) {}

    bool MemoryCard::power_on() {
        return this->powered_on ? false : this->powered_on = true;
    }

    bool MemoryCard::power_off() {
        return !this->powered_on ? false : !(this->powered_on = false);
    }

    bool MemoryCard::send(
        std::optional<std::uint8_t> command,
        std::optional<std::uint8_t>& data
    ) {
        return true;
    }
}
