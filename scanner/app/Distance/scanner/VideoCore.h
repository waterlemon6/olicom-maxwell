#ifndef __VIDEO_CORE_H__
#define __VIDEO_CORE_H__

#include "Entrance.h"

struct VideoCoreAttr {
    int cisWidth;
    int channelWidth;
    int frame;
    int scanLines;
    int shiftLines;
};

class VideoCore {
private:
    int dpi_ = 0;
    int depth_ = 0;
    unsigned char *imageOriginalPosition_ = nullptr;
    unsigned char *imageCurrentPosition_ = nullptr;
    struct VideoCoreAttr attr_ {};
    struct timeval tv_scan1_ {};
    struct timeval tv_scan2_ {};

    void SetTime();
    int CalculateScanLines();
public:
    VideoCore() = default;
    ~VideoCore() = default;
    void SetAttr(int dpi, int depth);
    void Activate(unsigned char *origin);

    int GetScanLines() { return attr_.scanLines; }
    int GetShiftLines() { return attr_.shiftLines; }
    int GetFrame() { return attr_.frame; }
    void UpdateScanLines();
    void UpdateShiftLines();
    void UpdateFrame();

    void ExtractImageSlice(unsigned char *dst, int offset, enum Page page);
};

void VideoCoreWait(int dpi, int depth, unsigned int lines);

#endif
