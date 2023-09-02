#include "debuggerwindow.h"
#include "./ui_debuggerwindow.h"

#include <QImage>
#include <QPixmap>
#include <iostream>
#include <string>
#include <QButtonGroup>
#include <cstring>

Worker::Worker(DebuggerWindow *parent) : parent(parent)
{

}

void Worker::doWork()
{
    // Emit the signal to notify the parent thread for updating the component
    int count = 0;
    while (parent->running)
    {
        parent->cpu->clock();
        emit updateComponentSignal();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return;
}

DebuggerWindow::DebuggerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DebuggerWindow)
{
    ui->setupUi(this);
    curNametable = 0;
}

DebuggerWindow::DebuggerWindow(QWidget *parent, Bus *bus, MOS6502 *cpu, RICOH2C02 *ppu)
    : parent(parent)
    , ui(new Ui::DebuggerWindow)
    , bus(bus)
    , cpu(cpu)
    , ppu(ppu)
{
    ui->setupUi(this);
    buttonGroup = new QButtonGroup();
    buttonGroup->addButton(ui->radioNametable0, 0);
    buttonGroup->addButton(ui->radioNametable1, 1);
    buttonGroup->addButton(ui->radioNametable2, 2);
    buttonGroup->addButton(ui->radioNametable3, 3);
    connect(ui->buttonSTEP, &QPushButton::clicked, this, &DebuggerWindow::step);
    connect(ui->buttonRESET, &QPushButton::clicked, this, &DebuggerWindow::reset);
    connect(ui->buttonCLOCKN, &QPushButton::clicked, this, &DebuggerWindow::clockN);
    connect(ui->buttonCLOCK, &QPushButton::clicked, this, &DebuggerWindow::clock);
    connect(ui->buttonCLOCKUNTIL, &QPushButton::clicked, this, &DebuggerWindow::clockUntil);
    connect(buttonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &DebuggerWindow::changeNametable);
    curNametable = 0;
    running = false;

    updateStat();
}

DebuggerWindow::~DebuggerWindow()
{
    delete ui;
}

void DebuggerWindow::changeNametable(QAbstractButton *button)
{
    curNametable = buttonGroup->id(button); // Get the ID of the clicked button
    QImage nametable((unsigned char*)(ppu->tileBuffer[curNametable]), 256, 240, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(nametable);
    ui->labelNametable->setPixmap(pixmap);
}

void DebuggerWindow::updateStat()
{
    static unsigned char paletteImage[32 * 3];
    // CPU
    ui->lineEditPC->setText(QString::number(cpu->PC, 16).toUpper());
    ui->lineEditA->setText(QString::number(cpu->A, 16).toUpper());
    ui->lineEditX->setText(QString::number(cpu->X, 16).toUpper());
    ui->lineEditY->setText(QString::number(cpu->Y, 16).toUpper());
    ui->lineEditSP->setText(QString::number(cpu->SP, 16).toUpper());
    QString flag = QString("%1").arg(cpu->FLAG, 8, 2, QChar('0'));
    ui->lineEditFLAG->setText(flag);

    // RAM / VRAM / OAM
    int bytesPerLine = 16;
    ui->displayRAM->clear();
    ui->displayVRAM->clear();
    ui->displayOAM->clear();
    for (int address = 0x0000; address < 0x2000; address += bytesPerLine)
    {
        QString ram;
        ram += QString("%1:").arg(address, 4, 16, QChar('0'));
        for (int i = 0; i < bytesPerLine; ++i)
            ram += QString("%1 ").arg(static_cast<quint8>(bus->cpuRead(address + i, true)), 2, 16, QChar('0'));
        ram += "\n";
        ui->displayRAM->insertPlainText(ram);
    }
    for (int address = 0x0000; address < 0x4000; address += bytesPerLine)
    {
        QString vram;
        vram += QString("%1:").arg(address, 4, 16, QChar('0'));
        for (int i = 0; i < bytesPerLine; ++i)
            vram += QString("%1 ").arg(static_cast<quint8>(bus->ppuRead(address + i)), 2, 16, QChar('0'));
        vram += "\n";
        ui->displayVRAM->insertPlainText(vram);
    }
    for (int address = 0x00; address < 0xFF; address += bytesPerLine)
    {
        QString oam;
        oam += QString("%1:").arg(address, 2, 16, QChar('0'));
        for (int i = 0; i < bytesPerLine; ++i)
        {
            oam += QString("%1 ").arg(static_cast<quint8>(ppu->getOAM((address + i) >> 2, (address + i) & 0x03)), 2, 16, QChar('0'));
        }
        oam += "\n";
        ui->displayOAM->insertPlainText(oam);
    }

    // Summary
    ui->displayStat->clear();
    QString stat;
    stat += QString("fetched: 0x%1 ").arg(cpu->fetched, 2, 16, QChar('0'));
    stat += QString("addr_abs: 0x%1 ").arg(cpu->addr_abs, 4, 16, QChar('0'));
    stat += QString("IR: 0x%1 ").arg(cpu->IR, 2, 16, QChar('0'));
    stat += QString("op_name: ") + QString(cpu->lookup[cpu->IR].name.c_str());
    stat += QString(" total_cycls: %1").arg(cpu->total_cycles);
    ui->displayStat->insertPlainText(stat);

    // PPU output
    QImage image(ppu->rendered(), 256, 240, QImage::Format_RGB888);
    QPixmap pixmap1 = QPixmap::fromImage(image);
    ui->labelDisplay->setPixmap(pixmap1);
    // Palette
    for (int i = 0; i <= 0x1F; i++)
    {
        auto [r, g, b] = ppu->evalColour(ppu->readPalette(i));
        paletteImage[i * 3] = r;
        paletteImage[i * 3 + 1] = g;
        paletteImage[i * 3 + 2] = b;
    }
    QImage palette(paletteImage, 16, 2, QImage::Format_RGB888);
    QPixmap pixmap2 = QPixmap::fromImage(palette);
    pixmap2 = pixmap2.scaled(320, 40);
    ui->labelPalette->setPixmap(pixmap2);
    // Nametable
    ppu->evalNametable(0);
    ppu->evalNametable(1);
    ppu->evalNametable(2);
    ppu->evalNametable(3);
    QImage nametable((unsigned char*)(ppu->tileBuffer[curNametable]), 256, 240, QImage::Format_RGB888);
    QPixmap pixmap3 = QPixmap::fromImage(nametable);
    ui->labelNametable->setPixmap(pixmap3);


    stat = QString("");
    stat += QString("scanline:    %1\n").arg(ppu->scanline);
    stat += QString("renderCycle: %1\n").arg(ppu->renderCycle);
    stat += QString("totalCycles: %1\n").arg(ppu->totalCycles);
    stat += QString("PPUCTRL:     %1\n").arg(*(uint8_t*)&ppu->PPUCTRL, 2, 16, QChar('0'));
    stat += QString("PPUMASK:     %1\n").arg(*(uint8_t*)&ppu->PPUMASK, 2, 16, QChar('0'));
    stat += QString("PPUSTATUS:   %1\n").arg(*(uint8_t*)&ppu->PPUSTATUS, 2, 16, QChar('0'));
    stat += QString("VRAM Addr:   %1\n").arg(ppu->V, 4, 16, QChar('0'));
    ui->labelScanline->setText(stat);
}

void DebuggerWindow::step()
{
    do
    {
        both();
    } while (!cpu->complete());

    updateStat();
}

void DebuggerWindow::both()
{
    uint8_t temp = bus->cpuRead(0x03D0, true);
    cpu->clock();
    if (temp != 0xf0 && bus->cpuRead(0x03D0, true) == 0xf0)
        std::cout << "fucking PC is " << std::hex << cpu->PC << std::endl;
    ppu->clock();
    ppu->clock();
    ppu->clock();
}

void DebuggerWindow::clock()
{
    both();

    updateStat();
}

void DebuggerWindow::clockN()
{
    int clocks = 0;
    clocks = ui->lineEditCLOCKN->text().toInt();
    if (clocks == 0)
        clocks = 1;
    qDebug() << "run " << clocks << "cpu cycles\n";
    for (int i = 0; i < clocks; i++)
    {
        both();
    }

    updateStat();
}

void DebuggerWindow::clockUntil()
{
    int targetPC = 0;
    targetPC = ui->lineEditCLOCKUNTIL->text().toInt(nullptr, 16);
    if (targetPC == 0)
        targetPC = cpu->PC + 1;
    both();
    while (!cpu->complete())
        both();
    while (cpu->PC != targetPC)
        both();
    both();
    while (!cpu->complete())
        both();

    updateStat();
}

void DebuggerWindow::run()
{
    running = true;
    worker = new Worker(this);
    thread_ptr = new std::thread(&Worker::doWork, worker);
    connect(worker, &Worker::updateComponentSignal, this, &DebuggerWindow::updateStat, Qt::BlockingQueuedConnection);

    ui->buttonSTEP->setEnabled(false);
    ui->buttonCLOCK->setEnabled(false);
    ui->buttonCLOCKUNTIL->setEnabled(true);
}

void DebuggerWindow::stop()
{
    running = false;
    thread_ptr->join();
    delete thread_ptr;
    disconnect(worker, &Worker::updateComponentSignal, this, &DebuggerWindow::updateStat);
    delete worker;

    ui->buttonSTEP->setEnabled(true);
    ui->buttonCLOCK->setEnabled(true);
    ui->buttonCLOCKUNTIL->setEnabled(false);
}

void DebuggerWindow::reset()
{
    cpu->reset();

    updateStat();
}
