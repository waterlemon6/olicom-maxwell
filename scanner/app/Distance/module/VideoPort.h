#ifndef __VIDEO_PORT_H__
#define __VIDEO_PORT_H__

#include "Module.h"

#define VIDEO_PORT_WRITE_REG                _IOW('w', 0, unsigned int)
#define VIDEO_PORT_GET_IMAGE_OFFSET         _IOR('w', 1, unsigned int)
#define VIDEO_PORT_RESUME                   _IO('w', 2)

/* register list*/
#define FPGA_FREQ_REG   1024
#define FPGA_R1_REG     1026
#define FPGA_R2_REG     1027
#define FPGA_G1_REG     1028
#define FPGA_G2_REG     1029
#define FPGA_B1_REG     1030
#define FPGA_B2_REG     1031
#define FPGA_IR1_REG    1032
#define FPGA_IR2_REG    1033
#define FPGA_COLOR_REG  1034
#define FPGA_DPI_REG    1035
#define FPGA_HEIGHT_REG 1036
#define FPGA_VD_REG     1037
#define FPGA_SCAN_REG   1551
#define FPGA_MODE_REG   1552
#define FPGA_CORRS_REG  2048
#define FPGA_CORRE_REG  2049

#define ADC_GAIN1_REG   1536
#define ADC_GAIN2_REG   1537
#define ADC_GAIN3_REG   1538
#define ADC_OFFSET1_REG 1539
#define ADC_OFFSET2_REG 1540
#define ADC_OFFSET3_REG 1541
#define ADC_GAIN4_REG   1542
#define ADC_GAIN5_REG   1543
#define ADC_GAIN6_REG   1544
#define ADC_OFFSET4_REG 1545
#define ADC_OFFSET5_REG 1546
#define ADC_OFFSET6_REG 1547
#define ADC_SYSTEM_REG  1548
#define ADC_AFE_REG     1549
#define ADC_START_REG   1550

/* dpi */
#define FPGA_DPI_200 1
#define FPGA_DPI_300 2
#define FPGA_DPI_600 3

/* color */
#define FPGA_COLOR_C 6 // color
#define FPGA_COLOR_G 1 // gray
#define FPGA_COLOR_I 2 // ir

/* frequency */
#define FPGA_FREQ_200D1 1333 // 200dpi 1depth
#define FPGA_FREQ_200D3 1333 // 200dpi 3depth
#define FPGA_FREQ_300D1 2000 // 300dpi 1depth
#define FPGA_FREQ_300D3 2000 // 300dpi 3depth
#define FPGA_FREQ_600D1 4000 // 600dpi 1depth
#define FPGA_FREQ_600D3 4000 // 600dpi 3depth

/* used when scan */
#define SCAN_TIME_PER_LINE_200D1 (1000000 / (8000000 / FPGA_FREQ_200D1))
#define SCAN_TIME_PER_LINE_200D3 (3 * 1000000 / (8000000 / FPGA_FREQ_200D3))
#define SCAN_TIME_PER_LINE_300D1 (1000000 / (8000000 / FPGA_FREQ_300D1))
#define SCAN_TIME_PER_LINE_300D3 (3 * 1000000 / (8000000 / FPGA_FREQ_300D3))
#define SCAN_TIME_PER_LINE_600D1 (1000000 / (8000000 / FPGA_FREQ_600D1))
#define SCAN_TIME_PER_LINE_600D3 (3 * 1000000 / (8000000 / FPGA_FREQ_600D3))

/* video mode */
enum VideoMode {
    VIDEO_MODE_NORMAL,
    VIDEO_MODE_NO_CORRECTION,
    VIDEO_MODE_GRADIENT
};

#define FPGA_MODE_NORMAL 1
#define FPGA_MODE_NOCORR 0
#define FPGA_MODE_GRADIENT 3

/* channel */
enum Channel {
    CHANNEL_R = 0x01,
    CHANNEL_G = 0x02,
    CHANNEL_B = 0x04,
    CHANNEL_GRAY = 0x08,
    CHANNEL_IR = 0x10,
};

#define FPGA_CHANNEL_R 201
#define FPGA_CHANNEL_G 202
#define FPGA_CHANNEL_B 203
#define FPGA_CHANNEL_IR 204
#define FPGA_CHANNEL_GRAY 205

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
