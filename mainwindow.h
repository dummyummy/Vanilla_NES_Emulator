#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <map>
#include <QKeyEvent>
#include "mos6502.h"
#include "ricoh2c02.h"
#include "bus.h"
#include "controller.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    explicit MainWindow(QWidget *parent, MOS6502 *cpu, RICOH2C02 *ppu, Bus *bus, Controller *joypad1, Controller *joypad2);
    ~MainWindow();

private:
    MOS6502 *cpu;
    RICOH2C02 *ppu;
    Bus *bus;
    Controller *joypad1;
    Controller *joypad2;
    std::map<Qt::Key, bool> keyStatus;
    bool keyPress(int key);
    Ui::MainWindow *ui;
    bool run;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void reset();
};

#endif // MAINWINDOW_H
