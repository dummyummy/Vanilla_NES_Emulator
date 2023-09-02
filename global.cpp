#include "global.h"

unsigned long long operator "" _KB(unsigned long long x)
{
    return x * 1024;
}
