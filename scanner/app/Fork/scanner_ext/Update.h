#ifndef __UPDATE_H__
#define __UPDATE_H__

#define MAGIC_NUMBER 20190428
#define UPDATE_IMG "update.img"

struct ImageHeader {
    unsigned int magic;     // Magic number.
    unsigned int length;    // The whole image file.
    unsigned int check;     // CRC32 check, SectionHeader + data.
    unsigned int section;   // The number of section.
};

struct SectionHeader {
    unsigned int length;    // The length of the valid file.
    unsigned int permission;// File permission
    char name[64];
};

unsigned int GetCRC32CheckSum(unsigned char *pchMessage, unsigned int dwLength);
void UpdateDecompress(unsigned char *buffer, unsigned int length);
unsigned int UpdateCheckImageHeader(unsigned char *buffer, unsigned int length);
void UpdateWriteSection(struct SectionHeader *sectionHeader, unsigned char *data);

bool Update();

#endif
