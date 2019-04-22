#ifndef NET_OUTPUTBUFFER_H_
#define NET_OUTPUTBUFFER_H_
#include "platform.h"



class OutputBuffer {
public:
    virtual void Write(const char *data, UInt32 bytes) = 0;
    virtual void Flush() = 0;

    virtual UInt32 Size() const = 0;

    virtual void Close() = 0;
};



#endif /* NET_OUTPUTBUFFER_H_ */

