#include <cstdlib>
#include "Jpeg.h"

Jpeg::Jpeg(int dpi, int depth) {
    depth_ = depth;
    dpi_ = dpi;
    width_ = 0;
    height_ = 0;
    dst_ = nullptr;
    length_ = 0;
    header_ = 0;
    step_ = JPEG_COMPRESS_STATE_IDLE;
    comp_.err = jpeg_std_error(&err_);
    jpeg_create_compress(&comp_);
    jpeg_mem_dest(&comp_, &dst_, &length_);
}

Jpeg::~Jpeg() {
    jpeg_destroy_compress(&comp_);
    if(dst_)
        free(dst_);
}

int Jpeg::StartCompress() {
    if ((!width_) || (!height_))
        return -1;

    if (depth_ == 3) {
        comp_.input_components = 3;
        comp_.in_color_space = JCS_YCbCr;
    }
    else if (depth_ == 1) {
        comp_.input_components = 1;
        comp_.in_color_space = JCS_GRAYSCALE;
    }
    else
        return -1;

    comp_.image_width = width_;
    comp_.image_height = height_;
    jpeg_set_defaults(&comp_);
    comp_.density_unit = 1;
    comp_.X_density = (unsigned short)dpi_;
    comp_.Y_density = (unsigned short)dpi_;
    jpeg_set_quality(&comp_, 35, TRUE);
    comp_.dct_method = JDCT_FASTEST;
    jpeg_start_compress(&comp_, TRUE);
    step_ = JPEG_COMPRESS_STATE_SCANNING;
}

void Jpeg::WriteScanLines(unsigned char *src, unsigned int lines) {
    jpeg_write_scanlines(&comp_, &src, lines);
}

void Jpeg::FinishCompress() {
    jpeg_finish_compress(&comp_);
    step_ = JPEG_COMPRESS_STATE_FINISHED;
}

void Jpeg::UpdateCompress() {
    struct jpeg_destination_mgr jpegDstMgr {};
    jpegDstMgr = *comp_.dest;
    jpegDstMgr.term_destination(&comp_);
}
