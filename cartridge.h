#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <vector>
#include <cstdint>
#include <string>

class Mapper;

class Cartridge // iNES 1.0 format
{
public:
    std::vector<uint8_t> PRG_RAM;       // CPU $6000–$7FFF
    std::vector<uint8_t> PRG_ROM[32];   // CPU $8000–$FFFF
    std::vector<uint8_t> CHR_RAM[32];   // PPU $0000-$1FFF
    std::vector<uint8_t> CHR_ROM[32];   // PPU $0000-$1FFF
    std::vector<uint8_t> trainer;
    // for PlayChoice, usually not in use
    std::vector<uint8_t> INST_ROM;
    std::vector<uint8_t> PROM;

public:
    union {
        struct // 16-byte header
        {
            uint8_t magic1;
            uint8_t magic2;
            uint8_t magic3;
            uint8_t magic4;
            uint8_t nPRG_ROM;
            uint8_t nCHR_ROM;
            /* Flag 6
            76543210
            ||||||||
            |||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11)
            |||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10)
            ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory
            |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data)
            ||||+---- 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM
            ++++----- Lower nybble of mapper number
            */
            uint8_t mirrorMode : 1;
            uint8_t hasPGR_RAM : 1;
            uint8_t hasTrainer : 1;
            uint8_t fourScreen : 1;
            uint8_t mapperL4 : 4;
            /* Flag 7
            76543210
            ||||||||
            |||||||+- VS Unisystem
            ||||||+-- PlayChoice-10 (8 KB of Hint Screen data stored after CHR data)
            ||||++--- If equal to 2, flags 8-15 are in NES 2.0 format
            ++++----- Upper nybble of mapper number
            */
            uint8_t VS_Unisystem : 1;
            uint8_t PlayChoise_10 : 1;
            uint8_t version2 : 2;
            uint8_t mapperH4 : 4;
            /* Flag 8
            76543210
            ||||||||
            ++++++++- PRG RAM size
            */
            uint8_t nPRG_RAM;
            /* Flag 9
            76543210
            ||||||||
            |||||||+- TV system (0: NTSC; 1: PAL)
            +++++++-- Reserved, set to zero
            */
            uint8_t TV_system : 1;
            uint8_t byte9Unused: 7;
            // Reserved bytes
            uint8_t byte10Unused;
            uint8_t byte11Unused;
            uint8_t byte12Unused;
            uint8_t byte13Unused;
            uint8_t byte14Unused;
            uint8_t byte15Unused;
        };
        uint8_t header[16];
    };
    uint8_t mapperID;
    Mapper *mapper;

public:
    Cartridge();
    Cartridge(const std::string &path);
    ~Cartridge();

    void load(const std::string &path); // load nes rom from the file that path is pointing at

private:
    void initRAM();
    void initROM();
    void initMapper();
};

#endif // CARTRIDGE_H
