#ifndef NET_BUFFER_LINKED_LINKEDBLOCK_H_
#define NET_BUFFER_LINKED_LINKEDBLOCK_H_
#include "platform.h"
class LinkedBlockPool;


            class LinkedBlock
            {
            public:
                LinkedBlock(LinkedBlockPool* pLinkedBlockPool);
                ~LinkedBlock();

                LinkedBlock *GetNext();
                void SetNext(LinkedBlock *next);

                UInt32 Available();
                UInt32 Remain();

                char *GetData();
                char *GetCursor();

                void Fill(UInt32 bytes);

                void Reuse();
                void Destroy();

            private:
                LinkedBlock *next_;

                char *begin_;
                char *end_;

                char *cursor_;
				LinkedBlockPool* m_pLinkedBlockPool;
            };


#endif /* NET_BUFFER_LINKED_LINKEDBLOCK_H_ */

