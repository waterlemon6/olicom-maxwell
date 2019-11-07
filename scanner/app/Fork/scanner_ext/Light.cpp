#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include "PrinterHost.h"
#include "main.h"
#include "Light.h"
#include "Correction.h"
#include "VideoCore.h"

unsigned char GetMaxPixelInOneLine(const unsigned char *src, int length) {
    unsigned char max = 0;
    for(int i = 0; i < length; i++) {
        if(src[i] > max)
            max = src[i];
    }
    return max;
}

void GetMaxPixelInColorSpace(struct LightAdjustment *handler, unsigned char *originalImage, int height) {
    int topPageLeftEdge = 0;
    int bottomPageLeftEdge = 0;
    int availableWidth = 0;

    switch (handler->dpi) {
        case 200:
            topPageLeftEdge = CIS_WIDTH_200DPI + CIS_EDGE_200DPI;
            bottomPageLeftEdge = CIS_EDGE_200DPI;
            availableWidth = IMAGE_WIDTH_200DPI;
            break;
        case 300:
            topPageLeftEdge = CIS_WIDTH_300DPI + CIS_EDGE_300DPI;
            bottomPageLeftEdge = CIS_EDGE_300DPI;
            availableWidth = IMAGE_WIDTH_300DPI;
            break;
        case 600:
            topPageLeftEdge = CIS_WIDTH_600DPI + CIS_EDGE_600DPI;
            bottomPageLeftEdge = CIS_EDGE_600DPI;
            availableWidth = IMAGE_WIDTH_600DPI;
            break;
        default:
            break;
    }

    switch (handler->color) {
        case 'C':
            switch (handler->dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 1, handler->sampleR, handler->sampleWidth, height, handler->depth);
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 2, handler->sampleG, handler->sampleWidth, height, handler->depth);
                    VerticalSample(originalImage, handler->sampleB, handler->sampleWidth, height, handler->depth);
                    break;
                case 600:
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 2, handler->sampleR, handler->sampleWidth, height, handler->depth * 2);
                    VerticalSample(originalImage + VIDEO_PORT_WIDTH * 4, handler->sampleG, handler->sampleWidth, height, handler->depth * 2);
                    VerticalSample(originalImage, handler->sampleB, handler->sampleWidth, height, handler->depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopR = GetMaxPixelInOneLine(handler->sampleR + topPageLeftEdge, availableWidth);
            handler->maxBottomR = GetMaxPixelInOneLine(handler->sampleR + bottomPageLeftEdge, availableWidth);
            handler->maxTopG = GetMaxPixelInOneLine(handler->sampleG + topPageLeftEdge, availableWidth);
            handler->maxBottomG = GetMaxPixelInOneLine(handler->sampleG + bottomPageLeftEdge, availableWidth);
            handler->maxTopB = GetMaxPixelInOneLine(handler->sampleB + topPageLeftEdge, availableWidth);
            handler->maxBottomB = GetMaxPixelInOneLine(handler->sampleB + bottomPageLeftEdge, availableWidth);
            handler->maxTopIR = 0;
            handler->maxBottomIR = 0;
            break;
        case 'G':
            switch (handler->dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage, handler->sampleR, handler->sampleWidth, height, handler->depth);
                    break;
                case 600:
                    VerticalSample(originalImage, handler->sampleR, handler->sampleWidth, height, handler->depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopR = GetMaxPixelInOneLine(handler->sampleR + topPageLeftEdge, availableWidth);
            handler->maxBottomR = GetMaxPixelInOneLine(handler->sampleR + bottomPageLeftEdge, availableWidth);
            handler->maxTopG = 0;
            handler->maxBottomG = 0;
            handler->maxTopB = 0;
            handler->maxBottomB = 0;
            handler->maxTopIR = 0;
            handler->maxBottomIR = 0;
            break;
        case 'I':
            switch (handler->dpi) {
                case 200:
                case 300:
                    VerticalSample(originalImage, handler->sampleIR, handler->sampleWidth, height, handler->depth);
                    break;
                case 600:
                    VerticalSample(originalImage, handler->sampleIR, handler->sampleWidth, height, handler->depth * 2);
                    break;
                default:
                    break;
            }
            handler->maxTopIR = GetMaxPixelInOneLine(handler->sampleIR + topPageLeftEdge, availableWidth);
            handler->maxBottomIR = GetMaxPixelInOneLine(handler->sampleIR + bottomPageLeftEdge, availableWidth);
            handler->maxTopR = 0;
            handler->maxBottomR = 0;
            handler->maxTopG = 0;
            handler->maxBottomG = 0;
            handler->maxTopB = 0;
            handler->maxBottomB = 0;
            break;
        default:
            break;
    }
}

int LightAdjustmentJudge(struct LightAdjustment *handler) {
    if(handler->finality++ > 5)
        return 1;

    if(handler->overtime++ > 40) {
        printf("Over Time in Light Adjusting\n");
        return 1;
    }

    printf("    top light   top max   bottom light   bottom max.\n");
    printf("R   %4d         %4d       %4d            %4d.        \n", handler->light.r1, handler->maxTopR, handler->light.r2, handler->maxBottomR);
    printf("G   %4d         %4d       %4d            %4d.        \n", handler->light.g1, handler->maxTopG, handler->light.g2, handler->maxBottomG);
    printf("B   %4d         %4d       %4d            %4d.        \n", handler->light.b1, handler->maxTopB, handler->light.b2, handler->maxBottomB);
    printf("IR  %4d         %4d       %4d            %4d.        \n", handler->light.ir1, handler->maxTopIR, handler->light.ir2, handler->maxBottomIR);

    handler->light.r1 += (handler->aim - handler->maxTopR) * handler->proportion;
    handler->light.r1 = (unsigned short) LIMIT_MIN_MAX(handler->light.r1, 100, handler->maxLight);
    handler->light.g1 += (handler->aim - handler->maxTopG) * handler->proportion;
    handler->light.g1 = (unsigned short) LIMIT_MIN_MAX(handler->light.g1, 100, handler->maxLight);
    handler->light.b1 += (handler->aim - handler->maxTopB) * handler->proportion;
    handler->light.b1 = (unsigned short) LIMIT_MIN_MAX(handler->light.b1, 100, handler->maxLight);
    handler->light.ir1 += (handler->aim - handler->maxTopIR) * handler->proportion;
    handler->light.ir1 = (unsigned short) LIMIT_MIN_MAX(handler->light.ir1, 100, handler->maxLight);

    handler->light.r2 += (handler->aim - handler->maxBottomR) * handler->proportion;
    handler->light.r2 = (unsigned short) LIMIT_MIN_MAX(handler->light.r2, 100, handler->maxLight);
    handler->light.g2 += (handler->aim - handler->maxBottomG) * handler->proportion;
    handler->light.g2 = (unsigned short) LIMIT_MIN_MAX(handler->light.g2, 100, handler->maxLight);
    handler->light.b2 += (handler->aim - handler->maxBottomB) * handler->proportion;
    handler->light.b2 = (unsigned short) LIMIT_MIN_MAX(handler->light.b2, 100, handler->maxLight);
    handler->light.ir2 += (handler->aim - handler->maxBottomIR) * handler->proportion;
    handler->light.ir2 = (unsigned short) LIMIT_MIN_MAX(handler->light.ir2, 100, handler->maxLight);

    switch(handler->color) {
        case 'C':
            if(abs(handler->aim - handler->maxTopR) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxTopG) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxTopB) > handler->error)
                handler->finality = 0;
            handler->light.ir1 = 0;
            if(abs(handler->aim - handler->maxBottomR) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxBottomG) > handler->error)
                handler->finality = 0;
            if(abs(handler->aim - handler->maxBottomB) > handler->error)
                handler->finality = 0;
            handler->light.ir2 = 0;
            break;
        case 'G':
            if(abs(handler->aim - handler->maxTopR) > handler->error)
                handler->finality = 0;
            handler->light.g1 = (unsigned short) (handler->light.r1 * 0.8);
            handler->light.b1 = (unsigned short) (handler->light.r1 * 0.6);
            handler->light.ir1 = 0;
            if(abs(handler->aim - handler->maxBottomR) > handler->error)
                handler->finality = 0;
            handler->light.g2 = (unsigned short) (handler->light.r2 * 0.8);
            handler->light.b2 = (unsigned short) (handler->light.r2 * 0.6);
            handler->light.ir2 = 0;
            break;
        case 'I':
            if(abs(handler->aim - handler->maxTopIR) > handler->error)
                handler->finality = 0;
            handler->light.r1 = 0;
            handler->light.g1 = 0;
            handler->light.b1 = 0;
            if(abs(handler->aim - handler->maxBottomIR) > handler->error)
                handler->finality = 0;
            handler->light.r2 = 0;
            handler->light.g2 = 0;
            handler->light.b2 = 0;
            break;
        default:
            break;
    }

    return 0;
}

void LightAdjustmentInit(struct LightAdjustment *handler, int dpi, char color) {
    handler->finality = 0;
    handler->overtime = 0;
    handler->aim = 237;
    handler->error = 8;

    handler->dpi = dpi;
    handler->color = color;

    switch(handler->dpi) {
        case 200:
            handler->sampleWidth = CIS_WIDTH_200DPI * 2;
            handler->maxLight = 1333;
            switch(handler->color) {
                case 'C':
                    handler->light.r2 = handler->light.r1 = 1000;
                    handler->light.g2 = handler->light.g1 = 800;
                    handler->light.b2 = handler->light.b1 = 600;
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'G':
                    handler->light.r2 = handler->light.r1 = 400;
                    handler->light.g2 = handler->light.g1 = (unsigned short) (handler->light.r1 * 0.8);
                    handler->light.b2 = handler->light.b1 = (unsigned short) (handler->light.r1 * 0.6);
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'I':
                    handler->light.r2 = handler->light.r1 = 0;
                    handler->light.g2 = handler->light.g1 = 0;
                    handler->light.b2 = handler->light.b1 = 0;
                    handler->light.ir2 = handler->light.ir1 = 800;
                    break;
                default:
                    break;
            }
            break;
        case 300:
            handler->sampleWidth = CIS_WIDTH_300DPI * 2;
            handler->maxLight = 2000;
            switch(handler->color) {
                case 'C':
                    handler->light.r2 = handler->light.r1 = 1700;
                    handler->light.g2 = handler->light.g1 = 1200;
                    handler->light.b2 = handler->light.b1 = 700;
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'G':
                    handler->light.r2 = handler->light.r1 = 500;
                    handler->light.g2 = handler->light.g1 = (unsigned short) (handler->light.r1 * 0.8);
                    handler->light.b2 = handler->light.b1 = (unsigned short) (handler->light.r1 * 0.6);
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'I':
                    handler->light.r2 = handler->light.r1 = 0;
                    handler->light.g2 = handler->light.g1 = 0;
                    handler->light.b2 = handler->light.b1 = 0;
                    handler->light.ir2 = handler->light.ir1 = 1000;
                    break;
                default:
                    break;
            }
            break;
        case 600:
            handler->sampleWidth = CIS_WIDTH_600DPI * 2;
            handler->maxLight = 4000;
            switch(handler->color) {
                case 'C':
                    handler->light.r2 = handler->light.r1 = 3000;
                    handler->light.g2 = handler->light.g1 = 2100;
                    handler->light.b2 = handler->light.b1 = 1200;
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'G':
                    handler->light.r2 = handler->light.r1 = 900;
                    handler->light.g2 = handler->light.g1 = (unsigned short) (handler->light.r1 * 0.8);
                    handler->light.b2 = handler->light.b1 = (unsigned short) (handler->light.r1 * 0.6);
                    handler->light.ir2 = handler->light.ir1 = 0;
                    break;
                case 'I':
                    handler->light.r2 = handler->light.r1 = 0;
                    handler->light.g2 = handler->light.g1 = 0;
                    handler->light.b2 = handler->light.b1 = 0;
                    handler->light.ir2 = handler->light.ir1 = 2300;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    switch (handler->color) {
        case 'C':
            handler->depth = 3;
            handler->proportion = 2;
            handler->sampleR = new unsigned char [handler->sampleWidth];
            handler->sampleG = new unsigned char [handler->sampleWidth];
            handler->sampleB = new unsigned char [handler->sampleWidth];
            handler->sampleIR = nullptr;
            break;
        case 'G':
            handler->depth = 1;
            handler->proportion = 0.7;
            handler->sampleR = new unsigned char [handler->sampleWidth];
            handler->sampleG = nullptr;
            handler->sampleB = nullptr;
            handler->sampleIR = nullptr;
            break;
        case 'I':
            handler->depth = 1;
            handler->proportion = 2;
            handler->sampleR = nullptr;
            handler->sampleG = nullptr;
            handler->sampleB = nullptr;
            handler->sampleIR = new unsigned char [handler->sampleWidth];
            break;
        default:
            break;
    }
}

void LightAdjustmentDeInit(struct LightAdjustment *handler) {
    delete [] handler->sampleR;
    delete [] handler->sampleG;
    delete [] handler->sampleB;
    delete [] handler->sampleIR;
}

void LightAdjustmentSave(struct LightAdjustment *handler) {
    char fileName[128];
    int magic = 34567;

    sprintf(fileName, "para_light_%d_%c", handler->dpi, handler->color);
    FILE *stream = fopen(fileName, "wb");
    if(stream == nullptr)
        return;

    fwrite(&magic, sizeof(int), 1, stream);
    fwrite(handler, sizeof(struct LightAdjustment), 1, stream);
    fflush(stream);
    fclose(stream);
    sync();
}

struct Light LightAdjustmentLocal(int dpi, char color) {
    struct LightAdjustment adjust {};
    adjust.light.enable = false;
    char fileName[128];
    int magic;

    sprintf(fileName, "para_light_%d_%c", dpi, color);
    FILE *stream = fopen(fileName, "rb");
    if(!stream)
        return adjust.light;

    fseek(stream, 0, SEEK_END);
    if(ftell(stream) == 0) {
        fclose(stream);
        return adjust.light;
    }
    fseek(stream, 0, SEEK_SET);
    fread(&magic, sizeof(int), 1, stream);

    if(magic == 34567)
        fread(&adjust, sizeof(struct LightAdjustment), 1, stream);
    fclose(stream);

    adjust.light.enable = true;
    return adjust.light;
}

struct Light LightAdjustmentDefault(int dpi, char color) {
    struct Light light {.enable = true};
    switch (dpi) {
        case 200:
        case 300:
        case 600:
            switch (color) {
                case 'G':
                    light.r2 = light.r1 = 400;
                    light.g2 = light.g1 = 360;
                    light.b2 = light.b1 = 240;
                    light.ir2 = light.ir1 = 0;
                    break;
                case 'C':
                case 'I':
                    light.r2 = light.r1 = 1200;
                    light.g2 = light.g1 = 1000;
                    light.b2 = light.b1 = 800;
                    light.ir2 = light.ir1 = 1200;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return light;
}

struct Light LightAdjustmentLoad(int dpi, char color) {
    if (dpi == 250)
        dpi = 300;

    struct Light light = LightAdjustmentLocal(dpi, color);
    if (light.enable) {
        printf("Light adjustment is ready.\n");
        return light;
    }
    else {
        printf("Error in light adjustment updating.\n");
        light = LightAdjustmentDefault(dpi, color);
        return light;
    }
}

void LightAdjust(int dpi, char color) {
    int depth = 1;
    if (color == 'C')
        depth = 3;

    struct LightAdjustment adjust {};
    LightAdjustmentInit(&adjust, dpi, color);
    adjust.light.enable = true;

    /* Extend begin */
    switch (color) {
        case 'C':
            adjust.light.r2 = adjust.light.r1 = adjust.maxLight;
            adjust.light.g2 = adjust.light.g1 = adjust.maxLight;
            adjust.light.b2 = adjust.light.b1 = adjust.maxLight;
            break;
        case 'G':
            adjust.light.r2 = adjust.light.r1 = adjust.maxLight;
            adjust.light.g2 = adjust.light.g1 = (unsigned short) (adjust.light.r1 * 0.8);
            adjust.light.b2 = adjust.light.b1 = (unsigned short) (adjust.light.r1 * 0.6);
            break;
        default:
            break;
    }

    do {
        VideoPort videoPort;
        videoPort.Open(VIDEO_PORT_PATH, O_RDWR);

        videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
        videoPort.WriteLightPara(adjust.light);
        videoPort.StartScan(1);
        VideoCoreWait(dpi, depth, 150);

        GetMaxPixelInColorSpace(&adjust, videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30, 100);

        videoPort.StopScan();
        videoPort.Close();
    } while (!LightAdjustmentPreJudge(&adjust));

    struct LightAdjustment adjustA = adjust;
    adjust.light.r1 /= 2;
    adjust.light.r2 /= 2;
    adjust.light.g1 /= 2;
    adjust.light.g2 /= 2;
    adjust.light.b1 /= 2;
    adjust.light.b2 /= 2;
    {
        VideoPort videoPort;
        videoPort.Open(VIDEO_PORT_PATH, O_RDWR);

        videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
        videoPort.WriteLightPara(adjust.light);
        videoPort.StartScan(1);
        VideoCoreWait(dpi, depth, 150);

        GetMaxPixelInColorSpace(&adjust, videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30, 100);

        videoPort.StopScan();
        videoPort.Close();

        printf("    top light   top max   bottom light   bottom max.\n");
        printf("R   %4d         %4d       %4d            %4d.        \n", adjust.light.r1, adjust.maxTopR, adjust.light.r2, adjust.maxBottomR);
        printf("G   %4d         %4d       %4d            %4d.        \n", adjust.light.g1, adjust.maxTopG, adjust.light.g2, adjust.maxBottomG);
        printf("B   %4d         %4d       %4d            %4d.        \n", adjust.light.b1, adjust.maxTopB, adjust.light.b2, adjust.maxBottomB);
        printf("IR  %4d         %4d       %4d            %4d.        \n", adjust.light.ir1, adjust.maxTopIR, adjust.light.ir2, adjust.maxBottomIR);

    }

    struct LightAdjustment adjustB = adjust;
    LightAdjustmentGoToAim(&adjustA, &adjustB, &adjust);
    /* Extend end */

    do {
        VideoPort videoPort;
        videoPort.Open(VIDEO_PORT_PATH, O_RDWR);

        videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
        videoPort.WriteLightPara(adjust.light);
        videoPort.StartScan(1);
        VideoCoreWait(dpi, depth, 150);

        GetMaxPixelInColorSpace(&adjust, videoPort.GetOriginImagePos() + VIDEO_PORT_WIDTH * 30, 100);

        videoPort.StopScan();
        videoPort.Close();
    } while (!LightAdjustmentJudge(&adjust));

    LightAdjustmentSave(&adjust);
    LightAdjustmentDeInit(&adjust);
}

int LightAdjustmentPreJudge(struct LightAdjustment *handler) {
    int retval = 0;
    int edge = 250;

    printf("    top light   top max   bottom light   bottom max.\n");
    printf("R   %4d         %4d       %4d            %4d.        \n", handler->light.r1, handler->maxTopR, handler->light.r2, handler->maxBottomR);
    printf("G   %4d         %4d       %4d            %4d.        \n", handler->light.g1, handler->maxTopG, handler->light.g2, handler->maxBottomG);
    printf("B   %4d         %4d       %4d            %4d.        \n", handler->light.b1, handler->maxTopB, handler->light.b2, handler->maxBottomB);
    printf("IR  %4d         %4d       %4d            %4d.        \n", handler->light.ir1, handler->maxTopIR, handler->light.ir2, handler->maxBottomIR);

    switch(handler->color) {
        case 'C':
            if (handler->maxTopR <= edge)
                retval++;
            else
                handler->light.r1 /= 2;

            if (handler->maxTopG <= edge)
                retval++;
            else
                handler->light.g1 /= 2;

            if (handler->maxTopB <= edge)
                retval++;
            else
                handler->light.b1 /= 2;

            if (handler->maxBottomR <= edge)
                retval++;
            else
                handler->light.r2 /= 2;

            if (handler->maxBottomG <= edge)
                retval++;
            else
                handler->light.g2 /= 2;

            if (handler->maxBottomB <= edge)
                retval++;
            else
                handler->light.b2 /= 2;

            if (retval == 6)
                retval = 1;
            else
                retval = 0;
            break;
        case 'G':
            if (handler->maxTopR <= edge)
                retval++;
            else {
                handler->light.r1 /= 2;
                handler->light.g1 = (unsigned short) (handler->light.r1 * 0.8);
                handler->light.b1 = (unsigned short) (handler->light.r1 * 0.6);
            }

            if (handler->maxBottomR <= edge)
                retval++;
            else {
                handler->light.r2 /= 2;
                handler->light.g2 = (unsigned short) (handler->light.r2 * 0.8);
                handler->light.b2 = (unsigned short) (handler->light.r2 * 0.6);
            }

            if (retval == 2)
                retval = 1;
            else
                retval = 0;
            break;
        default:
            break;
    }

    return retval;
}

void LightAdjustmentGoToAim(struct LightAdjustment *adjustA, struct LightAdjustment *adjustB, struct LightAdjustment *aim) {
    switch (aim->color) {
        case 'C':
            aim->light.r1 = LightAdjustmentCalculateAim(adjustA->light.r1, adjustA->maxTopR, adjustB->light.r1, adjustB->maxTopR, aim->aim);
            aim->light.r2 = LightAdjustmentCalculateAim(adjustA->light.r2, adjustA->maxBottomR, adjustB->light.r2, adjustB->maxBottomR, aim->aim);
            aim->light.g1 = LightAdjustmentCalculateAim(adjustA->light.g1, adjustA->maxTopG, adjustB->light.g1, adjustB->maxTopG, aim->aim);
            aim->light.g2 = LightAdjustmentCalculateAim(adjustA->light.g2, adjustA->maxBottomG, adjustB->light.g2, adjustB->maxBottomG, aim->aim);
            aim->light.b1 = LightAdjustmentCalculateAim(adjustA->light.b1, adjustA->maxTopB, adjustB->light.b1, adjustB->maxTopR, aim->aim);
            aim->light.b2 = LightAdjustmentCalculateAim(adjustA->light.b2, adjustA->maxBottomB, adjustB->light.b2, adjustB->maxBottomR, aim->aim);
            break;
        case 'G':
            aim->light.r1 = LightAdjustmentCalculateAim(adjustA->light.r1, adjustA->maxTopR, adjustB->light.r1, adjustB->maxTopR, aim->aim);
            aim->light.r2 = LightAdjustmentCalculateAim(adjustA->light.r2, adjustA->maxBottomR, adjustB->light.r2, adjustB->maxBottomR, aim->aim);
            aim->light.g1 = (unsigned short) (aim->light.r1 * 0.8);
            aim->light.g2 = (unsigned short) (aim->light.r2 * 0.8);
            aim->light.b1 = (unsigned short) (aim->light.r1 * 0.6);
            aim->light.b2 = (unsigned short) (aim->light.r2 * 0.8);
            break;
        default:
            break;
    }

}

unsigned short LightAdjustmentCalculateAim(unsigned short l1, unsigned char m1, unsigned short l2, unsigned char m2, unsigned char m3) {
    float x1 = l1, y1 = m1;
    float x2 = l2, y2 = m2;
    float x3, y3 = m3;

    float k = (y1 - y2) / (x1 - x2);
    float b = y1 - k * x1;

    x3 = (y3 - b) / k;
    return (unsigned short)x3;
}