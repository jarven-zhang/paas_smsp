#include "linkedblockout.h"

#include <stdlib.h>

#include "linkedblock.h"
#include "linkedbufferout.h"



            LinkedBlockOut::LinkedBlockOut(const char *data, int size, LinkedBufferOut *buffer)
            {
                data_ = data;
                size_ = size;
                buffer_ = buffer;
            }

            LinkedBlockOut::~LinkedBlockOut()
            {
            }

            const char *LinkedBlockOut::Data()
            {
                return data_;
            }

            int LinkedBlockOut::Size()
            {
                return size_;
            }

            void LinkedBlockOut::Consume(int bytes)
            {
                if (bytes <= (int) size_)
                {
                    buffer_->ConsumeBlock(bytes);
                }
            }

            void LinkedBlockOut::Release()
            {
                delete this;
            }



