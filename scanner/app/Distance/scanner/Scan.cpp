#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <queue>
#include <cstring>

#include "main.h"
#include "Scan.h"
#include "Jpeg.h"
#include "VideoPort.h"
#include "VideoCore.h"
#include "PrinterDevice.h"
#include "ColorRemap.h"

using namespace std;

pthread_t threadSendPicture;
pthread_mutex_t mutexSendPicture[IMAGE_MAX_NUM];

void Scan(int dpi, int depth, struct ImageSize *image)
{
    /**
      * Calculate and check maximum scan lines.
      */
    int maxScanLines = 0;
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        if (image[i].downEdge > maxScanLines)
            maxScanLines = image[i].downEdge;
    }
    if (!maxScanLines) {
        cout << "Maximum scan lines error." << endl;
        return;
    }

    /**
      * Prepare to scan.
      */
    VideoPort videoPort;
    videoPort.Open(VIDEO_PORT_PATH, O_RDWR);
    if (!videoPort.IsOpen())
        return;

    VideoCore videoCore;
    videoCore.SetAttr(dpi, depth);
    videoCore.Activate(videoPort.GetOriginImagePos());

    videoPort.StartScan((unsigned short)videoCore.GetFrame());
    videoCore.UpdateFrame();

    Jpeg jpeg[IMAGE_MAX_NUM];
    int dpi_jpeg = (dpi == 250) ? 200 : dpi;
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        jpeg[i].SetAttr(dpi_jpeg, depth);
        jpeg[i].SetSize((JDIMENSION)image[i].width, (JDIMENSION)image[i].height);
        jpeg[i].SetHeaderTag(HeaderTag(image[i].page));
        jpeg[i].StartCompress();
        pthread_mutex_init(&mutexSendPicture[i], nullptr);
    }
    pthread_create(&threadSendPicture, nullptr, SendPicture, jpeg);

    /**
      * Extract, compress images row by row.
      */
    int state = 1;
    unsigned char *line;
    queue <unsigned char*> imageSlice[IMAGE_MAX_NUM];
    auto arrange = new unsigned char[VIDEO_PORT_WIDTH * 3];

    while (state) {
        videoCore.UpdateScanLines();
        int scanLines = videoCore.GetScanLines();
        int shiftLines = videoCore.GetShiftLines();

        while (scanLines > shiftLines + 60) {
            if (shiftLines >= maxScanLines) {
                state = 0;
                break;
            }

            for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
                if (image[i].page) {
                    if ((shiftLines >= image[i].upEdge) && (shiftLines < image[i].downEdge)) {
                        line = new unsigned char [VIDEO_PORT_WIDTH * 3];
                        videoCore.ExtractImageSlice(line, VIDEO_PORT_WIDTH, image[i].page);
                        imageSlice[i].push(line);
                    }
                }
            }

            videoCore.UpdateShiftLines();
            shiftLines = videoCore.GetShiftLines();
            videoCore.UpdateFrame();
        }

        while (videoCore.GetScanLines() - scanLines < 60) {
            for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
                if (image[i].page) {
                    if (!imageSlice[i].empty()) {
                        line = imageSlice[i].front();
                        videoCore.colorRemap(line, arrange, depth, image[i].leftEdge, image[i].rightEdge, VIDEO_PORT_WIDTH);
                        pthread_mutex_lock(&mutexSendPicture[i]);
                        jpeg[i].WriteScanLines(arrange);
                        pthread_mutex_unlock(&mutexSendPicture[i]);
                        imageSlice[i].pop();
                        delete [] line;
                    }
                }
            }
            videoCore.UpdateScanLines();
        }
    }

    /**
      * Compress the remaining images.
      */
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        if (image[i].page) {
            while (!imageSlice[i].empty()) {
                line = imageSlice[i].front();
                videoCore.colorRemap(line, arrange, depth, image[i].leftEdge, image[i].rightEdge, VIDEO_PORT_WIDTH);
                pthread_mutex_lock(&mutexSendPicture[i]);
                jpeg[i].WriteScanLines(arrange);
                pthread_mutex_unlock(&mutexSendPicture[i]);
                imageSlice[i].pop();
                delete [] line;
            }
            jpeg[i].FinishCompress();
            cout << "Image" << i+1 << " length: " << jpeg[i].GetLength() << endl;
        }
    }

    /**
      * Exit.
      */
    pthread_join(threadSendPicture, nullptr);
    videoPort.StopScan();
    videoPort.Close();
    delete [] arrange;
}

void *SendPicture(void* jpeg) {
    ssize_t retval;
    auto *pJpeg = (Jpeg *)jpeg;
    const unsigned int delayTime = 1000; // us
    unsigned long sendLength[IMAGE_MAX_NUM] = {};

    const unsigned long headerSize = 1;
    const unsigned long dataSize = 4000;
    unsigned char pack[headerSize + dataSize];

    PrinterDevice printerDevice;
    printerDevice.Open(PRINTER_DEVICE_PATH, O_RDWR);
    if (!printerDevice.IsOpen()) {
        cout << "Open device error in thread SendPicture." << endl;
        return nullptr;
    }

    cout << "Hello world, here is thread SendPicture." << endl;

    /**
      * Before jpeg compress finished.
      */
    while (true) {
        unsigned int count = 0;
        for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
            if (pJpeg[i].GetState() != JPEG_COMPRESS_STATE_SCANNING)
                count++;
        }
        if (count == IMAGE_MAX_NUM)
            break;

        for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
            if (pJpeg[i].GetState() == JPEG_COMPRESS_STATE_SCANNING) {
                pthread_mutex_lock(&mutexSendPicture[i]);
                pJpeg[i].UpdateCompress();
                if (pJpeg[i].GetLength() > sendLength[i] + dataSize) {
                    Pack(pJpeg[i].GetDst() + sendLength[i], pack, dataSize, pJpeg[i].GetHeaderTag(), headerSize);
                    retval = printerDevice.Write(pack, headerSize + dataSize);
                    if (retval > headerSize)
                        sendLength[i] += (retval - headerSize);
                }
                pthread_mutex_unlock(&mutexSendPicture[i]);
            }
        }
        usleep(delayTime);
    }

    /**
      * After jpeg compress finished.
      */
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        if (pJpeg[i].GetState() == JPEG_COMPRESS_STATE_FINISHED) {
            while (pJpeg[i].GetLength() > sendLength[i] + dataSize) {
                Pack(pJpeg[i].GetDst() + sendLength[i], pack, dataSize, pJpeg[i].GetHeaderTag(), headerSize);
                retval = printerDevice.Write(pack, headerSize + dataSize);
                if (retval > headerSize)
                    sendLength[i] += (retval - headerSize);
                usleep(delayTime);
            }

            /* The last package. */
            while (pJpeg[i].GetLength() > sendLength[i]) {
                Pack(pJpeg[i].GetDst() + sendLength[i], pack, pJpeg[i].GetLength() - sendLength[i], pJpeg[i].GetHeaderTag(), headerSize);
                retval = printerDevice.Write(pack, headerSize + pJpeg[i].GetLength() - sendLength[i]);
                if (retval > headerSize)
                    sendLength[i] += (retval - headerSize);
                usleep(delayTime);
            }
        }
    }

    /**
      * Exit.
      */
    printerDevice.Close();
#if SAVE_PICTURE_IN_FILE
    ofstream stream;
    for (unsigned int i = 0; i < IMAGE_MAX_NUM; i++) {
        char name[20];
        sprintf(name, "picture%d.jpg", i + 1);
        stream.open(name);
        stream.write((char*)pJpeg[i].GetDst(), pJpeg[i].GetLength());
        stream.close();
    }
#endif
}

unsigned char HeaderTag(enum Page page) {
    if (page == PAGE_OBVERSE_SIDE)
        return 0xFA;
    else if (page == PAGE_OPPOSITE_SIDE)
        return 0xFB;
    else
        return 0x00;
}

void Pack(unsigned char *src, unsigned char *dst, unsigned long dataSize, unsigned char headerTag, unsigned long headerSize) {
    dst[0] = headerTag;
    memcpy(dst + headerSize, src, dataSize);
}