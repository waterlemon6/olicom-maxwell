#ifndef __SCAN_H__
#define __SCAN_H__

#include "Entrance.h"

void Scan(int dpi, int depth, struct ImageSize *image);
void *SendPicture(void* jpeg);
unsigned char HeaderTag(enum Page page);
void Pack(unsigned char *src, unsigned char *dst, unsigned long dataSize, unsigned char headerTag, unsigned long headerSize);

#endif
