# PS1 Memory Card Protocol
A fully virtual emulation of the protocol used by Memory Cards for the PlayStation.

This is a application-layer emulation of the Memory Card protocol, therefore the miscellany of the SPI protocol used at the signal level to communicate between the PlayStation and Memory Card is not represented exactly, instead we limit our representation to a model where bytes are sent to and from the virtual memory card, and provide a basic representation of Z-state (high impedance) inputs/outputs on the MOSI/MISO lines and a way to represent a missing card (ACK line never brought low).
