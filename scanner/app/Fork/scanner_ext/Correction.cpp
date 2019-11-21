#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <arm_neon.h>
#include "VideoCore.h"
#include "Correction.h"

struct Correction globalCorrection {};

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

void CorrectionMix(unsigned char *brightK, unsigned char *darkB, int length)
{
    for(int i = 0; i < length; i++) {
        //darkB[i] = 0;
        if((brightK[i] <= LIGHT_STANDARD_DOWN) || (brightK[i] <= darkB[i])) {
            brightK[i] = 0;
            darkB[i] = 0;
        }
        else {
            float temp = (LIGHT_STANDARD_UP / ((float)(brightK[i] - darkB[i])) - 1) * 64;
            temp = temp > 0 ? temp : 0;
            temp = temp < 255 ? temp : 255;
            brightK[i] = (unsigned char)temp;
        }
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
    if (dpi == 250)
        dpi = 300;

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
    //videoPort.SetVideoMode(VIDEO_MODE_GRADIENT);
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
    CorrectionMix(adjust.correction.RK, adjust.correction.RB, adjust.correction.width);
    if (adjust.depth == 3) {
        CorrectionMix(adjust.correction.GK, adjust.correction.GB, adjust.correction.width);
        CorrectionMix(adjust.correction.BK, adjust.correction.BB, adjust.correction.width);
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

    struct Light light {.enable = true};
    videoPort.WriteLightPara(light);

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


bool CorrectionGetEdge(int dpi, char color, unsigned short *edge) {
    struct Correction correction = CorrectionAdjustLoad(dpi, color);
    if (!correction.enable)
        return false;

    unsigned short cisEdge = 0;
    switch (dpi) {
        case 200:
            cisEdge = CIS_EDGE_200DPI;
            break;
        case 300:
            cisEdge = CIS_EDGE_300DPI;
            break;
        case 600:
            cisEdge = CIS_EDGE_600DPI;
            break;
        default:
            return false;
    }

    CorrectionChannelGetEdge(correction.RK, correction.width, cisEdge, &edge[0]);
    CorrectionChannelGetEdge(correction.GK, correction.width, cisEdge, &edge[4]);
    CorrectionChannelGetEdge(correction.BK, correction.width, cisEdge, &edge[8]);

    delete [] (correction.RK);
    delete [] (correction.RB);
    delete [] (correction.GK);
    delete [] (correction.GB);
    delete [] (correction.BK);
    delete [] (correction.BB);

    return true;
}

void CorrectionChannelGetEdge(const unsigned char *channel, int width, unsigned short cisEdge, unsigned short *edge) {
    int i = 0;

    // obverse
    for (i = width / 2 + cisEdge; i <= width - 1 - cisEdge && (!channel[i] || !channel[i + 1]); i++);
    edge[0] = (unsigned short)(i - width / 2);

    for (i = width - 1 - cisEdge; i >= width / 2 + cisEdge && (!channel[i] || !channel[i - 1]); i--);
    edge[1] = (unsigned short)(i - width / 2);

    // opposite
    for (i = cisEdge; i <= width / 2 - 1 - cisEdge && (!channel[i] || !channel[i + 1]); i++);
    edge[2] = (unsigned short)i;

    for (i = width / 2 - 1 - cisEdge; i >= cisEdge && (!channel[i] || !channel[i - 1]); i--);
    edge[3] = (unsigned short)i;
}

void GlobalCorrectionInit() {
    globalCorrection = CorrectionAdjustLoad(600, 'C');
}

void GlobalCorrectionDeepCopy(struct Correction &correction, char color) {
    unsigned short width = correction.width;
    globalCorrection.width = correction.width;
    globalCorrection.enable = correction.enable;
    globalCorrection.channel = correction.channel;
    memcpy(globalCorrection.RK, correction.RK, width);
    memcpy(globalCorrection.RB, correction.RB, width);
    if (color == 'C') {
        memcpy(globalCorrection.GK, correction.GK, width);
        memcpy(globalCorrection.GB, correction.GB, width);
        memcpy(globalCorrection.BK, correction.BK, width);
        memcpy(globalCorrection.BB, correction.BB, width);
    }

    for (int i = 0; i < width; i++) {
        globalCorrection.RK[i] = (unsigned char)(globalCorrection.RK[i] < 192 ? globalCorrection.RK[i] + 64 : 255);
        if (color == 'C') {
            globalCorrection.GK[i] = (unsigned char)(globalCorrection.GK[i] < 192 ? globalCorrection.GK[i] + 64 : 255);
            globalCorrection.BK[i] = (unsigned char)(globalCorrection.BK[i] < 192 ? globalCorrection.BK[i] + 64 : 255);
        }
    }
}

void GlobalCorrectionCalculate(unsigned char *src, int depth, int offset, enum Page page) {
    int value, K, B;
    int corrL = 4; // Video port offset

    if (page == PAGE_OBVERSE_SIDE)
        corrL += globalCorrection.width / 2;

    if (depth == 3) {
        unsigned char *srcB = src;
        unsigned char *srcR = src + offset;
        unsigned char *srcG = src + offset * 2;

        for (int i = 0; i < globalCorrection.width / 2; i++) {
            value = *(srcB + i);
            K = globalCorrection.BK[corrL + i];
            B = globalCorrection.BB[corrL + i];

            value = K * (value > B ? value - B : 0) / 64;
            *(srcB + i) = (unsigned char)(value < 255 ? value : 255);
        }

        for (int i = 0; i < globalCorrection.width / 2; i++) {
            value = *(srcR + i);
            K = globalCorrection.RK[corrL + i];
            B = globalCorrection.RB[corrL + i];

            value = K * (value > B ? value - B : 0) / 64;
            *(srcR + i) = (unsigned char)(value < 255 ? value : 255);
        }

        for (int i = 0; i < globalCorrection.width / 2; i++) {
            value = *(srcG + i);
            K = globalCorrection.GK[corrL + i];
            B = globalCorrection.GB[corrL + i];

            value = K * (value > B ? value - B : 0) / 64;
            *(srcG + i) = (unsigned char)(value < 255 ? value : 255);
        };
    }
    else if (depth == 1) {
        for (int i = 0; i < globalCorrection.width / 2; i++) {
            value = *(src + i);
            K = globalCorrection.RK[corrL + i];
            B = globalCorrection.RB[corrL + i];

            value = K * (value > B ? value - B : 0) / 64;
            *(src + i) = (unsigned char)(value < 255 ? value : 255);
        }
    }
}

void GlobalCorrectionCalculate_NEON(unsigned char *src, int depth, int offset, enum Page page) {
    uint8x8_t nk, nb, nv;
    uint16x8_t nvs, nls = vdupq_n_u16(255 << 6);
    int corrL = 4; // Video port offset

    if (page == PAGE_OBVERSE_SIDE)
        corrL += globalCorrection.width / 2;

    int neon_width = ((globalCorrection.width / 2) / 8) * 8;
    int normal_width = globalCorrection.width / 2 - neon_width; // Actually it's always zero in this kind of CIS.
    if (depth == 3) {
        unsigned char *srcB = src;
        unsigned char *srcR = src + offset;
        unsigned char *srcG = src + offset * 2;

        for (int i = 0; i < neon_width; i += 8) {
            nv = vld1_u8(srcB + i);
            nk = vld1_u8(globalCorrection.BK + corrL + i);
            nb = vld1_u8(globalCorrection.BB + corrL + i);

            nv = vqsub_u8(nv, nb);
            nvs = vmull_u8(nv, nk);
            nvs = vminq_u16(nvs, nls);
            nv = vshrn_n_u16(nvs, 6);
            vst1_u8(srcB + i, nv);
        }

        for (int i = 0; i < neon_width; i += 8) {
            nv = vld1_u8(srcR + i);
            nk = vld1_u8(globalCorrection.RK + corrL + i);
            nb = vld1_u8(globalCorrection.RB + corrL + i);

            nv = vqsub_u8(nv, nb);
            nvs = vmull_u8(nv, nk);
            nvs = vminq_u16(nvs, nls);
            nv = vshrn_n_u16(nvs, 6);
            vst1_u8(srcR + i, nv);
        }

        for (int i = 0; i < neon_width; i += 8) {
            nv = vld1_u8(srcG + i);
            nk = vld1_u8(globalCorrection.GK + corrL + i);
            nb = vld1_u8(globalCorrection.GB + corrL + i);

            nv = vqsub_u8(nv, nb);
            nvs = vmull_u8(nv, nk);
            nvs = vminq_u16(nvs, nls);
            nv = vshrn_n_u16(nvs, 6);
            vst1_u8(srcG + i, nv);
        }
    }
    else if(depth == 1) {
        for (int i = 0; i < neon_width; i += 8) {
            nv = vld1_u8(src + i);
            nk = vld1_u8(globalCorrection.RK + corrL + i);
            nb = vld1_u8(globalCorrection.RB + corrL + i);

            nv = vqsub_u8(nv, nb);
            nvs = vmull_u8(nv, nk);
            nvs = vminq_u16(nvs, nls);
            nv = vshrn_n_u16(nvs, 6);
            vst1_u8(src + i, nv);
        }
    }
}