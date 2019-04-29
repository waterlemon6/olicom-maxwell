#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Compress.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Input error.\n");
        return -1;
    }

    char *header = argv[1];
    char *data = argv[2];
    printf("Header File: %s.\n", header);
    printf("Data File: %s.\n", data);

    FILE *update_file = fopen(UPDATE_IMG, "wb+");
    FILE *header_file = fopen(header, "rb");
    FILE *data_file = fopen(data, "rb");
    if ((!update_file) || (!header_file) || (!data_file)) {
        printf("Open file error.\n");
        return -1;
    }

    /* step1, image header. */
    struct ImageHeader imageHeader = {};
    fwrite(&imageHeader, sizeof(imageHeader), 1, update_file);

    /* step2, section header. */
    struct SectionHeader sectionHeader = {};
    char length_buffer[64];
    char permission_buffer[64];
    while(fscanf(header_file, "%s %s %s", sectionHeader.name, length_buffer, permission_buffer) > 0) {
        sectionHeader.length = (unsigned int) strtol(length_buffer, NULL, 10);
        sectionHeader.permission = (unsigned int) strtol(permission_buffer, NULL, 8);
        printf("Input: %s %d %o.\n", sectionHeader.name, sectionHeader.length, sectionHeader.permission);
        fwrite(&sectionHeader, sizeof(sectionHeader), 1, update_file);

        imageHeader.section++;
        memset(sectionHeader.name, 0, sizeof(sectionHeader.name));
    }

    /* step3, data. */
    fseek(data_file, 0, SEEK_END);
    size_t data_length = (size_t) ftell(data_file);
    fseek(data_file, 0, SEEK_SET);
    unsigned char *data_buffer = malloc(data_length);
    fread(data_buffer, data_length, 1, data_file);
    fwrite(data_buffer, data_length, 1, update_file);
    free(data_buffer);

    /* step4, fix image header. */
    fseek(update_file, 0, SEEK_END);
    size_t update_length = (size_t) ftell(update_file);
    fseek(update_file, 0, SEEK_SET);
    unsigned char *update_buffer = malloc(update_length);
    fread(update_buffer, update_length, 1, update_file);

    struct ImageHeader *fix = (struct ImageHeader *)update_buffer;
    fix->magic = MAGIC_NUMBER;
    fix->length = (unsigned int) update_length;
    fix->check = GetCRC32CheckSum(update_buffer + sizeof(*fix), fix->length - sizeof(*fix));
    fix->section = imageHeader.section;
    fseek(update_file, 0, SEEK_SET);
    fwrite(update_buffer, update_length, 1 ,update_file);
    free(update_buffer);

    /* exit */
    fclose(data_file);
    fclose(header_file);
    fclose(update_file);
    return 0;
}