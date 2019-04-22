#ifndef NET_INTERNALSERVICE_H_
#define NET_INTERNALSERVICE_H_

#include <queue>
#include "epollselector.h"
//#include "internaltaskmanager.h"
#include "linkedbuffermanager.h"
#include "internalserversocket.h"

class CThread;

	//class ServerSocket;
	class InternalServerSocket;
	
    class InternalService
    {
    public:
        InternalService();
        virtual ~InternalService();
		
		bool Init();
        //InternalTaskManager *GetTaskManager();
        LinkedBufferManager *GetBufferManager();
        EPollSelector *GetSelector();
		InternalServerSocket *CreateServerSocket(CThread* pthread, bool ipV6 = false);

    private:
        EPollSelector *selector_;
        //InternalTaskManager *taskManager_;
        LinkedBufferManager *bufferManager_;
    };



#endif /* NET_INTERNALSERVICE_H_ */

