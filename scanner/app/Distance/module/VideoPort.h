#ifndef __VIDEO_PORT_H__
#define __VIDEO_PORT_H__

#include "Module.h"

#define VIDEO_PORT_WRITE_REG                _IOW('w', 0, unsigned int)
#define VIDEO_PORT_GET_IMAGE_OFFSET         _IOR('w', 1, unsigned int)
#define VIDEO_PORT_RESUME                   _IO('w', 2)

enum VideoMode {
    VIDEO_MODE_NORMAL,
    VIDEO_MODE_NO_CORRECTION,
    VIDEO_MODE_GRADIENT
};

enum Channel {
    CHANNEL_R = 0x01,
    CHANNEL_G = 0x02,
    CHANNEL_B = 0x04,
    CHANNEL_GRAY = 0x08,
    CHANNEL_IR = 0x10,
};

struct Light {
    bool enable;
    unsigned short r1;
    unsigned short g1;
    unsigned short b1;
    unsigned short ir1;
    unsigned short r2;
    unsigned short g2;
    unsigned short b2;
    unsigned short ir2;
};

struct Correction {
    bool enable;
    unsigned short channel;
    unsigned short width;
    unsigned char *RK; // K -> Bright sample space
    unsigned char *RB; // B -> Dark sample space
    unsigned char *GK;
    unsigned char *GB;
    unsigned char *BK;
    unsigned char *BB;
};

class VideoPort : public Module {
private:
    unsigned char *origin_ = nullptr;
    void WriteReg(unsigned short address, unsigned short value);
    void WriteData(unsigned char *data, unsigned int size);
    void WriteCorrectionChannel(unsigned char *srcK, unsigned char *srcB, int length, enum Channel channel);
public:
    VideoPort() = default;
    ~VideoPort() override;
    void SetVideoMode(enum VideoMode mode);
    unsigned char *GetOriginImagePos();
    void StartScan(unsigned short VD);
    void StopScan();

    void WriteConfigPara();
    void WriteScanPara(int dpi, char color);
    void WriteLightPara(struct Light light);
    void WriteCorrectionPara(struct Correction correction);

    void Resume();
};

void VideoPortSetScanMode(int dpi, char color);

#endif
