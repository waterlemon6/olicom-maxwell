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
            cmd = 201;
            break;
        case CHANNEL_G:
            cmd = 202;
            break;
        case CHANNEL_B:
            cmd = 203;
            break;
        case CHANNEL_GRAY:
            cmd = 205;
            break;
        case CHANNEL_IR:
            cmd = 204;
            break;
        default:
            break;
    }

    WriteReg(2048, cmd);
    WriteData(srcK, (unsigned int)length);
    WriteData(srcB, (unsigned int)length);
    WriteReg(2049, 100);
}

void VideoPort::SetVideoMode(enum VideoMode mode) {
    switch (mode) {
        case VIDEO_MODE_NORMAL:
            WriteReg(1552, 1);
            break;
        case VIDEO_MODE_NO_CORRECTION:
            WriteReg(1552, 0);
            break;
        case VIDEO_MODE_GRADIENT:
            WriteReg(1552, 3);
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
    WriteReg(1037, VD);
    WriteReg(1551, 1);
}

void VideoPort::StopScan() {
    WriteReg(1551, 0);
}

void VideoPort::WriteConfigPara() {
    WriteReg(0, 0);
    WriteReg(1024, 1333);//frequency
    WriteReg(1026, 1000);//R1
    WriteReg(1027, 1000);//R2
    WriteReg(1028, 800);//G1
    WriteReg(1029, 800);//G2
    WriteReg(1030, 600);//B1
    WriteReg(1031, 600);//B2
    WriteReg(1032, 800);//IR1 light
    WriteReg(1033, 800);//IR2 light
    WriteReg(1034, 6);//color space
    WriteReg(1035, 1);//dpi
    WriteReg(1036, 7200);//height, no effect
    WriteReg(1536, 150);//R1 gain, 0~255 ?
    WriteReg(1537, 150);//G1 gain
    WriteReg(1538, 150);//B1 gain
    WriteReg(1539, 127);//R1 offset, -128~127 ?
    WriteReg(1540, 127);//G1 offset
    WriteReg(1541, 127);//B1 offset
    WriteReg(1542, 150);//R2 gain
    WriteReg(1543, 150);//G2 gain
    WriteReg(1544, 150);//B2 gain
    WriteReg(1545, 127);//R2 offset
    WriteReg(1546, 127);//G2 offset
    WriteReg(1547, 127);//B2 offset
    WriteReg(1548, 15);//AD system
    WriteReg(1549, 0);//AD AFE
    WriteReg(1550, 1);//AD start
}

void VideoPort::WriteScanPara(int dpi, char color) {
    unsigned short frequency = 0;

    switch (dpi) {
        case 200:
            dpi = 1;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 1333;
                    break;
                case 'G':
                    color = 1;
                    frequency = 1333;
                    break;
                case 'I':
                    color = 2;
                    frequency = 1333;
                    break;
                default:
                    break;
            }
            break;
        case 300:
            dpi = 2;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 2000;
                    break;
                case 'G':
                    color = 1;
                    frequency = 2000;
                    break;
                case 'I':
                    color = 2;
                    frequency = 2000;
                    break;
                default:
                    break;
            }
            break;
        case 600:
            dpi = 3;
            switch (color) {
                case 'C':
                    color = 6;
                    frequency = 4000;
                    break;
                case 'G':
                    color = 1;
                    frequency = 4000;
                    break;
                case 'I':
                    color = 2;
                    frequency = 4000;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    WriteReg(1024, frequency);
    WriteReg(1034, (unsigned short) color);
    WriteReg(1035, (unsigned short) dpi);
}

void VideoPort::WriteLightPara(struct Light light) {
    if (!light.enable)
        return;

    WriteReg(1026, light.r2);
    WriteReg(1027, light.r1);
    WriteReg(1028, light.g2);
    WriteReg(1029, light.g1);
    WriteReg(1030, light.b2);
    WriteReg(1031, light.b1);
    WriteReg(1032, light.ir2);
    WriteReg(1033, light.ir1);
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
    if (correction.enable)
        videoPort.SetVideoMode(VIDEO_MODE_NORMAL);
    else
        videoPort.SetVideoMode(VIDEO_MODE_NO_CORRECTION);
    videoPort.WriteCorrectionPara(correction);

    videoPort.Close();
}
