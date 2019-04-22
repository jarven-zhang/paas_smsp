#ifndef NET_INPUTBUFFER_H_
#define NET_INPUTBUFFER_H_
#include "platform.h"

#ifndef API
#include <string>
#endif /* API */



class InputBuffer {
public:
    virtual bool Read(char *data, UInt32 bytes) = 0;

    virtual UInt32 Available() const = 0;

    virtual void Mark() = 0;
    virtual void Reset() = 0;

    virtual void Close() = 0;
};

#endif /* NET_INPUTSTRINGBUFFER_H_ */

