#ifndef __MODULE_H__
#define __MODULE_H__

class Module {
private:
    int m_descriptor = -1;
public:
    Module() = default;
    virtual ~Module() = default;
    void Open(const char *file, int oflag);
    void Close();
    int GetDescriptor() { return m_descriptor; }
    bool IsOpen() { return (m_descriptor >= 0); }
};

#endif
