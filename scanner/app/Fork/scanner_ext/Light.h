#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "VideoPort.h"

struct LightAdjustment{
    int dpi;
    int depth;
    char color;

    struct Light light;

    unsigned char maxTopR;
    unsigned char maxTopG;
    unsigned char maxTopB;
    unsigned char maxTopIR;

    unsigned char maxBottomR;
    unsigned char maxBottomG;
    unsigned char maxBottomB;
    unsigned char maxBottomIR;

    unsigned char *sampleR;
    unsigned char *sampleG;
    unsigned char *sampleB;
    unsigned char *sampleIR;

    unsigned short maxLight;
    unsigned short sampleWidth;

    unsigned char finality;
    unsigned char overtime;
    unsigned char aim;
    unsigned char error;
    float proportion;
};

unsigned char GetMaxPixelInOneLine(const unsigned char *src, int length);
void GetMaxPixelInColorSpace(struct LightAdjustment *handler, unsigned char *originalImage, int height);
int LightAdjustmentJudge(struct LightAdjustment *handler);

void LightAdjustmentInit(struct LightAdjustment *handler, int dpi, char color);
void LightAdjustmentDeInit(struct LightAdjustment *handler);

void LightAdjustmentSave(struct LightAdjustment *handler);
struct Light LightAdjustmentLocal(int dpi, char color);
struct Light LightAdjustmentDefault(int dpi, char color);
struct Light LightAdjustmentLoad(int dpi, char color);

void LightAdjust(int dpi, char color);
#endif
