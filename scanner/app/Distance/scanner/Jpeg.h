#ifndef __JPEG_H__
#define __JPEG_H__

#include <cstdio>
#include "/home/waterlemon/jpeg/libjpeg-turbo-2.0.1/a64-64bit/a64/jpeglib.h"

enum JpegCompressState {
    JPEG_COMPRESS_STATE_IDLE,
    JPEG_COMPRESS_STATE_SCANNING,
    JPEG_COMPRESS_STATE_FINISHED
};

class Jpeg {
private:
    int depth_;
    int dpi_;
    JDIMENSION width_;
    JDIMENSION height_;
    unsigned char *dst_;
    unsigned long length_;
    unsigned char header_;
    enum JpegCompressState step_;
    struct jpeg_compress_struct comp_;
    struct jpeg_error_mgr err_;
public:
    explicit Jpeg(int dpi = 200, int depth = 3);
    ~Jpeg();

    void SetAttr(int dpi, int depth) { dpi_ = dpi; depth_ = depth; }
    void SetSize(JDIMENSION width, JDIMENSION height) { width_ = width; height_ = height; }
    void SetHeader(unsigned char header) { header_ = header; }
    unsigned char GetHeader() { return header_; }
    unsigned long GetLength() { return length_; };
    unsigned char* GetDst() { return dst_; };
    enum JpegCompressState GetState() { return step_; }

    int StartCompress();
    void WriteScanLines(unsigned char *src, unsigned int lines = 1);
    void FinishCompress();
    void UpdateCompress();
};

#endif
