#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"

#define USBLP_PATH "/dev/usb/lp0"

int main() {
    int usblp = open(USBLP_PATH, O_RDWR);
    while (usblp < 0) {
        printf("usb lp0 not found.\n");
        usleep(400*1000);
        usblp = open(USBLP_PATH, O_RDWR);
    }

    int twoints[2] = {};
    ioctl(usblp, LPIOC_GET_VID_PID(sizeof(twoints)), twoints);
    int VID = twoints[0];
    int PID = twoints[1];

    struct deviceIDDesc deviceID;
    memset(deviceID.data, 0, USBLP_DEVICE_ID_SIZE);
    ioctl(usblp, LPIOC_GET_DEVICE_ID(USBLP_DEVICE_ID_SIZE), deviceID.data);
    deviceID.bytes = (deviceID.data[0] << 8) | deviceID.data[1];
    deviceID.str = &deviceID.data[2];

    struct stringDesc manufacturerString;
    memset(manufacturerString.data, 0, USBLP_STRING_SIZE);
    manufacturerString.size = ioctl(usblp, LPIOC_GET_MANUFACTURER_STRING(USBLP_STRING_SIZE), manufacturerString.data);
    manufacturerString.str = &manufacturerString.data[0];

    struct stringDesc productString;
    memset(productString.data, 0, USBLP_STRING_SIZE);
    productString.size = ioctl(usblp, LPIOC_GET_PRODUCT_STRING(USBLP_STRING_SIZE), productString.data);
    productString.str = &productString.data[0];

    struct stringDesc serialString;
    memset(serialString.data, 0, USBLP_STRING_SIZE);
    serialString.size = ioctl(usblp, LPIOC_GET_SERIAL_STRING(USBLP_STRING_SIZE), serialString.data);
    serialString.str = &serialString.data[0];

    printf("Printer Information:\n"
                   "VID: 0x%.4x, PID: 0x%.4x\n"
                   "Device ID, received bytes: %d, string: %s\n"
                   "Manufacturer String, size: %d, string: %s\n"
                   "Product String, size: %d, string: %s\n"
                   "Serial String, size: %d, string: %s\n\n",
           VID, PID,
           deviceID.bytes, deviceID.str,
           manufacturerString.size, manufacturerString.str,
           productString.size, productString.str,
           serialString.size, serialString.str);

    char command[1024];
    memset(command, 0, 1024);
    sprintf(command,
            "insmod printer_device.ko idVendor=0x%.4x idProduct=0x%.4x iManufacturer=\'\"%s\"\' "
                    "iProduct=\'\"%s\"\' iSerialNumber=\'\"%s\"\' iPNPstring=\'\"%s\"\'",
            VID, PID, manufacturerString.str, productString.str, serialString.str, deviceID.str);

    //printf("command:\n %s\n\n", command);
    fflush(stdout);
    usleep(10*1000);
    system(command);
    close(usblp);
    return 0;
}