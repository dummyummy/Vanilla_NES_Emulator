#include "mapper.h"

Mapper::Mapper()
{
    this->cart = nullptr;
}

Mapper::Mapper(Cartridge *cart)
{
    this->cart = cart;
}
