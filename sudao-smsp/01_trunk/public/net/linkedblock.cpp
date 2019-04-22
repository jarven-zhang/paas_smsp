#include "linkedblock.h"

#include <stdlib.h>

#include "linkedblockpool.h"

 #include <string.h>
using namespace std;




            LinkedBlock::LinkedBlock(LinkedBlockPool* pLinkedBlockPool)
            {
                begin_ = new char[30000];
                memset(begin_,0,30000);
                end_ = begin_ + 30000;
                cursor_ = begin_;
                next_ = NULL;
                m_pLinkedBlockPool = pLinkedBlockPool;
            }

            LinkedBlock::~LinkedBlock()
            {
                delete[] begin_;
            }

            LinkedBlock *LinkedBlock::GetNext()
            {
                return next_;
            }

            void LinkedBlock::SetNext(LinkedBlock *next)
            {
                next_ = next;
            }

            UInt32 LinkedBlock::Available()
            {
                return cursor_ - begin_;
            }

            UInt32 LinkedBlock::Remain()
            {
                return end_ - cursor_;
            }

            char *LinkedBlock::GetData()
            {
                return begin_;
            }

            char *LinkedBlock::GetCursor()
            {
                return cursor_;
            }

            void LinkedBlock::Fill(UInt32 bytes)
            {
                cursor_ += bytes;
                if (cursor_ > end_)
                {
                    cursor_ = end_;
                }
            }

            void LinkedBlock::Reuse()
            {
                cursor_ = begin_;
                next_ = NULL;
                memset(begin_,0,30000);

            }


            void LinkedBlock::Destroy()
            {
                Reuse();
                m_pLinkedBlockPool->PutBlock(this);
            }


