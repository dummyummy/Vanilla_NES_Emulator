#include "controller.h"
#include "mainwindow.h"
#include "debuggerwindow.h"
#include "cartridge.h"
#include "ricoh2c02.h"
#include "mos6502.h"

#include <QApplication>
#include <QKeyEvent>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[])
{
    Cartridge *cartridge = new Cartridge();
    if (argc < 2)
    {
        std::cerr << "Must specify rom in the 2nd argument" << std::endl;
        exit(EXIT_FAILURE);
    }
    try
    {
        cartridge->load(argv[1]);
    }
    catch (std::string e)
    {
        std::cerr << e << std::endl;
    }
    MOS6502 *cpu = new MOS6502();
    RICOH2C02 *ppu = new RICOH2C02();
    Bus *bus = new Bus();
    std::map<KEY_MAP, int> mapping1, mapping2;
    mapping1[KEY_A] = Qt::Key_J;
    mapping1[KEY_B] = Qt::Key_K;
    mapping1[KEY_UP] = Qt::Key_W;
    mapping1[KEY_DOWN] = Qt::Key_S;
    mapping1[KEY_LEFT] = Qt::Key_A;
    mapping1[KEY_RIGHT] = Qt::Key_D;
    mapping1[KEY_SELECT] = Qt::Key_V;
    mapping1[KEY_START] = Qt::Key_B;
    mapping2[KEY_A] = Qt::Key_2;
    mapping2[KEY_B] = Qt::Key_3;
    mapping2[KEY_UP] = Qt::Key_Up;
    mapping2[KEY_DOWN] = Qt::Key_Down;
    mapping2[KEY_LEFT] = Qt::Key_Left;
    mapping2[KEY_RIGHT] = Qt::Key_Right;
    Controller *joypad1 = new Controller();
    Controller *joypad2 = new Controller();
    joypad1->setMapping(mapping1);
    joypad2->setMapping(mapping2);

    cpu->connectBus(bus);
    ppu->connectBus(bus);
    bus->connectAll(cpu, ppu, cartridge->mapper);
    bus->connectJoypad1(joypad1);
    bus->connectJoypad2(joypad2);

    cpu->reset();
    ppu->reset();

    QApplication a(argc, argv);
    MainWindow w(nullptr, cpu, ppu, bus, joypad1, joypad2);
    w.show();
//    DebuggerWindow debugger(nullptr, bus, cpu, ppu);
//    debugger.show();
    return a.exec();
}
