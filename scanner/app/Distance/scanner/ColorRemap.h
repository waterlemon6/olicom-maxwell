#ifndef __COLOR_REMAP_H__
#define __COLOR_REMAP_H__

enum ColorMap {
    COLOR_MAP_ORIGIN,
    COLOR_MAP_BRIGHT_L1,
    COLOR_MAP_BRIGHT_L2
};

void GrayScaleMapBuild(const unsigned char *map);
void LuminanceMapBuild(const unsigned char *map);
void ChrominanceMapBuild();

void SetColorMap(enum ColorMap level = COLOR_MAP_ORIGIN);
enum ColorMap DeployColorMap(unsigned char data);
void ColorRemap1To1(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset);
void ColorRemap3To2(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset);

#endif
