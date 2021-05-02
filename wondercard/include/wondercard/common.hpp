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

    /**
     * @brief User-defined literal for `std::uint8_t` types
     * @details Such literals can be written as `0x13_u8` and strict compilers
     * will then not complain of a narrowing type conversion (there are no
     * `std::uint8_t` literals in the language)
     * @param literal Literal value
     * @returns literal's value
     */
    constexpr std::uint8_t operator "" _u8(unsigned long long literal) {
        return (std::uint8_t)(literal);
    }


    /**
     * @brief User-defined literal for `std::uint16_t` types
     * @details Such literals can be written as `0x13FF_u16` and strict
     * compilers will then not complain of a narrowing type conversion (there
     * are no `std::uint16_t` literals in the language)
     * @param literal Literal value
     * @returns literal's value
     */
    constexpr std::uint16_t operator "" _u16(unsigned long long literal) {
        return (std::uint16_t)(literal);
    }
}

#endif // include guard
