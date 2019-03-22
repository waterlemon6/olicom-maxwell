#ifndef __SWITCH_H__
#define __SWITCH_H__

struct switchRequest {
    unsigned char *buffer;
    unsigned char *position;
    long length;
};

void SwitchRequestInit(struct switchRequest *req, long len);
void SwitchRequestShow(struct switchRequest *req);
bool SwitchRequestScanner(struct switchRequest *req);

#endif
