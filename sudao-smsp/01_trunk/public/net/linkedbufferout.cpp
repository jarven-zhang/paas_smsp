#include "linkedbufferout.h"

#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "iflushhandler.h"
#include "linkedblock.h"
#include "linkedblockout.h"
#include "linkedblockpool.h"

LinkedBufferOut::LinkedBufferOut(LinkedBlockPool* pLinkedBlockPool)
{
    first_ = NULL;
    last_ = NULL;
    origin_ = 0;
    length_ = 0;
    //blockPool_ =NULL;
    m_pLinkedBlockPool = pLinkedBlockPool;

}

LinkedBufferOut::~LinkedBufferOut()
{
    while (first_ != NULL)
    {
        LinkedBlock * block = first_->GetNext();
        first_->Destroy();
        first_ = block;
    }
}

void LinkedBufferOut::ConsumeBlock(UInt32 length)
{
    if (length_ < (int) length)
    {
        return;
    }
    length_ -= length;
    if (first_->Available() > length + origin_)
    {
        origin_ += length;
    }
    else
    {
        LinkedBlock *block = first_->GetNext();
        first_->Destroy();
        first_ = block;
        if (first_ == NULL)
        {
            last_ = NULL;
        }
        origin_ = 0;
    }
}

IBlockOut *LinkedBufferOut::GetBlock()
{
    if (first_ == NULL)
    {
        return new LinkedBlockOut(NULL, 0, this);
    }
    else
    {
        return new LinkedBlockOut(first_->GetData() + origin_, first_->Available() - origin_, this);
    }
}

void LinkedBufferOut::SetFlushHandler(IFlushHandler *handler)
{
    handler_ = handler;
}

void LinkedBufferOut::Write(char byte)
{
}

void LinkedBufferOut::Write(const char *data, UInt32 bytes)
{
    if (bytes == 0)
    {
        return;
    }
    length_ += bytes;
    while (bytes != 0)
    {
        if (last_ == NULL || last_->Remain() == 0)
        {
            if (last_ == NULL)
            {
                first_ = last_ = m_pLinkedBlockPool->GetBlock();
            }
            else
            {
                last_->SetNext(m_pLinkedBlockPool->GetBlock());
                last_ = last_->GetNext();
            }
        }
        int count = std::min(last_->Remain(), bytes);
        memcpy(last_->GetCursor(), data, count);
        last_->Fill(count);
        bytes -= count;
        data += count;
    }
    return;
}

void LinkedBufferOut::Flush()
{
    if (handler_ != NULL)
    {
        handler_->OnFlush(this);
    }
}

UInt32 LinkedBufferOut::Size() const
{
    return length_;
}

void LinkedBufferOut::Close()
{
    delete this;
}


