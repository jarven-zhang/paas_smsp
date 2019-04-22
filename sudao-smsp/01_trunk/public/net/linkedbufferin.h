#ifndef NET_BUFFER_LINKED_LINKEDBUFFERIN_H_
#define NET_BUFFER_LINKED_LINKEDBUFFERIN_H_

#include "ibufferin.h"


class LinkedBlock;
class LinkedBlockPool;
class LinkedBufferIn : public IBufferIn
{
public:
    LinkedBufferIn(LinkedBlockPool* pLinkedBlockPool);
    virtual ~LinkedBufferIn();

public:
    void PutBlock(LinkedBlock *block, UInt32 length);

protected:
    virtual IBlockIn *GetBlock();

protected:
    virtual bool Read(char byte);
    virtual bool Read(char *data, UInt32 bytes);

    virtual UInt32 Available() const;

    virtual void Mark();
    virtual void Reset();

    virtual InputBuffer *Copy(UInt32 bytes);
    virtual void Close();

private:
    LinkedBlock *first_;
    LinkedBlock *last_;
    LinkedBlock *current_;

    //LinkedBlockPool *blockPool_;

    int origin_;
    int readed_;
    int offset_;
    int length_;
	LinkedBlockPool* m_pLinkedBlockPool;
};


#endif /* NET_BUFFER_LINKED_LINKEDBUFFERIN_H_ */

