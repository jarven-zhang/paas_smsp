#ifndef NET_BUFFER_LINKED_LINKEDBLOCKPOOL_H_
#define NET_BUFFER_LINKED_LINKEDBLOCKPOOL_H_
#include "platform.h"

#include <stack>


class LinkedBlock;

class LinkedBlockPool
{
public:
    LinkedBlockPool();
    ~LinkedBlockPool();

public:
    LinkedBlock *GetBlock();
    bool PutBlock(LinkedBlock *block);

private:
    typedef std::stack<LinkedBlock* > BlockList;

private:
    BlockList blocks_;
    uint32_t maxCount_;
};

#endif /* NET_BUFFER_LINKED_LINKEDBLOCKPOOL_H_ */
