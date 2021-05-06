# Wondercard
A fully virtual emulation of the protocol used by Memory Cards for the PlayStation.

This is a application-layer emulation of the Memory Card protocol, therefore the miscellany of the SPI protocol used at the signal level to communicate between the PlayStation and Memory Card is not represented exactly.

Instead we limit our representation to a model where bytes are sent to and from the virtual memory card, and provide a basic representation of Z-state (high impedance) inputs/outputs on the MOSI/MISO lines and a way to represent a missing card (ACK line never brought low).

## Installation
This project builds with CMake v3.15+ on recent versions of Linux, macOS and Windows. You need a compiler supporting some C++20 features to build it.

```bash
cd build  # or whatever other place you want to compile
cmake ..
make
make install  # optional
```

## Usage

[MemoryCard]: @ref com::saxbophone::wondercard::MemoryCard
[MemoryCardSlot]: @ref com::saxbophone::wondercard::MemoryCardSlot

Everything lives in the com::saxbophone::wondercard namespace.

The current functionality comprises a virtual [MemoryCard] class which implements the logic found on memory cards, and a virtual [MemoryCardSlot] class for reading and writing the cards.

### Complete Intro Demo

```cpp
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
```

If the default constructor for [MemoryCard] is used, then the card is initialised with all-zero data.

Reading and writing cards is supported for the entire card, by Block (analogous to the save blocks used by the PlayStation card manager) and by Sector. A Card is divided into 16 Blocks, each of these being divided into 64 Sectors. Here is a table of Card, Block and Sector size conversions:

| Row per Column | Card   | Block | Sector |
|----------------|--------|-------|--------|
| Block          | 16     | 1     | N/A    |
| Sector         | 1024   | 64    | 1      |
| Byte           | 128KiB | 8192  | 128    |

## API Reference

[Online Documentation](https://saxbophone.com/wondercard)

> The docs site is separated by minor version. You can access documentation for other versions of the project by changing the `v*.*` part of the URL. Currently, only minor versions are intended to be compatible with eachother.
