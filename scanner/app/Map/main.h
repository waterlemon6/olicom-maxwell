#ifndef MAP_MAIN_H
#define MAP_MAIN_H

#define USBLP_DEVICE_ID_SIZE	1024
#define USBLP_STRING_SIZE		1024

#define IOCNR_GET_DEVICE_ID						1
#define IOCNR_GET_PROTOCOLS						2
#define IOCNR_SET_PROTOCOL						3
#define IOCNR_HP_SET_CHANNEL					4
#define IOCNR_GET_BUS_ADDRESS					5
#define IOCNR_GET_VID_PID						6
#define IOCNR_SOFT_RESET						7
#define IOCNR_GET_MANUFACTURER_STRING	        8
#define IOCNR_GET_PRODUCT_STRING			    9
#define IOCNR_GET_SERIAL_STRING				    10
#define IOCNR_GET_STATUS						11

/* Get device_id string: */
#define LPIOC_GET_DEVICE_ID(len)                _IOC(_IOC_READ, 'P', IOCNR_GET_DEVICE_ID, len)
/* The following ioctls were added for http://hpoj.sourceforge.net: */
/* Get two-int array:
 * [0]=current protocol (1=7/1/1, 2=7/1/2, 3=7/1/3),
 * [1]=supported protocol mask (mask&(1<<n)!=0 means 7/1/n supported): */
#define LPIOC_GET_PROTOCOLS(len)                _IOC(_IOC_READ, 'P', IOCNR_GET_PROTOCOLS, len)
/* Set protocol (arg: 1=7/1/1, 2=7/1/2, 3=7/1/3): */
#define LPIOC_SET_PROTOCOL                      _IOC(_IOC_WRITE, 'P', IOCNR_SET_PROTOCOL, 0)
/* Set channel number (HP Vendor-specific command): */
#define LPIOC_HP_SET_CHANNEL                    _IOC(_IOC_WRITE, 'P', IOCNR_HP_SET_CHANNEL, 0)
/* Get two-int array: [0]=bus number, [1]=device address: */
#define LPIOC_GET_BUS_ADDRESS(len)              _IOC(_IOC_READ, 'P', IOCNR_GET_BUS_ADDRESS, len)
/* Get two-int array: [0]=vendor ID, [1]=product ID: */
#define LPIOC_GET_VID_PID(len)                  _IOC(_IOC_READ, 'P', IOCNR_GET_VID_PID, len)
/* Perform class specific soft reset */
#define LPIOC_SOFT_RESET                        _IOC(_IOC_NONE, 'P', IOCNR_SOFT_RESET, 0);
/* Get string descriptor */
#define LPIOC_GET_MANUFACTURER_STRING(len)      _IOC(_IOC_READ, 'P', IOCNR_GET_MANUFACTURER_STRING, len)
#define LPIOC_GET_PRODUCT_STRING(len)           _IOC(_IOC_READ, 'P', IOCNR_GET_PRODUCT_STRING, len)
#define LPIOC_GET_SERIAL_STRING(len)            _IOC(_IOC_READ, 'P', IOCNR_GET_SERIAL_STRING, len)
/* Get status */
#define LPIOC_GET_STATUS(len)                   _IOC(_IOC_READ, 'P', IOCNR_GET_STATUS, len)

struct deviceIDDesc {
    int bytes;
    unsigned char *str;
    unsigned char data[USBLP_DEVICE_ID_SIZE];
};

struct stringDesc {
    int size;
    unsigned char *str;
    unsigned char data[USBLP_STRING_SIZE];
};

#endif
