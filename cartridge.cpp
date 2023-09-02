#include "cartridge.h"
#include "mapper000.h"
#include "mapper001.h"
#include "mapper002.h"
#include "global.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>



Cartridge::Cartridge()
{
    std::fill(header, header + 16, 0);
}

Cartridge::Cartridge(const std::string &path)
{
    std::fill(header, header + 16, 0);
}

Cartridge::~Cartridge()
{

}

void Cartridge::load(const std::string &path)
{
    // todo:
    // 1. read ines game pak
    // 2. evaluate ines pak
    // 4. dump binary code into storage
    // 3. configure mapper(bank switching and nametable mirroring)

    // 16-byte header
    std::ifstream rom(path, std::ios::binary | std::ios::in);
    if (!rom)
    {
        rom.close();
        throw std::string("Cannot open rom ") + path;
    }
    rom.read(reinterpret_cast<char*>(header), 16);
    // Check magic number
    if (magic1 != 0x4E || magic2 != 0x45 || magic3 != 0x53 || magic4 != 0x1A)
    {
        rom.close();
        throw std::string("Magic number not match");
    }
    if (version2 == 2)
    {
        rom.close();
        throw std::string("Do not support iNes 2.0 format");
    }
    // A general rule of thumb:
    // if the last 4 bytes of byte 10 are not all zero,
    // and the header is not marked for NES 2.0 format,
    // an emulator should either mask off the upper 4 bits
    // of the mapper number or simply refuse to load the ROM.
    if ((byte10Unused & 0x0F) && version2 != 2)
    {
        mapperID &= 0x0F;
        // throw std::string("Unrecognized ROM format(byte 10 in header is set)");
    }
    std::cout << "PRG ROM(16KB unit) count: " << (int)nPRG_ROM << std::endl;
    std::cout << "CHR ROM(8KB unit) count: " << (int)nCHR_ROM << std::endl;
    if (fourScreen)
        std::cout << "ROM is using four-screen mirroring" << std::endl;
    else
        std::cout << "ROM is using " << (mirrorMode ? "vertical" : "horizontal") << " mirroring" << std::endl;
    mapperID = (mapperH4 << 4) + mapperL4;
    std::cout << "Mapper ID is " << (int)mapperID << std::endl;
    std::cout << "PRG RAM(8KB unit) count: " << (int)nPRG_RAM << std::endl;
    std::cout << "TV system is " << (TV_system ? "PAL" : "NTSC") << std::endl;

    // init storage and mapper
    std::cout << std::endl << "Cartridge loaded" << std::endl;
    initROM();
    initRAM();
    initMapper();

    // trainer(is present)
    if (hasTrainer)
        trainer.resize(512);

    char buf[16_KB];
    // PGR_ROM
    if (nPRG_ROM)
    {
        std::cout << "Reading PRG ROM..." << std::endl;
        for (int i = 0; i < nPRG_ROM; i++)
        {
            rom.read(buf, 16_KB);
            std::copy_n(buf, 16_KB, PRG_ROM[i].begin());
        }
    }
    // CHR_ROM
    if (nCHR_ROM)
    {
        std::cout << "Reading CHR ROM..." << std::endl;
        for (int i = 0; i < nCHR_ROM; i++)
        {
            rom.read(buf, 8_KB);
            std::copy_n(buf, 8_KB, CHR_ROM[i].begin());
        }
    }
    // ROMS for PlayChoice

    // additional 128-byte title

    rom.close();
}

void Cartridge::initRAM()
{
    std::cout << "Initializing PRG RAM..." << std::endl;
    PRG_RAM.resize(8_KB, 0);
    nPRG_RAM = 1;
    std::cout << "Initializing CHR RAM..." << std::endl;
    if (!nCHR_ROM)
    {
        for (int i = 0; i < 32; i++)
            CHR_RAM[i].resize(8_KB, 0);
    }
}

void Cartridge::initROM()
{
    std::cout << "Initializing PRG ROM..." << std::endl;
    for (int i = 0; i < nPRG_ROM; i++)
        PRG_ROM[i].resize(16_KB, 0);
    std::cout << "Initializing CHR ROM..." << std::endl;
    for (int i = 0; i < nCHR_ROM; i++)
        CHR_ROM[i].resize(8_KB, 0);
}

void Cartridge::initMapper()
{
    std::cout << "Initializing mapper..." << std::endl;
    mapper = nullptr;
    switch (mapperID)
    {
    case 0:
        mapper = new Mapper000(this);
        break;
    case 1:
        mapper = new Mapper001(this);
        break;
    case 2:
        mapper = new Mapper002(this);
        break;
    default:
        throw "Unsupported mapper " + std::to_string(mapperID);
    }
    if (mapper)
        mapper->init();
}
