#include "controller.h"

#include <chrono>
#include <thread>

Controller::Controller()
{
    reload = false;
    input = 0xFF;
}

Controller::Controller(std::map<KEY_MAP, int> mapping, std::function<bool (int)> callback)
    : mapping(mapping), callback(callback)
{
    reload = false;
    input = 0xFF;
}

void Controller::setMapping(std::map<KEY_MAP, int> mapping)
{
    this->mapping = mapping;
}

void Controller::setInputCallback(std::function<bool (int)> callback)
{
    this->callback = callback;
}

void Controller::setStrobe(uint8_t value)
{
    this->latch = value & 0x07;
    this->reload = value & 0x01;
    poll();
}

uint8_t Controller::getInput(bool readOnly)
{
    uint8_t keyStatus = input & 0x01;
    if (!readOnly)
        input = (input >> 1) | 0x80;
    return keyStatus;
}

void Controller::poll()
{
    if (reload)
    {
        uint8_t temp = 0x00;
        for (KEY_MAP i = KEY_A; i <= KEY_RIGHT; i = (KEY_MAP)(i + 1))
        {
            if (mapping.count(i))
            {
                bool down = callback ? callback(mapping[i]) : 0;
                temp = temp | ((uint8_t)down << i);
            }
        }
        input = temp;
    }
}
