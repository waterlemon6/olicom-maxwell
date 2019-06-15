#ifndef __SCAN_H__
#define __SCAN_H__

#include "Entrance.h"

void Scan(int dpi, int depth, struct ImageSize *image, int quality, int cmd);
void *SendPicture(void* jpeg);
unsigned char HeaderTag0(enum Page page);
unsigned char HeaderTag1(enum Page page, unsigned int i);
void Pack(unsigned char *src, unsigned char *dst, unsigned long dataSize, unsigned char headerTag, unsigned long headerSize);

#endif
