#include <array>

#include <wondercard/common.hpp>
#include <wondercard/MemoryCard.hpp>
#include <wondercard/MemoryCardSlot.hpp>

int main() {
    using namespace com::saxbophone;
    /*
     * Fill the whole card with value 0x13
     * We can use any other type for the container as long as it is convertible
     * to std::span
     */
    std::array<wondercard::Byte, wondercard::MemoryCard::CARD_SIZE> card_data;
    card_data.fill(0x13);
    // initialise card with data
    wondercard::MemoryCard card(card_data);
    wondercard::MemoryCardSlot slot;
    // we must insert the card into the slot before we can read or write it
    if (not slot.insert_card(card)) {
        return -1; // failed to insert card
    }
    // read card data back out
    std::array<wondercard::Byte, wondercard::MemoryCard::CARD_SIZE> read_data;
    if (not slot.read_card(read_data)) {
        return -1; // read failure
    }
    // read data should be identical to card data
    if (not (read_data == card_data)) {
        return -1;
    }
    // we can also write data to the card, or write just specific parts of it
    std::array<wondercard::Byte, wondercard::MemoryCard::SECTOR_SIZE> sector = {
        0x13, 0x14, 0x15, 0x16, 0xCD, 0xBC, 0xB6, 0xB9, /* remainder unspecified */
    };
    // use sector values 0..1023 / 0x000..0x3FF
    if (not slot.write_sector(0x115, sector)) {
        return -1; // read failure
    }
    slot.remove_card();
}
