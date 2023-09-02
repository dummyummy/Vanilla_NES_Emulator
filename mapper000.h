#ifndef MAPPER000_H
#define MAPPER000_H


#include "mapper.h"
class Mapper000: public Mapper
{
public:
    using Mapper::Mapper;

    virtual void init();
    virtual uint8_t cpuRead(uint16_t addr);
    virtual void cpuWrite(uint16_t addr, uint8_t value);
    virtual uint8_t ppuRead(uint16_t addr);
    virtual void ppuWrite(uint16_t addr, uint8_t value);
    virtual uint16_t mirrored(uint16_t addr); // evaluate mirrored address in nametable
};

#endif // MAPPER000_H
