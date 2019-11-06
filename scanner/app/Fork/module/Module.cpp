#include <fcntl.h>
#include <unistd.h>
#include "Module.h"

void Module::Open(const char *file, int oflag) {
    descriptor_ = open(file, oflag);
}

void Module::Close() {
    close(descriptor_);
    descriptor_ = -1;
}


