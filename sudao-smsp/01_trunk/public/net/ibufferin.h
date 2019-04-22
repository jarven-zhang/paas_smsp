#ifndef NET_IBUFFERIN_H_
#define NET_IBUFFERIN_H_

#include "inputbuffer.h"



    class IBlockIn;

    class IBufferIn : public InputBuffer
    {
    public:
        virtual IBlockIn *GetBlock() = 0;
    };



#endif /* NET_IBUFFERIN_H_ */

