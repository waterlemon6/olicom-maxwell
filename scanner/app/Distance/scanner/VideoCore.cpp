#include <iostream>
#include <cstring>
#include <algorithm>
#include <sys/time.h>
#include <unistd.h>

#include "main.h"
#include "VideoCore.h"
#include "VideoPort.h"
#include "ColorRemap.h"

void VideoCore::SetTime() {
    struct timeval tv {};

    /**
      * It may get wrong time in A64, which could be very large.
      * But this won't lead to continuous errors if it gets wrong time once.
      * Therefore, I use a single check there to void most disastrous results.
      * This can't solve all problems but takes only a little additional time.
      * For real time, maybe using hardware timer here is the right way.
      */
    while (gettimeofday(&tv, nullptr))
        usleep(10);
    usleep(10);
    while (gettimeofday(&tv_scan1_, nullptr))
        usleep(10);

    if(labs(tv.tv_sec - tv_scan1_.tv_sec) > 2) {
        while (gettimeofday(&tv_scan1_, nullptr))
            usleep(10);
    }
    tv_scan2_ = tv_scan1_;
    std::cout << "Scan sec: " << tv_scan1_.tv_sec << " usec: " << tv_scan1_.tv_usec << std::endl;
}

int VideoCore::CalculateScanLines() {
    __time_t scanTime;
    struct timeval tv {};
    int unit;

    /**
      * Same as above.
      */
    while (gettimeofday(&tv, nullptr))
        usleep(10);
    while(labs(tv.tv_sec - tv_scan2_.tv_sec) > 2) {
        while (gettimeofday(&tv, nullptr))
            usleep(10);
        std::cout << "Error in getting time." << std::endl;
        usleep(10);
    }
    tv_scan2_ = tv;
    scanTime = (tv_scan2_.tv_sec - tv_scan1_.tv_sec) * 1000000 + tv_scan2_.tv_usec - tv_scan1_.tv_usec;

    switch (dpi_) {
        case 200:
            unit = (depth_ == 3) ? SCAN_TIME_PER_LINE_200D3 : SCAN_TIME_PER_LINE_200D1;
            break;
        case 250:
            unit = (depth_ == 3) ? SCAN_TIME_PER_LINE_250D3 : SCAN_TIME_PER_LINE_250D1;
            break;
        case 300:
            unit = (depth_ == 3) ? SCAN_TIME_PER_LINE_300D3 : SCAN_TIME_PER_LINE_300D1;
            break;
        case 600:
            unit = (depth_ == 3) ? SCAN_TIME_PER_LINE_600D3 : SCAN_TIME_PER_LINE_600D1;
            break;
        default:
            return 0;
    }

    return (int)(scanTime / unit);
}

void VideoCore::SetAttr(int dpi, int depth) {
    dpi_ = dpi;
    depth_ = depth;

    attr_.scanLines = 0;
    attr_.shiftLines = 0;
    switch (dpi_) {
        case 200:
            attr_.cisWidth = CIS_WIDTH_200DPI;
            attr_.channelWidth = VIDEO_PORT_WIDTH;
            attr_.frame = 1;
            colorRemap = ColorRemap1To1;
            break;
        case 250:
            attr_.cisWidth = CIS_WIDTH_300DPI;
            attr_.channelWidth = VIDEO_PORT_WIDTH;
            attr_.frame = 1;
            colorRemap = ColorRemap3To2;
            break;
        case 300:
            attr_.cisWidth = CIS_WIDTH_300DPI;
            attr_.channelWidth = VIDEO_PORT_WIDTH;
            attr_.frame = (depth_ == 3) ? 2 : 1;
            colorRemap = ColorRemap1To1;
            //colorRemap = ColorRemap1To1_NEON; // ColorRemap1To1_NEON use RGB in JPEG.
            break;
        case 600:
            attr_.cisWidth = CIS_WIDTH_600DPI;
            attr_.channelWidth = VIDEO_PORT_WIDTH * 2;
            attr_.frame = (depth_ == 3) ? 6 : 2;
            colorRemap = ColorRemap1To1;
            break;
        default:
            return;
    }
}

void VideoCore::Activate(unsigned char *origin) {
    imageOriginalPosition_ = origin;
    imageCurrentPosition_ = imageOriginalPosition_;
    SetTime();
}


void VideoCore::UpdateScanLines() {
    attr_.scanLines = CalculateScanLines();
    if(attr_.scanLines > 8000)
        std::cout << "Error in getting scan lines: " << attr_.scanLines << std::endl;
}

void VideoCore::UpdateShiftLines() {
    imageCurrentPosition_ += attr_.channelWidth * depth_;
    attr_.shiftLines++;
}

void VideoCore::UpdateFrame() {
    int rowsPerLines = 1;

    if (depth_ == 3)
        rowsPerLines *= 3;
    if (dpi_ == 600)
        rowsPerLines *= 2;

    if (attr_.frame && !(attr_.shiftLines*rowsPerLines % 7800)) {
        struct timeval tv {};
        gettimeofday(&tv, nullptr);
        std::cout << "New frame! lines: " << attr_.shiftLines
                  << " sec: " << tv.tv_sec
                  << " usec: " << tv.tv_usec << std::endl;
        imageCurrentPosition_ = imageOriginalPosition_;
        attr_.frame --;
    }
}

/**
  * @brief  Extract image slice (one line in image model) from video port to fill queues.
  *         No matter which mode, organize data to the same structure.
  *         --  channel 1  --  0 ~ offset-1  --
  *         --  channel 2  --  offset ~ 2*offset-1  -- (only in color model)
  *         --  channel 3  --  2*offset ~ 3*offset-1  -- (only in color model)
  * @param  dst: Dst position to store image data. (length of dst >= offset*3)
  *         offset: Offset of each channel in dst. (offset >= max length of one channel)
  *         page: Extract image slice in which page.
  * @retval None.
  */
void VideoCore::ExtractImageSlice(unsigned char *dst, int offset, enum Page page) {
    unsigned char *src = imageCurrentPosition_;
    int pageWidth = 0;

    if (page == PAGE_OBVERSE_SIDE)
        pageWidth = attr_.cisWidth;

    memcpy(dst, src + pageWidth, (size_t)attr_.cisWidth);
    if (depth_ == 3) {
        memcpy(dst + offset, src + attr_.channelWidth + pageWidth, (size_t)attr_.cisWidth);
        memcpy(dst + offset * 2, src + attr_.channelWidth * 2 + pageWidth, (size_t)attr_.cisWidth);
    }
}

void VideoCoreWait(int dpi, int depth, unsigned int lines)
{
    VideoCore videoCore;
    videoCore.SetAttr(dpi, depth);
    videoCore.Activate(nullptr);
    /**
      * Same as above.
      */
    while(videoCore.GetScanLines() < lines)
        videoCore.UpdateScanLines();
    while(videoCore.GetScanLines() < lines)
        videoCore.UpdateScanLines();
}

