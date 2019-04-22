#include "linkedblockpool.h"

#include "linkedblock.h"



            namespace
            {
                uint32_t DEFAULT_MAX_COUNT = 1024;
            }

            LinkedBlockPool::LinkedBlockPool()
            {
                maxCount_ = DEFAULT_MAX_COUNT;
                //base::GetConfigManager()->Load();
                //base::PropertyUtils::GetValue("net.pool.maxcount", maxCount_);
            }

            LinkedBlockPool::~LinkedBlockPool()
            {
                while (!blocks_.empty())
                {
                    LinkedBlock *block = blocks_.top();
                    blocks_.pop();
                    if(NULL != block)
                    {
                        delete block;
                        block = NULL;
                    }
                }
            }

            LinkedBlock *LinkedBlockPool::GetBlock()
            {
                if (blocks_.size()>0)
                {
                    LinkedBlock *block = blocks_.top();
                    blocks_.pop();
                    return block;
                }
                else
                {
                    return new LinkedBlock(this);
                }
            }

//收数据将PutBlock进去
            bool LinkedBlockPool::PutBlock(LinkedBlock *block)
            {
                if (blocks_.size() < maxCount_)
                {
                    block->Reuse();
                    if (block == NULL)
                    {
                        return false;
                    }
                    blocks_.push(block);
                }
                else
                {
                    if(NULL != block)
                    {
                        delete block;
                        block = NULL;
                    }
                }
                return true;
            }


