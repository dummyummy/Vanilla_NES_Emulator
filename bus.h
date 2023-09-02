#ifndef BUS_H
#define BUS_H

#include <vector>
#include <cstdint>
#include "controller.h"

class MOS6502;
class RICOH2C02;
class Mapper;

class Bus // the bus for cpu and ppu
{
private:
    MOS6502 *cpu;
    RICOH2C02 *ppu;
    Mapper *mapper;
    Controller *joypad1;
    Controller *joypad2;
    // assume that cartridge can only be accessed through mapper
    std::vector<uint8_t> RAM; // 2KB
    std::vector<uint8_t> CIRAM; // 2KB
private:
    std::vector<uint8_t> testRAM;
    uint8_t keyLatch1;
    uint8_t keyLatch2;
private:
    bool connectCPU(MOS6502 *cpu);
    bool connectPPU(RICOH2C02 *ppu);
    bool connectMapper(Mapper *mapper);
public:
    Bus();
    bool connectAll(MOS6502 *cpu, RICOH2C02 *ppu, Mapper *mapper);
    bool connectJoypad1(Controller *joypad);
    bool connectJoypad2(Controller *joypad);
    uint8_t cpuRead(uint16_t addr, bool readOnly);
    void cpuWrite(uint16_t addr, uint8_t value); // write a byte
    uint8_t ppuRead(uint16_t addr);
    void ppuWrite(uint16_t addr, uint8_t value); // write a byte
    void nmi();
    void irq();
};

#endif // BUS_H
