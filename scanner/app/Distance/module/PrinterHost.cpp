#include <cstdio>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include "main.h"
#include "PrinterHost.h"

int PrinterHost::Poll(short int events, int time) {
    struct pollfd fd[1];
    fd[0].fd = GetDescriptor();
    fd[0].events = events;

    int retval = poll(fd, 1, time);
    if (retval && (fd[0].revents & events))
        return 1;

    return 0;
}

ssize_t PrinterHost::Write(unsigned char *data, unsigned long size) {
    ssize_t retval = -1;
    if (Poll(POLLOUT | POLLWRNORM, POLL_TIMEOUT)) { // ms
        retval = write(GetDescriptor(), data, size);
    }
    return retval;
}

ssize_t PrinterHost::Read(unsigned char *data, unsigned long size) {
    ssize_t retval = -1;
    if (Poll(POLLIN | POLLRDNORM, POLL_TIMEOUT)) { // ms
        retval = read(GetDescriptor(), data, size);
    }
    return retval;
}

void PrinterHostQuickSend(unsigned char *data, unsigned long size)
{
    PrinterHost printerHost;
    printerHost.Open(PRINTER_HOST_PATH, O_RDWR);
    printerHost.Write(data, size);
    printerHost.Close();
}
