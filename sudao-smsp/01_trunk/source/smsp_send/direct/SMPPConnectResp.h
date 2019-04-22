#ifndef SMPPCONNECTRESP_H_
#define SMPPCONNECTRESP_H_
#include "SMPPBase.h"
namespace smsp
{

    class SMPPConnectResp: public SMPPBase
    {
    public:
        enum CMPPCONNSTATE
        {
            CONNSTATESUC = 0, CONNSTSTEFAILED,
        };
        SMPPConnectResp();
        SMPPConnectResp(const SMPPBase& other)
        {
            this->_commandLength=other._commandLength;
            this->_commandStatus=other._commandStatus;
            this->_sequenceNum=other._sequenceNum;
            this->_commandId=other._commandId;
        }
        virtual ~SMPPConnectResp();
    public:
        virtual bool Pack(pack::Writer &writer) const;
        virtual bool Unpack(pack::Reader &reader);
        bool isLogin()
        {
            return _isOK;
        }
    public:

        std::string _systemId;
        bool _isOK;
    };

} /* namespace smsp */
#endif /* SMPPCONNECTRESP_H_ */
