#include "ricoh2c02.h"

#include <iostream>
#include <cstdlib>
#include <iomanip>

RICOH2C02::RICOH2C02()
{
    // initialize palette
    std::string path = "";
    if (initColourTable(""))
    {
        std::cout << "Palette loaded from " << path << std::endl;
    }
    else
    {
        uint8_t _colourTable[64][3] {
            0x5A, 0x53, 0x5E,
            0x00, 0x17, 0x7E,
            0x00, 0x08, 0x93,
            0x2F, 0x06, 0x83,
            0x5C, 0x03, 0x54,
            0x75, 0x00, 0x13,
            0x71, 0x00, 0x00,
            0x4F, 0x08, 0x00,
            0x18, 0x1B, 0x00,
            0x00, 0x29, 0x00,
            0x00, 0x2F, 0x00,
            0x00, 0x2D, 0x09,
            0x00, 0x25, 0x4C,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0xAE, 0xA1, 0xB6,
            0x00, 0x55, 0xD9,
            0x24, 0x3E, 0xFB,
            0x71, 0x29, 0xEE,
            0xB3, 0x1A, 0xB6,
            0xD9, 0x17, 0x62,
            0xD9, 0x20, 0x08,
            0xAD, 0x35, 0x00,
            0x68, 0x4E, 0x00,
            0x1C, 0x63, 0x00,
            0x00, 0x6F, 0x00,
            0x00, 0x70, 0x37,
            0x00, 0x67, 0x91,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0xFF, 0xF9, 0xFF,
            0x3B, 0xA3, 0xFF,
            0x65, 0x80, 0xFF,
            0xA1, 0x71, 0xFF,
            0xF9, 0x76, 0xFF,
            0xFF, 0x78, 0xCD,
            0xFF, 0x7E, 0x81,
            0xFF, 0x87, 0x34,
            0xD8, 0x9B, 0x00,
            0x8A, 0xB2, 0x04,
            0x43, 0xC2, 0x36,
            0x17, 0xC7, 0x88,
            0x11, 0xC0, 0xE5,
            0x40, 0x3B, 0x43,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0xFF, 0xF9, 0xFF,
            0xB1, 0xD9, 0xFF,
            0xBB, 0xC4, 0xFF,
            0xD5, 0xBB, 0xFF,
            0xFC, 0xC0, 0xFF,
            0xFF, 0xC2, 0xFF,
            0xFF, 0xC4, 0xDC,
            0xFF, 0xC8, 0xBD,
            0xFB, 0xD0, 0xA6,
            0xDC, 0xDA, 0xA5,
            0xBD, 0xE1, 0xB7,
            0xA9, 0xE4, 0xD7,
            0xA4, 0xE2, 0xFD,
            0xB8, 0xAA, 0xC0,
            0x00, 0x00, 0x00,
            0x00, 0x00, 0x00
        };
        memcpy(colourTable, _colourTable, 64 * 3);
    }

    scanline = 0; // pre-render scanline
    totalCycles = -3 * 7; // wait cpu to reset
    renderCycle = -3 * 7; // wait cpu to reset
    // power-up state
    *(uint8_t*)&PPUCTRL = 0;
    *(uint8_t*)&PPUMASK = 0;
    *(uint8_t*)&PPUSTATUS = 0;
    *(uint8_t*)&PPUSCROLL = 0;
    *(uint8_t*)&PPUSTATUS = 0;
    oddFlag = false;
    writeAddrHigh = false;
    writeReg(0x2006, 0);
    writeReg(0x2006, 0);
    internalBuffer = 0;
    V = T = X = 0;
    W = false;
    renderBg = renderSpr = true;
    okFlag = false;
}

void RICOH2C02::connectBus(Bus *bus)
{
    this->bus = bus;
}

uint8_t RICOH2C02::readReg(uint16_t addr, bool readOnly)
{
    uint8_t ret = 0;
    switch (addr)
    {
    case 0x2000:
        ret = *(uint8_t*)(&PPUCTRL);
        break;
    case 0x2001:
        ret = *(uint8_t*)(&PPUMASK);
        break;
    case 0x2002:
        ret = *(uint8_t*)(&PPUSTATUS);
        PPUSTATUS.V = 0; // cleared after reading $2002
        W = 0;
        break;
    case 0x2003:
        ret = OAMADDR;
        break;
    case 0x2004:
        OAMDATA = *((uint8_t*)OAM + OAMADDR);
        ret = OAMDATA;
        break;
    case 0x2005:
        ret = PPUSCROLL;
        break;
    case 0x2006:
        ret = PPUADDR;
        break;
    case 0x2007:
        if (V > 0x3EFF && V <= 0x3FFF)
        {
            ret = readPalette(V);
            if (!readOnly)
                internalBuffer = read(V);
        }
        else
        {
            if (readOnly)
            {
                ret = read(V);
            }
            else
            {
                ret = internalBuffer;
                // update the internal buffer with the byte at the current VRAM address
                internalBuffer = read(V);
            }
        }
        if (!readOnly)
            V += (PPUCTRL.I ? 32 : 1);
        break;
    default:
        std::cerr << "PPU register $" << std::hex << std::setw(4) << std::setfill('0') << addr << " is write-only" << std::endl;
        ret = 0;
    }
    return ret;
}

void RICOH2C02::writeReg(uint16_t addr, uint8_t value)
{
    bool renderFlag;
    switch (addr)
    {
    case 0x2000:
        *(uint8_t*)(&PPUCTRL) = value;
        T = (((value & 0x3) << 10) | (T & (~0x0C00)));
        break;
    case 0x2001:
        *(uint8_t*)(&PPUMASK) = value;
        break;
    case 0x2003:
        OAMADDR = value;
        break;
    case 0x2004:
        OAMDATA = value;
        *((uint8_t*)OAM + OAMADDR) = OAMDATA;
        OAMADDR++;
        break;
    case 0x2005: // PPUSCROLL write x2
        if (!W)
        {
            /*
            t: ....... ...ABCDE <- d: ABCDE...
            x:              FGH <- d: .....FGH
            w:                  <- 1
            */
            T = (((value & 0xF8) >> 3) | (T & (~0x001F)));
            X = (value & 0x07);
        }
        else
        {
            /*
            t: FGH..AB CDE..... <- d: ABCDEFGH
            w:                  <- 0
            */
            T = (((value & 0xF8) << 2) | (T & (~0x03E0)));
            T = (((value & 0x07) << 12) | (T & (~0x7000)));
        }
        W = !W;
        PPUSCROLL = value;
        break;
    case 0x2006: // PPUADDR write x2
        if (!W)
        {

            /*
            t: .CDEFGH ........ <- d: ..CDEFGH
                   <unused>     <- d: AB......
            t: Z...... ........ <- 0 (bit Z is cleared)
            */
            T = (((value & 0x3F) << 8) | (T & (~0x3F00)));
            T &= 0x3FFF;
        }
        else
        {
            /*
            t: ....... ABCDEFGH <- d: ABCDEFGH
            v: <...all bits...> <- t: <...all bits...>
            */
            T = (value | (T & (~0x00FF)));
            V = T;
        }
        W = !W;
        PPUADDR = value;
        break;
    case 0x2007:
        if (V > 0x3EFF && V <= 0x3FFF)
        {
            writePalette(V, value);
        }
        else
        {
            write(V, value);
        }
        renderFlag = (PPUMASK.s || PPUMASK.b);
        if (renderFlag && (scanline <= 239 || scanline == 261))
        {
            coarseXInc();
            fineYInc();
        }
        else
            V += (PPUCTRL.I ? 32 : 1);
        break;
    default:
        std::cerr << "PPU register $" << std::hex << std::setw(4) << std::setfill('0') << addr << " is read-only" << std::endl;
    }
}

void RICOH2C02::clock()
{
    // Render background and sprites
    render();

    // Sprite Evaluation
    spriteEval();

    if (scanline == 239 && renderCycle == 256)
        frame.swapBuffer();
    if (scanline == 261 && renderCycle == 339 && (PPUMASK.s || PPUMASK.b))
        renderCycle++; // skip one cycle
    if (scanline == 261 && renderCycle == 340)
        oddFlag = !oddFlag;
    if (renderCycle == 340)
        scanline = (scanline + 1) % 262;
    renderCycle = (renderCycle + 1) % 341;
    totalCycles++;
}

void RICOH2C02::clock3()
{
    clock();
    clock();
    clock();
}

void RICOH2C02::reset()
{
    totalCycles = -3 * 7; // wait cpu to reset
    renderCycle = -3 * 7; // wait cpu to reset
}

bool RICOH2C02::ok()
{
    return scanline > 239;
}

unsigned char *RICOH2C02::rendered()
{
    return frame.getRawImage();
}

bool RICOH2C02::initColourTable(const std::string path)
{
    return false;
}

void RICOH2C02::setOAM(int spr, int b, uint8_t value)
{
    OAM[spr][b] = value;
}

uint8_t RICOH2C02::getOAM(int spr, int b)
{
    return OAM[spr][b];
}

void RICOH2C02::coarseXInc()
{
    // coarse X increment
    if ((V & 0x001F) == 31)    // if coarse X == 31
    {
        V &= ~0x001F;          // coarse X = 0
        V ^= 0x0400;           // switch horizontal nametable
    }
    else
        V += 1;                // increment coarse X
}

void RICOH2C02::fineYInc()
{
    if ((V & 0x7000) != 0x7000)             // if fine Y < 7
    {
        V += 0x1000;                        // increment fine Y
    }
    else
    {

        V &= ~0x7000;                       // fine Y = 0
        int y = ((V & 0x03E0) >> 5);        // let y = coarse Y
        if (y == 29)
        {
            y = 0;                          // coarse Y = 0
            V ^= 0x0800;                    // switch vertical nametable
        }
        else if (y == 31)
            y = 0;                          // coarse Y = 0, nametable not switched
        else
            y += 1;                         // increment coarse Y
        V = ((V & ~0x03E0) | (y << 5));     // put coarse Y back into v
    }
}

void RICOH2C02::render()
{
    // each clock cycle produces one pixel
    // The PPU renders 262 scanlines per frame
    // Each scanline lasts for 341 PPU clock cycles
    // 1 CPU cycle = 3 PPU cycles
    static uint8_t latch[4];
    static uint16_t addr; // temp addr
    static uint16_t sAddr; // sprite pattern addr
    static uint8_t pAddr; // palette addr for background
    static uint8_t aOffset; // attribute offset for background
    static uint8_t cR, cG, cB;
    static uint8_t renderFlag;
    static uint8_t X0 = X;

    /*
    every 8 cycles:
    1. Fetch a nametable entry from $2000-$2FBF.
    2. Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same row.
    3. Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
    4. Fetch the high-order byte of this sliver from an address 8 bytes higher.
    5. Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data using priority.
    */


    renderFlag = ((PPUMASK.s << 1) | PPUMASK.b);
    if (totalCycles < 0 || renderCycle < 0)
    {

    }
    else if (scanline == 261 || scanline <= 239) // visible scanlines
    {
        if (renderCycle == 0)
        {
            // fetch 8x8 sprite data
            for (int i = curSpriteCounter - 1; i >= 0; i--)
            {
                uint8_t pOffset = scanline - OAMcur[i][0];
                uint16_t tempAddr;
                if (OAMcur[i][2] & 0x80) // flip sprite vertically
                    pOffset = (PPUCTRL.H ? 15 : 7) - pOffset;
                if (!PPUCTRL.H)
                    tempAddr = ((PPUCTRL.S << 12) | (OAMcur[i][1] << 4) | pOffset);
                else
                {
                    if (pOffset <= 7)
                        tempAddr = (((OAMcur[i][1] & 0x01) << 12) | ((OAMcur[i][1] & 0xFE) << 4) | pOffset);
                    else
                        tempAddr = (((OAMcur[i][1] & 0x01) << 12) | ((OAMcur[i][1] & 0xFE) << 4) | (pOffset & 0x07) | 0x10);
                }
                uint8_t sprLow = read(tempAddr);
                uint8_t sprHigh = read(tempAddr + 8);
                for (int j = 0; j < 8; j++)
                {
                    if (OAMcur[i][2] & 0x40) // flip sprite horizontally
                        sprPalleteInd[i][j] = (((sprHigh << 1) & 0x02) | (sprLow & 0x01));
                    else
                        sprPalleteInd[i][7 - j] = (((sprHigh << 1) & 0x02) | (sprLow & 0x01));
                    sprHigh >>= 1;
                    sprLow >>= 1;
                }
            }
        }
        else if (renderCycle <= 256)
        {
            if (renderFlag)
            {
                /* background */
                int stage = (renderCycle - 1) % 8;
                bool bgFlag = false;
                if (renderBg && scanline != 261 && PPUMASK.b && (renderCycle > 8 || PPUMASK.m)) // background
                {
                    int offset = ((7 - stage) - X0);
                    if (offset < 0)
                        offset += 16;
                    uint8_t bgLow = ((bgShiftReg16[0] >> offset) & 0x01);
                    uint8_t bgHigh = (((bgShiftReg16[1] >> offset) & 0x01) << 1);

                    pAddr = ((bgPalette[offset > 7 ? 1 : 0] << 2) | bgHigh | bgLow);
                    if (pAddr & 0x03)
                        bgFlag = true;
                    // it seems although 04, 08 and 1c maybe different from 00(the background)
                    // but during rendering, just treat them as background colour
                    if (!bgHigh && !bgLow)
                        pAddr = 0;
                    std::tie(cR, cG, cB) = evalColour(readPalette(pAddr));
                    frame.setColor(scanline, renderCycle - 1, cR, cG, cB);
                }
                // is there visual persistence?
                else if (scanline != 261)
                {
                    frame.setColor(scanline, renderCycle - 1, 0, 0, 0);
                    pAddr = 0;
                }

                /* sprite */
                if (renderSpr && scanline != 261 && PPUMASK.s && (renderCycle > 8 || PPUMASK.M))
                {
                    for (int i = curSpriteCounter - 1; i >= 0; i--)
                    {
                        if (OAMcur[i][3] <= renderCycle - 1 && OAMcur[i][3] + 7 >= renderCycle - 1)
                        {
                            sAddr = (0x10 | ((OAMcur[i][2] << 2) & 0x0C) | sprPalleteInd[i][renderCycle - 1 - OAMcur[i][3]]);
                            if ((!(OAMcur[i][2] & 0x20) || !bgFlag) && sprPalleteInd[i][renderCycle - 1 - OAMcur[i][3]]) // sprite in foreground
                            {
                                std::tie(cR, cG, cB) = evalColour(readPalette(sAddr));
                                frame.setColor(scanline, renderCycle - 1, cR, cG, cB);
                            }
                            if (renderFlag == 3 && bgFlag && sprPalleteInd[i][renderCycle - 1 - OAMcur[i][3]] && curSpr0Ind == i && renderCycle != 256)
                                PPUSTATUS.S = 1;
                        }
                    }
                }
                switch (stage)
                {
                case 1: // nametable entry
                    addr = (0x2000 | (V & 0x0FFF));
                    latch[0] = read(addr);
                    break;
                case 3: // attribute table(palette information)
                    addr = ((0x23C0 | (V & 0x0C00) | ((V >> 4) & 0x38) | ((V >> 2) & 0x07)));
                    latch[1] = read(addr);
                    aOffset = (((V >> 5) & 0x02) | ((V >> 1) & 0x01)); // position in an attribute block
                case 5: // pattern table low byte
                    addr = ((PPUCTRL.B << 12) | (latch[0] << 4) | ((V >> 12) & 0x07));
                    latch[2] = read(addr);
                    break;
                case 7: // pattern table high byte
                    addr += 8;
                    latch[3] = read(addr);
                    break;
                default:
                    break;
                }
                if (stage == 7)
                {
                    bgShiftReg16[0] >>= 8;
                    bgShiftReg16[1] >>= 8;
                    bgShiftReg16[0] |= (static_cast<uint16_t>(latch[2]) << 8);
                    bgShiftReg16[1] |= (static_cast<uint16_t>(latch[3]) << 8);
                    bgPalette[0] = bgPalette[1];
                    bgPalette[1] = ((latch[1] >> (aOffset << 1)) & 0x03);
                }
            }
        }
        else if (renderCycle <= 320)
        {

        }
        else if (renderCycle <= 336)
        {
            if (renderFlag)
            {
                int stage = (renderCycle - 1) % 8;
                switch (stage)
                {
                case 1: // nametable entry
                    addr = (0x2000 | (V & 0x0FFF));
                    latch[0] = read(addr);
                    break;
                case 3: // attribute table(palette information)
                    addr = ((0x23C0 | (V & 0x0C00) | ((V >> 4) & 0x38) | ((V >> 2) & 0x07)));
                    latch[1] = read(addr);
                    aOffset = (((V >> 5) & 0x02) | ((V >> 1) & 0x01)); // position in an attribute block
                    break;
                case 5: // pattern table low byte
                    addr = ((PPUCTRL.B << 12) | (latch[0] << 4) | ((V >> 12) & 0x07));
                    latch[2] = read(addr);
                    break;
                case 7: // pattern table high byte
                    addr += 8;
                    latch[3] = read(addr);
                    break;
                default:
                    break;
                }
                if (stage == 7)
                {
                    bgShiftReg16[0] >>= 8;
                    bgShiftReg16[1] >>= 8;
                    bgShiftReg16[0] |= (static_cast<uint16_t>(latch[2]) << 8);
                    bgShiftReg16[1] |= (static_cast<uint16_t>(latch[3]) << 8);
                    bgPalette[0] = bgPalette[1];
                    bgPalette[1] = ((latch[1] >> (aOffset << 1)) & 0x03);
                }
            }
        }
        else
        {
            // two garbage nametable read
        }
        // Scrolling
        if (renderFlag && scanline == 261 && renderCycle >= 280 && renderCycle <= 304)
        {
            // v: GHIA.BC DEF..... <- t: GHIA.BC DEF.....
            V = ((T & 0x7BE0) | (V & (~0x7BE0)));
        }
        if (renderFlag && (renderCycle >= 328 || (renderCycle > 0 && renderCycle <= 256)) && (renderCycle & 0x07) == 0)
        {
            coarseXInc();
        }
        if (renderFlag && renderCycle == 256)
        {
            fineYInc();
        }
        if (renderFlag && renderCycle == 257)
        {
            // v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
            V = ((T & 0x041F) | (V & (~0x041F)));
        }
        if (scanline == 261 && renderCycle == 1)
        {
            PPUSTATUS.O = 0;
            PPUSTATUS.S = 0;
            PPUSTATUS.V = 0;
        }
        if (renderCycle == 256)
            X0 = X;
    }
    else if (scanline == 240) // post-render scanline
    {
        // idle
    }
    else if (scanline == 241) // vertical blanking lines (241-260)
    {
        if (renderCycle == 1)
        {
            PPUSTATUS.V = 1;
            if (PPUCTRL.V)
                bus->nmi();
        }
    }
    else
    {

    }
}

// https://www.nesdev.org/wiki/PPU_sprite_evaluation
void RICOH2C02::spriteEval() // cycle chart is not accurate
{
    // just a state machine
    static int n;
    static int p; // for copy
    static int stage;
    bool renderFlag = (PPUMASK.s || PPUMASK.b);
    if (scanline > 239 || !renderFlag) // sprite evaluation takes place during all visible scanlines
        return;
    if (totalCycles < 0 || renderCycle < 0)
    {

    }
    else if (renderCycle == 0)
    {
        n = p = 0;
        spriteCounter = 0;
        spr0Ind = -1;
        stage = 1;
    }
    else if (renderCycle <= 64)
    {
        if (!(renderCycle & 0x01))
            *((uint8_t*)OAMnext + (renderCycle >> 1) - 1) = 0xFF;
    }
    else if (renderCycle <= 256)
    {
        switch (stage)
        {
        case 1:
            OAMnext[spriteCounter][0] = OAM[n][0];
            if (OAM[n][0] <= scanline + 1 && OAM[n][0] + (PPUCTRL.H ? 15 : 7) >= scanline + 1)
            {
                OAMnext[spriteCounter][1] = OAM[n][1];
                OAMnext[spriteCounter][2] = OAM[n][2];
                OAMnext[spriteCounter][3] = OAM[n][3];
                spr0Ind = (n == 0 ? spriteCounter : spr0Ind);
                spriteCounter++;
            }
            stage = 2;
            break;
        case 2:
            n++;
            if (n >= 64)
                stage = 4;
            else if (spriteCounter < 8)
                stage = 1;
            else
                stage = 3;
            break;
        case 3:
            if (OAM[n][0] <= scanline + 1 && OAM[n][0] + (PPUCTRL.H ? 15 : 7) >= scanline + 1)
                PPUSTATUS.O = 1;
            n++;
            if (n >= 64)
                stage = 4;
            break;
        case 4:
            // fail copy operation
            n++;
            break;
        default:
            break;
        }
    }
    else if (renderCycle <= 320)
    {
        if (p == (spriteCounter << 2))
        {
            *((uint8_t*)OAMcur + p) = OAM[63][0];
            p++;
        }
        else if ((renderCycle - 1) % 8 <= 3)
        {
            *((uint8_t*)OAMcur + p) = *((uint8_t*)OAMnext + p);
            p++;
        }
        curSpriteCounter = spriteCounter;
        curSpr0Ind = spr0Ind;
        OAMADDR = 0;
    }
}

uint8_t RICOH2C02::read(uint16_t addr)
{
    if (addr >= 0x3F00 && addr <= 0x3FFF)
        return readPalette(addr);
    else
        return bus->ppuRead(addr);
}

void RICOH2C02::write(uint16_t addr, uint8_t value)
{
    if (addr >= 0x3F00 && addr <= 0x3FFF)
        writePalette(addr, value);
    else
        bus->ppuWrite(addr, value);
}

/*
43210
|||||
|||++- Pixel value from tile data
|++--- Palette number from attribute table or OAM
+----- Background/Sprite select
*/
uint8_t RICOH2C02::readPalette(uint16_t addr)
{
    addr = (addr & 0x1F);
    uint8_t pixelInd = (addr & 0x03);
    if (!pixelInd)
        return palette[addr & 0xEF];
    else
        return palette[addr];
}

void RICOH2C02::writePalette(uint16_t addr, uint8_t value)
{
    addr = (addr & 0x1F);
    uint8_t pixelInd = (addr & 0x03);
    if (!pixelInd)
        palette[addr & 0xEF] = value;
    else
        palette[addr] = value;
}

std::tuple<uint8_t, uint8_t, uint8_t> RICOH2C02::evalColour(uint8_t colorInd)
{
    return std::make_tuple(colourTable[colorInd][0], colourTable[colorInd][1], colourTable[colorInd][2]);
}

void RICOH2C02::evalNametable(int id)
{
    for (uint16_t i = 0x0000; i < 0x03C0; i++)
    {
        uint16_t addr = ((id << 10) | i);
        uint8_t entry = read(0x2000 | addr);
        uint16_t startAddr = ((static_cast<uint16_t>(PPUCTRL.B) << 12) | (static_cast<uint16_t>(entry) << 4));
        for (int row = 0; row < 8; row++)
        {
            uint8_t patternLow = read(startAddr + row);
            uint8_t patternHigh = read(startAddr + row + 8);
            int sy = (i >> 5);
            int sx = (i & 0x1F);
            for (int col = 0; col < 8; col++)
            {
                int ind = ((sy << 11) + (sx << 3)) + ((row << 8) + 7 - col);
                std::tie(tileBuffer[id][ind][0], tileBuffer[id][ind][1], tileBuffer[id][ind][2]) =
                    evalColour(readPalette(((patternHigh & 0x01) << 1) | (patternLow & 0x01)));
                patternHigh >>= 1;
                patternLow >>= 1;
            }
        }
    }
}

