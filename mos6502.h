#ifndef MOS6502_H
#define MOS6502_H

#include "bus.h"
#include <cstdint>
#include <string.h>
#include <string>
#include "ricoh2c02.h"

class MOS6502
{
public:
    MOS6502();
    ~MOS6502();

private:
    Bus *bus;
    uint8_t temp; // to avoid changing fetched
    uint8_t fetch(bool readOnly); // fetch operand into fetched
    uint16_t cycle; // clock count left to complete current instruction
                    // when it hits 0, fetch a new instruction then execute

public:
    uint8_t fetched; // current operand
    uint16_t addr_abs; // for direct addressing mode
    unsigned long long total_cycles;

private:
    // addressing modes
    // 6502 has 13 addressing modes, including these 12
    // and addressing with accmulator(which is implied in the instruction)
    // refer to this table
    /*
    A      Accumulator          OPC A	     operand is AC (implied single byte instruction)
    abs    absolute	            OPC $LLHH	 operand is address $HHLL *
    abs,X  absolute, X-indexed  OPC $LLHH,X	 operand is address; effective address is address incremented by X with carry **
    abs,Y  absolute, Y-indexed  OPC $LLHH,Y	 operand is address; effective address is address incremented by Y with carry **
    #      immediate	        OPC #$BB	 operand is byte BB
    impl   implied	            OPC	         operand implied
    ind    indirect	            OPC ($LLHH)	 operand is address; effective address is contents of word at address: C.w($HHLL)
    X,ind  X-indexed, indirect  OPC ($LL,X)	 operand is zeropage address; effective address is word in (LL + X, LL + X + 1), inc. without carry: C.w($00LL + X)
    ind,Y  indirect, Y-indexed  OPC ($LL),Y	 operand is zeropage address; effective address is word in (LL, LL + 1) incremented by Y with carry: C.w($00LL) + Y
    rel    relative	            OPC $BB	     branch target is PC + signed offset BB ***
    zpg    zeropage             OPC $LL	     operand is zeropage address (hi-byte is zero, address = $00LL)
    zpg,X  zeropage, X-indexed  OPC $LL,X	 operand is zeropage address; effective address is address incremented by X without carry **
    zpg,Y  zeropage, Y-indexed  OPC $LL,Y	 operand is zeropage address; effective address is address incremented by Y without carry **
    */
    // the return value indicates whether page boundary is crossed
    uint8_t ACC();
    uint8_t ABS(); uint8_t ABX(); uint8_t ABY();
    uint8_t IMM(); uint8_t IMP(); uint8_t IND();
    uint8_t IZX(); uint8_t IZY(); uint8_t REL();
    uint8_t ZP0(); uint8_t ZPX(); uint8_t ZPY();

    // the return value helps to judge potential extra cycle
    // legal opcodes
    uint8_t ADC(); uint8_t AND(); uint8_t ASL(); uint8_t BCC(); uint8_t BCS(); uint8_t BEQ(); uint8_t BIT(); uint8_t BMI(); uint8_t BNE(); uint8_t BPL(); uint8_t BRK(); uint8_t BVC(); uint8_t BVS(); uint8_t CLC();
    uint8_t CLD(); uint8_t CLI(); uint8_t CLV(); uint8_t CMP(); uint8_t CPX(); uint8_t CPY(); uint8_t DEC(); uint8_t DEX(); uint8_t DEY(); uint8_t EOR(); uint8_t INC(); uint8_t INX(); uint8_t INY(); uint8_t JMP();
    uint8_t JSR(); uint8_t LDA(); uint8_t LDX(); uint8_t LDY(); uint8_t LSR(); uint8_t NOP(); uint8_t ORA(); uint8_t PHA(); uint8_t PHP(); uint8_t PLA(); uint8_t PLP(); uint8_t ROL(); uint8_t ROR(); uint8_t RTI();
    uint8_t RTS(); uint8_t SBC(); uint8_t SEC(); uint8_t SED(); uint8_t SEI(); uint8_t STA(); uint8_t STX(); uint8_t STY(); uint8_t TAX(); uint8_t TAY(); uint8_t TSX(); uint8_t TXA(); uint8_t TXS(); uint8_t TYA();

    // illegal opcode
    uint8_t XXX();

public:
    struct Instruction
    {
        std::string name;
        uint8_t (MOS6502::*operation)(void);
        uint8_t (MOS6502::*mode)(void);
        uint8_t cycle;
    };

    std::vector<Instruction> lookup;

public:
    void connectBus(Bus *bus);

private:
    uint8_t read(uint16_t addr, bool readOnly); // read from bus
    void write(uint16_t addr, uint8_t value); // write to bus
    void push(uint8_t value); // push value onto stack
    uint8_t pop(); // pop one byte from stack

public: // registers
    uint16_t PC; // program counter
    uint8_t IR; // opcode
    uint8_t A; // accumulator
    uint8_t X; // index X
    uint8_t Y; // index Y
    uint8_t SP; // stack pointer
    union // status register
    {
        struct // use bit field for simplicity
        {
            uint8_t C : 1; // carry
            uint8_t Z : 1; // zero
            uint8_t I : 1; // disable irq
            uint8_t D : 1; // unused
            uint8_t B : 1; // brk command
            uint8_t U : 1; // unused
            uint8_t V : 1; // overflow
            uint8_t N : 1; // negative
        };
        uint8_t FLAG;
    };

public: // execution and interrupts
    void OAMDMA(uint8_t addr, RICOH2C02 *ppu); // start transfer data to OAM in PPU
    void clock(); // let cpu run 1 clock cycle
    void irq(); // maskable interrupt
    void nmi(); // non maskable interrupt
    void reset(); // forced reset
    bool complete(); // check if current instruction complete
};

#endif // MOS6502_H
