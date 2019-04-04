#include <iostream>
#include <unistd.h>

#include "Light.h"
#include "Correction.h"
#include "Entrance.h"
#include "Scan.h"
#include "ColorRemap.h"
#include "PrinterHost.h"
#include "Update.h"

char Scanner::color_;
int Scanner::dpi_;
int Scanner::depth_;
int Scanner::videoPortOffset_;

void Scanner::DeployCommand(unsigned char data) {
    switch (data) {
        case 0x08:
            cmd_ = SCANNER_COMMAND_DARK_SAMPLE;
            break;
        case 0x09:
            cmd_ = SCANNER_COMMAND_BRIGHT_SAMPLE;
            break;
        case 0x0B:
            cmd_ = SCANNER_COMMAND_SCAN;
            break;
        case 0x0D:
            cmd_ = SCANNER_COMMAND_ADJUST_EXPOSURE;
            break;
        case 0x0E:
            cmd_ = SCANNER_COMMAND_ADJUST_BRIGHTNESS;
            break;
        case 0x0F:
            cmd_ = SCANNER_COMMAND_GET_VERSION;
            break;
        case 0x13:
            cmd_ = SCANNER_COMMAND_SWITCH_MODE;
            break;
        case 0x14:
            cmd_ = SCANNER_COMMAND_SLEEP;
            break;
        case 0x15:
            cmd_ = SCANNER_COMMAND_UPDATE;
            break;
        default:
            cmd_ = SCANNER_COMMAND_IDLE;
            break;
    }
}

void Scanner::SetCompressPage(unsigned char data) {
    switch (data) {
        case 0x01:
            image_[0].page = PAGE_OBVERSE_SIDE;
            image_[1].page = PAGE_NULL;
            break;
        case 0x02:
            image_[0].page = PAGE_OBVERSE_SIDE;
            image_[1].page = PAGE_OPPOSITE_SIDE;
            break;
        case 0x03:
            image_[0].page = PAGE_NULL;
            image_[1].page = PAGE_OPPOSITE_SIDE;
            break;
        default:
            image_[0].page = PAGE_NULL;
            image_[1].page = PAGE_NULL;
            break;
    }
}

void Scanner::SetCompressEdge(const unsigned char *data) {
    int minUpEdge = 0, maxDownEdge = 0, minLeftEdge = 0, maxRightEdge = 0;

    switch (dpi_) {
        case 200:
        case 250:
            minUpEdge = 1;
            maxDownEdge = IMAGE_HEIGHT_200DPI;
            minLeftEdge = CIS_EDGE_200DPI;
            maxRightEdge = CIS_EDGE_200DPI + IMAGE_WIDTH_200DPI;
            break;
        case 300:
            minUpEdge = 1;
            maxDownEdge = IMAGE_HEIGHT_300DPI;
            minLeftEdge = CIS_EDGE_300DPI;
            maxRightEdge = CIS_EDGE_300DPI + IMAGE_WIDTH_300DPI;
            break;
        case 600:
            minUpEdge = 1;
            maxDownEdge = IMAGE_HEIGHT_600DPI;
            minLeftEdge = CIS_EDGE_600DPI;
            maxRightEdge = CIS_EDGE_600DPI + IMAGE_WIDTH_600DPI;
            break;
        default:
            break;
    }

    if (image_[0].page) {
        image_[0].upEdge = (data[2] << 8) + data[3];
        image_[0].downEdge = (data[6] << 8) + data[7];
        image_[0].leftEdge = (data[0] << 8) + data[1];
        image_[0].rightEdge = (data[4] << 8) + data[5];
    }

    if (image_[1].page) {
        image_[1].upEdge = (data[10] << 8) + data[11];
        image_[1].downEdge = (data[14] << 8) + data[15];
        image_[1].leftEdge = (data[8] << 8) + data[9];
        image_[1].rightEdge = (data[12] << 8) + data[13];
    }

    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        if (image_[i].page) {
            image_[i].upEdge = LIMIT_MIN_MAX(image_[i].upEdge, minUpEdge, maxDownEdge);
            image_[i].downEdge = LIMIT_MIN_MAX(image_[i].downEdge, image_[i].upEdge, maxDownEdge);
            image_[i].leftEdge = LIMIT_MIN_MAX(image_[i].leftEdge, minLeftEdge, maxRightEdge) + videoPortOffset_;
            image_[i].rightEdge = LIMIT_MIN_MAX(image_[i].rightEdge, image_[i].leftEdge, maxRightEdge) + videoPortOffset_;
            image_[i].height = image_[i].downEdge - image_[i].upEdge;
            image_[i].width = image_[i].rightEdge - image_[i].leftEdge;
        }
    }
}

void Scanner::ShowCompressMessage() {
    std::cout << "-------Config-------" << std::endl
              << "-cmd: " << cmd_ << std::endl
              << "-dpi: " << dpi_ << std::endl
              << "-color: " << color_ << std::endl;
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        std::cout << "-------Image"<< image_[i].page <<"-------" << std::endl
                  << "-up: " << image_[i].upEdge << std::endl
                  << "-down: " << image_[i].downEdge << std::endl
                  << "-left: " << image_[i].leftEdge << std::endl
                  << "-right: " << image_[i].rightEdge << std::endl
                  << "-height: " << image_[i].height << std::endl
                  << "-width: " << image_[i].width << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}

void Scanner::SetMode(int dpi, char color, int videoPortOffset) {
    dpi_ = dpi;
    color_ = color;
    depth_ = (color_ == 'C') ? 3 : 1;
    if (!videoPortOffset)
        videoPortOffset_ = videoPortOffset;
}

bool Scanner::SetMode(unsigned char dpi_magic, unsigned char color_magic) {
    int dpi_temp = dpi_;
    char color_temp = color_;
    int depth_temp = depth_;

    switch(dpi_magic) {
        case 0:
            dpi_temp = 300;
            break;
        case 1:
            dpi_temp = 600;
            break;
        case 2:
            dpi_temp = 200;
            break;
        case 3:
            dpi_temp = 250;
            break;
        default:
            break;
    }

    switch(color_magic) {
        case 0:
            color_temp = 'G';
            depth_temp = 1;
            break;
        case 1:
            color_temp = 'C';
            depth_temp = 3;
            break;
        case 2:
            color_temp = 'I';
            depth_temp = 1;
            break;
        default:
            break;
    }

    if ((dpi_ == dpi_temp) && (color_temp == color_))
        return false;

    dpi_ = dpi_temp;
    color_ = color_temp;
    depth_ = depth_temp;
    return true;
}

enum ExitEvent Scanner::Activate(unsigned char *data, int size) {
    unsigned char ack = 0xff;
    unsigned char version[4] = {0x01, 0x00, 0x02, 0x00};
    enum ExitEvent event = EXIT_EVENT_COMMAND_ERROR;
    if (data[0] != 0x01)
        return event;
    DeployCommand(data[1]);

    if (cmd_ == SCANNER_COMMAND_SCAN) {
        if (size != 22)
            return event;
    }
    else {
        if (size != 5)
            return event;
    }
    event = EXIT_EVENT_NOTHING;

    switch (cmd_) {
        case SCANNER_COMMAND_SCAN:
            printf("Go to scan.\n");
            SetCompressPage(data[21]);
            SetCompressEdge(&data[5]);
            ShowCompressMessage();
            if ((dpi_ == 300) && (depth_ == 1))
                usleep(60*1000);
            else if ((dpi_ == 200) && (depth_ == 3))
                usleep(70*1000);
            else if ((dpi_ == 200) && (depth_ == 1))
                usleep(80*1000);
            Scan(dpi_, depth_, image_);
            break;

        case SCANNER_COMMAND_ADJUST_EXPOSURE:
            printf("Go to adjust exposure.\n");
            LightAdjust(dpi_, color_);
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_DARK_SAMPLE:
            printf("Go to do dark sample.\n");
            CorrectionAdjustNoPaper(dpi_, color_, videoPortOffset_);
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_BRIGHT_SAMPLE:
            printf("Go to do bright sample.\n");
            CorrectionAdjust(dpi_, color_, videoPortOffset_);
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_SWITCH_MODE:
            printf("Go to switch mode.\n");
            if(SetMode(data[3], data[4])) {
                VideoPortSetScanMode(dpi_, color_);
                ShowCompressMessage();
            }
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_ADJUST_BRIGHTNESS:
            printf("Go to adjust brightness.\n");
            SetColorMap(DeployColorMap(data[4]));
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_GET_VERSION:
            printf("Go to get version.\n");
            PrinterHostQuickSend(version, sizeof(version));
            break;

        case SCANNER_COMMAND_UPDATE:
            printf("Go to update.\n");
            if (Update())
                event = EXIT_EVENT_UPDATE_OK;
            else
                event = EXIT_EVENT_UPDATE_FAILED;
            PrinterHostQuickSend(&ack, 1);
            break;

        case SCANNER_COMMAND_SLEEP:
            printf("Go to sleep.\n");
            PrinterHostQuickSend(&ack, 1);
            system("echo standby > /sys/power/state");
            usleep(100*1000);
            break;

        case SCANNER_COMMAND_IDLE:
        default:
            event = EXIT_EVENT_COMMAND_ERROR;
            break;
    }
    return event;
}

void SetScannerMode(int dpi, char color, int videoPortOffset) {
    Scanner scanner;
    scanner.SetMode(dpi, color, videoPortOffset);
}

enum ExitEvent ActivateScanner(unsigned char* data, int size) {
    Scanner scanner;
    return scanner.Activate(data, size);
}