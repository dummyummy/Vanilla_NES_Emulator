#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <thread>
#include <chrono>


MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow)
{
    run = true;
    ui->setupUi(this);
}

MainWindow::MainWindow(QWidget *parent, MOS6502 *cpu, RICOH2C02 *ppu, Bus *bus, Controller *joypad1, Controller *joypad2) :
    QWidget(parent),
    ui(new Ui::MainWindow)

{
    ui->setupUi(this);
    ui->buttonReset->setFocusPolicy(Qt::NoFocus);
    connect(ui->buttonReset, &QPushButton::clicked, this, &MainWindow::reset);
    run = true;
    this->cpu = cpu;
    this->ppu = ppu;
    this->bus = bus;
    this->joypad1 = joypad1;
    this->joypad2 = joypad2;
    if (this->joypad1)
    {
        this->joypad1->setInputCallback([&](int key) -> bool
        {
            return this->keyPress(key);
        });
    }
    if (this->joypad2)
    {
        this->joypad2->setInputCallback([&](int key) -> bool
        {
            return this->keyPress(key);
        });
    }
    using namespace std::chrono;
    // two threads
    // one for ticking, one for render
    auto tick = std::thread([&]()
    {
        constexpr auto clockDelay = 559ns;
        while (run)
        {
            this->cpu->clock();
            if (joypad1)
                this->joypad1->poll();
            if (joypad2)
                this->joypad2->poll();
            this->ppu->clock3();
            std::this_thread::sleep_for(clockDelay);
        }
    });
    auto render = std::thread([&]()
    {
        constexpr auto renderDelay = 6ms;
        const int scale = 2;
        while (run)
        {
            QImage image(this->ppu->rendered(), 256, 240, QImage::Format_RGB888);
            image = image.scaled(256 * scale, 240 * scale);
            QPixmap pixmap = QPixmap::fromImage(image);
            ui->labelOutput->setPixmap(pixmap);
            std::this_thread::sleep_for(renderDelay);
        }
    });
    tick.detach();
    render.detach();
}

MainWindow::~MainWindow()
{
    run = false;
    delete ui;
}

bool MainWindow::keyPress(int key)
{
    return keyStatus[(Qt::Key)key];
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    keyStatus[(Qt::Key)event->key()] = true;
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    keyStatus[(Qt::Key)event->key()] = false;
}

void MainWindow::reset()
{
    cpu->reset();
    ppu->reset();
}
