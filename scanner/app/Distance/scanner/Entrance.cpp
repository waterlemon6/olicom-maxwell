#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

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
int Scanner::quality_;

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
        case 0x0C:
            cmd_ = SCANNER_COMMAND_MULTISCAN;
            break;
        case 0x0D:
            cmd_ = SCANNER_COMMAND_ADJUST_EXPOSURE;
            break;
        case 0x0E:
            cmd_ = SCANNER_COMMAND_ADJUST_QUALITY;
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

void Scanner::SetMultiCompressPage(const unsigned char *data, unsigned int length) {
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        image_[i].page = PAGE_NULL;
    }

    for (unsigned int i = 0, j = 0; i < length; i += 9, j++) {
        if (data[i] == 0x01)
            image_[j].page = PAGE_OBVERSE_SIDE;
        else if (data[i] == 0x03)
            image_[j].page = PAGE_OPPOSITE_SIDE;
    }
}

void Scanner::SetMultiCompressEdge(const unsigned char *data, unsigned int length) {
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

    for (unsigned int i = 1, j = 0; i < length; i += 9, j++) {
        if (image_[j].page) {
            image_[j].upEdge = (data[i+2] << 8) + data[i+3];
            image_[j].downEdge = (data[i+6] << 8) + data[i+7];
            image_[j].leftEdge = (data[i+0] << 8) + data[i+1];
            image_[j].rightEdge = (data[i+4] << 8) + data[i+5];
        }
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
        if (!image_[i].page)
            continue;

        std::cout << "-------Image"<< i <<"-------" << std::endl
                  << "-page: " << image_[i].page << std::endl
                  << "-up: " << image_[i].upEdge << std::endl
                  << "-down: " << image_[i].downEdge << std::endl
                  << "-left: " << image_[i].leftEdge << std::endl
                  << "-right: " << image_[i].rightEdge << std::endl
                  << "-height: " << image_[i].height << std::endl
                  << "-width: " << image_[i].width << std::endl;
    }
    std::cout << "--------------------" << std::endl;
}

void Scanner::SetMode(int dpi, char color, int videoPortOffset, int quality) {
    dpi_ = dpi;
    color_ = color;
    depth_ = (color_ == 'C') ? 3 : 1;
    quality_ = quality;
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
            //dpi_temp = 250;
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
    unsigned char version[4] = {0x01, 0x01, 0x04, 0x03};
    enum ExitEvent event = EXIT_EVENT_COMMAND_ERROR;
    if (data[0] != 0x01)
        return event;
    DeployCommand(data[1]);

    if (cmd_ == SCANNER_COMMAND_SCAN) {
        if (size != 22)
            return event;
    }
    else if (cmd_ == SCANNER_COMMAND_MULTISCAN) {
        if ((size-5) % 9 != 0)
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
            SetExtiCount(0);
            SetCompressPage(data[21]);
            SetCompressEdge(&data[5]);
            ShowCompressMessage();
            while (!GetExtiCount())
                usleep(2000);
            Scan(dpi_, depth_, image_, quality_, 0);
            break;

        case SCANNER_COMMAND_MULTISCAN:
            printf("Go to multi-scan.\n");
            SetExtiCount(0);
            SetMultiCompressPage(&data[5], (unsigned int)(size - 9));
            SetMultiCompressEdge(&data[5], (unsigned int)(size - 9));
            ShowCompressMessage();
            while (!GetExtiCount())
                usleep(2000);
            Scan(dpi_, depth_, image_, quality_, 1);
            break;

        case SCANNER_COMMAND_ADJUST_EXPOSURE:
            printf("Go to adjust exposure.\n");
            LightAdjust(dpi_, color_);
            break;

        case SCANNER_COMMAND_DARK_SAMPLE:
            printf("Go to do dark sample.\n");
            CorrectionAdjustNoPaper(dpi_, color_, videoPortOffset_);
            break;

        case SCANNER_COMMAND_BRIGHT_SAMPLE:
            printf("Go to do bright sample.\n");
            CorrectionAdjust(dpi_, color_, videoPortOffset_);
            break;

        case SCANNER_COMMAND_SWITCH_MODE:
            printf("Go to switch mode.\n");
            if(SetMode(data[3], data[4])) {
                VideoPortSetScanMode(dpi_, color_);
                ShowCompressMessage();
            }
            break;

        case SCANNER_COMMAND_ADJUST_QUALITY:
            printf("Go to adjust quality.\n");
            //SetColorMap(DeployColorMap(data[4]), data[2]);
            SetColorMap(DeployColorMap(data[4]), data[2]);
            quality_ = data[3] < 100 ? data[3] : 100;
            break;

        case SCANNER_COMMAND_GET_VERSION:
            printf("Go to get version %d.%d.%d.%d .\n", version[0], version[1], version[2], version[3]);
            PrinterHostQuickSend(version, sizeof(version));
            break;

        case SCANNER_COMMAND_UPDATE:
            printf("Go to update.\n");
            if (Update())
                event = EXIT_EVENT_UPDATE_OK;
            else
                event = EXIT_EVENT_UPDATE_FAILED;
            break;

        case SCANNER_COMMAND_SLEEP:
            printf("Go to sleep.\n");
            usleep(100*1000);
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

bool Scanner::GetCorrectionParaEdge(unsigned char* data, int size, unsigned char *edge) {
    if (data[0] != 0x01 || size != 5)
        return false;

    DeployCommand(data[1]);
    if (cmd_ != SCANNER_COMMAND_BRIGHT_SAMPLE || dpi_ != 300 || color_ != 'C')
        return false;

    unsigned short edgePerCh[12] = {};
    if (!CorrectionGetEdge(dpi_, color_, edgePerCh))
        return false;

    unsigned short edgeMax[4] = {};
    edgeMax[0] = edgePerCh[0] > edgePerCh[4] ? edgePerCh[0] : edgePerCh[4];
    edgeMax[0] = edgeMax[0] > edgePerCh[8] ? edgeMax[0] : edgePerCh[8];
    edgeMax[1] = edgePerCh[1] < edgePerCh[5] ? edgePerCh[1] : edgePerCh[5];
    edgeMax[1] = edgeMax[1] < edgePerCh[9] ? edgeMax[1] : edgePerCh[9];
    edgeMax[2] = edgePerCh[2] > edgePerCh[6] ? edgePerCh[2] : edgePerCh[6];
    edgeMax[2] = edgeMax[2] > edgePerCh[10] ? edgeMax[2] : edgePerCh[10];
    edgeMax[3] = edgePerCh[3] < edgePerCh[7] ? edgePerCh[3] : edgePerCh[7];
    edgeMax[3] = edgeMax[3] < edgePerCh[11] ? edgeMax[3] : edgePerCh[11];

    edgeMax[0] += videoPortOffset_;
    edgeMax[1] += videoPortOffset_;
    edgeMax[2] += videoPortOffset_;
    edgeMax[3] += videoPortOffset_;

    unsigned short cisEdge = CIS_EDGE_300DPI;
    unsigned short cisWidth = CIS_WIDTH_300DPI;
    edgeMax[0] = edgeMax[0] > cisEdge ? edgeMax[0] : cisEdge;
    edgeMax[1] = edgeMax[1] < cisWidth - cisEdge ? edgeMax[1] : cisWidth - cisEdge;
    edgeMax[2] = edgeMax[2] > cisEdge ? edgeMax[2] : cisEdge;
    edgeMax[3] = edgeMax[3] < cisWidth - cisEdge ? edgeMax[3] : cisWidth - cisEdge;

    edge[0] = (unsigned char)((edgeMax[0] >> 8) & 0xff);
    edge[1] = (unsigned char)(edgeMax[0] & 0xff);
    edge[2] = (unsigned char)((edgeMax[1] >> 8) & 0xff);
    edge[3] = (unsigned char)(edgeMax[1] & 0xff);
    edge[4] = (unsigned char)((edgeMax[2] >> 8) & 0xff);
    edge[5] = (unsigned char)(edgeMax[2] & 0xff);
    edge[6] = (unsigned char)((edgeMax[3] >> 8) & 0xff);
    edge[7] = (unsigned char)(edgeMax[3] & 0xff);

    std::cout << "edge R: " << edgePerCh[0] << "  " << edgePerCh[1] << "  " << edgePerCh[2] << "  " << edgePerCh[3] << "  " << std::endl
              << "edge G: " << edgePerCh[4] << "  " << edgePerCh[5] << "  " << edgePerCh[6] << "  " << edgePerCh[7] << "  " << std::endl
              << "edge B: " << edgePerCh[8] << "  " << edgePerCh[9] << "  " << edgePerCh[10] << "  " << edgePerCh[11] << "  " << std::endl
              << "edge max: " << edgeMax[0] << "  " << edgeMax[1] << "  " << edgeMax[2] << "  " << edgeMax[3] << "  " << std::endl;
    return true;
}

void SetScannerMode(int dpi, char color, int videoPortOffset) {
    Scanner scanner;
    scanner.SetMode(dpi, color, videoPortOffset);
}

enum ExitEvent ActivateScanner(unsigned char* data, int size) {
    Scanner scanner;
    return scanner.Activate(data, size);
}

void SetExtiCount(int count) {
    int desc = open(EXTI_PATH, O_RDWR);
    if (desc < 0)
        return;

    char buffer[64];
    sprintf(buffer, "%d", count);
    write(desc, buffer, strlen(buffer));
    close(desc);
}

int GetExtiCount() {
    int count = 0;
    int desc = open(EXTI_PATH, O_RDWR);
    if (desc < 0)
        return 1;

    char buffer[64];
    read(desc, buffer, sizeof(buffer));
    close(desc);
    count = (int) strtol(buffer, nullptr, 10);
    if (count)
        printf("Get valid exti count: %d.\n", count);
    return count;
}

bool GetScannerCorrectionParaEdge(unsigned char* data, int size, unsigned char *edge) {
    Scanner scanner;
    return scanner.GetCorrectionParaEdge(data, size, edge);
}
