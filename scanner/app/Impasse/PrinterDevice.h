#ifndef __PRINTER_DEVICE_H__
#define __PRINTER_DEVICE_H__

#include "Module.h"

#define PRINTER_NOT_ERROR 0x08
#define PRINTER_SELECTED 0x10
#define PRINTER_PAPER_EMPTY 0x20

#define GET_PRINTER_STATUS _IOR('w', 0x21, unsigned char)
#define SET_PRINTER_STATUS _IOWR('w', 0x22, unsigned char)

class PrinterDevice : public Module {
public:
    PrinterDevice() = default;
    ~PrinterDevice() override = default;
    int GetStatus();
    int SetStatus(unsigned char status);
    void ShowStatus();

    int Poll(short int events, int time);
    void ReadAndSave(const char *file, int cut);
    void LoadAndWrite(const char *file, int cut);
};

#endif
