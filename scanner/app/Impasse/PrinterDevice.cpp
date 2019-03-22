#include <cstdio>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include "PrinterDevice.h"

int PrinterDevice::GetStatus() {
    int retval = ioctl(GetDescriptor(), GET_PRINTER_STATUS);
    if (retval < 0) {
        printf("ERROR: Failed to get printer status\n");
        return -1;
    }
    return retval;
}

int PrinterDevice::SetStatus(unsigned char status) {
    if (ioctl(GetDescriptor(), SET_PRINTER_STATUS, status)) {
        printf("ERROR: Failed to set printer status\n");
        return -1;
    }
    return 0;
}

void PrinterDevice::ShowStatus() {
    int status = GetStatus();
    if (status < 0)
        return;
    printf("Printer status is:\n");

    if (status & PRINTER_SELECTED)
        printf("    Printer is Selected\n");
    else
        printf("    Printer is NOT Selected\n");

    if (status & PRINTER_PAPER_EMPTY)
        printf("    Paper is Out\n");
    else
        printf("    Paper is Loaded\n");

    if (status & PRINTER_NOT_ERROR)
        printf("    Printer OK\n");
    else
        printf("    Printer ERROR\n");
}

int PrinterDevice::Poll(short int events, int time) {
    struct pollfd fd[1];
    fd[0].fd = GetDescriptor();
    fd[0].events = events;

    int retval = poll(fd, 1, time);
    if (retval && (fd[0].revents & events))
        return 1;

    return 0;
}

void PrinterDevice::ReadAndSave(const char *file, const int cut) {
    FILE *stream = fopen(file, "wb");
    unsigned char buffer[cut];

    while (true) {
        if (Poll(POLLIN | POLLRDNORM, 1000)) {
            ssize_t bytes = read(GetDescriptor(), buffer, (size_t) cut);
            if(bytes > 0)
                fwrite(buffer, 1, static_cast<size_t>(bytes), stream);

            printf("Package size: %ld.\n", bytes);
            if (bytes < cut)
                break;
        }
    }

    fflush(stream);
    fclose(stream);
}

void PrinterDevice::LoadAndWrite(const char *file, const int cut) {
    FILE *stream = fopen(file, "rb");
    unsigned char buffer[cut];

    while (true) {
        if (Poll(POLLOUT | POLLWRNORM, 1000)) {
            size_t bytes = fread(buffer, 1, (size_t) cut, stream);
            if(bytes > 0)
                write(GetDescriptor(), buffer, bytes);

            printf("Package size: %ld.\n", bytes);
            if (bytes < cut)
                break;
        }
    }

    fsync(GetDescriptor());
    fclose(stream);
}
