#ifndef COM_SAXBOPHONE_WONDERCARD_PRIVATE_TESTS_TEST_HELPERS_HPP
#define COM_SAXBOPHONE_WONDERCARD_PRIVATE_TESTS_TEST_HELPERS_HPP

#include <random>

#include <cstdint>

#include <wondercard/common.hpp>


using namespace com::saxbophone::wondercard;

namespace com::saxbophone::wondercard::PRIVATE::test_helpers {
    template<std::size_t SIZE>
    std::array<Byte, SIZE> generate_random_bytes() {
        std::array<Byte, SIZE> data;
        std::default_random_engine engine;
        // N.B: Can't use uint8_t for type as generator doesn't support char types
        std::uniform_int_distribution<std::uint16_t> prng(0x0000, 0x00FF);
        for (auto& d : data) {
            d = (Byte)prng(engine);
        }
        return data;
    }
}

#endif // include guard
