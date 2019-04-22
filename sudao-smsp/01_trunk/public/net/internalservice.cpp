#include "internalservice.h"
#include "internalserversocket.h"
#include "internalsocket.h"
#include "address.h"
#include "ibuffermanager.h"


    InternalService::InternalService()
    {
    }

    InternalService::~InternalService()
    {
    }

    EPollSelector *InternalService::GetSelector()
    {
        return selector_;
    }
    /*
    InternalTaskManager *InternalService::GetTaskManager()
    {
        return taskManager_;
    }*/

    LinkedBufferManager *InternalService::GetBufferManager()
    {
        return bufferManager_;
    }

    bool InternalService::Init()
    {
        selector_ = new EPollSelector();
        bufferManager_ = new LinkedBufferManager();

        selector_->Init();
        bufferManager_->Init();
        return true;
    }

    InternalServerSocket *InternalService::CreateServerSocket(CThread* pthread, bool ipV6)
    {
        return new InternalServerSocket(this, pthread);
    }


