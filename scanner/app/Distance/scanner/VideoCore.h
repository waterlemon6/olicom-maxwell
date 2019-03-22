#ifndef __VIDEO_CORE_H__
#define __VIDEO_CORE_H__

#
#include "Entrance.h"

#define SCAN_TIME_PER_LINE_200DPI_C3 167 // 8000000 / 1333 = 6000 lines, 1000000 / 6000 = 167 usecs.
#define SCAN_TIME_PER_LINE_200DPI_C1 167 // 8000000 / 2400 = 3333 lines, 1000000 / 3333 = 300 usecs
#define SCAN_TIME_PER_LINE_300DPI 250 // 8000000 / 2000 = 4000 lines, 1000000 / 4000 = 250 usecs.
#define SCAN_TIME_PER_LINE_600DPI 500 // 8000000 / 4000 = 2000 lines, 1000000 / 2000 = 500 usecs.

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
