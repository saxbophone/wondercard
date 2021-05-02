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

#ifndef COM_SAXBOPHONE_WONDERCARD_COMMON_HPP
#define COM_SAXBOPHONE_WONDERCARD_COMMON_HPP

#include <optional>

#include <cstdint>


namespace com::saxbophone::wondercard {
    typedef std::uint8_t Byte; /**< An 8-bit unsigned byte as used by the protocol code */
    typedef std::optional<Byte> TriState; /**< A Tri-State Byte which uses `std::nullopt` to represent High-Z state */
}

#endif // include guard
