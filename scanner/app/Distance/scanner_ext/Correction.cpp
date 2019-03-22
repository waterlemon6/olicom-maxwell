#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include "VideoCore.h"
#include "Correction.h"

void VerticalSample(unsigned char *src, unsigned char *dst, int width, int height, int depth)
{
    for(int col = 0; col < width; col++) {
        int sum = 0;
        for(int row = 0; row < height; row++) {
            sum += *(src + VIDEO_PORT_WIDTH * row * depth);
        }
        *dst++ = (unsigned char) (sum / height);
        src++;
    }
}

void CorrectionMix(unsigned char *brightK, unsigned char *darkB, int length, float lightStandard)
{
    int i = 0;
    float temp;

    for(i = 0; i < length; i++) {
        //*darkB = 0;
        if(*brightK > 255)
            *brightK = 0;
        else if(*brightK <= 60)
            *brightK = 0;
        else {
            temp = lightStandard / ((float)(*brightK - *darkB)) - 1;
            *brightK = (unsigned char)(temp * 64.0f);
        }

        brightK++;
        darkB++;
    }
}

void CorrectionAdjustInit(struct CorrectionAdjust *handler, int dpi, char color)
{
    handler->dpi = dpi;
    handler->color = color;

    switch (handler->dpi) {
        case 200:
            handler->correction.width = CIS_WIDTH_200DPI * 2;
            break;
        case 300:
            handler->correction.width = CIS_WIDTH_300DPI * 2;
            break;
        case 600:
            handler->correction.width = CIS_WIDTH_600DPI * 2;
            break;
        default:
            break;
    }

    switch (handler->color) {
        case 'C':
            handler->depth = 3;
            handler->correction.RK = new unsigned char [handler->correction.width];
            handler->correction.RB = new unsigned char [handler->correction.width];
            handler->correction.GK = new unsigned char [handler->correction.width];
            handler->correction.GB = new unsigned char [handler->correction.width];
            handler->correction.BK = new unsigned char [handler->correction.width];
            handler->correction.BB = new unsigned char [handler->correction.width];
            break;
        case 'G':
        case 'I':
            handler->depth = 1;
            handler->correction.RK = new unsigned char [handler->correction.width];
            handler->correction.RB = new unsigned char [handler->correction.width];
            handler->correction.GK = nullptr;
            handler->correction.GB = nullptr;
            handler->correction.BK = nullptr;
            handler->correction.BB = nullptr;
            break;
        default:
            break;
    }
}

void CorrectionAdjustDeInit(struct CorrectionAdjust *handler)
{
    delete [] handler->correction.RK;
    delete [] handler->correction.GK;
    delete [] handler->correction.BK;
    delete [] handler->correction.RB;
    delete [] handler->correction.GB;
    delete [] handler->correction.BB;
}


void CorrectionAdjustSave(struct CorrectionAdjust *handler)
{
    char fileName[128];
    int magic = 67890;

    sprintf(fileName, "para_correction_%d_%c", handler->dpi, handler->color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == nullptr)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler->correction.RK, (size_t) handler->correction.width, 1, stream);
    fwrite(handler->correction.RB, (size_t) handler->correction.width, 1, stream);
    if(handler->depth == 3) {
        fwrite(handler->correction.GK, (size_t) handler->correction.width, 1, stream);
        fwrite(handler->correction.GB, (size_t) handler->correction.width, 1, stream);
        fwrite(handler->correction.BK, (size_t) handler->correction.width, 1, stream);
        fwrite(handler->correction.BB, (size_t) handler->correction.width, 1, stream);
    }
    fflush(stream);
    fclose(stream);
    sync();
}

struct Correction CorrectionAdjustLocal(int dpi, char color)
{
    struct Correction correction {.enable = false};
    char fileName[128];
    int magic;

    sprintf(fileName, "para_correction_%d_%c", dpi, color);
    FILE *stream = fopen(fileName, "rb");
    if(stream == nullptr)
        return correction;

    fseek(stream, 0, SEEK_END);
    if(ftell(stream) == 0) {
        fclose(stream);
        return correction;
    }
    fseek(stream, 0, SEEK_SET);
    fread(&magic, sizeof(int), 1, stream);

    switch (dpi) {
        case 200:
            correction.width = CIS_WIDTH_200DPI * 2;
            break;
        case 300:
            correction.width = CIS_WIDTH_300DPI * 2;
            break;
        case 600:
            correction.width = CIS_WIDTH_600DPI * 2;
            break;
        default:
            break;
    }

    if(magic == 67890) {
        correction.RK = new unsigned char [correction.width];
        correction.RB = new unsigned char [correction.width];
        fread(correction.RK, (size_t) correction.width, 1, stream);
        fread(correction.RB, (size_t) correction.width, 1, stream);
        if(color == 'C') {
            correction.GK = new unsigned char [correction.width];
            correction.GB = new unsigned char [correction.width];
            correction.BK = new unsigned char [correction.width];
            correction.BB = new unsigned char [correction.width];
            fread(correction.GK, (size_t) correction.width, 1, stream);
            fread(correction.GB, (size_t) correction.width, 1, stream);
            fread(correction.BK, (size_t) correction.width, 1, stream);
            fread(correction.BB, (size_t) correction.width, 1, stream);
        }
    }
    fclose(stream);

    correction.enable = true;
    return correction;
}

struct Correction CorrectionAdjustLoad(int dpi, char color, unsigned short channel)
{
    struct Correction correction = CorrectionAdjustLocal(dpi, color);

    if (channel)
        correction.channel = channel;
    else {
        switch (color) {
            case 'C':
                correction.channel = CHANNEL_R | CHANNEL_G | CHANNEL_B;
                break;
            case 'G':
                correction.channel = CHANNEL_GRAY;
                break;
            case 'I':
                correction.channel = CHANNEL_IR;
                break;
            default:
                break;
        }
    }

    if (correction.enable) {
        printf("Correction parameters is ready.\n");
        return correction;
    }
    else {
        printf("Error in correction parameters updating.\n");
        return correction;
    }
}

void CorrectionAdjust(int dpi, char color, int videoPortOffset) {
    struct CorrectionAdjust adjust {};
    CorrectionAdjustInit(&adjust, dpi, color);

    VideoPort videoPort;
    videoPort.Open(VIDEO_PORT_PATH, O_RDWR);
    
    videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
    videoPort.StartScan(1);
    VideoCoreWait(adjust.dpi, adjust.depth, 800);

    switch (adjust.dpi) {
        case 200:
        case 300:
            VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 31 + videoPortOffset,
                           adjust.correction.RK, adjust.correction.width, 750, adjust.depth);
            if (adjust.depth == 3) {
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 32 + videoPortOffset,
                               adjust.correction.GK, adjust.correction.width, 750, adjust.depth);
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30 + videoPortOffset,
                               adjust.correction.BK, adjust.correction.width, 750, adjust.depth);
            }
            break;
        case 600:
            VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 32 + videoPortOffset,
                           adjust.correction.RK, adjust.correction.width, 750, adjust.depth * 2);
            if (adjust.depth == 3) {
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 34 + videoPortOffset,
                               adjust.correction.GK, adjust.correction.width, 750, adjust.depth * 2);
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30 + videoPortOffset,
                               adjust.correction.BK, adjust.correction.width, 750, adjust.depth * 2);
            }
            break;
        default:
            break;
    }

    videoPort.StopScan();
    videoPort.Close();

    CorrectionAdjustNoPaperDataLoad(&adjust);
    CorrectionMix(adjust.correction.RK, adjust.correction.RB, adjust.correction.width, 255);
    if (adjust.depth == 3) {
        CorrectionMix(adjust.correction.GK, adjust.correction.GB, adjust.correction.width, 255);
        CorrectionMix(adjust.correction.BK, adjust.correction.BB, adjust.correction.width, 255);
    }
    CorrectionAdjustSave(&adjust);
    CorrectionAdjustDeInit(&adjust);
}

void CorrectionAdjustNoPaperDataSave(struct CorrectionAdjust *handler)
{
    char fileName[128];
    int magic = 12345;

    sprintf(fileName, "no_paper_data_%d_%c", handler->dpi, handler->color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == nullptr)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler->correction.RB, (size_t) handler->correction.width, 1, stream);
    if(handler->depth == 3) {
        fwrite(handler->correction.GB, (size_t) handler->correction.width, 1, stream);
        fwrite(handler->correction.BB, (size_t) handler->correction.width, 1, stream);
    }
    fflush(stream);
    fclose(stream);
    sync();
}

void CorrectionAdjustNoPaperDataLoad(struct CorrectionAdjust *handler)
{
    char fileName[128];
    int magic;

    sprintf(fileName, "no_paper_data_%d_%c", handler->dpi, handler->color);
    FILE *stream = fopen(fileName, "rb");
    if(stream == nullptr) {
        memset(handler->correction.RB, 0, (size_t) handler->correction.width);
        if(handler->depth == 3) {
            memset(handler->correction.GB, 0, (size_t) handler->correction.width);
            memset(handler->correction.BB, 0, (size_t) handler->correction.width);
        }
        return;
    }

    fseek(stream, 0, SEEK_END);
    if(ftell(stream) == 0) {
        fclose(stream);
        memset(handler->correction.RB, 0, (size_t) handler->correction.width);
        if(handler->depth == 3) {
            memset(handler->correction.GB, 0, (size_t) handler->correction.width);
            memset(handler->correction.BB, 0, (size_t) handler->correction.width);
        }
        return;
    }
    fseek(stream, 0, SEEK_SET);
    fread(&magic, sizeof(int), 1, stream);

    if(magic == 12345) {
        fread(handler->correction.RB, (size_t) handler->correction.width, 1, stream);
        if(handler->depth == 3) {
            fread(handler->correction.GB, (size_t) handler->correction.width, 1, stream);
            fread(handler->correction.BB, (size_t) handler->correction.width, 1, stream);
        }
    }
    fclose(stream);
}

void CorrectionAdjustNoPaper(int dpi, char color, int videoPortOffset)
{
    struct CorrectionAdjust adjust {};
    CorrectionAdjustInit(&adjust, dpi, color);

    VideoPort videoPort;
    videoPort.Open(VIDEO_PORT_PATH, O_RDWR);

    videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
    videoPort.StartScan(1);
    VideoCoreWait(adjust.dpi, adjust.depth, 300);

    switch (dpi) {
        case 200:
        case 300:
            VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 31 + videoPortOffset,
                           adjust.correction.RB, adjust.correction.width, 250, adjust.depth);
            if (adjust.depth == 3) {
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 32 + videoPortOffset,
                               adjust.correction.GB, adjust.correction.width, 250, adjust.depth);
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30 + videoPortOffset,
                               adjust.correction.BB, adjust.correction.width, 250, adjust.depth);
            }
            break;
        case 600:
            VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 32 + videoPortOffset,
                           adjust.correction.RB, adjust.correction.width, 250, adjust.depth * 2);
            if (adjust.depth == 3) {
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 34 + videoPortOffset,
                               adjust.correction.GB, adjust.correction.width, 250, adjust.depth * 2);
                VerticalSample(videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30 + videoPortOffset,
                               adjust.correction.BB, adjust.correction.width, 250, adjust.depth * 2);
            }
            break;
        default:
            break;
    }

    videoPort.StopScan();
    videoPort.Close();

    CorrectionAdjustNoPaperDataSave(&adjust);
    CorrectionAdjustDeInit(&adjust);
}
