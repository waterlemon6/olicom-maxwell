#include <iostream>
#include "Switch.h"

void SwitchRequestInit(struct switchRequest *req, long len) {
    if (!req) {
        printf("Switch request error.\n");
        return;
    }

    req->buffer = new unsigned char [len];
    req->position = nullptr;
    req->length = 0;
}

void SwitchRequestShow(struct switchRequest *req) {
    if (!req) {
        printf("Switch request error.\n");
        return;
    }

    printf("req len: %ld, pos: %ld, data: ", req->length, req->position - req->buffer);
    for (long i = req->position - req->buffer; i < req->length; i++)
        printf("%.2x ", req->buffer[i]);
    printf("\n");
}

bool SwitchRequestScanner(struct switchRequest *req) {
    if (!req) {
        printf("Switch request error.\n");
        return false;
    }

    if (req->length <= 10)
        return false;

    unsigned char *head = &req->buffer[0];
    unsigned char *tail = &req->buffer[req->length - 5];
    if((head[0] == 0x01) && (head[1] == 0x0A) && (head[2] == 0x00) && (head[3] == 0x00) && (head[4] == 0x00))
        if((tail[0] == 0x01) && (tail[1] == 0x0B) && (tail[2] == 0x00) && (tail[3] == 0x00) && (tail[4] == 0x00)) {
            req->position = &req->buffer[5];
            req->length -= 10;
            return true;
        }
}