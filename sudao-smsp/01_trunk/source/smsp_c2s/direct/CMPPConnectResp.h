#ifndef CMPPCONNECTRESP_H_
#define CMPPCONNECTRESP_H_
#include "CMPPBase.h"
namespace smsp
{

    class CMPPConnectResp: public CMPPBase
    {
    public:
        enum CMPPCONNSTATE
        {
            CONNSTATESUC = 0, CONNSTSTEFAILED,
        };
        CMPPConnectResp();
        virtual ~CMPPConnectResp();
    public:
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);
		virtual UInt32 bodySize() const;
        bool isLogin()
        {
            return _isOK;
        }
    public:
        //CMPP_CONNECT_RESP 1,16,1
        UInt8 _status;
        UInt8 _version;
        bool _isOK;
    };

} /* namespace smsp */
#endif /* CMPPCONNECTRESP_H_ */
