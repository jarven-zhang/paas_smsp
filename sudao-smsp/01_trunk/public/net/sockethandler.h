#ifndef NET_SOCKETHANDLER_H_
#define NET_SOCKETHANDLER_H_


    //class Socket;
	class InternalSocket;
    class SocketHandler
    {
    public:
        virtual void OnEvent(int type, InternalSocket *socket) = 0;
    };


#endif /* NET_SOCKETHANDLER_H_ */
