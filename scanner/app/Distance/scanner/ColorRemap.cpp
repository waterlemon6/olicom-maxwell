#include <cmath>
#include <cstring>
#include <arm_neon.h>
#include "ColorRemap.h"

int Y_RTable[256], Y_GTable[256], Y_BTable[256];
int Cr_RTable[256], Cr_GTable[256], Cr_BTable[256];
int Cb_RTable[256], Cb_GTable[256], Cb_BTable[256];
unsigned char Y_YTable[256];

const unsigned char colorMap0[256] = {
        0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
        16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
        32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
        64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
        80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
        96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
        112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
        128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
        144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
        160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
        176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
        192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
        208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
        224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255,
};

const unsigned char colorMap1[256] = {
        0,   0,   1,   1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   4,   5,   5,
        6,   6,   7,   7,   7,   8,   8,   9,   9,  10,  10,  11,  11,  12,  12,  13,
        14,  14,  15,  15,  16,  17,  17,  18,  19,  19,  20,  21,  21,  22,  23,  23,
        24,  25,  26,  27,  27,  28,  29,  30,  31,  32,  33,  34,  34,  35,  36,  37,
        38,  39,  40,  41,  42,  43,  45,  46,  47,  48,  49,  50,  51,  52,  54,  55,
        56,  57,  59,  60,  61,  62,  64,  65,  66,  68,  69,  70,  72,  73,  75,  76,
        78,  79,  80,  82,  83,  85,  86,  88,  90,  91,  93,  94,  96,  97,  99, 100,
        102, 104, 105, 107, 109, 110, 112, 113, 115, 117, 118, 120, 122, 123, 125, 127,
        128, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 146, 148, 150, 151, 153,
        155, 156, 158, 159, 161, 162, 164, 165, 167, 169, 170, 172, 173, 175, 176, 177,
        179, 180, 182, 183, 185, 186, 187, 189, 190, 191, 193, 194, 195, 196, 198, 199,
        200, 201, 203, 204, 205, 206, 207, 208, 209, 210, 212, 213, 214, 215, 216, 217,
        218, 219, 220, 221, 221, 222, 223, 224, 225, 226, 227, 228, 228, 229, 230, 231,
        232, 232, 233, 234, 234, 235, 236, 236, 237, 238, 238, 239, 240, 240, 241, 241,
        242, 243, 243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 248, 249, 249,
        250, 250, 251, 251, 251, 252, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255
};

const unsigned char colorMap2[256] = {
        0,   0,   1,   1,   1,   2,   2,   2,   3,   3,   3,   4,   4,   4,   5,   5,
        6,   6,   7,   7,   8,   8,   9,   9,  10,  10,  11,  11,  12,  12,  13,  14,
        14,  15,  16,  16,  17,  18,  19,  19,  20,  21,  22,  23,  24,  24,  25,  26,
        27,  28,  29,  30,  31,  32,  33,  34,  36,  37,  38,  39,  40,  42,  43,  44,
        45,  47,  48,  49,  51,  52,  54,  55,  57,  58,  60,  61,  63,  65,  66,  68,
        70,  71,  73,  75,  77,  78,  80,  82,  84,  86,  88,  89,  91,  93,  95,  97,
        99, 101, 103, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 130,
        132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162,
        164, 166, 167, 169, 171, 173, 175, 177, 178, 180, 182, 184, 185, 187, 189, 190,
        192, 194, 195, 197, 198, 200, 201, 203, 204, 206, 207, 208, 210, 211, 212, 213,
        215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
        231, 232, 233, 234, 235, 236, 236, 237, 238, 239, 239, 240, 241, 241, 242, 243,
        243, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251,
        251, 251, 252, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

void GrayScaleMapBuild(const unsigned char *map) {
    memcpy(Y_YTable, map, 256);
}

void LuminanceMapBuild(const unsigned char *map) {
    for (int i = 0; i < 256; i++) {
        Y_RTable[i] = (int) std::lround((float)map[i] * (0.299f * 65536.0f));
        Y_GTable[i] = (int) std::lround((float)map[i] * (0.587f * 65536.0f));
        Y_BTable[i] = (int) std::lround((float)map[i] * (0.114f * 65536.0f));
    }
}

void ChrominanceMapBuild() {
    for (int i = 0; i < 256; i++) {
        Cb_RTable[i] = (int) std::lround((float)i * (-0.169f * 65536.0f));
        Cb_GTable[i] = (int) std::lround((float)i * (-0.331f * 65536.0f));
        Cb_BTable[i] = (int) std::lround((float)i * (0.500f * 65536.0f) - (128 << 16));
        Cr_RTable[i] = (int) std::lround((float)i * (0.500f * 65536.0f));
        Cr_GTable[i] = (int) std::lround((float)i * (-0.419f * 65536.0f));
        Cr_BTable[i] = (int) std::lround((float)i * (-0.081f * 65536.0f) - (128 << 16));
    }
}

void SetColorMap(enum ColorMap level) {
    static bool init = false;
    const unsigned char *map = colorMap0;

    switch (level) {
        case COLOR_MAP_ORIGIN:
            map = colorMap0;
            break;
        case COLOR_MAP_BRIGHT_L1:
            map = colorMap1;
            break;
        case COLOR_MAP_BRIGHT_L2:
            map = colorMap2;
            break;
        default:
            break;
    }

    GrayScaleMapBuild(map);
    LuminanceMapBuild(map);
    if (!init) {
        ChrominanceMapBuild();
        init = true;
    }
}

enum ColorMap DeployColorMap(unsigned char data)
{
    switch (data) {
        case 1:
            return COLOR_MAP_BRIGHT_L1;
        case 2:
            return COLOR_MAP_BRIGHT_L2;
        default:
            return COLOR_MAP_ORIGIN;
    }
}

/**
  * @brief  Use color map to transform model and brightness of effective image data row by row.
  *         In single channel, each pixels changes its brightness (the only channel).
  *         In multiple channel, each pixels changes its model (BRG to YCbCr), structure (row by row to pixel by pixel)
  *         and brightness (Y channel).
  * @param  src: Src position to store image data.
  *         dst: Dst position to store image data.
  *         depth: Depth of images.
  *         leftEdge: Left edge of source images.
  *         rightEdge: Right edge of source images.
  *         offset: Offset of each channel in BRG model, only used in multiple channel.
  * @retval None.
  */
void ColorRemap1To1(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset) {
    unsigned char R, G, B;
    int srcCount = 0, dstCount = 0;
    int doubleOffset = offset * 2;

    if (depth == 3) {
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 1, dstCount += 3) {
            B = src[srcCount];
            R = src[srcCount + offset];
            G = src[srcCount + doubleOffset];

            dst[dstCount] = (unsigned char)((Y_RTable[R] + Y_GTable[G] + Y_BTable[B]) >> 16);
            dst[dstCount + 1] = (unsigned char)((Cb_RTable[R] + Cb_GTable[G] + Cb_BTable[B]) >> 16);
            dst[dstCount + 2] = (unsigned char)((Cr_RTable[R] + Cr_GTable[G] + Cr_BTable[B]) >> 16);
        }
    }
    else if (depth == 1) {
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 1, dstCount += 1) {
            dst[dstCount] = Y_YTable[src[srcCount]];
        }
    }
}

void ColorRemap1To1_NEON(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset) {
    int srcCount = 0, dstCount = 0;
    int doubleOffset = offset * 2;
    uint8x16x3_t s = {};

    if (depth == 3) { // no brightness adjust
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 16, dstCount += 48) {
            s.val[2] = vld1q_u8(src + srcCount); // B
            s.val[0] = vld1q_u8(src + srcCount + offset); // R
            s.val[1] = vld1q_u8(src + srcCount + doubleOffset); // G

            vst3q_u8(dst + dstCount, s);
        }
    }
    else if (depth == 1) {
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 1, dstCount += 1) {
            dst[dstCount] = Y_YTable[src[srcCount]];
        }
    }
}

void ColorRemap3To2(const unsigned char *src, unsigned char *dst, int depth, int leftEdge, int rightEdge, int offset)
{
    unsigned char R[3], G[3], B[3];
    int srcCount = 0, dstCount = 0;
    int doubleOffset = offset * 2;

    leftEdge = leftEdge * 3 / 2;
    rightEdge = rightEdge * 3 / 2;

    if (depth == 3) {
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 3, dstCount += 6) {
            for (int i = 0; i < 3; i++) {
                B[i] = src[srcCount + i];
                R[i] = src[srcCount + offset + i];
                G[i] = src[srcCount + doubleOffset + i];
            }

            dst[dstCount] = (unsigned char)((Y_RTable[R[0]] + Y_GTable[G[0]] + Y_BTable[B[0]]) >> 16);
            dst[dstCount + 1] = (unsigned char)((Cb_RTable[R[0]] + Cb_GTable[G[0]] + Cb_BTable[B[0]]) >> 16);
            dst[dstCount + 2] = (unsigned char)((Cr_RTable[R[0]] + Cr_GTable[G[0]] + Cr_BTable[B[0]]) >> 16);
            dst[dstCount + 3] = (unsigned char)((Y_RTable[R[1]] + Y_GTable[G[1]] + Y_BTable[B[1]]) >> 17) +
                                (unsigned char)((Y_RTable[R[2]] + Y_GTable[G[2]] + Y_BTable[B[2]]) >> 17);
            dst[dstCount + 4] = (unsigned char)((Cb_RTable[R[1]] + Cb_GTable[G[1]] + Cb_BTable[B[1]]) >> 17) +
                                (unsigned char)((Cb_RTable[R[2]] + Cb_GTable[G[2]] + Cb_BTable[B[2]]) >> 17);
            dst[dstCount + 5] = (unsigned char)((Cr_RTable[R[1]] + Cr_GTable[G[1]] + Cr_BTable[B[1]]) >> 17) +
                                (unsigned char)((Cr_RTable[R[2]] + Cr_GTable[G[2]] + Cr_BTable[B[2]]) >> 17);
        }
    }
    else if (depth == 1) {
        for (srcCount = leftEdge; srcCount < rightEdge; srcCount += 3, dstCount += 2) {
            dst[dstCount] = Y_YTable[src[srcCount]];
            dst[dstCount + 1] = (Y_YTable[src[srcCount + 1]] >> 1) + (Y_YTable[src[srcCount + 2]] >> 1);
        }
    }
}
