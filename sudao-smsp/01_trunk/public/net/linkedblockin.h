#ifndef NET_BUFFER_LINKED_LINKEDBLOCKIN_H_
#define NET_BUFFER_LINKED_LINKEDBLOCKIN_H_

#include "iblockin.h"



            class LinkedBlock;
            class LinkedBufferIn;

            class LinkedBlockIn : public IBlockIn
            {
            public:
                LinkedBlockIn(LinkedBlock *block, LinkedBufferIn *buffer, bool nodelete = false);
                virtual ~LinkedBlockIn();

            protected:
                virtual char *Data();
                virtual int Size();
                virtual void Fill(int bytes);
                virtual void Release();

            private:
                LinkedBlock *block_;
                LinkedBufferIn *buffer_;
                bool filled_;
            };


#endif /* NET_BUFFER_LINKED_LINKEDBLOCKIN_H_ */

