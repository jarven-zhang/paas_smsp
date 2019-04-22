#include "linkedbufferin.h"

#include <string.h>

#include "linkedblock.h"
#include "linkedblockin.h"
#include "linkedblockpool.h"








            LinkedBufferIn::LinkedBufferIn(LinkedBlockPool* pLinkedBlockPool)
            {
                last_ = NULL;
                first_ = NULL;
                current_ = NULL;
                //blockPool_ = NUll;

                origin_ = 0;
                readed_ = 0;
                offset_ = 0;
                length_ = 0;
                m_pLinkedBlockPool = pLinkedBlockPool;
            }

            LinkedBufferIn::~LinkedBufferIn()
            {
                LinkedBlock *block = first_;
                while (block != last_) {
                    first_ = block->GetNext();
                    block->Destroy();
                    block = first_;
                }
                if (last_ != NULL) {
                    last_->Destroy();
                }
            }
            
            void LinkedBufferIn::PutBlock(LinkedBlock *block, UInt32 length)
            {
                length_ += length;
                if (last_ == NULL)
                {
                    first_ = block;
                    last_ = block;
                    current_ = block;
                }
                if (block != last_)
                {
                    last_->SetNext(block);
                    last_ = block;
                }
            }

            IBlockIn *LinkedBufferIn::GetBlock()
            {
                if (last_ == NULL || last_->Remain() < 2000)
                {
                    return new LinkedBlockIn(m_pLinkedBlockPool->GetBlock(), this);
                }
                else
                {
                    return new LinkedBlockIn(last_, this, true);
                }
            }

            bool LinkedBufferIn::Read(char byte)
            {
                return true;
            }

            bool LinkedBufferIn::Read(char *data, UInt32 bytes)     
            {
                if (bytes > Available())
                {
                    return false;
                }
                readed_ += bytes;
                UInt32 readed = 0;
                while (current_ != NULL && bytes > 0)
                {
                    int count = std::min(bytes, current_->Available() - offset_);
                    memcpy(data + readed, current_->GetData() + offset_, count);
                    bytes -= count;
                    offset_ += count;
                    if ((int)current_->Available() == offset_ && current_ != last_)
                    {
                        current_ = current_->GetNext();
                        offset_ = 0;
                    }
                    readed += count;
                }
                return true;
            }

            UInt32 LinkedBufferIn::Available() const
            {
                return length_ - readed_;
            }

            void LinkedBufferIn::Mark()
            {
                LinkedBlock *block = first_;
                while (block != current_)
                {
                    first_ = block->GetNext();
                    block->Destroy();
                    block = first_;
                }
                length_ -= readed_;
                readed_ = 0;
                origin_ = offset_;
            }

            void LinkedBufferIn::Reset()
            {
                offset_ = origin_;
                readed_ = 0;
                current_ = first_;
            }

            InputBuffer *LinkedBufferIn::Copy(UInt32 bytes)
            {
                return NULL;
            }

            void LinkedBufferIn::Close()
            {
                delete this;
            }



