#ifndef NET_IBLOCKIN_H_
#define NET_IBLOCKIN_H_



    class IBlockIn
    {
    public:
        virtual char *Data() = 0;
        virtual int Size() = 0;
        virtual void Fill(int bytes) = 0;
        virtual void Release() = 0;
    };



#endif /* NET_IBLOCKIN_H_ */

