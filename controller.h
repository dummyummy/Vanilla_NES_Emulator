#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <atomic>
#include <functional>
#include <map>
#include <mutex>

enum KEY_MAP
{
    KEY_A = 0,
    KEY_B = 1,
    KEY_SELECT = 2,
    KEY_START = 3,
    KEY_UP = 4,
    KEY_DOWN = 5,
    KEY_LEFT = 6,
    KEY_RIGHT = 7
};

class Controller
{
public:
    Controller();
    Controller(std::map<KEY_MAP, int> mapping, std::function<bool(int)> callback);
    void setMapping(std::map<KEY_MAP, int> mapping);
    void setInputCallback(std::function<bool(int)> callback); // set keyboard input
    void setStrobe(uint8_t value);
    uint8_t getInput(bool readOnly);
    void poll(); // start polling

private:
    uint8_t latch;
    std::atomic_bool reload;
    std::atomic_uint8_t input;
    std::map<KEY_MAP, int> mapping;
    std::function<bool(int)> callback;

};

#endif // CONTROLLER_H
