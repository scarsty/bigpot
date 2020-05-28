#pragma once

#include "PotStream.h"
extern "C"
{
#include "libswscale/swscale.h"
}

class RealSR;
class Waifu2x;

class PotStreamVideo : public PotStream
{
public:
    PotStreamVideo();
    ~PotStreamVideo();
    enum
    {
        NoVideo = -1,
        VideoFrameShowed = 0,
        VideoFrameBeforeTime = 1,
        NoVideoFrame = 2,
    } VideoFrameState;

private:
    //void clear();
    virtual void freeContent(void* p) override;
    virtual FrameContent convertFrameToContent() override;

public:
    int show(int time);

public:
    int getSDLPixFmt();

private:
    int texture_pix_fmt_;
    SwsContext* img_convert_ctx_ = nullptr;
    RealSR* realsr = nullptr;
    Waifu2x* waifu2x = nullptr;
};
