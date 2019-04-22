#include "linkedbuffermanager.h"

#include "linkedbufferin.h"
#include "linkedbufferout.h"



            LinkedBufferManager::LinkedBufferManager()
            {
            }

            LinkedBufferManager::~LinkedBufferManager()
            {
            }

            bool LinkedBufferManager::Init()
            {
                return true;
            }

            bool LinkedBufferManager::Destroy()
            {
                return true;
            }

            IBufferIn *LinkedBufferManager::CreateBufferIn(LinkedBlockPool* pLinkedBlockPool)
            {
                return new LinkedBufferIn(pLinkedBlockPool);
            }

            IBufferOut *LinkedBufferManager::CreateBufferOut(LinkedBlockPool* pLinkedBlockPool)
            {
                return new LinkedBufferOut(pLinkedBlockPool);
            }



