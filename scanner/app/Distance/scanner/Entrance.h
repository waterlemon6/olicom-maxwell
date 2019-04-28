#ifndef __ENTRANCE_H__
#define __ENTRANCE_H__

#include "main.h"

#define IMAGE_MAX_NUM 2

enum ScannerCommand {
    SCANNER_COMMAND_IDLE = 0,
    SCANNER_COMMAND_SCAN,
    SCANNER_COMMAND_ADJUST_EXPOSURE,
    SCANNER_COMMAND_DARK_SAMPLE,
    SCANNER_COMMAND_BRIGHT_SAMPLE,
    SCANNER_COMMAND_SWITCH_MODE,
    SCANNER_COMMAND_ADJUST_BRIGHTNESS,
    SCANNER_COMMAND_GET_VERSION,
    SCANNER_COMMAND_UPDATE,
    SCANNER_COMMAND_SLEEP
};

enum Page {
    PAGE_NULL = 0,
    PAGE_OBVERSE_SIDE,
    PAGE_OPPOSITE_SIDE
};

struct ImageSize {
    enum Page page;
    int leftEdge;
    int rightEdge;
    int upEdge;
    int downEdge;
    int width;
    int height;
};

class Scanner {
private:
    static char color_;
    static int dpi_;
    static int depth_;
    static int videoPortOffset_;

    enum ScannerCommand cmd_ = SCANNER_COMMAND_IDLE;
    struct ImageSize image_[IMAGE_MAX_NUM] {};

    void DeployCommand(unsigned char data);
    void SetCompressPage(unsigned char data);
    void SetCompressEdge(const unsigned char *data);
    void ShowCompressMessage();
public:
    Scanner() = default;
    ~Scanner() = default;
    void SetMode(int dpi, char color, int videoPortOffset = 0);
    bool SetMode(unsigned char dpi_magic, unsigned char color_magic);
    enum ExitEvent Activate(unsigned char* data, int size);
};

void SetScannerMode(int dpi, char color, int maxScanLines);
enum ExitEvent ActivateScanner(unsigned char* data, int size);

void SetExtiCount(int count);
int GetExtiCount();

#endif
