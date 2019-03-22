#ifndef __SCAN_H__
#define __SCAN_H__

#include "Entrance.h"

void Scan(int dpi, int depth, struct ImageSize *image);
void *SendPicture(void* jpeg);

#endif
