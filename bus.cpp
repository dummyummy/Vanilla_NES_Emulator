#include "bus.h"
#include "global.h"
#include "mos6502.h"
#include "ricoh2c02.h"
#include "mapper.h"

#include <iostream>

bool Bus::connectCPU(MOS6502 *cpu)
{
    if (cpu == nullptr)
        return false;
    this->cpu = cpu;
    return true;
}

bool Bus::connectPPU(RICOH2C02 *ppu)
{
    if (ppu == nullptr)
        return false;
    this->ppu = ppu;
    return true;
}

bool Bus::connectMapper(Mapper *mapper)
{
    if (mapper == nullptr)
        return false;
    this->mapper = mapper;
    return true;
}

bool Bus::connectJoypad1(Controller *joypad)
{
    if (joypad == nullptr)
        return false;
    this->joypad1 = joypad;
    return true;
}

bool Bus::connectJoypad2(Controller *joypad)
{
    if (joypad == nullptr)
        return false;
    this->joypad2 = joypad;
    return true;
}

Bus::Bus()
{
    RAM.resize(2_KB);
    CIRAM.resize(2_KB);
    testRAM.resize(64_KB);
    cpu = nullptr;
    ppu = nullptr;
    mapper = nullptr;
    joypad1 = nullptr;
    joypad2 = nullptr;
}

bool Bus::connectAll(MOS6502 *cpu, RICOH2C02 *ppu, Mapper *mapper)
{
    return connectCPU(cpu) && connectPPU(ppu) && connectMapper(mapper);
}

/*
$0000–$07FF	 $0800	2 KB internal RAM
$0800–$0FFF	 $0800	Mirrors of $0000–$07FF
$1000–$17FF	 $0800
$1800–$1FFF	 $0800
$2000–$2007	 $0008	NES PPU registers
$2008–$3FFF	 $1FF8	Mirrors of $2000–$2007 (repeats every 8 bytes)
$4000–$4017	 $0018	NES APU and I/O registers
$4018–$401F	 $0008	APU and I/O functionality that is normally disabled. See CPU Test Mode.
$4020–$FFFF	 $BFE0	Cartridge space: PRG ROM, PRG RAM, and mapper registers
*/
uint8_t Bus::cpuRead(uint16_t addr, bool readOnly)
{
    if (addr <= 0x1FFF) // internal RAM
    {
        return RAM[addr & 0x07FF];
    }
    else if (addr <= 0x3FFF) // PPU registers
    {
        return ppu->readReg((addr & 0x0007) | 0x2000, readOnly);
    }
    else if (addr <= 0x4017) // NES APU and I/O registers
    {
        if (addr == 0x4016 && joypad1)
            return joypad1->getInput(readOnly);
        if (addr == 0x4017 && joypad2)
            return joypad2->getInput(readOnly);
    }
    else if (addr <= 0x401F) // disabled
    {

    }
    else // Cartridge
    {
        return mapper->cpuRead(addr);
    }
    return 0;
}

void Bus::cpuWrite(uint16_t addr, uint8_t value)
{
    if (addr <= 0x1FFF) // internal RAM
    {
        RAM[addr & 0x07FF] = value;
    }
    else if (addr <= 0x3FFF) // PPU registers
    {
        ppu->writeReg(((addr & 0x0007) | 0x2000), value);
    }
    else if (addr <= 0x4017) // NES APU and I/O registers
    {
        if (addr == 0x4014)
            cpu->OAMDMA(value, ppu);
        if (addr == 0x4016)
        {
            if (joypad1)
                joypad1->setStrobe(value);
            if (joypad2)
                joypad2->setStrobe(value);
        }
    }
    else if (addr <= 0x401F) // disabled
    {

    }
    else // Cartridge
    {
        mapper->cpuWrite(addr, value);
    }
}

uint8_t Bus::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF)
    {
        return mapper->ppuRead(addr);
    }
    // no need to do boundary check
    else
    {
        uint16_t maddr = mapper->mirrored(addr);
        if (maddr >= 0x0800)
            maddr -= 0x0400;
        return CIRAM[maddr];
    }
}

void Bus::ppuWrite(uint16_t addr, uint8_t value)
{
    if (addr <= 0x1FFF)
    {
        mapper->ppuWrite(addr, value);
    }
    else
    {
        uint16_t maddr = mapper->mirrored(addr);
        if (maddr >= 0x0800)
            maddr -= 0x0400;
        CIRAM[maddr] = value;
    }
}

void Bus::nmi()
{
    cpu->nmi();
}

void Bus::irq()
{
    cpu->irq();
}
