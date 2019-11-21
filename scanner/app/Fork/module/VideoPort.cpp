#include <iostream>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "VideoPort.h"
#include "main.h"
#include "Light.h"
#include "Correction.h"

VideoPort::~VideoPort() {
    if (origin_) {
        munmap(origin_, VIDEO_PORT_WIDTH * VIDEO_PORT_HEIGHT);
        std::cout << "video port clear" << std::endl;
    }
}

void VideoPort::WriteReg(unsigned short address, unsigned short value) {
    unsigned int data = ((unsigned int)address << 16) | value;
    ioctl(GetDescriptor(), VIDEO_PORT_WRITE_REG, &data);
}

void VideoPort::WriteData(unsigned char *data, unsigned int size) {
    write(GetDescriptor(), data, size);
}

void VideoPort::WriteCorrectionChannel(unsigned char *srcK, unsigned char *srcB, int length, enum Channel channel) {
    unsigned short cmd = 0;

    switch (channel) {
        case CHANNEL_R:
            cmd = FPGA_CHANNEL_R;
            break;
        case CHANNEL_G:
            cmd = FPGA_CHANNEL_G;
            break;
        case CHANNEL_B:
            cmd = FPGA_CHANNEL_B;
            break;
        case CHANNEL_GRAY:
            cmd = FPGA_CHANNEL_GRAY;
            break;
        case CHANNEL_IR:
            cmd = FPGA_CHANNEL_IR;
            break;
        default:
            return;
    }

    /*for (int i = 0; i < length; i++) {
        srcK[i] = (unsigned char)((i+128) % 256);
        srcB[i] = (unsigned char)(i % 256);
    }*/

    WriteReg(FPGA_CORRS_REG, cmd);
    WriteData(srcK, (unsigned int)length);
    WriteData(srcB, (unsigned int)length);
    WriteReg(FPGA_CORRE_REG, 100);
}

void VideoPort::SetVideoMode(enum VideoMode mode) {
    switch (mode) {
        case VIDEO_MODE_NORMAL:
            WriteReg(FPGA_MODE_REG, FPGA_MODE_NORMAL);
            break;
        case VIDEO_MODE_NO_CORRECTION:
            WriteReg(FPGA_MODE_REG, FPGA_MODE_NOCORR);
            break;
        case VIDEO_MODE_GRADIENT:
            WriteReg(FPGA_MODE_REG, FPGA_MODE_GRADIENT);
            break;
        default:
            break;
    }
}

unsigned char *VideoPort::GetOriginImagePos() {
    if (!origin_) {
        origin_ = (unsigned char *)mmap(nullptr, VIDEO_PORT_WIDTH * VIDEO_PORT_HEIGHT, PROT_READ, MAP_SHARED, GetDescriptor(), 0);
        origin_ += ioctl(GetDescriptor(), VIDEO_PORT_GET_IMAGE_OFFSET);
    }
    return origin_;
}

void VideoPort::StartScan(unsigned short VD) {
    WriteReg(FPGA_VD_REG, VD);
    WriteReg(FPGA_SCAN_REG, 1);
}

void VideoPort::StopScan() {
    WriteReg(FPGA_SCAN_REG, 0);
}

void VideoPort::WriteConfigPara() {
    WriteReg(0, 0);
    WriteReg(FPGA_FREQ_REG, FPGA_FREQ_200D1);
    WriteReg(FPGA_R1_REG, 1000);
    WriteReg(FPGA_R2_REG, 1000);
    WriteReg(FPGA_G1_REG, 800);
    WriteReg(FPGA_G2_REG, 800);
    WriteReg(FPGA_B1_REG, 600);
    WriteReg(FPGA_B2_REG, 600);
    WriteReg(FPGA_IR1_REG, 800);
    WriteReg(FPGA_IR2_REG, 800);
    WriteReg(FPGA_COLOR_REG, FPGA_COLOR_C);
    WriteReg(FPGA_DPI_REG, FPGA_DPI_200);
    WriteReg(FPGA_HEIGHT_REG, 7200);
    WriteReg(ADC_GAIN1_REG, 250);
    WriteReg(ADC_GAIN2_REG, 250);
    WriteReg(ADC_GAIN3_REG, 250);
    WriteReg(ADC_OFFSET1_REG, 127);
    WriteReg(ADC_OFFSET2_REG, 127);
    WriteReg(ADC_OFFSET3_REG, 127);
    WriteReg(ADC_GAIN4_REG, 250);
    WriteReg(ADC_GAIN5_REG, 250);
    WriteReg(ADC_GAIN6_REG, 250);
    WriteReg(ADC_OFFSET4_REG, 127);
    WriteReg(ADC_OFFSET5_REG, 127);
    WriteReg(ADC_OFFSET6_REG, 127);
    WriteReg(ADC_SYSTEM_REG, 15);
    WriteReg(ADC_AFE_REG, 0);
    WriteReg(ADC_START_REG, 1);
}

void VideoPort::WriteScanPara(int dpi, char color) {
    unsigned short frequency = 0;

    switch (dpi) {
        case 200:
            dpi = FPGA_DPI_200;
            switch (color) {
                case 'C':
                    color = FPGA_COLOR_C;
                    frequency = FPGA_FREQ_200D3;
                    break;
                case 'G':
                    color = FPGA_COLOR_G;
                    frequency = FPGA_FREQ_200D1;
                    break;
                case 'I':
                    color = FPGA_COLOR_I;
                    frequency = FPGA_FREQ_200D1;
                    break;
                default:
                    break;
            }
            break;
        case 250:
            dpi = FPGA_DPI_300;
            switch (color) {
                case 'C':
                    color = FPGA_COLOR_C;
                    frequency = FPGA_FREQ_250D3;
                    break;
                case 'G':
                    color = FPGA_COLOR_G;
                    frequency = FPGA_FREQ_250D1;
                    break;
                case 'I':
                    color = FPGA_COLOR_I;
                    frequency = FPGA_FREQ_250D1;
                    break;
                default:
                    break;
            }
            break;
        case 300:
            dpi = FPGA_DPI_300;
            switch (color) {
                case 'C':
                    color = FPGA_COLOR_C;
                    frequency = FPGA_FREQ_300D3;
                    break;
                case 'G':
                    color = FPGA_COLOR_G;
                    frequency = FPGA_FREQ_300D1;
                    break;
                case 'I':
                    color = FPGA_COLOR_I;
                    frequency = FPGA_FREQ_300D1;
                    break;
                default:
                    break;
            }
            break;
        case 600:
            dpi = FPGA_DPI_600;
            switch (color) {
                case 'C':
                    color = FPGA_COLOR_C;
                    frequency = FPGA_FREQ_600D1;
                    break;
                case 'G':
                    color = FPGA_COLOR_G;
                    frequency = FPGA_FREQ_600D1;
                    break;
                case 'I':
                    color = FPGA_COLOR_I;
                    frequency = FPGA_FREQ_600D3;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    WriteReg(FPGA_FREQ_REG, frequency);
    WriteReg(FPGA_COLOR_REG, (unsigned short) color);
    WriteReg(FPGA_DPI_REG, (unsigned short) dpi);
}

void VideoPort::WriteLightPara(struct Light light) {
    if (!light.enable)
        return;

    WriteReg(FPGA_R1_REG, light.r2);
    WriteReg(FPGA_R2_REG, light.r1);
    WriteReg(FPGA_G1_REG, light.g2);
    WriteReg(FPGA_G2_REG, light.g1);
    WriteReg(FPGA_B1_REG, light.b2);
    WriteReg(FPGA_B2_REG, light.b1);
    WriteReg(FPGA_IR1_REG, light.ir2);
    WriteReg(FPGA_IR2_REG, light.ir1);
}

void VideoPort::WriteCorrectionPara(struct Correction correction) {
    if (!correction.enable) {
        SetVideoMode(VIDEO_MODE_NO_CORRECTION);

        delete [] (correction.RK);
        delete [] (correction.RB);
        delete [] (correction.GK);
        delete [] (correction.GB);
        delete [] (correction.BK);
        delete [] (correction.BB);
        return;
    }

    SetVideoMode(VIDEO_MODE_NORMAL);

    if (correction.channel & CHANNEL_R) {
        WriteCorrectionChannel(correction.RK, correction.RB, correction.width, CHANNEL_R);
        if (correction.channel & CHANNEL_G)
            WriteCorrectionChannel(correction.GK, correction.GB, correction.width, CHANNEL_G);
        if (correction.channel & CHANNEL_B)
            WriteCorrectionChannel(correction.BK, correction.BB, correction.width, CHANNEL_B);
    }
    else if (correction.channel & CHANNEL_GRAY)
        WriteCorrectionChannel(correction.RK, correction.RB, correction.width, CHANNEL_GRAY);
    else if (correction.channel & CHANNEL_IR)
        WriteCorrectionChannel(correction.RK, correction.RB, correction.width, CHANNEL_IR);

    delete [] (correction.RK);
    delete [] (correction.RB);
    delete [] (correction.GK);
    delete [] (correction.GB);
    delete [] (correction.BK);
    delete [] (correction.BB);
}

void VideoPort::Resume() {
    ioctl(GetDescriptor(), VIDEO_PORT_RESUME);
}

void VideoPortSetScanMode(int dpi, char color) {
    VideoPort videoPort;
    videoPort.Open(VIDEO_PORT_PATH, O_RDWR);
    videoPort.WriteScanPara(dpi, color);

    struct Light light = LightAdjustmentLoad(dpi, color);
    videoPort.WriteLightPara(light);

    struct Correction correction = CorrectionAdjustLoad(dpi, color);
    /**
      * Extend begin. When update light and correction, cancel it.
      */
    GlobalCorrectionDeepCopy(correction, color);
    /**
      * Extend end.
      */
    if (correction.enable)
        videoPort.SetVideoMode(VIDEO_MODE_NORMAL);
    else
        videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
    videoPort.WriteCorrectionPara(correction);

    videoPort.Close();
}
