#ifndef NET_BUFFER_LINKED_LINKEDBUFFERMANAGER_H_
#define NET_BUFFER_LINKED_LINKEDBUFFERMANAGER_H_

#include "ibuffermanager.h"


class LinkedBufferManager : public IBufferManager
{
public:
    enum { Priority = 2 };

public:
    LinkedBufferManager();
    virtual ~LinkedBufferManager();
    virtual bool Init();
    virtual bool Destroy();

    virtual IBufferIn *CreateBufferIn(LinkedBlockPool* pLinkedBlockPool);
    virtual IBufferOut *CreateBufferOut(LinkedBlockPool* pLinkedBlockPool);

protected:

};



#endif /* NET_BUFFER_LINKED_LINKEDBUFFERMANAGER_H_ */

