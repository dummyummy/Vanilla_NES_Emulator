#ifndef MAPPER_H
#define MAPPER_H

#include "cartridge.h"
#include <cstdint>

class Cartridge;

class Mapper
{
protected:
    Cartridge *cart;
public:
    Mapper();
    Mapper(Cartridge *cart);
    virtual void init() = 0;
    virtual uint8_t cpuRead(uint16_t addr) = 0;
    virtual void cpuWrite(uint16_t addr, uint8_t value) = 0;
    virtual uint8_t ppuRead(uint16_t addr) = 0;
    virtual void ppuWrite(uint16_t addr, uint8_t value) = 0;
    virtual uint16_t mirrored(uint16_t addr) = 0; // evaluate mirrored address
};

#endif // MAPPER_H
