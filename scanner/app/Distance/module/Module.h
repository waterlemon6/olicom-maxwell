#ifndef __MODULE_H__
#define __MODULE_H__

class Module {
private:
    int descriptor_ = -1;
public:
    Module() = default;
    virtual ~Module() = default;
    void Open(const char *file, int oflag);
    void Close();
    int GetDescriptor() { return descriptor_; }
    bool IsOpen() { return (descriptor_ >= 0); }
};

#endif
