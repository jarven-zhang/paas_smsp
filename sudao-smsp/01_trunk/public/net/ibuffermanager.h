#ifndef NET_IBUFFERMANAGER_H_
#define NET_IBUFFERMANAGER_H_

class IBufferIn;
class IBufferOut;
class LinkedBlockPool;
class IBufferManager
{
public:
    virtual bool Init() = 0;
    virtual bool Destroy() = 0;

    virtual IBufferIn *CreateBufferIn(LinkedBlockPool* pLinkedBlockPool) = 0;
    virtual IBufferOut *CreateBufferOut(LinkedBlockPool* pLinkedBlockPool) = 0;
};

#endif /* NET_IBUFFERMANAGER_H_ */

