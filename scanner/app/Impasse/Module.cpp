#include <fcntl.h>
#include <unistd.h>
#include "Module.h"

void Module::Open(const char *file, int oflag) {
    m_descriptor = open(file, oflag);
}

void Module::Close() {
    close(m_descriptor);
    m_descriptor = -1;
}


