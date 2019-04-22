#ifndef NET_IBLOCKOUT_H_
#define NET_IBLOCKOUT_H_



    class IBlockOut
    {
    public:
        virtual const char *Data() = 0;
        virtual int Size() = 0;
        virtual void Consume(int bytes) = 0;
        virtual void Release() = 0;
    };


#endif /* NET_IBLOCKOUT_H_ */

