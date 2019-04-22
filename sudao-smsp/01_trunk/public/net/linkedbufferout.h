#ifndef NET_BUFFER_LINKED_LINKEDBUFFEROUT_H_
#define NET_BUFFER_LINKED_LINKEDBUFFEROUT_H_

#include "ibufferout.h"
class LinkedBlockPool;


class LinkedBlock;

class LinkedBufferOut : public IBufferOut
{
public:
    LinkedBufferOut(LinkedBlockPool* pLinkedBlockPool);
    virtual ~LinkedBufferOut();

public:
    void ConsumeBlock(UInt32 length);

protected:
    virtual IBlockOut *GetBlock();
    virtual void SetFlushHandler(IFlushHandler *handler);

protected:
    virtual void Write(char byte);
    virtual void Write(const char *data, UInt32 bytes);
    virtual void Flush();

    virtual UInt32 Size() const;

    virtual void Close();

private:
    LinkedBlock *first_;
    LinkedBlock *last_;

    int origin_;
    int length_;

    IFlushHandler *handler_;
	LinkedBlockPool* m_pLinkedBlockPool;

    //LinkedBlockPool *blockPool_;
};


#endif /* NET_BUFFER_LINKED_LINKEDBUFFEROUT_H_ */

