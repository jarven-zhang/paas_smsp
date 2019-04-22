#ifndef NET_INTERNALSERVERSOCKET_H_
#define NET_INTERNALSERVERSOCKET_H_

//#include "serversocket.h"
//#include "task.h"
#include "ihandler.h"
//#include "internalservice.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "address.h"
#include "Thread.h"


class TAcceptSocketMsg : public TMsg
{	
	public:
		int m_iSocket;
		Address m_address;
};


	class InternalService;

    class InternalServerSocket :
        public IHandler
    {

    public:
        InternalServerSocket(InternalService* server, CThread* pThread);
        virtual ~InternalServerSocket();


        bool Listen(const Address &address);
        void Close();


    protected:
        virtual int GetDescriptor();
        virtual void OnRead();
        virtual void OnWrite();

    private:
       // InternalSocket *Accept();

    private:
        //ServerSocketHandler *handler_;
        int socket_;
		CThread* m_pThread;
        InternalService *server_;
    };



#endif /* NET_INTERNALSERVERSOCKET_H_ */

