#ifndef CMPP3CONNECTRESP_H_
#define CMPP3CONNECTRESP_H_
#include "CMPP3Base.h"
namespace cmpp3
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
		virtual UInt32 bodySize() const;
        virtual bool Pack(Writer &writer) const;
        virtual bool Unpack(Reader &reader);
        bool isLogin()
        {
            return _isOK;
        }
    public:
        //CMPP_CONNECT_RESP 1,16,1
        UInt32 _status;
		UInt8 _version;
        bool _isOK;
    };

} /* namespace smsp */
#endif /* CMPPCONNECTRESP_H_ */
