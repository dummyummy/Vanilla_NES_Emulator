#include "mapper002.h"
#include <iostream>
#include <iomanip>

void Mapper002::init()
{
    pgrBank = 0;
}

uint8_t Mapper002::cpuRead(uint16_t addr)
{
    if (0x6000 <= addr && addr <= 0x7FFF) // Battery-backed save or work RAM
    {
        return cart->nPRG_RAM ? cart->PRG_RAM[addr - 0x6000] : 0;
    }
    else if (0x8000 <= addr && addr <= 0xBFFF)
    {
        return cart->PRG_ROM[pgrBank][addr - 0x8000];
    }
    else if (0xC000 <= addr && addr <= 0xFFFF)
    {
        return cart->PRG_ROM[cart->nPRG_ROM - 1][addr - 0xC000];
    }
    else
    {
        std::cerr << "cpuRead out of bound in mapper " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
        return 0;
    }
}

void Mapper002::cpuWrite(uint16_t addr, uint8_t value)
{
    if (0x6000 <= addr && addr <= 0x7FFF) // battery-backed save or work RAM
    {
        if (cart->nPRG_RAM) cart->PRG_RAM[addr] = value;
    }
    else if (0x8000 <= addr && addr <= 0xFFFF)
    {
        if (cart->nPRG_ROM > 8) // UOROM
            pgrBank = value & 0x0F;
        else
            pgrBank = value & 0x07;
    }
    else
    {
        // do nothing
    }
}

uint8_t Mapper002::ppuRead(uint16_t addr)
{
    if (0x0000 <= addr && addr <= 0x1FFF) // pattern table 0
    {
        return cart->nCHR_ROM ?
               cart->CHR_ROM[0][addr] : // using CHR-ROM
               cart->CHR_RAM[0][addr];
    }
    else
    {
        std::cerr << "ppuRead out of bound in mapper " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
        return 0;
    }
}

void Mapper002::ppuWrite(uint16_t addr, uint8_t value)
{
    if (cart->nCHR_ROM)
        std::cerr << "Invalid PPU write to CHR-ROM " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
    else if (addr <= 0x1FFF)
        cart->CHR_RAM[0][addr] = value;
    else
        std::cerr << "ppuWrite out of bound in mapper " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
}

uint16_t Mapper002::mirrored(uint16_t addr)
{
    addr &= (cart->mirrorMode ? (~0xF800) : (~0xF400)); // vertical mirroring : horizontal mirroring
    return addr;
}
