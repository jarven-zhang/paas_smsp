#ifndef NET_IHANDLER_H_
#define NET_IHANDLER_H_



    class IHandler
    {
    public:
        virtual int GetDescriptor() = 0;

        virtual void OnRead() = 0;
        virtual void OnWrite() = 0;
    };


#endif /* NET_IHANDLER_H_ */

