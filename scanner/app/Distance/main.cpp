#include <iostream>
#include <fcntl.h>
#include <queue>
#include <unistd.h>

#include "main.h"
#include "VideoPort.h"
#include "Entrance.h"
#include "ColorRemap.h"
#include "PrinterDevice.h"
#include "PrinterHost.h"
#include "Switch.h"
#include "Light.h"
#include "Correction.h"

using namespace std;

int main(int argc, char *argv[]) {
    int dpi = 200;
    char color = 'C';
    int videoPortOffset = -4;
    char backdoor = 0;

    switch (argc) {
        case 5:
            backdoor = argv[4][0];
        case 4:
            videoPortOffset = (int) strtol(argv[3], nullptr, 10);
        case 3:
            color = argv[2][0];
        case 2:
            dpi = (int) strtol(argv[1], nullptr, 10);
        default:
            break;
    }

    switch (dpi) {
        case 200:
        case 250:
        case 300:
        case 600:
            break;
        default:
            cout << "Input dpi error." << endl;
            return -1;
    }

    switch (color) {
        case 'c':
        case 'C':
            color = 'C';
            break;
        case 'g':
        case 'G':
            color = 'G';
            break;
        case 'i':
        case 'I':
            color = 'I';
            break;
        default:
            cout << "Input color error." << endl;
            return -1;
    }

    if (labs(videoPortOffset) > 100) {
        cout << "Input offset error." << endl;
        return -1;
    }

    switch (backdoor) {
        case 's':
        case 'S':
            backdoor = 'S'; // Scan
            break;
        case 'd':
        case 'D':
            backdoor = 'D'; // Dormancy
            break;
        case 'u':
        case 'U':
            backdoor = 'U'; // Update
            break;
        case 0:
            break;
        default:
            cout << "Input backdoor error." << endl;
            return -1;
    }

    enum ExitEvent ret = MainProcess(dpi, color, videoPortOffset, backdoor);
    switch (ret) {
        case EXIT_EVENT_UPDATE_OK:
            cout << "Update successfully." << endl;
            break;
        case EXIT_EVENT_UPDATE_FAILED:
            cout << "Update unsuccessfully." << endl;
            break;
        case EXIT_EVENT_MODULES_NOT_INSTALLED:
            cout << "Modules not installed." << endl;
            break;
        default:
            cout << "Unknown exit event." << endl;
            break;
    }
    return 0;
}

enum ExitEvent MainProcess(int dpi, char color, int videoPortOffset, char backdoor)
{
    /**
      * Modules Initialization.
      */
    VideoPort videoPort;
    videoPort.Open(VIDEO_PORT_PATH, O_RDWR);
    if (!videoPort.IsOpen())
        return EXIT_EVENT_MODULES_NOT_INSTALLED;

    videoPort.WriteConfigPara();
    videoPort.WriteScanPara(dpi, color);
    videoPort.SetVideoMode(VIDEO_MODE_NORMAL);
    /* light and correction parameters */
    struct Light light = LightAdjustmentLoad(dpi, color);
    videoPort.WriteLightPara(light);
    struct Correction correction = CorrectionAdjustLoad(dpi, color);
    videoPort.WriteCorrectionPara(correction);
    videoPort.SetVideoMode(VIDEO_MODE_GRADIENT);
    videoPort.Close();

    /**
      * Applications Initialization.
      */
    cout << "-------Olicom-------" << endl;
    cout << "Initial mode, dpi: " << dpi << " color: " << color << " offset: " << videoPortOffset << endl;

    SetScannerMode(dpi, color, videoPortOffset);
    SetColorMap(COLOR_MAP_ORIGIN);

    /**
      * Transform data between PC and printer, prepare to scan.
      */
    PrinterDevice printerDevice;
    printerDevice.Open(PRINTER_DEVICE_PATH, O_RDWR);
    if (!printerDevice.IsOpen())
        return EXIT_EVENT_MODULES_NOT_INSTALLED;

    PrinterHost printerHost;
    printerHost.Open(PRINTER_HOST_PATH, O_RDWR);
    if (!printerHost.IsOpen())
        return EXIT_EVENT_MODULES_NOT_INSTALLED;

    ssize_t retval;
    struct switchRequest *req, *scanReq = nullptr;
    queue<struct switchRequest *> tx, rx, idle;
    for (int i = 0; i < SWITCH_REQUEST_NUM; i++) {
        req = new struct switchRequest;
        SwitchRequestInit(req, SWITCH_REQUEST_LEN);
        idle.push(req);
    }

    while (true) {
        /**
         * transform data from PC to printer
         */
        if (!idle.empty()) {
            req = idle.front();
            idle.pop();
            retval = printerDevice.Read(req->buffer, SWITCH_REQUEST_LEN);

            if (retval > 0) {
                req->length = retval;
                req->position = req->buffer;
                //SwitchRequestShow(req);
                tx.push(req);
            }
            else
                idle.push(req);
        }

        if (!tx.empty()) {
            req = tx.front();
            retval = printerHost.Write(req->position, (unsigned long)req->length);

            if (retval > 0) {
                req->length -= retval;
                req->position += retval;
            }
            if (req->length == 0) {
                req->position = nullptr;
                tx.pop();
                idle.push(req);
            }
        }

        /**
         * transform data from printer to PC, and check whether to activate scanner
         */
        if (!idle.empty()) {
            req = idle.front();
            idle.pop();
            retval = printerHost.Read(req->buffer, SWITCH_REQUEST_LEN);

            if (retval > 0) {
                req->length = retval;
                req->position = req->buffer;
                SwitchRequestShow(req);
                if (SwitchRequestScanner(req))
                    scanReq = req;
                else
                    rx.push(req);
            }
            else
                idle.push(req);
        }

        if (!rx.empty()) {
            req = rx.front();
            retval = printerDevice.Write(req->position, (unsigned long)req->length);

            if (retval > 0) {
                req->length -= retval;
                req->position += retval;
            }
            if (req->length == 0) {
                req->position = nullptr;
                rx.pop();
                idle.push(req);
            }
        }

        /**
         * activate scanner
         */
        if (scanReq || backdoor) {
            enum ExitEvent event;
            printerDevice.Close();
            printerHost.Close();

            if (scanReq) {
                event = ActivateScanner(scanReq->position, (int)scanReq->length);
                idle.push(scanReq);
            }
            else
                event = BackDoor(backdoor);

            scanReq = nullptr;
            backdoor = 0;
            if (event == EXIT_EVENT_COMMAND_ERROR)
                cout << "Scanner command error.\n" << endl;
            else if (event != EXIT_EVENT_NOTHING)
                return event;

            printerDevice.Open(PRINTER_DEVICE_PATH, O_RDWR);
            printerHost.Open(PRINTER_HOST_PATH, O_RDWR);
        }
    }
}

enum ExitEvent BackDoor(char backdoor) {
    enum ExitEvent event;
    if (backdoor == 'S') {
        unsigned char buffer[22] = {
                0x01, 0x0B, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x06, 0xC0, 0x09, 0x60,
                0x00, 0x00, 0x00, 0x00, 0x06, 0xC0, 0x09, 0x60,
                0x02
        };
        event =  ActivateScanner(buffer, sizeof(buffer));
    }
    else if (backdoor == 'D') {
        unsigned char buffer[5] = {
                0x01, 0x14, 0x00, 0x00, 0x00
        };
        event =  ActivateScanner(buffer, sizeof(buffer));
    }
    else if (backdoor == 'U') {
        unsigned char buffer[5] = {
                0x01, 0x15, 0x00, 0x00, 0x00
        };
        event =  ActivateScanner(buffer, sizeof(buffer));
    }
    else
        event = EXIT_EVENT_COMMAND_ERROR;

    return event;
}
