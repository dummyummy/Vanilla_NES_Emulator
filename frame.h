#ifndef FRAME_H
#define FRAME_H


class Frame
{
private:
    unsigned char framebuffer[2][240][256][3];
    int bufferNow;
public:
    Frame();
    void setColor(int x, int y, unsigned char r, unsigned char g, unsigned char b);
    void swapBuffer();
    unsigned char *getRawImage();
};

#endif // FRAME_H
