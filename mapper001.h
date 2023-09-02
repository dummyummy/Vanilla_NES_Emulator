#ifndef MAPPER001_H
#define MAPPER001_H

#include "mapper.h"

class Mapper001 : public Mapper // actually MMC1B
{
public:
    using Mapper::Mapper;

    virtual void init();
    virtual uint8_t cpuRead(uint16_t addr);
    virtual void cpuWrite(uint16_t addr, uint8_t value);
    virtual uint8_t ppuRead(uint16_t addr);
    virtual void ppuWrite(uint16_t addr, uint8_t value);
    virtual uint16_t mirrored(uint16_t addr); // evaluate mirrored address in nametable

private: // internal regs
    int writeCount;
    uint8_t shift;
    uint8_t control;
    uint8_t chrBank0;
    uint8_t chrBank1;
    uint8_t pgrBank;
};

#endif // MAPPER001_H
