#ifndef __CORRECTION_H__
#define __CORRECTION_H__

#include "VideoPort.h"
#include "Entrance.h"

#define LIGHT_STANDARD_UP 255
#define LIGHT_STANDARD_DOWN 64

struct CorrectionAdjust{
    int dpi;
    int depth;
    char color;

    struct Correction correction;
};

void VerticalSample(unsigned char *src, unsigned char *dst, int width, int height, int depth);
void CorrectionMix(unsigned char *brightK, unsigned char *darkB, int length);

void CorrectionAdjustInit(struct CorrectionAdjust *handler, int dpi, char color);
void CorrectionAdjustDeInit(struct CorrectionAdjust *handler);

void CorrectionAdjustSave(struct CorrectionAdjust *handler);
struct Correction CorrectionAdjustLocal(int dpi, char color);
struct Correction CorrectionAdjustLoad(int dpi, char color, unsigned short channel = 0);
void CorrectionAdjust(int dpi, char color, int videoPortOffset);

void CorrectionAdjustNoPaperDataSave(struct CorrectionAdjust *handler);
void CorrectionAdjustNoPaperDataLoad(struct CorrectionAdjust *handler);
void CorrectionAdjustNoPaper(int dpi, char color, int videoPortOffset);

bool CorrectionGetEdge(int dpi, char color, unsigned short *edge);
void CorrectionChannelGetEdge(const unsigned char *channel, int width, unsigned short cisEdge, unsigned short *edge);

void GlobalCorrectionInit();
void GlobalCorrectionDeepCopy(struct Correction &correction);
void GlobalCorrectionCalculate(unsigned char *src, int depth, int offset, enum Page page);

#endif
