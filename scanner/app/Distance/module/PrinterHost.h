#ifndef __PRINTER_HOST_H__
#define __PRINTER_HOST_H__

#include "Module.h"

class PrinterHost : public Module {
public:
    PrinterHost() = default;
    ~PrinterHost() override = default;

    int Poll(short int events, int time);
    ssize_t Write(unsigned char *data, unsigned long size);
    ssize_t Read(unsigned char *data, unsigned long size);
};

void PrinterHostQuickSend(unsigned char *data, unsigned long size);

#endif
