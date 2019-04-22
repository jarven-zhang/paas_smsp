#ifndef NET_INTERNALSOCKET_H_
#define NET_INTERNALSOCKET_H_

//#include "socket.h"
//#include "task.h"
#include "ihandler.h"
#include "iflushhandler.h"
#include "internalservice.h"
#include "sockethandler.h"



    class IBufferIn;
    class IBufferOut;
	class InputBuffer;
	class OutputBuffer;

    class InternalSocket :
        public IHandler,
        public IFlushHandler
    {
    private:
        enum State
        {
            STATE_CONNECTED = 0x001,
            STATE_CONNECTING = 0x002,
            STATE_CLOSING = 0x004,
            STATE_WRITABLE = 0x008,
            STATE_DELAYED = 0x010,
        };

    public:
		InternalSocket();
        bool Init(InternalService * service,LinkedBlockPool* pLinkedBlockPool);
        bool Init(InternalService * service, int socket, const Address &address,LinkedBlockPool* pLinkedBlockPool);
        virtual ~InternalSocket();

    public:
        virtual void Connect(const Address &remote, int timeout);
        virtual void Connect(const Address &remote);

        virtual void SetPeek(bool flag);
        virtual void Close();
        virtual const char *GetLastError() const;

        virtual Address GetLocalAddress();
        virtual Address GetRemoteAddress();

        virtual InputBuffer *In();
        virtual OutputBuffer *Out();

        virtual void SetHandler(SocketHandler *handler);
        virtual int GetSocketDescriptor() const;

    public:
        virtual int GetDescriptor();

        virtual void OnRead();
        virtual void OnWrite();



    protected:
        virtual void OnFlush(IBufferOut *buffer);

    public:
        void OnConnected();
        void NotifyEvent(int type);
        void Send();
  

        bool HasState(UInt8 state);
        bool SetState(UInt8 state, bool flag = true);

    private:
        UInt8 state_;
        bool flag_;



        IBufferIn *input_;
        IBufferOut *output_;

        int socket_;
        SocketHandler *handler_;
        InternalService *service_;
    };



#endif /* NET_INTERNALSOCKET_H_ */

