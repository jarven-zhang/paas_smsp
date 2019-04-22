#ifndef NET_IBUFFEROUT_H_
#define NET_IBUFFEROUT_H_

#include "outputbuffer.h"


    class IBlockOut;
    class IFlushHandler;

    class IBufferOut : public OutputBuffer
    {
    public:
        virtual IBlockOut *GetBlock() = 0;

        virtual void SetFlushHandler(IFlushHandler *handler) = 0;
    };


#endif /* NET_IBUFFEROUT_H_ */

