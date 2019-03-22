#include <cstdio>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include "PrinterDevice.h"

//#define USE_QUEUE

#ifdef USE_QUEUE
#include <queue>
#endif

#define BUF_SIZE 512
PrinterDevice printerDevice;
pthread_t thread;

static int read_printer_data()
{
    static char buf[BUF_SIZE];
    ssize_t bytes_read;
    while (true) {
        if(printerDevice.Poll(POLLIN | POLLRDNORM, 1000)) {
            bytes_read = read(printerDevice.GetDescriptor(), buf, BUF_SIZE);

            if (bytes_read < 0) {
                printf("Error in reading data from printer device.\n");
                return -1;
            }
            else if (bytes_read > 0) {
                /* Write data to standard OUTPUT (stdout). */
                fwrite(buf, 1, static_cast<size_t>(bytes_read), stdout);
                fflush(stdout);
            }
        }
    }
}

static void get_set_status()
{
    printerDevice.ShowStatus();
    printerDevice.SetStatus(PRINTER_NOT_ERROR | PRINTER_SELECTED | PRINTER_PAPER_EMPTY);
    printerDevice.ShowStatus();
}

static int write_printer_data()
{
    static char buf[BUF_SIZE];
    while (true) {
        int retval;
        /* Read data from standard INPUT (stdin). */
        printf("User input:");
        auto bytes_read = fread(buf, 1, 10, stdin);

        if (!bytes_read)
            break;

        while (bytes_read) {
            if(printerDevice.Poll(POLLOUT | POLLWRNORM, 1000)) {
                retval = static_cast<int>(write(printerDevice.GetDescriptor(), buf, bytes_read));

                if (retval < 0) {
                    printf("Error in writing data to printer device.\n");
                    return(-1);
                }
                else
                    bytes_read -= retval;
            }
        }
    }
    return 0;
}

void usage()
{
    printf("-------Help-------\n");
    printf("-q: Quit.\n");
    printf("-r: Read.\n");
    printf("-s: Status.\n");
    printf("-w: Write.\n");
    printf("-\n");
    printf("-1: Read and save.\n");
    printf("-2: Load and write.\n");
    printf("-3: Read and push queue.\n");
    printf("------------------\n");
}

struct package {
    unsigned char *data;
    ssize_t size;
};

#define PRINTER_BUFFER_SIZE 8192
void *ThreadSendData()
{
    printf("Hello everyone.\n");
#ifdef USE_QUEUE
    std::queue <struct package> rx;
    struct package pack {};

    while (true) {
        if (printerDevice.Poll(POLLIN | POLLRDNORM, 1000)) {
            pack.data = new unsigned char [PRINTER_BUFFER_SIZE];
            pack.size = read(printerDevice.GetDescriptor(), pack.data, PRINTER_BUFFER_SIZE);

            printf("Package size: %ld.\n", pack.size);
            rx.push(pack);
            if (pack.size < PRINTER_BUFFER_SIZE)
                break;
        }
    }

    FILE *stream = fopen("temp", "wb");
    while (!rx.empty()) {
        pack = rx.front();
        fwrite(pack.data, 1, static_cast<size_t>(pack.size), stream);
        rx.pop();
    }
    fflush(stream);
    fclose(stream);
#endif
}

int main()
{
    printerDevice.Open("/dev/wl_printer", O_RDWR);
    if(!printerDevice.IsOpen())
        return -1;

    printf("Hello,world.\n");
    char key = 0;
    while (true) {
        printf("User command:");
        scanf("%c", &key);
        getchar();

        switch (key) {
            case 'h':
                usage();
                break;
            case 'q':
                printf("Quit.\n");
                break;
            case 'r':
                read_printer_data();
                break;
            case 's':
                get_set_status();
                break;
            case 'w':
                write_printer_data();
                break;

            case '1':
                printf("Read data and save as #execute#.\n");
                printerDevice.ReadAndSave("execute", 8000);
                break;
            case '2':
                printf("Load #picture# and write data.\n");
                printerDevice.LoadAndWrite("picture", 8000);
                break;
            case '3':
                printf("Read data and save as #temp#.\n");
                pthread_create(&thread, nullptr, reinterpret_cast<void *(*)(void *)>(ThreadSendData), nullptr);
                pthread_join(thread, nullptr);
                break;
            default:
                printf("No such command, maybe show help via h.\n");
                break;
        }

        if(key == 'q')
            break;
    }

    printerDevice.Close();
    return 0;
}