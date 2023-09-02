#include "mos6502.h"

inline static uint16_t addr(uint8_t h8, uint8_t l8)
{
    return (static_cast<uint16_t>(h8) << 8) | l8;
}

inline static uint8_t sign(uint8_t x)
{
    return x >> 7;
}

inline static uint16_t s8tos16(uint8_t x)
{
    return sign(x) ? (0xFF00 | static_cast<uint16_t>(x)) : static_cast<uint16_t>(x);
}

inline static uint16_t u8tou16(uint8_t x)
{
    return static_cast<uint16_t>(x);
}

MOS6502::MOS6502()
{
    // power-up state
    // https://www.pagetable.com/?p=410
    PC = 0x0000;
    A = 0x00;
    X = Y = 0x00;
    SP = 0xFD;
    FLAG = 0x24; // nv‑bdIzc
    addr_abs = 0x0000;
    fetched = 0x00;
    temp = 0x00;
    cycle = 0;
    total_cycles = 0;

    // instruction set
    // table taken from OneLoneCoder
    // I add the accumulator addressing mode
    // suppose XXX has total cycles of 2
    using a = MOS6502;
    this->lookup = {
        { "BRK", &a::BRK, &a::IMM, 7 },{ "ORA", &a::ORA, &a::IZX, 6 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "ORA", &a::ORA, &a::ZP0, 3 },{ "ASL", &a::ASL, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "PHP", &a::PHP, &a::IMP, 3 },{ "ORA", &a::ORA, &a::IMM, 2 },{ "ASL", &a::ASL, &a::ACC, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ABS, 4 },{ "ASL", &a::ASL, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BPL", &a::BPL, &a::REL, 2 },{ "ORA", &a::ORA, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ZPX, 4 },{ "ASL", &a::ASL, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "CLC", &a::CLC, &a::IMP, 2 },{ "ORA", &a::ORA, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "ORA", &a::ORA, &a::ABX, 4 },{ "ASL", &a::ASL, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
        { "JSR", &a::JSR, &a::ABS, 6 },{ "AND", &a::AND, &a::IZX, 6 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "BIT", &a::BIT, &a::ZP0, 3 },{ "AND", &a::AND, &a::ZP0, 3 },{ "ROL", &a::ROL, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "PLP", &a::PLP, &a::IMP, 4 },{ "AND", &a::AND, &a::IMM, 2 },{ "ROL", &a::ROL, &a::ACC, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "BIT", &a::BIT, &a::ABS, 4 },{ "AND", &a::AND, &a::ABS, 4 },{ "ROL", &a::ROL, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BMI", &a::BMI, &a::REL, 2 },{ "AND", &a::AND, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "AND", &a::AND, &a::ZPX, 4 },{ "ROL", &a::ROL, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "SEC", &a::SEC, &a::IMP, 2 },{ "AND", &a::AND, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "AND", &a::AND, &a::ABX, 4 },{ "ROL", &a::ROL, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
        { "RTI", &a::RTI, &a::IMP, 6 },{ "EOR", &a::EOR, &a::IZX, 6 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "EOR", &a::EOR, &a::ZP0, 3 },{ "LSR", &a::LSR, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "PHA", &a::PHA, &a::IMP, 3 },{ "EOR", &a::EOR, &a::IMM, 2 },{ "LSR", &a::LSR, &a::ACC, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "JMP", &a::JMP, &a::ABS, 3 },{ "EOR", &a::EOR, &a::ABS, 4 },{ "LSR", &a::LSR, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BVC", &a::BVC, &a::REL, 2 },{ "EOR", &a::EOR, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "EOR", &a::EOR, &a::ZPX, 4 },{ "LSR", &a::LSR, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "CLI", &a::CLI, &a::IMP, 2 },{ "EOR", &a::EOR, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "EOR", &a::EOR, &a::ABX, 4 },{ "LSR", &a::LSR, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
        { "RTS", &a::RTS, &a::IMP, 6 },{ "ADC", &a::ADC, &a::IZX, 6 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 3 },{ "ADC", &a::ADC, &a::ZP0, 3 },{ "ROR", &a::ROR, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "PLA", &a::PLA, &a::IMP, 4 },{ "ADC", &a::ADC, &a::IMM, 2 },{ "ROR", &a::ROR, &a::ACC, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "JMP", &a::JMP, &a::IND, 5 },{ "ADC", &a::ADC, &a::ABS, 4 },{ "ROR", &a::ROR, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BVS", &a::BVS, &a::REL, 2 },{ "ADC", &a::ADC, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "ADC", &a::ADC, &a::ZPX, 4 },{ "ROR", &a::ROR, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "SEI", &a::SEI, &a::IMP, 2 },{ "ADC", &a::ADC, &a::ABY, 4 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "ADC", &a::ADC, &a::ABX, 4 },{ "ROR", &a::ROR, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
        { "???", &a::NOP, &a::IMP, 2 },{ "STA", &a::STA, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 6 },{ "STY", &a::STY, &a::ZP0, 3 },{ "STA", &a::STA, &a::ZP0, 3 },{ "STX", &a::STX, &a::ZP0, 3 },{ "???", &a::XXX, &a::IMP, 3 },{ "DEY", &a::DEY, &a::IMP, 2 },{ "???", &a::NOP, &a::IMP, 2 },{ "TXA", &a::TXA, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "STY", &a::STY, &a::ABS, 4 },{ "STA", &a::STA, &a::ABS, 4 },{ "STX", &a::STX, &a::ABS, 4 },{ "???", &a::XXX, &a::IMP, 4 },
        { "BCC", &a::BCC, &a::REL, 2 },{ "STA", &a::STA, &a::IZY, 6 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 6 },{ "STY", &a::STY, &a::ZPX, 4 },{ "STA", &a::STA, &a::ZPX, 4 },{ "STX", &a::STX, &a::ZPY, 4 },{ "???", &a::XXX, &a::IMP, 4 },{ "TYA", &a::TYA, &a::IMP, 2 },{ "STA", &a::STA, &a::ABY, 5 },{ "TXS", &a::TXS, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 5 },{ "???", &a::NOP, &a::IMP, 5 },{ "STA", &a::STA, &a::ABX, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "???", &a::XXX, &a::IMP, 5 },
        { "LDY", &a::LDY, &a::IMM, 2 },{ "LDA", &a::LDA, &a::IZX, 6 },{ "LDX", &a::LDX, &a::IMM, 2 },{ "???", &a::XXX, &a::IMP, 6 },{ "LDY", &a::LDY, &a::ZP0, 3 },{ "LDA", &a::LDA, &a::ZP0, 3 },{ "LDX", &a::LDX, &a::ZP0, 3 },{ "???", &a::XXX, &a::IMP, 3 },{ "TAY", &a::TAY, &a::IMP, 2 },{ "LDA", &a::LDA, &a::IMM, 2 },{ "TAX", &a::TAX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "LDY", &a::LDY, &a::ABS, 4 },{ "LDA", &a::LDA, &a::ABS, 4 },{ "LDX", &a::LDX, &a::ABS, 4 },{ "???", &a::XXX, &a::IMP, 4 },
        { "BCS", &a::BCS, &a::REL, 2 },{ "LDA", &a::LDA, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 5 },{ "LDY", &a::LDY, &a::ZPX, 4 },{ "LDA", &a::LDA, &a::ZPX, 4 },{ "LDX", &a::LDX, &a::ZPY, 4 },{ "???", &a::XXX, &a::IMP, 4 },{ "CLV", &a::CLV, &a::IMP, 2 },{ "LDA", &a::LDA, &a::ABY, 4 },{ "TSX", &a::TSX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 4 },{ "LDY", &a::LDY, &a::ABX, 4 },{ "LDA", &a::LDA, &a::ABX, 4 },{ "LDX", &a::LDX, &a::ABY, 4 },{ "???", &a::XXX, &a::IMP, 4 },
        { "CPY", &a::CPY, &a::IMM, 2 },{ "CMP", &a::CMP, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "CPY", &a::CPY, &a::ZP0, 3 },{ "CMP", &a::CMP, &a::ZP0, 3 },{ "DEC", &a::DEC, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "INY", &a::INY, &a::IMP, 2 },{ "CMP", &a::CMP, &a::IMM, 2 },{ "DEX", &a::DEX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 2 },{ "CPY", &a::CPY, &a::ABS, 4 },{ "CMP", &a::CMP, &a::ABS, 4 },{ "DEC", &a::DEC, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BNE", &a::BNE, &a::REL, 2 },{ "CMP", &a::CMP, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "CMP", &a::CMP, &a::ZPX, 4 },{ "DEC", &a::DEC, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "CLD", &a::CLD, &a::IMP, 2 },{ "CMP", &a::CMP, &a::ABY, 4 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "CMP", &a::CMP, &a::ABX, 4 },{ "DEC", &a::DEC, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
        { "CPX", &a::CPX, &a::IMM, 2 },{ "SBC", &a::SBC, &a::IZX, 6 },{ "???", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "CPX", &a::CPX, &a::ZP0, 3 },{ "SBC", &a::SBC, &a::ZP0, 3 },{ "INC", &a::INC, &a::ZP0, 5 },{ "???", &a::XXX, &a::IMP, 5 },{ "INX", &a::INX, &a::IMP, 2 },{ "SBC", &a::SBC, &a::IMM, 2 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::SBC, &a::IMP, 2 },{ "CPX", &a::CPX, &a::ABS, 4 },{ "SBC", &a::SBC, &a::ABS, 4 },{ "INC", &a::INC, &a::ABS, 6 },{ "???", &a::XXX, &a::IMP, 6 },
        { "BEQ", &a::BEQ, &a::REL, 2 },{ "SBC", &a::SBC, &a::IZY, 5 },{ "???", &a::XXX, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 8 },{ "???", &a::NOP, &a::IMP, 4 },{ "SBC", &a::SBC, &a::ZPX, 4 },{ "INC", &a::INC, &a::ZPX, 6 },{ "???", &a::XXX, &a::IMP, 6 },{ "SED", &a::SED, &a::IMP, 2 },{ "SBC", &a::SBC, &a::ABY, 4 },{ "NOP", &a::NOP, &a::IMP, 2 },{ "???", &a::XXX, &a::IMP, 7 },{ "???", &a::NOP, &a::IMP, 4 },{ "SBC", &a::SBC, &a::ABX, 4 },{ "INC", &a::INC, &a::ABX, 7 },{ "???", &a::XXX, &a::IMP, 7 },
    };
}

MOS6502::~MOS6502()
{

}


uint8_t MOS6502::read(uint16_t addr, bool readOnly) // read from bus
{
    return bus->cpuRead(addr, readOnly);
}

void MOS6502::write(uint16_t addr, uint8_t value) // write from bus
{
    bus->cpuWrite(addr, value);
}

void MOS6502::push(uint8_t value) // push value onto stack
{
    write(0x0100 + SP, value);
    SP--;
}

uint8_t MOS6502::pop() // pop one byte from stack
{
    SP++;
    return read(0x0100 + SP, false);
}

void MOS6502::OAMDMA(uint8_t addr, RICOH2C02 *ppu)
{
    cycle = 513 + (total_cycles & 1);
    uint16_t startAddr = (u8tou16(addr) << 8);
    for (int i = 0; i < 256; i++)
        ppu->setOAM(i >> 2, i & 0x03, read(startAddr + i, false));
}

uint8_t MOS6502::fetch(bool readOnly)
{
    if (this->lookup[IR].mode != &MOS6502::ACC && this->lookup[IR].mode != &MOS6502::IMP)
        fetched = read(addr_abs, readOnly);
    else if (this->lookup[IR].mode != &MOS6502::ACC)
        fetched = 0;
    return fetched;
}

void MOS6502::connectBus(Bus *bus)
{
    this->bus = bus;
}

void MOS6502::irq()
{
    if (!I)
    {
        push((PC & 0xFF00) >> 8);
        push(PC & 0x00FF);
        push(FLAG);
        B = 0;
        I = 1;
        addr_abs = 0xFFFE;
        PC = addr(read(addr_abs + 1, false), read(addr_abs, false));
        cycle += 7;
    }
}

void MOS6502::nmi()
{
    push((PC & 0xFF00) >> 8);
    push(PC & 0x00FF);
    push(FLAG);
    B = 0;
    I = 1;
    addr_abs = 0xFFFA;
    PC = addr(read(addr_abs + 1, false), read(addr_abs, false));
    cycle += 7;
}

void MOS6502::reset()
{
    // after-reset state
    // https://www.pagetable.com/?p=410
    // does not change Accumulator and Index
    // A = X = Y = 0;
    addr_abs = 0xFFFC;
    PC = addr(read(addr_abs + 1, false), read(addr_abs, false));
    SP = 0xFD;
    FLAG = 0x24; // nv‑bdIzc
    addr_abs = 0x0000;
    fetched = 0x00;
    temp = 0x00;
    cycle = 7; // it takes 7 cycles to fetch the first actual instruction
    total_cycles = 0;
}

bool MOS6502::complete()
{
    if (cycle == 0) // for debug
        IR = read(PC, false);
    return cycle == 0;
}

void MOS6502::clock()
{
    IR = read(PC, true);
    if (cycle == 0) // execution finished
    {
        IR = read(PC++, false);

        cycle = this->lookup[IR].cycle;
        uint8_t extra1 = (this->*lookup[IR].mode)();
        if (lookup[IR].name == "STA" || lookup[IR].name == "STX" || lookup[IR].name == "STY")
            fetch(true);
        else
            fetch(false);
        uint8_t extra2 = (this->*lookup[IR].operation)();
        if (extra1 && extra2)
            cycle++;
    }
    cycle--;
    total_cycles++;
}

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
uint8_t MOS6502::ACC()
{
    fetched = A;
    return 0;
}

uint8_t MOS6502::ABS()
{
    addr_abs = addr(read(PC + 1, false), read(PC, false));
    PC += 2;
    return 0;
}

uint8_t MOS6502::ABX()
{
    uint8_t h8 = read(PC + 1, false);
    uint8_t l8 = read(PC, false);
    PC += 2;
    addr_abs = addr(h8, l8) + X;
    return (addr_abs >> 8) != h8;
}

uint8_t MOS6502::ABY()
{
    uint8_t h8 = read(PC + 1, false);
    uint8_t l8 = read(PC, false);
    PC += 2;
    addr_abs = addr(h8, l8) + Y;
    return (addr_abs >> 8) != h8;
}

uint8_t MOS6502::IMM()
{
    addr_abs = PC++;
    return 0;
}

uint8_t MOS6502::IMP()
{
    return 0;
}

// this explaination is from OneLoneCoder
// The supplied 16-bit address is read to get the actual 16-bit address. This is
// instruction is unusual in that it has a bug in the hardware! To emulate its
// function accurately, we also need to emulate this bug. If the low byte of the
// supplied address is 0xFF, then to read the high byte of the actual address
// we need to cross a page boundary. This doesnt actually work on the chip as
// designed, instead it wraps back around in the same page, yielding an
// invalid actual address
uint8_t MOS6502::IND()
{
    uint8_t h8 = read(PC + 1, false);
    uint8_t l8 = read(PC, false);
    PC += 2;
    uint8_t h80 = h8;
    if (l8 == 0xFF)
    {
        h8 = read(addr(h80, 0x00), false);
        l8 = read(addr(h80, 0xFF), false);
    }
    else
    {
        h8 = read(addr(h80, l8 + 1), false);
        l8 = read(addr(h80, l8), false);
    }
    addr_abs = addr(h8, l8);
    return 0;
}

uint8_t MOS6502::IZX()
{
    uint8_t l8 = read(PC++, false);
    addr_abs = addr(read((l8 + X + 1) & 0x00FF, false), read((l8 + X) & 0x00FF, false));
    return 0;
}

uint8_t MOS6502::IZY()
{
    uint8_t l8 = read(PC++, false);
    uint8_t h8 = read((l8 + 1) & 0x00FF, false);
    l8 = read(l8, false);
    addr_abs = addr(h8, l8) + Y;
    return (addr_abs >> 8) != h8;
}

uint8_t MOS6502::REL()
{
    uint16_t addr_rel = read(PC++, false);
    if (addr_rel & 0x80)
        addr_rel |= 0xFF00;
    addr_abs = PC + addr_rel;
    return (addr_abs >> 8) != (PC >> 8);
}

uint8_t MOS6502::ZP0()
{
    addr_abs = read(PC++, false);
    return 0;
}

uint8_t MOS6502::ZPX()
{
    addr_abs = ((read(PC++, false) + X) & 0x00FF);
    return 0;
}

uint8_t MOS6502::ZPY()
{
    addr_abs = ((read(PC++, false) + Y) & 0x00FF);
    return 0;
}

// opcodes
uint8_t MOS6502::ADC()
{
    uint16_t A1 = u8tou16(A) + u8tou16(fetched) + u8tou16(C);
    C = A1 > 0xFF;
    A1 &= 0xFF;
    Z = A1 == 0;
    N = sign(A1);
    V = ((!(sign(A) ^ sign(fetched))) & (sign(A) ^ N));
    A = A1;
    return 1;
}

uint8_t MOS6502::AND()
{
    A = (A & fetched);
    Z = A == 0;
    N = sign(A);
    return 1;
}

uint8_t MOS6502::ASL()
{
    temp = fetched;
    C = sign(temp);
    temp <<= 1;
    Z = temp == 0;
    N = sign(temp);
    if (lookup[IR].mode == &MOS6502::ACC)
        A = temp;
    else
        write(addr_abs, temp);
    return 0;
}

uint8_t MOS6502::BCC()
{
    if (!C)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BCS()
{
    if (C)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BEQ()
{
    if (Z)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BIT()
{
    temp = (A & fetched);
    Z = temp == 0;
    V = ((fetched >> 6) & 0x01);
    N = ((fetched >> 7) & 0x01);
    return 0;
}

uint8_t MOS6502::BMI()
{
    if (N)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BNE()
{
    if (!Z)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BPL()
{
    if (!N)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BRK()
{
    push((PC & 0xFF00) >> 8);
    push(PC & 0x00FF);
    push(FLAG | 0x10);
    B = 1;
    return 0;
}

uint8_t MOS6502::BVC()
{
    if (!V)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::BVS()
{
    if (V)
    {
        PC = addr_abs;
        cycle++;
        return 1;
    }
    return 0;
}

uint8_t MOS6502::CLC()
{
    C = 0;
    return 0;
}

uint8_t MOS6502::CLD()
{
    D = 0;
    return 0;
}

uint8_t MOS6502::CLI()
{
    I = 0;
    return 0;
}

uint8_t MOS6502::CLV()
{
    V = 0;
    return 0;
}

uint8_t MOS6502::CMP()
{
    temp = A - fetched;
    C = A >= fetched;
    Z = temp == 0;
    N = sign(temp);
    return 1;
}

uint8_t MOS6502::CPX()
{

    temp = X - fetched;
    C = X >= fetched;
    Z = temp == 0;
    N = sign(temp);
    return 0;
}

uint8_t MOS6502::CPY()
{
    temp = Y - fetched;
    C = Y >= fetched;
    Z = temp == 0;
    N = sign(temp);
    return 0;
}

uint8_t MOS6502::DEC()
{
    temp = fetched - 1;
    Z = temp == 0;
    N = sign(temp);
    write(addr_abs, temp);
    return 0;
}

uint8_t MOS6502::DEX()
{
    X--;
    Z = X == 0;
    N = sign(X);
    return 0;
}

uint8_t MOS6502::DEY()
{
    Y--;
    Z = Y == 0;
    N = sign(Y);
    return 0;
}

uint8_t MOS6502::EOR()
{
    A = (A ^ fetched);
    Z = A == 0;
    N = sign(A);
    return 1;
}

uint8_t MOS6502::INC()
{
    temp = fetched + 1;
    Z = temp == 0;
    N = sign(temp);
    write(addr_abs, temp);
    return 0;
}

uint8_t MOS6502::INX()
{
    X++;
    Z = X == 0;
    N = sign(X);
    return 0;
}

uint8_t MOS6502::INY()
{
    Y++;
    Z = Y == 0;
    N = sign(Y);
    return 0;
}

uint8_t MOS6502::JMP()
{
    PC = addr_abs;
    return 0;
}

uint8_t MOS6502::JSR()
{
    PC--;
    push((PC & 0xFF00) >> 8);
    push(PC & 0x00FF);
    PC = addr_abs;
    return 0;
}

uint8_t MOS6502::LDA()
{
    A = fetched;
    Z = A == 0;
    N = sign(A);
    return 1;
}

uint8_t MOS6502::LDX()
{
    X = fetched;
    Z = X == 0;
    N = sign(X);
    return 1;
}

uint8_t MOS6502::LDY()
{
    Y = fetched;
    Z = Y == 0;
    N = sign(Y);
    return 1;
}

uint8_t MOS6502::LSR()
{
    temp = fetched;
    C = (temp & 0x01);
    temp >>= 1;
    Z = temp == 0;
    N = sign(temp);
    if (lookup[IR].mode == &MOS6502::ACC)
        A = temp;
    else
        write(addr_abs, temp);
    return 0;
}

uint8_t MOS6502::NOP()
{
    return 0;
}

uint8_t MOS6502::ORA()
{
    A = (A | fetched);
    Z = A == 0;
    N = sign(A);
    return 1;
}

uint8_t MOS6502::PHA()
{
    push(A);
    return 0;
}

uint8_t MOS6502::PHP()
{
    push(FLAG | 0x30);
    return 0;
}

uint8_t MOS6502::PLA()
{
    A = pop();
    Z = A == 0;
    N = sign(A);
    return 0;
}

uint8_t MOS6502::PLP()
{
    FLAG = ((pop() & 0xCF) | (FLAG & 0x30));
    return 0;
}

uint8_t MOS6502::ROL()
{
    temp = fetched;
    uint8_t C1 = sign(temp);
    temp = ((temp << 1) | C);
    C = C1;
    Z = temp == 0;
    N = sign(temp);
    if (lookup[IR].mode == &MOS6502::ACC)
        A = temp;
    else
        write(addr_abs, temp);
    return 0;
}

uint8_t MOS6502::ROR()
{
    temp = fetched;
    uint8_t C1 = temp & 0x01;
    temp = ((temp >> 1) | (C << 7));
    C = C1;
    Z = temp == 0;
    N = sign(temp);
    if (lookup[IR].mode == &MOS6502::ACC)
        A = temp;
    else
        write(addr_abs, temp);
    return 0;

}

uint8_t MOS6502::RTI()
{
    FLAG = ((pop() & 0xCF) | (FLAG & 0x30));
    uint8_t l8 = pop();
    uint8_t h8 = pop();
    PC = addr(h8, l8);
    return 0;
}

uint8_t MOS6502::RTS()
{
    uint8_t l8 = pop();
    uint8_t h8 = pop();
    PC = addr(h8, l8);
    PC++;
    return 0;
}

uint8_t MOS6502::SBC()
{
    // forum https://forums.nesdev.org/viewtopic.php?p=19080#p19080
    uint16_t A1 = A - fetched - !C;
    V = (A ^ A1) & (A ^ fetched) & 0x80;
    C = !(A1 >> 8);
    A = static_cast<uint8_t>(A1);
    N = sign(A);
    Z = A == 0;
    /* My implemetation
    uint8_t fetched1 = (~fetched) + static_cast<uint8_t>(1); // -M
    uint8_t notC = (~(!C)) + static_cast<uint8_t>(1); // -(1-C)
    uint16_t sub = s8tos16(fetched1) + s8tos16(notC);
    uint16_t A1 = s8tos16(A) + sub;
    C = ((A1 & 0xFF00) > 0) == sign(A);
    A1 &= 0xFF;
    Z = A1 == 0;
    N = sign(A1);
    V = ((!(sign(A) ^ sign(sub))) & (sign(A) ^ N));
    A = A1;
    */
    return 1;
}

uint8_t MOS6502::SEC()
{
    C = 1;
    return 0;
}

uint8_t MOS6502::SED()
{
    D = 1;
    return 0;
}

uint8_t MOS6502::SEI()
{
    I = 1;
    return 0;
}

uint8_t MOS6502::STA()
{
    write(addr_abs, A);
    return 0;
}

uint8_t MOS6502::STX()
{
    write(addr_abs, X);
    return 0;
}

uint8_t MOS6502::STY()
{
    write(addr_abs, Y);
    return 0;
}

uint8_t MOS6502::TAX()
{
    X = A;
    Z = X == 0;
    N = sign(X);
    return 0;
}

uint8_t MOS6502::TAY()
{
    Y = A;
    Z = Y == 0;
    N = sign(Y);
    return 0;
}

uint8_t MOS6502::TSX()
{
    X = SP;
    Z = X == 0;
    N = sign(X);
    return 0;
}

uint8_t MOS6502::TXA()
{
    A = X;
    Z = A == 0;
    N = sign(A);
    return 0;
}

uint8_t MOS6502::TXS()
{
    SP = X;
    return 0;
}

uint8_t MOS6502::TYA()
{
    A = Y;
    Z = A == 0;
    N = sign(A);
    return 0;
}

uint8_t MOS6502::XXX()
{
    return 0;
}
