#ifndef RICOH2C02_H
#define RICOH2C02_H

#include "bus.h"
#include "frame.h"
#include <cstdint>
#include <string>
#include <tuple>

class RICOH2C02
{
public:
    RICOH2C02();
    ~RICOH2C02();
    void connectBus(Bus *bus);
    uint8_t readReg(uint16_t addr, bool readOnly);
    void writeReg(uint16_t addr, uint8_t value);
    void clock(); // let ppu run 1 clock cycle (1 cpu cycle = 3 ppu cycle)
    void clock3(); // let ppu run 1 clock cycle (1 cpu cycle = 3 ppu cycle)
    void reset();
    bool ok();
    unsigned char *rendered();

private:
    Bus *bus;

public:
    /*
    7  bit  0
    ---- ----
    VPHB SINN
    |||| ||||
    |||| ||++- Base nametable address
    |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
    |||| |+--- VRAM address increment per CPU read/write of PPUDATA
    |||| |     (0: add 1, going across; 1: add 32, going down)
    |||| +---- Sprite pattern table address for 8x8 sprites
    ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
    |||+------ Background pattern table address (0: $0000; 1: $1000)
    ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
    |+-------- PPU master/slave select
    |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
    +--------- Generate an NMI at the start of the
               vertical blanking interval (0: off; 1: on)
    */
    struct {
        uint8_t NN : 2;
        uint8_t I : 1;
        uint8_t S : 1;
        uint8_t B : 1;
        uint8_t H : 1;
        uint8_t P : 1;
        uint8_t V : 1;
    } PPUCTRL; // write-only
    /*
    7  bit  0
    ---- ----
    BGRs bMmG
    |||| ||||
    |||| |||+- Greyscale (0: normal color, 1: produce a greyscale display)
    |||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
    |||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
    |||| +---- 1: Show background
    |||+------ 1: Show sprites
    ||+------- Emphasize red (green on PAL/Dendy)
    |+-------- Emphasize green (red on PAL/Dendy)
    +--------- Emphasize blue
    */
    struct {
        uint8_t GS : 1; // grey scale
        uint8_t m : 1;
        uint8_t M : 1;
        uint8_t b : 1;
        uint8_t s : 1;
        uint8_t R : 1;
        uint8_t G : 1;
        uint8_t B : 1;
    } PPUMASK; // write-only
    /*
    7  bit  0
    ---- ----
    VSO. ....
    |||| ||||
    |||+-++++- PPU open bus. Returns stale PPU bus contents.
    ||+------- Sprite overflow. The intent was for this flag to be set
    ||         whenever more than eight sprites appear on a scanline, but a
    ||         hardware bug causes the actual behavior to be more complicated
    ||         and generate false positives as well as false negatives; see
    ||         PPU sprite evaluation. This flag is set during sprite
    ||         evaluation and cleared at dot 1 (the second dot) of the
    ||         pre-render line.
    |+-------- Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 overlaps
    |          a nonzero background pixel; cleared at dot 1 of the pre-render
    |          line.  Used for raster timing.
    +--------- Vertical blank has started (0: not in vblank; 1: in vblank).
               Set at dot 1 of line 241 (the line *after* the post-render
               line); cleared after reading $2002 and at dot 1 of the
               pre-render line.
    */
    struct {
        uint8_t OB : 5;
        uint8_t O : 1;
        uint8_t S : 1;
        uint8_t V : 1;
    } PPUSTATUS; // read-only
    uint8_t OAMADDR; // write-only
    uint8_t OAMDATA; // read/write
    uint8_t PPUSCROLL;
    uint8_t PPUADDR; // write x2
    uint8_t PPUDATA; // read/write
    // OAMDMA

private:
    uint8_t IOBusBuffer; // to emulate the open bus behaviour
    uint8_t internalBuffer; // to emulate the dummy read
    bool writeAddrHigh; // is high byte of address already written to PPUADDR
    bool writeScrollY;
    bool oddFlag;
    uint8_t scrollX;
    uint8_t scrollY;
    uint8_t palette[32];

private:
    uint8_t colourTable[64][3];
    bool initColourTable(const std::string path);

public: // for rendering
    int scanline; // current scanline
    int renderCycle;
    long long totalCycles;
    Frame frame;
    bool renderBg;
    bool renderSpr;
    bool okFlag;

public:
    void setOAM(int spr, int b, uint8_t value);
    uint8_t getOAM(int spr, int b);
private:
    uint8_t OAM[64][4];
private:
    uint8_t OAMcur[8][4];
    uint8_t OAMnext[8][4];
    uint8_t sprPalleteInd[8][8];
    int spriteCounter;
    int spr0Ind;
    int curSpriteCounter;
    int curSpr0Ind;

public: // internal registers
    uint16_t V;
    uint16_t T;
    uint8_t X;
    bool W;

private: // backgound rendering
    uint16_t bgShiftReg16[2]; // pattern table data for two tiles
    // use current palette instead
    // uint8_t bgShiftReg8[2]; // palette attributes for the lower 8 pixels of the 16-bit shift register
    uint8_t bgPalette[2];
    void coarseXInc();
    void fineYInc();
    void render();
private: // sprite rendering
    void spriteEval();

private:
    uint8_t read(uint16_t addr); // read from bus
    void write(uint16_t addr, uint8_t value); // write to bus

public:
    uint8_t readPalette(uint16_t addr); // read from palette
    void writePalette(uint16_t addr, uint8_t value); // write to palette
    std::tuple<uint8_t, uint8_t, uint8_t> evalColour(uint8_t colorInd);
    unsigned char tileBuffer[4][240 * 256][3];
    void evalNametable(int id);
};

#endif // RICOH2C02_H
