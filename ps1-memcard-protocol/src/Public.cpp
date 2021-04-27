/**
 * This is a sample source file corresponding to a public header file.
 *
 * <Copyright information goes here>
 */

#include <ps1-memcard-protocol/Public.hpp>

#include "Private.hpp"

namespace com::saxbophone::ps1_memcard_protocol {
    bool library_works() {
        return PRIVATE::library_works();
    }
}
