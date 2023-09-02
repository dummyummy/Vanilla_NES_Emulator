#ifndef DEBUGGERWINDOW_H
#define DEBUGGERWINDOW_H

#include <QWidget>
#include "mos6502.h"
#include "ricoh2c02.h"
#include <thread>
#include <QAbstractButton>
#include <QButtonGroup>

namespace Ui {
class DebuggerWindow;
}

class Worker;
class DebuggerWindow;

class Worker : public QObject
{
    Q_OBJECT
private:
    DebuggerWindow *parent;
public:
    Worker() = delete;
    Worker(DebuggerWindow *parent);
signals:
    // Declare a signal to emit in the child thread to notify the parent thread for updating the component
    void updateComponentSignal();

public slots:
    void doWork();
};

class DebuggerWindow : public QWidget
{
    Q_OBJECT

public:
    QWidget *parent;
    explicit DebuggerWindow(QWidget *parent = nullptr);
    explicit DebuggerWindow(QWidget *parent, Bus *bus, MOS6502 *cpu, RICOH2C02 *ppu);
    ~DebuggerWindow();

public:
    Bus *bus;
    MOS6502 *cpu;
    RICOH2C02 *ppu;
private:
    QButtonGroup *buttonGroup;
    int curNametable;

private:
    Ui::DebuggerWindow *ui;
    Worker *worker;

private slots:
    void changeNametable(QAbstractButton *button);

private:
    std::thread *thread_ptr;

public:
    std::atomic_bool running;

public slots:
    void step();
    void both();
    void clock();
    void clockN();
    void clockUntil();
    void run();
    void stop();
    void reset();
    void updateStat();
};

#endif // DEBUGGERWINDOW_H
