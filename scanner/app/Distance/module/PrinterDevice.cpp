#include <cstdio>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include "main.h"
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

ssize_t PrinterDevice::Write(unsigned char *data, unsigned long size) {
    ssize_t retval = -1;
    if (Poll(POLLOUT | POLLWRNORM, POLL_TIMEOUT)) { // ms
        retval = write(GetDescriptor(), data, size);
    }
    return retval;
}

ssize_t PrinterDevice::Read(unsigned char *data, unsigned long size) {
    ssize_t retval = -1;
    if (Poll(POLLIN | POLLRDNORM, POLL_TIMEOUT)) { // ms
        retval = read(GetDescriptor(), data, size);
    }
    return retval;
}
