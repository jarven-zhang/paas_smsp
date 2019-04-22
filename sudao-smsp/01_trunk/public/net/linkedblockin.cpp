#include "linkedblockin.h"

#include "linkedblock.h"
#include "linkedbufferin.h"


            LinkedBlockIn::LinkedBlockIn(LinkedBlock *block, LinkedBufferIn *buffer, bool nodelete)
            {
                block_ = block;
                buffer_ = buffer;
                filled_ = nodelete;
            }

            LinkedBlockIn::~LinkedBlockIn()
            {
                if (!filled_)
                {
                    block_->Destroy();
                }
            }

            char *LinkedBlockIn::Data()
            {
                return block_->GetCursor();
            }

            int LinkedBlockIn::Size()
            {
                return block_->Remain();
            }

            void LinkedBlockIn::Fill(int bytes)
            {
                if (bytes <= (int) block_->Remain())
                {
                    block_->Fill(bytes);
                    buffer_->PutBlock(block_, bytes);
                    filled_ = true;
                }
            }

            void LinkedBlockIn::Release()
            {
                delete this;
            }



