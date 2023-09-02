#include "frame.h"
#include <cstring>

Frame::Frame()
{
    memset(framebuffer, 0, sizeof(framebuffer));
    bufferNow = 0;
}

void Frame::setColor(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    framebuffer[bufferNow][x][y][0] = r;
    framebuffer[bufferNow][x][y][1] = g;
    framebuffer[bufferNow][x][y][2] = b;
}

void Frame::swapBuffer()
{
    bufferNow = !bufferNow;
}

unsigned char *Frame::getRawImage()
{
    return (unsigned char *)framebuffer[!bufferNow];
}
