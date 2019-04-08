#ifndef __MAIN_H__
#define __MAIN_H__

/* Configure */
#define SAVE_PICTURE_IN_FILE 1

#define VIDEO_PORT_PATH "/dev/wl_video"
#define PRINTER_DEVICE_PATH "/dev/wl_printer"
#define PRINTER_HOST_PATH "/dev/usb/lp0"

#define SWITCH_REQUEST_NUM 20
#define SWITCH_REQUEST_LEN 4000

#define POLL_TIMEOUT 4 //ms

/* The width and height of the video port, defined in video module. */
#define VIDEO_PORT_WIDTH        5184
#define VIDEO_PORT_HEIGHT       8000
#define VIDEO_PORT_FRAME_HEIGHT 7800

/* The width and edge of the cis. CIS_WIDTH = CIS_EDGE + CIS_AVAILABLE_WIDTH + CIS_EDGE. */
#define CIS_WIDTH_200DPI        1728
#define CIS_EDGE_200DPI         14

#define CIS_WIDTH_300DPI        2592
#define CIS_EDGE_300DPI         20

#define CIS_WIDTH_600DPI        5184
#define CIS_EDGE_600DPI         41

/* Available width per page, decided by cis and video port.
   Available height per page, decided by me. */
#define IMAGE_WIDTH_200DPI  (CIS_WIDTH_200DPI - 2 * CIS_EDGE_200DPI)
#define IMAGE_HEIGHT_200DPI (VIDEO_PORT_HEIGHT / 3)

#define IMAGE_WIDTH_300DPI  (CIS_WIDTH_300DPI - 2 * CIS_EDGE_300DPI)
#define IMAGE_HEIGHT_300DPI ((VIDEO_PORT_HEIGHT + VIDEO_PORT_FRAME_HEIGHT) / 3)

#define IMAGE_WIDTH_600DPI  (CIS_WIDTH_600DPI - 2 * CIS_EDGE_600DPI)
#define IMAGE_HEIGHT_600DPI ((VIDEO_PORT_HEIGHT + 5 * VIDEO_PORT_FRAME_HEIGHT) / 3)

#define LIMIT_MIN_MAX(x, min, max) (((x)<=(min)) ? (min) : (((x)>=(max))?(max):(x)))

enum ExitEvent {
    EXIT_EVENT_NOTHING = 0,
    EXIT_EVENT_UPDATE_OK,
    EXIT_EVENT_UPDATE_FAILED,
    EXIT_EVENT_MODULES_NOT_INSTALLED,
    EXIT_EVENT_COMMAND_ERROR,
    EXIT_EVENT_PRINTER_NOT_CONNECTED
};

enum ExitEvent MainProcess(int dpi, char color, int videoPortOffset, char backdoor);
enum ExitEvent BackDoor(char backdoor);

#endif
