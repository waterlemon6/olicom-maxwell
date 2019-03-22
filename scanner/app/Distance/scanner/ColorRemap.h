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
void ColorRemap(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset);

#endif
