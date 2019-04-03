#ifndef __SCAN_H__
#define __SCAN_H__

#include "Entrance.h"

void Scan(int dpi, int depth, struct ImageSize *image);
void *SendPicture(void* jpeg);
unsigned char Header(enum Page page);
void Pack(unsigned char *src, unsigned char *dst, unsigned long dataSize, unsigned char header, unsigned long headerSize);

#endif
