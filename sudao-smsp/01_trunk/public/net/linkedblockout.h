#ifndef NET_BUFFER_LINKED_LINKEDBLOCKOUT_H_
#define NET_BUFFER_LINKED_LINKEDBLOCKOUT_H_

#include "iblockout.h"



            class LinkedBlock;
            class LinkedBufferOut;

            class LinkedBlockOut : public IBlockOut
            {
            public:
                LinkedBlockOut(const char *data, int size, LinkedBufferOut *buffer);
                virtual ~LinkedBlockOut();

            protected:
                virtual const char *Data();
                virtual int Size();
                virtual void Consume(int bytes);
                virtual void Release();

            private:
                const char *data_;
                int size_;
                LinkedBufferOut *buffer_;
            };





#endif /* NET_BUFFER_LINKED_LINKEDBLOCKOUT_H_ */

