#ifndef NET_IFLUSHHANDLER_H_
#define NET_IFLUSHHANDLER_H_



    class IBufferOut;

    class IFlushHandler
    {
    public:
        virtual void OnFlush(IBufferOut *buffer) = 0;
    };



#endif /* NET_IFLUSHHANDLER_H_ */

