#include "mapper001.h"

#include <iostream>
#include <iomanip>

void Mapper001::init()
{
    writeCount = 0;
    shift = 0;
    control = 0x0C;
    chrBank0 = 0;
    chrBank1 = 0;
    pgrBank = 0x00;
}

uint8_t Mapper001::cpuRead(uint16_t addr)
{
    int P = pgrBank & 0x0F;
    int R = (pgrBank >> 4) & 0x01;
    uint8_t ret;
    if (0x6000 <= addr && addr <= 0x7FFF) // Battery-backed save or work RAM
    {
        ret = (!R ? cart->PRG_RAM[addr - 0x6000] : 0);
    }
    else
    {
        switch ((control >> 2) & 0x03)
        {
        case 0:
        case 1:
            ret = cart->PRG_ROM[P][addr & 0x3FFF];
            break;
        case 2:
            ret = cart->PRG_ROM[addr < 0xC000 ? 0 : P][addr & 0x3FFF];
            break;
        case 3:
            ret = cart->PRG_ROM[addr < 0xC000 ? P : cart->nPRG_ROM - 1][addr & 0x3FFF];
            break;
        default:
            break;
        }
    }
    return ret;
}

void Mapper001::cpuWrite(uint16_t addr, uint8_t value)
{
    int R = (pgrBank >> 4) & 0x01;
    if (0x6000 <= addr && addr <= 0x7FFF && !R) // Battery-backed save or work RAM
    {
        cart->PRG_RAM[addr - 0x6000] = value;
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        if (value & 0x80)
        {
            shift = 0;
//            control = 0x0C;
            writeCount = 0;
        }
        else
        {
            shift = (shift >> 1) | ((value & 0x01) << 4);
            writeCount++;
        }
        if (writeCount == 5)
        {
            writeCount = 0;
            switch (addr & 0xE000)
            {
            case 0x8000: // Control
                control = shift;
                break;
            case 0xA000: // CHR Bank 0
                chrBank0 = shift;
                break;
            case 0xC000: // CHR Bank 1
                chrBank1 = shift;
                break;
            case 0xE000: // PGR Bank
                pgrBank = shift;
                break;
            default:
                break;
            }
            shift = 0;
        }
    }
}

uint8_t Mapper001::ppuRead(uint16_t addr)
{
    if (addr <= 0x1FFF) // maximum 8KB of CHR memory
    {
//        int C = (control >> 4) & 0x01;
        int upperBank = addr >= 0x1000;
        return cart->nCHR_ROM ?
               cart->CHR_ROM[upperBank ? (chrBank1 >> 1) : (chrBank0 >> 1)][addr] :
               cart->CHR_RAM[upperBank ? (chrBank1 >> 1) : (chrBank0 >> 1)][addr];
    }
    else
    {
        std::cerr << "ppuWrite out of bound in mapper " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
        return 0;
    }
}

void Mapper001::ppuWrite(uint16_t addr, uint8_t value)
{
    if (cart->nCHR_ROM)
        std::cerr << "Invalid PPU write to CHR-ROM " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
    else if (addr <= 0x1FFF) // maximum 8KB of CHR memory
    {
//        int C = (control >> 4) & 0x01;
        int upperBank = addr >= 0x1000;
        cart->CHR_RAM[upperBank ? (chrBank1 >> 1) : (chrBank0 >> 1)][addr] = value;
    }
    else
        std::cerr << "ppuWrite out of bound in mapper " << "addr=" << std::hex << std::setw(4) << std::setfill('0') << addr << std::endl;
}

uint16_t Mapper001::mirrored(uint16_t addr)
{
    int mirrorMode = control & 0x03;
    switch (mirrorMode)
    {
    case 0:
        addr &= 0x03FF;
        break;
    case 1:
        addr = (addr & 0x03FF) | 0x0400;
        break;
    case 2:
        addr &= ~0xF800;
        break;
    case 3:
        addr &= ~0xF400;
        break;
    default:
        break;
    }
    return addr;
}

